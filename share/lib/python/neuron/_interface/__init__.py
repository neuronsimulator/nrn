"""myneuron — Python interface to NEURON via the C API and ctypes.

Provides the NEURON singleton (n/h), Section, and dynamic class dispatch.
"""
from .nrnref import NrnVarRef, NrnDoubleRef, NrnStrRef, NrnObjectRef
import builtins
import collections.abc
import ctypes
import sys
import threading

from .object import Object, List, Vector, SectionList
from .sections import Section
from .utils import (
    FuncWrapper,
    _Singleton,
    list_functions,
    _PythonObjectArg,
    _is_python_object_arg,
)


# Shared slot used by the type-307 (template public section) dispatch in
# object.py. The callback _mn_capture_cas writes here while inside the
# `cell.soma { _mn_capture_cas() }` block; object.py reads it back out.
# Module-global because the callback is registered as a C function and
# can't capture closure state.
_template_section_capture = ctypes.c_void_p(0)


def _register_template_section_callback():
    """Register a HOC-callable proc that snapshots cas() for type-307 access.

    Type 307 (section in a HOC template) isn't dispatched by
    nrn_method_call_nothrow — the bytecode path is sec_access_push, which the
    public C API doesn't expose. Workaround: from Python, run
    `cell.soma { _mn_capture_cas() }` via nrn_hoc_call. Inside the braces,
    the section is the current accessed section, so our callback reads cas()
    and stashes the pointer where object.py can pick it up.
    """
    from .api import (
        _nrn_register_function,
        _nrn_cas,
        _nrn_hoc_ret,
        _nrn_double_push,
    )

    def _capture_cas():
        _template_section_capture.value = _nrn_cas() or 0
        # Type 280 in HOC means "function returning double" — we don't care
        # about the return value, but the bytecode dispatcher expects one
        # pushed onto the stack and hoc_ret called.
        _nrn_hoc_ret()
        _nrn_double_push(0.0)

    cb_type = ctypes.CFUNCTYPE(None)
    # Hold a module-level reference to the wrapped callback so its trampoline
    # isn't garbage-collected after registration (NEURON only stores the raw
    # pointer).
    global _mn_capture_cas_cb
    _mn_capture_cas_cb = cb_type(_capture_cas)
    _nrn_register_function(
        ctypes.cast(_mn_capture_cas_cb, ctypes.c_void_p),
        b"_mn_capture_cas",
        280,
    )


_register_template_section_callback()

# Cache of dynamically created NEURON classes (name → class).
# List and Vector are pre-registered so they use hand-coded subclasses.
_dynamic_classes = {"List": List, "Vector": Vector, "SectionList": SectionList}

# Registry for Python callbacks invoked from HOC. Used by
# FInitializeHandler(type, python_callable). Each callable is given an
# integer id; the HOC-callable `_mn_py_callback(id)` dispatches by id.
# Modeled on the MATLAB interface's (matlabneuroninterface) callback-registry
# pattern, which registers each callable under an integer key the same way.
#
# Eviction happens in Object.__del__ (object.py), which calls
# _unregister_py_callback when the FInitializeHandler wrapper is GC'd.
# TODO(gap-46): that is the only eviction path. An id registered here but never
# bound to a wrapper (construction raises after _register_py_callback) or
# a wrapper that lives in a reference cycle leaks its entry. A weakref
# scheme would auto-evict but breaks the "registry keeps the callable
# alive" contract the FIH wrapper relies on — needs design.
_PY_CALLBACKS = {}
_PY_CALLBACK_NEXT_ID = 0
# The counter and registry are shared across threads: a wrapper can be
# constructed on the main thread while another is evicted from the gui timer
# thread's GC (Object.__del__ -> _unregister_py_callback). The id read-inc-store
# is not atomic, so serialize registry mutations under this lock. RLock so a
# future re-entrant caller on the same thread can't self-deadlock.
_PY_CALLBACK_LOCK = threading.RLock()

# Set when a Python callback (e.g. an FInitializeHandler) raises inside the HOC
# frame. A C++ exception can't unwind back through the ctypes callback
# trampoline (that SIGSEGVs), so the trampoline stashes the exception here and
# the dispatch chokepoints re-raise it once the HOC call (finitialize / run /
# continuerun) returns — instead of letting the simulation proceed on a
# half-initialized state and produce a plausible-but-wrong trace.
_pending_callback_exc = None


def _stash_callback_exc(exc):
    """Record an exception raised inside a HOC-frame C callback for re-raise.

    Used by both the FInitializeHandler trampoline and the gap-41
    py2n_component bridge (api.py): a C++ frame can't carry a Python exception
    back through the ctypes callout, so we stash it and the dispatch
    chokepoints surface it once the HOC call returns. First-writer-wins (a
    later callback in the same call doesn't clobber the original cause).
    """
    global _pending_callback_exc
    if _pending_callback_exc is None:
        _pending_callback_exc = exc


def _raise_pending_callback_exc():
    """Re-raise (once) any exception a HOC-frame Python callback stashed.

    Called at the dispatch chokepoints right after a HOC call returns, so a
    failed FInitializeHandler (or a py2n_component component read) surfaces out
    of the HOC call the way real NEURON's hoc_execerror does, not swallowed.
    """
    global _pending_callback_exc
    if _pending_callback_exc is not None:
        exc = _pending_callback_exc
        _pending_callback_exc = None
        raise exc


def _register_py_callback_dispatcher():
    """Register `_mn_py_callback(id)` as a HOC function.

    The HOC FInitializeHandler is constructed with the command
    `_mn_py_callback(N)` where N selects the Python callable from
    _PY_CALLBACKS. The function returns 0 so it remains type-280
    (FUN_BLTIN, returning double).
    """
    from .api import (
        _nrn_register_function,
        _nrn_getarg,
        _nrn_hoc_ret,
        _nrn_double_push,
    )

    def _dispatch():
        # Initialize before the try: if _nrn_getarg raises, the except
        # handler still references cb_id — leaving it unbound turns the
        # original error into a NameError that masks the real cause.
        global _pending_callback_exc
        cb_id = None
        try:
            cb_id = int(_nrn_getarg(1)[0])
            # If an earlier handler in this same init already failed, don't run
            # more user code on a now-inconsistent state — mirror real NEURON,
            # which aborts the init on the first callback error.
            if _pending_callback_exc is None:
                cb = _PY_CALLBACKS.get(cb_id)
                if cb is not None:
                    cb()
        except Exception as exc:
            # A C++ exception can't unwind back through the ctypes callback
            # trampoline (that SIGSEGVs), so stash it and re-raise on the
            # Python side once the HOC call returns — the
            # simulation must not proceed on a half-initialized state. Note it
            # on stderr too for immediate visibility.
            _pending_callback_exc = exc
            import sys
            print(
                f"myneuron: callback id={cb_id} raised: "
                f"{type(exc).__name__}: {exc}",
                file=sys.stderr,
            )
        # hoc_ret before pushing the return slot (oc/code.cpp order); the
        # reverse leaves the pushed value as the return slot.
        _nrn_hoc_ret()
        _nrn_double_push(0.0)

    cb_type = ctypes.CFUNCTYPE(None)
    global _mn_py_callback_cb
    _mn_py_callback_cb = cb_type(_dispatch)
    _nrn_register_function(
        ctypes.cast(_mn_py_callback_cb, ctypes.c_void_p),
        b"_mn_py_callback",
        280,
    )


_register_py_callback_dispatcher()


def _register_nrnpython_callback():
    """Register `nrnpython()` as a HOC function that executes Python code.

    In real NEURON, `nrnpython("code")` is wired to
    `neuron::python::methods.hoc_nrnpython`, which libnrnpython.so
    populates with a `PyRun_SimpleString`-based handler. With no
    libnrnpython companion and no side-effect `import neuron`, that
    pointer stays NULL and HOC code calling `nrnpython(...)` is a no-op.

    We register our own handler unconditionally (this runs on every
    import). `nrn_register_function` → `hoc_install` always emallocs a
    new symbol into hoc_top_level_symlist (symbol.cpp:77); it does not
    overwrite an existing entry. Ours wins anyway because `hoc_lookup`
    checks hoc_top_level_symlist before hoc_built_in_symlist
    (symbol.cpp:63-75), so our type-280 (FUN_BLTIN) entry shadows the
    libnrnpython one. In environments where `import neuron` already ran,
    both handlers exist in separate symbol tables and ours takes
    priority. Same pattern as nrnmatlab (MATLAB/neuron_api.cpp:495-526).

    Semantics matched against real NEURON:
      * single string argument, executed in `__main__` (matches
        `PyRun_SimpleString`),
      * returns 1.0 on success, 0.0 on error (matches
        `(PyRun_SimpleString == 0)`),
      * Python exceptions are printed to stderr but do not propagate
        into the HOC frame.
    """
    from .api import (
        _nrn_register_function,
        _nrn_gargstr,
        _nrn_hoc_ret,
        _nrn_double_push,
        _call_at_top_level,
        _install_methods_slot,
        _METHODS_HOC_NRNPYTHON_OFFSET,
    )

    def _nrnpython_callback():
        try:
            raw = _nrn_gargstr(1)
            code = raw.decode("utf-8") if isinstance(raw, bytes) else raw
            ns = sys.modules["__main__"].__dict__
            # Like NEURON's nrnpython_real (HocTopContextManager): the code
            # runs at HOC top level even when called from a template.
            _call_at_top_level(exec, (code, ns))  # noqa: S102 — documented HOC behavior
            status = 1.0
        except BaseException as exc:
            import traceback
            print(
                f"nrnpython error: {type(exc).__name__}: {exc}",
                file=sys.stderr,
            )
            traceback.print_exc(file=sys.stderr)
            status = 0.0
        _nrn_hoc_ret()
        _nrn_double_push(status)

    cb_type = ctypes.CFUNCTYPE(None)
    global _mn_nrnpython_cb
    _mn_nrnpython_cb = cb_type(_nrnpython_callback)
    _nrn_register_function(
        ctypes.cast(_mn_nrnpython_cb, ctypes.c_void_p),
        b"nrnpython",
        280,
    )
    _install_methods_slot(_METHODS_HOC_NRNPYTHON_OFFSET, _mn_nrnpython_cb)


_register_nrnpython_callback()


def _register_py_callback(fn):
    """Add *fn* to the callback registry and return its id."""
    global _PY_CALLBACK_NEXT_ID
    with _PY_CALLBACK_LOCK:
        cb_id = _PY_CALLBACK_NEXT_ID
        _PY_CALLBACK_NEXT_ID += 1
        _PY_CALLBACKS[cb_id] = fn
    return cb_id


def _unregister_py_callback(cb_id):
    """Drop the callback registered under *cb_id*. Idempotent."""
    with _PY_CALLBACK_LOCK:
        return _PY_CALLBACKS.pop(cb_id, None) is not None


# Per-name HOC obfunc trampolines that return the bound Object for a
# top-level objref. Caching avoids redefining the obfunc on every
# `n.foo` access — once `_mn_objgrab_foo` exists, we just call it.
_OBJECTVAR_TRAMPOLINES = set()


def _read_hoc_objectvar(neuron_singleton, name):
    """Return the Object bound to the top-level objref ``name``, or None if nil.

    Defines `_mn_objgrab_<name>() { return <name> }` on first use and
    caches the trampoline name so subsequent reads skip the HOC define.

    Nil guard: an unset `objref` (the common `if h.x == None` idiom, and
    `objref` arrays before assignment) yields a nil HOC object. Popping it via
    the trampoline hands `_object_pop` a nil `Object*` — NOT NULL, a non-NULL
    sentinel — so `nrn_class_name` dereferences it and SIGSEGVs. `object_id`
    is the crash-safe, HOC-blessed nil test (`object_id(nil) == 0`); check it
    before the pop and return Python None, matching real NEURON's `h.x is None`.
    """
    from .api import _nrn_symbol, _nrn_symbol_is_array, _nrn_symbol_array_length

    sym = _nrn_symbol(name.encode("utf-8"))
    if sym and _nrn_symbol_is_array(sym):
        # `objref g[N]` — return a subscriptable proxy. Real NEURON returns a
        # HocObject array supporting g[i], len(), iteration; nil elements read
        # as None.
        return _ObjrefArray(neuron_singleton, name, int(_nrn_symbol_array_length(sym)))

    fn = f"_mn_objgrab_{name}"
    if fn not in _OBJECTVAR_TRAMPOLINES:
        neuron_singleton(f"obfunc {fn}() {{ return {name} }}")
        _OBJECTVAR_TRAMPOLINES.add(fn)
        # Refresh _top_level so the new obfunc is dispatchable.
        neuron_singleton._top_level = list_functions()
    # object_id(name) routed through the hoc_ac_ trampoline (object_id takes a
    # HOC object arg, not a Python wrapper, so it must be evaluated by name).
    neuron_singleton(("hoc_ac_ = object_id(" + name + ")").encode("utf-8"))
    if neuron_singleton.hoc_ac_ == 0.0:
        return None
    return getattr(neuron_singleton, fn)()


def _read_hoc_objectvar_index(neuron_singleton, name, idx):
    """Return the Object at ``name[idx]`` for a top-level objref array, or None.

    Same nil guard as the scalar `_read_hoc_objectvar` — `object_id(name[idx])`
    is the crash-safe nil test before the obfunc pop.
    """
    fn = f"_mn_objgrabidx_{name}"
    if fn not in _OBJECTVAR_TRAMPOLINES:
        neuron_singleton(f"obfunc {fn}() {{ return {name}[$1] }}")
        _OBJECTVAR_TRAMPOLINES.add(fn)
        neuron_singleton._top_level = list_functions()
    neuron_singleton(
        ("hoc_ac_ = object_id(" + name + "[" + str(int(idx)) + "])").encode("utf-8")
    )
    if neuron_singleton.hoc_ac_ == 0.0:
        return None
    return getattr(neuron_singleton, fn)(idx)


class _ObjrefArray:
    """Subscriptable proxy for a top-level HOC ``objref name[N]`` array.

    Mirrors real NEURON's array HocObject: ``n.name[i]``, ``len(n.name)``, and
    iteration. Each index resolves through ``_read_hoc_objectvar_index`` so nil
    elements read as None instead of segfaulting.
    """

    __slots__ = ("_n", "_name", "_size")

    def __init__(self, neuron_singleton, name, size):
        self._n = neuron_singleton
        self._name = name
        self._size = size

    def __getitem__(self, idx):
        if isinstance(idx, slice):
            return [self[i] for i in range(*idx.indices(self._size))]
        if isinstance(idx, bool) or not isinstance(idx, int):
            raise TypeError(
                f"objref array indices must be integers, not {type(idx).__name__}"
            )
        if idx < 0:
            idx += self._size
        if not 0 <= idx < self._size:
            raise IndexError(
                f"index {idx} out of range [0, {self._size}) for {self._name}"
            )
        return _read_hoc_objectvar_index(self._n, self._name, idx)

    def __setitem__(self, idx, value):
        # Write an objref array element to HOC (e.g. `n.hoc_obj_[0] = vec`),
        # via a per-name proc trampoline `NAME[$2] = $o1`. None nils the
        # element. Mirrors NEURON.__setattr__'s scalar object-write path.
        if isinstance(idx, bool) or not isinstance(idx, int):
            raise TypeError(
                f"objref array indices must be integers, not {type(idx).__name__}"
            )
        if idx < 0:
            idx += self._size
        if not 0 <= idx < self._size:
            raise IndexError(
                f"index {idx} out of range [0, {self._size}) for {self._name}"
            )
        from .object import Object

        n = self._n
        if value is None:
            n("objref _mn_nilref")  # a persistent nil objref
            fn = "_mn_setobjidxnil_%s" % self._name
            if fn not in _OBJECTVAR_TRAMPOLINES:
                n("proc %s() { %s[$1] = _mn_nilref }" % (fn, self._name))
                _OBJECTVAR_TRAMPOLINES.add(fn)
                n._top_level = list_functions()
            getattr(n, fn)(idx)
            return
        if not isinstance(value, Object):
            raise TypeError(
                f"'{self._name}[{idx}]' is a HOC objref; cannot assign a "
                f"{type(value).__name__}."
            )
        fn = "_mn_setobjidx_%s" % self._name
        if fn not in _OBJECTVAR_TRAMPOLINES:
            n("proc %s() { %s[$2] = $o1 }" % (fn, self._name))
            _OBJECTVAR_TRAMPOLINES.add(fn)
            n._top_level = list_functions()
        getattr(n, fn)(value, idx)

    def __len__(self):
        return self._size

    def __iter__(self):
        for i in range(self._size):
            yield self[i]


# Sentinels used to delimit the captured strdef value in HOC `print`
# output. Unique enough not to collide with any plausible model output.
_STR_BEGIN = "\x01_mn_str_begin_\x01"
_STR_END = "\x01_mn_str_end_\x01"
_DISTANCE_MISSING = builtins.object()


def _capture_hoc_string(action):
    """Run *action* while capturing one sentinel-delimited HOC string."""
    from .api import install_stdout_redirect

    captured = []

    def sink(stream, msg):
        captured.append(msg)

    install_stdout_redirect(sink=sink)
    try:
        action()
    finally:
        install_stdout_redirect()  # restore default sink

    text = "".join(captured)
    try:
        # The value sits exactly between the sentinels — HOC's comma separator
        # in `print a, b, c` inserts nothing, and the trailing newline is
        # outside the END sentinel. Do NOT .strip(): that would remove the
        # value's own leading/trailing whitespace.
        return text.split(_STR_BEGIN, 1)[1].rsplit(_STR_END, 1)[0]
    except IndexError:
        return ""


def _read_hoc_string(neuron_singleton, expr):
    """Read the value of a top-level HOC strdef or string expression."""
    return _capture_hoc_string(
        lambda: neuron_singleton(f'print "{_STR_BEGIN}", {expr}, "{_STR_END}"')
    )


# Conservative model-mutation epoch. Bumped on raw HOC commands
# (`n(cmd)`), top-level/object FuncWrapper dispatch, and direct C mutation paths
# such as `Section.insert()`. Python-side cache invalidation can only clear the
# wrapper that performed a mutation; HOC-side changes and alias wrappers for the
# same native Section need a shared freshness signal. Caches that can go stale
# that way (Section inserted-mech cache, Mechanism validity, allsec pointer cache)
# compare this epoch and rebuild on change. structure_change_cnt is NOT
# sufficient — it does not bump on insert, and some interior delete paths evade
# cheap head-link checks. Over-invalidation (a harmless `n("print x")` also
# bumps) is intentional and conservative; optimize by classifying commands only
# if it ever matters.
_MODEL_EPOCH = 0


def _model_epoch():
    return _MODEL_EPOCH


def _bump_model_epoch():
    global _MODEL_EPOCH
    _MODEL_EPOCH += 1


def _allsec_head_links():
    """Cheap freshness probe for the C section_list.

    Reads the (next, prev) pointers of the list sentinel returned by
    ``nrn_allsec()``, plus the ``element`` pointer at offset 0 of each
    node those pointers point to. ``hoc_Item`` is
    laid out as: element @0, next @8, prev @16, itemtype @24 on 64-bit
    (see nrn/src/oc/hoclist.h:34). Append + remove at either end
    changes ``next`` or ``prev``; reordering strictly interior items
    doesn't.

    Reading just (next, prev) is not enough: NEURON's hoc_Item allocator
    reuses freed nodes from a pool, so the same hoc_Item address can
    wrap a different Section*. By also reading the element pointers at
    those positions we get an alias-resistant fingerprint at the
    expense of two extra ``from_address`` reads (~200 ns total). Still
    cheap enough to consult on every ``allsec()`` call.

    Not a fingerprint of the full list — deletions of strictly-interior
    C-list sections between calls may not invalidate, but a subsequent
    ``finitialize`` (or any operation that bumps ``structure_change_cnt``)
    will.
    """
    from .api import _nrn_allsec

    head = _nrn_allsec()
    addr = ctypes.cast(head, ctypes.c_void_p).value or 0
    if not addr:
        return (0, 0, 0, 0)
    nxt = ctypes.c_void_p.from_address(addr + 8).value or 0
    prv = ctypes.c_void_p.from_address(addr + 16).value or 0
    # Empty list: sentinel.next == sentinel itself; no element to read.
    nxt_elem = ctypes.c_void_p.from_address(nxt).value or 0 if nxt and nxt != addr else 0
    prv_elem = ctypes.c_void_p.from_address(prv).value or 0 if prv and prv != addr else 0
    return (nxt, prv, nxt_elem, prv_elem)


def hclass(cls):
    """Return *cls* unchanged — source-level parity shim.

    Real NEURON requires ``class Sub(neuron.hclass(h.IClamp)): ...`` because
    ``h.IClamp`` is a HocObject proxy, not a class, so it can't appear in a
    base-class list directly. In myneuron ``n.IClamp`` IS the dynamic class
    (see ``_HocClassMeta`` in object.py), so wrapping is unnecessary. The
    function is provided so cross-NEURON code that uses the hclass idiom
    keeps working under myneuron without conditional imports:

        # works in both real NEURON and myneuron
        class MyCell(n.hclass(n.IClamp)):
            pass
    """
    return cls


class NEURON(metaclass=_Singleton):
    def __init__(self):
        from .api import _nrn_hoc_call

        self._nrn_hoc_call = _nrn_hoc_call
        self._top_level = list_functions()
        # Initialized to a placeholder; rebuilt by _rebuild_function_types()
        # after template-scoped type-code discovery completes (PROCEDURE
        # and FUNCTION codes are filled in then).
        self._function_types = frozenset()
        self.Section = Section
        # Identity shim so cross-NEURON code can write
        # ``class MyCell(n.hclass(n.IClamp)): ...`` — see the
        # module-level hclass() above for the why.
        self.hclass = hclass
        # Smart allsec() cache. `_allsec_ptrs` is a Python list of raw
        # section pointer ints, not a ctypes c_void_p array: a list is
        # faster for both per-element indexing and iteration (the
        # iteration gap is the large one — a ctypes array boxes a fresh
        # Python int on every step). A ctypes array would cut steady-state
        # storage to ~1/4 (~14 KiB at 500 sections), but that saving is
        # negligible at realistic model sizes and the warm-hit iteration
        # is the allsec() hot path. See ALLSEC_WARM_RATIO_VS_REAL in
        # benchmarks/python/canonical_constants.py.
        # `_allsec_wrappers` is a parallel list of lazily-constructed
        # Section wrappers — once populated, later iterations yield cached
        # wrappers without re-running `_from_ptr`. `_allsec_version` is
        # (head_link_tuple, struct_change_cnt): head-link catches section
        # appends/end-removals between calls, struct_change_cnt catches
        # finitialize() and other global topology rebuilds mid-iteration.
        self._allsec_ptrs = None
        self._allsec_wrappers = None
        self._allsec_count = 0
        self._allsec_version = None

    def _rebuild_function_types(self):
        """Repopulate the function-type set from the now-complete TYPES."""
        from .api import TYPES
        self._function_types = frozenset(
            t for t in (
                TYPES.BLTIN, TYPES.FUNCTION, TYPES.PROCEDURE,
                TYPES.FUN_BLTIN, TYPES.STRINGFUNC, TYPES.OBJECTFUNC,
                TYPES.OBFUNCTION,
            ) if t is not None
        )

    def __dir__(self):
        # HOC symbols plus Python-side helpers (cas, ref, allsec, Section).
        # _Singleton's metaclass doesn't expose these via dict(type(self)),
        # so we enumerate explicitly.
        names = set(self._top_level)
        names.update(
            name for name in type(self).__dict__
            if not name.startswith("_") and name not in ("Section",)
        )
        names.update([
            "Section", "ref", "cas", "allsec", "hclass",
            "allsec_names", "allsec_data",
        ])
        return sorted(names)

    def __getattr__(self, name):
        # Top-level fork of NEURON's dispatch (component fork lives in
        # Object.__getattr__). See nrnpython/nrnpy_hoc.cpp:1302-1407 for the
        # full type-code switch real NEURON runs. We currently cover:
        #   263 (VAR, subtype 2) — global doubles (t, dt, celsius)
        #   264/270/271/280 — callable functions/procedures (FuncWrapper)
        #   284/296 — OBFUNCTION / OBJECTFUNC, return wrapped Object
        #   295 — string-returning function
        #   311 — density mechanism (DensityMechanism)
        #   324 (OBJECTVAR) — bound Object via per-name obfunc trampoline
        #   325 — HOC class (returned as the dynamic subclass of Object;
        #         subclassable and indexable via _HocClassMeta)
        # RANGEOBJ is segment/object scoped and runtime-probed after a
        # RANDOM-bearing MOD is loaded; see gap-22.
        from .api import TYPES
        # Two passes: dispatch against the cached _top_level, then after a
        # fresh list_functions() scan (end of loop) if the name wasn't
        # found — handles HOC symbols defined after import.
        for _ in [0, 1]:
            if name.startswith("_ref_"):
                base_name = name[5:]
                if base_name in self._top_level:
                    _type, _subtype = self._top_level[base_name]
                    if _type == TYPES.VAR and _subtype in (0, 2):
                        return NrnVarRef(base_name)
            else:
                if name in self._top_level:
                    _type, _subtype = self._top_level[name]
                    if _type == TYPES.VAR and _subtype in (1, 2):
                        # subtype 2 = USERDOUBLE (t, dt, celsius)
                        # subtype 1 = USERINT (stoprun, use_mcell_ran4)
                        # Both stored at the symbol's u.pval address but
                        # interpreted as either double* or int*.
                        from .api import _nrn_symbol, _nrn_symbol_dataptr

                        sym = _nrn_symbol(name.encode("utf-8"))
                        if not sym:
                            raise AttributeError(f"Variable '{name}' not found.")
                        value_ptr = _nrn_symbol_dataptr(sym)
                        if not value_ptr:
                            raise AttributeError(
                                f"Could not get data pointer for '{name}'."
                            )
                        if _subtype == TYPES.USERINT:
                            return int(ctypes.cast(
                                value_ptr, ctypes.POINTER(ctypes.c_int))[0])
                        return ctypes.cast(value_ptr, ctypes.POINTER(ctypes.c_double))[
                            0
                        ]
                    if _type == TYPES.VAR and _subtype == TYPES.USERPROPERTY:
                        # L, nseg, Ra, rallbranch — section-level properties.
                        # Real NEURON's h.L reads the currently-accessed
                        # section's value; with no section accessed it raises
                        # TypeError. cas() does exactly that (raises TypeError
                        # "Section access unspecified"), and Section exposes
                        # each as a property, so route through it.
                        return getattr(self.cas(), name)
                    if _type == TYPES.VAR and _subtype == 0:
                        # NOTUSER (hocdec.h:82) — runtime-created HOC scalar.
                        # `n('newvar = 42')` populates this. The data lives in
                        # `hoc_top_level_data[sym->u.oboff].pval`, not at
                        # `sym->u.pval`. A libnrniv carrying the upstream fix
                        # (nrn#3815) makes `nrn_symbol_dataptr` return that real
                        # address, so we can dereference it directly; older
                        # builds return the raw offset (a small integer), which
                        # we detect via `_MIN_VALID_DATAPTR` and never
                        # dereference, falling back to the hoc_ac_ trampoline.
                        from .api import (
                            _nrn_symbol,
                            _nrn_symbol_dataptr,
                            _MIN_VALID_DATAPTR,
                        )

                        sym0 = _nrn_symbol(name.encode("utf-8"))
                        if sym0:
                            dptr = _nrn_symbol_dataptr(sym0)
                            addr = ctypes.cast(dptr, ctypes.c_void_p).value
                            if addr and addr >= _MIN_VALID_DATAPTR:
                                return dptr[0]

                        # Fallback for a libnrniv predating the fix: route
                        # through the `hoc_ac_` trampoline (assign
                        # `hoc_ac_ = <name>` in HOC, then read hoc_ac_'s
                        # USERDOUBLE dataptr). Mirrors the strdef sentinel
                        # pattern in `_read_hoc_string`.
                        ac_sym = _nrn_symbol(b"hoc_ac_")
                        if not ac_sym:
                            raise AttributeError(
                                f"Cannot read '{name}': hoc_ac_ trampoline missing."
                            )
                        if self._nrn_hoc_call(
                            ("hoc_ac_ = " + name).encode("utf-8")
                        ) != 0:
                            raise AttributeError(
                                f"Failed to read HOC variable '{name}'."
                            )
                        ac_ptr = _nrn_symbol_dataptr(ac_sym)
                        return ctypes.cast(
                            ac_ptr, ctypes.POINTER(ctypes.c_double)
                        )[0]

                    elif _type in self._function_types:

                        def nrn_func(*args, sec=None):
                            from .api import (
                                _nrn_symbol,
                                _nrn_function_call,
                                _nrn_double_pop,
                                _nrn_str_pop,
                                _nrn_section_push,
                                _nrn_section_pop,
                                _check_nrn_error,
                                _ERR_BUF_SIZE,
                            )

                            from .utils import _push_args
                            from .sections import Segment

                            if sec is not None and any(
                                isinstance(arg, Segment) for arg in args
                            ):
                                raise ValueError(
                                    "Cannot specify both 'sec' and a Segment argument."
                                )
                            # Central liveness guard, BEFORE any push: a section
                            # freed through HOC would let a C++ exception cross
                            # ctypes and abort the process. Checking the explicit
                            # sec= here (and a Segment arg's section inside
                            # _push_args) means a dead-section call raises before
                            # a single operand is pushed, so nothing is orphaned
                            # on the HOC stack.
                            if sec is not None:
                                sec._check_alive()
                            temp_strs, seg_sec, rollback_args = _push_args(
                                args, return_rollback=True
                            )
                            use_sec = sec if sec is not None else seg_sec
                            if use_sec is not None:
                                _nrn_section_push(use_sec._sec)
                            try:
                                sym = _nrn_symbol(name.encode("utf-8"))
                                _err_buf = ctypes.create_string_buffer(_ERR_BUF_SIZE)
                                ret = _nrn_function_call(sym, len(args), _err_buf, _ERR_BUF_SIZE)
                                # A top-level function/procedure can mutate model
                                # structure through delete_section(), insert, user
                                # HOC procs, etc. Conservatively invalidate
                                # model-sensitive Python caches after every call,
                                # just as NEURON.__call__ does for raw HOC. This
                                # also catches warmed allsec() after
                                # n.delete_section(sec=s), whose deletion may not
                                # change the cheap head-link fingerprint.
                                _bump_model_epoch()
                                if ret:
                                    # The nothrow call restores the stack as it
                                    # was AFTER argument push. Release those
                                    # caller-owned slots on failure (gap-56).
                                    rollback_args()
                                _check_nrn_error(ret, _err_buf)
                                if _type in (TYPES.BLTIN, TYPES.FUNCTION, TYPES.FUN_BLTIN):
                                    result = _nrn_double_pop()
                                elif _type == TYPES.STRINGFUNC:
                                    result = _nrn_str_pop()
                                elif _type == TYPES.PROCEDURE:
                                    # PROCEDURE: hoc_procret pushes a sentinel
                                    # 0.0 (oc/code.cpp:1540, "will be popped
                                    # immediately"). The HOC parser pops it
                                    # for statement-level calls; we have to
                                    # pop it ourselves for API-level calls.
                                    # Without this, every continuerun/run/
                                    # step/advance leaks one NSTACK slot and
                                    # the 1000-deep operand stack (hoc_nstack
                                    # in oc/code.cpp:422) overflows after
                                    # ~1000 calls. Covered by tests/stress/;
                                    # closure history is in KNOWN_GAPS.md.
                                    _nrn_double_pop()
                                    result = None
                                elif _type in (TYPES.OBJECTFUNC, TYPES.OBFUNCTION):
                                    # OBJECTFUNC (296) = C built-in obfunc.
                                    # OBFUNCTION (284) = HOC `obfunc` definition.
                                    # Both push the result onto the object stack.
                                    from .utils import _object_pop
                                    result = _object_pop()
                                else:
                                    result = None
                                # finitialize/run/continuerun and HOC->Python
                                # component callbacks may stash a Python exception
                                # during the call above. Surface it only after
                                # popping the expected HOC return/sentinel, or the
                                # operand stack leaks one slot per failed call.
                                _raise_pending_callback_exc()
                                return result
                            finally:
                                if use_sec is not None:
                                    from .api import _nrn_section_is_active

                                    if _nrn_section_is_active(use_sec._sec):
                                        _nrn_section_pop()
                                    else:
                                        # The call deleted the pushed section
                                        # (e.g. h.delete_section(sec=s)). The C
                                        # nrn_section_pop runs chk_access and
                                        # aborts "Accessing a deleted section" on
                                        # the now-invalid stack top; HOC
                                        # pop_section bypasses that check
                                        # (cabcode.cpp:2165), the same route
                                        # Section.__del__ uses.
                                        NEURON().pop_section()
                                from .api import _hoc_unref_defer

                                _hoc_unref_defer()

                        return FuncWrapper(
                            nrn_func,
                            f"n.{name}",
                            f"NEURON top-level function: n.{name}()",
                            doc_key=name,
                        )
                    elif _type == TYPES.TEMPLATE:
                        # Return the dynamic class itself. It is:
                        #   - callable: ``n.IClamp(seg)`` runs
                        #     type.__call__ → Object.__init__'s
                        #     user-construction path
                        #   - indexable: ``n.IClamp[0]`` runs through
                        #     _HocClassMeta.__getitem__
                        #   - subclassable: ``class MyIClamp(n.IClamp): ...``
                        #     just works, no neuron.hclass wrapper needed
                        if name not in _dynamic_classes:
                            # Empty __slots__ on the dynamic class
                            # inherits Object's slots without adding
                            # a per-instance __dict__ — matches the
                            # hand-coded subclasses (List, Vector,
                            # SectionList). _hoc_class_name binds the
                            # class to its HOC template name so both
                            # the construction path and metaclass
                            # indexing can find it.
                            cls = type(name, (Object,), {
                                "__module__": __name__,
                                "__slots__": (),
                                "_hoc_class_name": name,
                            })
                            _dynamic_classes[name] = cls
                            from . import object as _obj_mod
                            setattr(_obj_mod, name, cls)
                            setattr(sys.modules[__name__], name, cls)
                        return _dynamic_classes[name]

                    elif _type == TYPES.MECHANISM:
                        from .mechanism import DensityMechanism

                        return DensityMechanism(name)

                    elif _type == TYPES.OBJECTVAR:
                        # Top-level objref. Real NEURON's path
                        # (nrnpy_hoc.cpp:1346-1357) goes through the
                        # bytecode-level OBJECTVAR opcode; we can't
                        # invoke that from ctypes. Instead, lazily
                        # define a per-name HOC obfunc that returns the
                        # variable, then dispatch through that. The
                        # obfunc is cached on the registry.
                        return _read_hoc_objectvar(self, name)

                    elif _type == TYPES.STRING:
                        # Top-level strdef: we can't read string
                        # globals via _nrn_symbol_dataptr in NEURON 9
                        # (data_handle abstraction). Capture HOC's
                        # `print` output via the existing stdout
                        # redirect — slow but reliable.
                        return _read_hoc_string(self, name)

                    elif _type == TYPES.SECTION:
                        # Top-level HOC-created section: `create
                        # soma` yields a scalar section; `create dend[N]` an
                        # array. Neither is dispatchable via nrn_method_call
                        # (the symbol parses as a section reference, not a
                        # call), so we reuse the same brace + _mn_capture_cas
                        # snapshot used for template sections (cell.soma),
                        # with no owner prefix. See object.py:
                        # _resolve_template_section / _TemplateSectionArray.
                        from .api import (
                            _nrn_symbol,
                            _nrn_symbol_is_array,
                            _nrn_symbol_array_length,
                        )
                        from .object import (
                            _resolve_template_section,
                            _TemplateSectionArray,
                        )

                        sym = _nrn_symbol(name.encode("utf-8"))
                        if not sym:
                            raise AttributeError(
                                f"Section '{name}' not found."
                            )
                        if _nrn_symbol_is_array(sym):
                            size = int(_nrn_symbol_array_length(sym))
                            return _TemplateSectionArray(None, name, size)
                        return _resolve_template_section(None, name, idx=None)

            # update the function list before giving up
            self._top_level = list_functions()

        raise AttributeError(f"'NEURON' object has no attribute '{name}'")

    def __setattr__(self, name, value):
        # Internal Python state lives as real attributes on the singleton:
        # the named slots, `hclass`, and every underscore-prefixed attribute
        # (_top_level, _function_types, _allsec_*, _nrn_hoc_call, ...). Every
        # other name is a top-level HOC variable, written through to HOC so it
        # matches real NEURON instead of silently shadowing it with a Python
        # attribute (the old behavior — and a footgun: `n.s = "x"` looked like
        # it set the HOC strdef but didn't).
        if name.startswith("_") or name in ("Section", "hclass"):
            super().__setattr__(name, value)
            return

        from .api import TYPES

        info = self._top_level.get(name)
        if info is None:
            # The var may have been declared after the cache was built —
            # refresh once (mirrors the two-pass lookup in __getattr__).
            self._top_level = list_functions()
            info = self._top_level.get(name)
        if info is None:
            # Real NEURON raises here (LookupError: not a defined hoc variable
            # name) rather than creating a shadowing Python attribute.
            raise LookupError(f"'{name}' is not a defined hoc variable name.")

        _type, _subtype = info

        # Built-in double globals (dt, celsius, t): direct pointer write.
        if _type == TYPES.VAR and _subtype in (1, 2):
            from .api import _nrn_symbol, _nrn_symbol_dataptr

            sym = _nrn_symbol(name.encode("utf-8"))
            if not sym:
                raise AttributeError(f"Variable '{name}' not found.")
            value_ptr = _nrn_symbol_dataptr(sym)
            if not value_ptr:
                raise AttributeError(f"Could not get data pointer for '{name}'.")
            if _subtype == TYPES.USERINT:
                ctypes.cast(value_ptr, ctypes.POINTER(ctypes.c_int))[0] = int(value)
            else:
                ctypes.cast(value_ptr, ctypes.POINTER(ctypes.c_double))[0] = float(value)
            return

        # Section-level property (L, nseg, Ra, rallbranch): write the
        # currently-accessed section's value, like real NEURON's h.L = v.
        # cas() raises TypeError if no section is accessed. Must
        # precede the generic subtype-0 VAR branch below, which would otherwise
        # try to assign a non-existent top-level scalar through HOC.
        if _type == TYPES.VAR and _subtype == TYPES.USERPROPERTY:
            setattr(self.cas(), name, value)
            return

        # Runtime double scalar (subtype 0, created via `n('x = ...')`): its
        # data is NOT at sym->u.pval on an unfixed libnrniv (a pointer write
        # there segfaults — see the subtype-0 read branch in __getattr__).
        if _type == TYPES.VAR:
            # On a libnrniv carrying the upstream fix (nrn#3815),
            # nrn_symbol_dataptr returns the real storage, so write through it
            # directly; on older builds it returns the raw offset (a small
            # integer), which we detect via _MIN_VALID_DATAPTR and never write
            # through, falling back to HOC-assign.
            from .api import _nrn_symbol, _nrn_symbol_dataptr, _MIN_VALID_DATAPTR

            sym0 = _nrn_symbol(name.encode("utf-8"))
            if sym0:
                dptr = _nrn_symbol_dataptr(sym0)
                addr = ctypes.cast(dptr, ctypes.c_void_p).value
                if addr and addr >= _MIN_VALID_DATAPTR:
                    dptr[0] = float(value)
                    return
            # Fallback: assign through HOC. repr(float(...)) round-trips exactly.
            self("%s = %s" % (name, repr(float(value))))
            return

        # Top-level strdef: assign through HOC with the literal escaped.
        if _type == TYPES.STRING:
            if not isinstance(value, (str, bytes)):
                raise TypeError(
                    f"'{name}' is a HOC string (strdef); cannot assign a "
                    f"{type(value).__name__}."
                )
            s = value.decode("utf-8") if isinstance(value, bytes) else value
            if "\x00" in s:
                # A NUL would terminate the C string mid-literal, leaving HOC
                # with an unterminated quote — the assignment silently failed
                # and the strdef read back empty. Reject like real NEURON
                # (ValueError: embedded null character) instead.
                raise ValueError("embedded null character")
            esc = (
                s.replace("\\", "\\\\")
                .replace('"', '\\"')
                .replace("\n", "\\n")
                .replace("\t", "\\t")
                .replace("\r", "\\r")
            )
            self('%s = "%s"' % (name, esc))
            return

        # Top-level objref: assign through a per-name proc trampoline
        # (`proc _mn_setobj_<name>() { <name> = $o1 }`) with the object pushed.
        # Mirrors the _read_hoc_objectvar read trampoline. None re-nils it.
        if _type == TYPES.OBJECTVAR:
            from .object import Object
            from .api import _nrn_symbol, _nrn_symbol_is_array

            _sym = _nrn_symbol(name.encode("utf-8"))
            if _sym and _nrn_symbol_is_array(_sym):
                # An objref array (`objref g[2]`) needs an index: n.g[i] = obj,
                # via the _ObjrefArray the read path returns. A scalar assign to
                # the array name silently wrote element 0 (n.g = obj) or, worse,
                # redeclared/collapsed the whole array (n.g = None). Real NEURON
                # rejects it with a dimensionality error.
                raise IndexError(
                    f"'{name}' is an object array; assign an element "
                    f"(n.{name}[i] = ...), not the whole array"
                )
            if value is None:
                self("objref %s" % name)
                return
            if not isinstance(value, Object):
                # HOC must see the same object as Python (notably movie_timer).
                # An explicit object argument keeps numeric/string payloads from
                # becoming HOC scalars; _push_args owns conversion and rollback.
                value = _PythonObjectArg(value)
            fn = "_mn_setobj_%s" % name
            if fn not in _OBJECTVAR_TRAMPOLINES:
                self("proc %s() { %s = $o1 }" % (fn, name))
                _OBJECTVAR_TRAMPOLINES.add(fn)
                self._top_level = list_functions()
            getattr(self, fn)(value)
            return

        # Functions, mechanisms, templates, sections: not assignable.
        raise TypeError(
            f"Cannot assign to HOC symbol '{name}' (symbol type {_type})."
        )

    def __repr__(self):
        return "<NEURON>"

    def ref(self, value):
        """Mint a fresh anonymous reference cell — real NEURON's ``h.ref(x)``.

        ``n.ref(3.0)`` returns a settable double cell, ``n.ref('')`` /
        ``n.ref('s')`` a settable string cell. The returned object is indexed
        with ``[0]`` and can be passed to a HOC function expecting a pointer
        (``$&1``) or string (``$s1``) argument, which may write through it
        (gap-37). Other values create an object cell: ``n.ref({})`` retains
        the dictionary's identity and supports HOC ``$o1`` writeback. As in
        real NEURON, ``h.ref('t')`` is the *string* ``'t'`` — NOT a reference
        to the global ``t``.

        To get a pointer to an existing global (the old ``n.ref('t')``
        behavior, and the MATLAB-interface idiom), use the ``_ref_`` syntax
        instead: ``n._ref_t`` (validated for double-valued globals in
        ``__getattr__``). Range vars use ``seg._ref_v``; object properties
        ``ic._ref_amp``.
        """
        # bool is an int subclass; real NEURON treats h.ref(True) as a number.
        if isinstance(value, (int, float)) or not _is_python_object_arg(value):
            return NrnDoubleRef(float(value))
        if isinstance(value, (str, bytes)):
            return NrnStrRef(value)
        return NrnObjectRef(value)

    def setpointer(self, source_ref, pointer_name, target):
        """Assign a numeric NEURON reference to an NMODL POINTER target."""
        from .mechanism import Mechanism
        from .nrnref import (
            NrnDoubleRef,
            NrnObjectPropertyRef,
            NrnRangeVarRef,
            NrnVarRef,
        )
        from .object import Object

        usage = (
            "setpointer(_ref_hocvar, 'POINTER_name', point_process or "
            "nrn.Mechanism)"
        )
        numeric_refs = (
            NrnDoubleRef,
            NrnObjectPropertyRef,
            NrnRangeVarRef,
            NrnVarRef,
        )
        if not isinstance(source_ref, numeric_refs):
            raise TypeError(usage)
        if not isinstance(pointer_name, str):
            raise TypeError(usage)
        try:
            pointer_name.encode("ascii")
        except UnicodeEncodeError:
            raise TypeError(
                "POINTER name can contain only ascii characters"
            ) from None

        if isinstance(target, Object):
            target._check_host_alive()
        elif isinstance(target, Mechanism):
            target._seg._sec._check_alive()
        else:
            raise TypeError(usage)

        try:
            target._assign_pointer(pointer_name, source_ref)
        except (NameError, RuntimeError):
            raise TypeError(usage) from None
        return None

    def __call__(self, cmd, *, sec=None, raise_on_error=False,
                 _no_epoch_bump=False):
        # Mirror real NEURON h(): HOC errors print to stderr but do not
        # raise. Returns True on success, False on HOC execution error.
        # Raising broke load_file("init.hoc") for ModelDB models whose
        # init scripts trigger HOC warnings real NEURON would swallow.
        # raise_on_error=True opts into the old strict behavior. See
        # docs/ARCHITECTURE.md "Deliberate departures from h()".
        from .api import _nrn_section_push, _nrn_section_pop

        if sec is not None:
            sec._check_alive()  # Reject a deleted section before pushing it.
            _nrn_section_push(sec._sec)
        try:
            if isinstance(cmd, str):
                cmd = cmd.encode("utf-8")
            ret = self._nrn_hoc_call(cmd)
            # A HOC command may have inserted/uninserted a mechanism or changed
            # topology in a way Python-side cache invalidation cannot see; bump
            # the model epoch so stale-able caches rebuild. Bump
            # unconditionally — even a failed command may have partially mutated.
            # _no_epoch_bump is internal-only, for fixed-shape commands that
            # provably cannot change topology/mechanisms (the seg.diam write
            # path); bumping there invalidated allsec() caches on every write
            # (216 µs vs 21 µs over 300 sections).
            # Dated context: benchmarks/README.md#historical-implementation-measurements.
            if not _no_epoch_bump:
                _bump_model_epoch()
            # A Python callback (FInitializeHandler) that raised during the HOC
            # call is a genuine Python error, not a HOC error — surface it even
            # in the default non-strict mode. The "n(cmd) never
            # raises" contract (rule 9) is about HOC-level errors only.
            _raise_pending_callback_exc()
            if ret != 0 and raise_on_error:
                raise RuntimeError(f"HOC execution failed: {cmd!r}")
            return ret == 0
        finally:
            if sec is not None:
                _nrn_section_pop()

    def cas(self):
        """Return the currently accessed Section.

        Matches real NEURON's `h.cas()`: raises ``TypeError`` if no
        section is on the access stack
        (``nrn/src/nrnpython/nrnpy_nrn.cpp:2967-2978``).
        """
        from .api import _nrn_cas

        sec_ptr = _nrn_cas()
        if not sec_ptr:
            raise TypeError("Section access unspecified")
        return Section._from_ptr(sec_ptr)

    def distance(
        self,
        arg1=_DISTANCE_MISSING,
        arg2=_DISTANCE_MISSING,
        *,
        sec=_DISTANCE_MISSING,
        **kwargs,
    ):
        """Path distance between two locations on the section tree.

        Supports three call shapes:

            n.distance(seg1, seg2)      -> distance between two segments
            n.distance(0, seg)          -> set origin (legacy)
            n.distance(seg)             -> distance from current origin
            n.distance(x, sec=sec)      -> location in an explicit section

        The two-segment form routes through nrn_distance directly because
        the generic HOC dispatch in _push_args loses the first segment's
        section when both args are Segments (it overwrites seg_sec).
        """
        from .sections import Section, Segment
        from .api import _nrn_distance, _nrn_function_call, _nrn_double_pop, _ERR_BUF_SIZE, _check_nrn_error, _nrn_symbol
        from .utils import _push_args

        if kwargs:
            raise RuntimeError("invalid keyword argument")
        explicit_sec = sec is not _DISTANCE_MISSING
        if explicit_sec:
            if not isinstance(sec, Section):
                raise TypeError("sec is not a Section")
            sec._check_alive()

        if (
            arg1 is not _DISTANCE_MISSING
            and arg2 is not _DISTANCE_MISSING
            and isinstance(arg1, Segment)
            and isinstance(arg2, Segment)
        ):
            # The direct nrn_distance path skips the deleted-section checks that
            # HOC dispatch would apply, so a segment on a section deleted through
            # HOC dereferences freed nodes and SIGABRTs. Guard both
            # ends; _check_alive raises RuntimeError, as real NEURON does here.
            arg1.sec._check_alive()
            arg2.sec._check_alive()
            return float(_nrn_distance(arg1.sec._sec, arg1.x, arg2.sec._sec, arg2.x))

        # Legacy patterns dispatch through the HOC `distance` function.
        # We call it directly here rather than re-entering __getattr__ to
        # avoid the double lookup on a simulation-hot path.
        args = tuple(
            arg for arg in (arg1, arg2) if arg is not _DISTANCE_MISSING
        )
        sym = _nrn_symbol(b"distance")
        if not sym:
            raise RuntimeError("HOC 'distance' symbol not found")
        temp_strs, seg_sec, rollback_args = _push_args(args, return_rollback=True)
        sec_pushed = False
        call_sec = seg_sec if seg_sec is not None else (sec if explicit_sec else None)
        if call_sec is not None:
            from .api import _nrn_section_push, _nrn_section_pop
            _nrn_section_push(call_sec._sec)
            sec_pushed = True
        try:
            err = ctypes.create_string_buffer(_ERR_BUF_SIZE)
            ret = _nrn_function_call(sym, len(args), err, _ERR_BUF_SIZE)
            if ret:
                # Nothrow recovery restores the stack after argument push;
                # these caller-owned slots still need release on failure.
                rollback_args()
            _check_nrn_error(ret, err)
            return _nrn_double_pop()
        finally:
            if sec_pushed:
                _nrn_section_pop()
            from .api import _hoc_unref_defer

            _hoc_unref_defer()

    def _gather_section_ptrs(self):
        """Return (list-of-Section-pointers, count) for every live section.

        c_void_p restype already unwraps to a Python int, so no per-element
        conversion is needed. See the cache design notes in NEURON.__init__
        for why a list, not a ctypes array.

        Runs only on a cache miss (cold gather). ``nrn_sectionlist_to_array``
        (nrn#3834) snapshots the whole list in two FFI crossings: a count pass,
        then a fill pass.
        """
        from .sections import _NRN_ALLSEC as _allsec
        from .api import _nrn_sectionlist_to_array

        items = _allsec()
        total = _nrn_sectionlist_to_array(items, None, 0)
        if total <= 0:
            return [], 0
        buf = (ctypes.c_void_p * total)()
        # Under the held GIL (PyDLL), nothing mutates the list between calls.
        got = _nrn_sectionlist_to_array(items, buf, total)
        count = min(got, total)
        return [buf[i] for i in range(count)], count

    def allsec(self):
        """Iterate over all sections currently in the model.

        Yields non-owning Section wrappers. Internally cached: a Python
        list of section pointer ints is reused across calls when
        topology is stable, and regathered when the C list mutates.

        Cache freshness is checked at the start of each iteration via
        the (head-link, ``structure_change_cnt``, model epoch) version tuple:
          * head-link probes the section_list sentinel's (next, prev)
            pointers — catches every append and every removal at either
            end of the C list, including the empty ↔ non-empty transition.
          * ``structure_change_cnt`` catches ``finitialize()`` and other
            global topology rebuilds.
          * the model epoch catches HOC/Python mutation paths that do not
            move the C-list endpoints or bump ``structure_change_cnt``.

        ``structure_change_cnt`` is also re-read at every yield to catch a
        rebuild mid-iteration (see the in-loop comment for the resume-by-
        pointer logic).

        Known limit inherited from the cheap probe: native strictly-interior
        reordering by a path that also skips the model epoch may not invalidate.
        Calling ``finitialize`` resets the cache.
        """
        from .api import _structure_change_cnt
        _from_ptr = Section._from_ptr

        link = _allsec_head_links()
        sc = _structure_change_cnt.value
        # The head-link probe misses strictly-interior deletions (see docstring);
        # the model epoch catches HOC-command and FuncWrapper structural changes
        # between iterations, e.g. n("delete_section()") and
        # n.delete_section(sec=s).
        version = (link, sc, _model_epoch())

        ptrs = self._allsec_ptrs
        if ptrs is None or self._allsec_version != version:
            ptrs, count = self._gather_section_ptrs()
            self._allsec_ptrs = ptrs
            self._allsec_count = count
            self._allsec_version = version
            self._allsec_wrappers = [None] * count
        ptrs = self._allsec_ptrs
        wrappers = self._allsec_wrappers
        count = self._allsec_count

        idx = 0
        while idx < count:
            new_sc = _structure_change_cnt.value
            if new_sc != sc:
                # Topology rebuilt mid-iteration. Regather and resume
                # by finding the section we were about to yield in the
                # new array. If it has been deleted, skip to the next
                # logical position.
                current_ptr = ptrs[idx]
                new_link = _allsec_head_links()
                ptrs, count = self._gather_section_ptrs()
                self._allsec_ptrs = ptrs
                self._allsec_count = count
                self._allsec_version = (new_link, new_sc, _model_epoch())
                self._allsec_wrappers = [None] * count
                wrappers = self._allsec_wrappers
                sc = new_sc

                # Search forward from the old index first (likely the
                # parents-before-children ordering keeps positions close);
                # fall back to a from-zero scan.
                # TODO(gap-33): O(n) linear scan on every mid-iteration topology
                # rebuild. Build a ptr→index dict instead once allsec_count
                # routinely exceeds ~5000 sections.
                found = -1
                for j in range(min(idx, count), count):
                    if ptrs[j] == current_ptr:
                        found = j
                        break
                if found < 0:
                    for j in range(0, min(idx, count)):
                        if ptrs[j] == current_ptr:
                            found = j
                            break
                if found < 0:
                    # Section was deleted; resume at the same numeric
                    # index in the new array (clamped). This matches
                    # the "continue from here" semantic.
                    idx = min(idx, count)
                else:
                    idx = found

                if idx >= count:
                    break

            w = wrappers[idx]
            if w is None:
                w = _from_ptr(ptrs[idx])
                wrappers[idx] = w
            yield w
            idx += 1

    # --- allsec_names, allsec_data ---
    #
    # Bulk accessors that skip Section wrapper construction entirely.
    # These are distinct from ``allsec()`` because they return raw data
    # (strings or numbers), not Section objects — so the unified-iterator
    # cache doesn't apply. They live alongside ``allsec()`` as opt-in
    # fast paths for analysis loops that only need property values.

    def allsec_names(self):
        """Return a list of section names without constructing wrappers.

        Equivalent in result to ``[s.name() for s in n.allsec()]`` but
        faster because no Section Python objects are minted — the C
        ``nrn_secname`` is called once per section pointer straight from
        the iterator. Useful for quick enumeration and reporting.
        """
        from .sections import (
            _NRN_ALLSEC as _allsec,
            _SL_ITER_NEW as _new,
            _SL_ITER_NEXT as _next,
            _SL_ITER_FREE as _free,
        )
        from .api import _nrn_secname

        iterator = _new(_allsec())
        out = []
        push = out.append
        try:
            while True:
                ptr = _next(iterator)
                if not ptr:
                    break
                raw = _nrn_secname(ptr).decode("utf-8")
                # nrn_secname prefixes Python-created sections with
                # "_pysec." — strip to match Section.name()'s contract.
                push(raw[7:] if raw.startswith("_pysec.") else raw)
        finally:
            _free(iterator)
        return out

    def allsec_data(self, *names):
        """Bulk-fetch one or more section properties without wrappers.

        ``n.allsec_data('L')`` returns ``[L0, L1, ...]``; with multiple
        names ``n.allsec_data('L', 'nseg')`` returns a list of tuples.
        Skips Section construction for every yielded section, so it
        beats the list-comp form — and real NEURON's equivalent, which
        also pays per-step wrapper construction — for the bulk case.

        Currently supports: ``L``, ``Ra``, ``nseg``, ``rallbranch``,
        ``name``. Other names raise ``ValueError`` — extending the set
        is a one-line addition per property if a direct C binding
        exists.
        """
        from .sections import (
            _NRN_ALLSEC as _allsec,
            _SL_ITER_NEW as _new,
            _SL_ITER_NEXT as _next,
            _SL_ITER_FREE as _free,
        )
        from .api import (
            _nrn_section_length_get,
            _nrn_section_Ra_get,
            _nrn_nseg_get,
            _nrn_section_rallbranch_get,
            _nrn_secname,
        )

        if not names:
            raise ValueError("allsec_data() requires at least one property name")

        prop_table = {
            "L": (_nrn_section_length_get, float),
            "Ra": (_nrn_section_Ra_get, float),
            "nseg": (_nrn_nseg_get, int),
            "rallbranch": (_nrn_section_rallbranch_get, float),
        }

        def _name_get(p):
            raw = _nrn_secname(p).decode("utf-8")
            return raw[7:] if raw.startswith("_pysec.") else raw

        getters = []
        for nm in names:
            if nm == "name":
                getters.append((_name_get, str))
            elif nm in prop_table:
                getters.append(prop_table[nm])
            else:
                raise ValueError(
                    f"allsec_data: unknown property {nm!r}. Known: "
                    "L, Ra, nseg, rallbranch, name"
                )

        single = len(getters) == 1
        out = []
        push = out.append
        iterator = _new(_allsec())
        try:
            if single:
                fn, cast = getters[0]
                while True:
                    ptr = _next(iterator)
                    if not ptr:
                        break
                    push(cast(fn(ptr)))
            else:
                while True:
                    ptr = _next(iterator)
                    if not ptr:
                        break
                    push(tuple(cast(fn(ptr)) for fn, cast in getters))
        finally:
            _free(iterator)
        return out


n = h = NEURON()


# ── Module initialization sequence ──
# Finish type-code discovery now that the NEURON singleton can execute HOC.
# discover() ran at api.py import time for top-level codes; this fills in
# the codes that require an active HOC session (PROCEDURE, SECTION,
# OBFUNCTION, METHOD_OBFUNC, METHOD_STRFUNC via probe template / classes).
from .api import TYPES as _TYPES
_TYPES.discover_template_scoped(n)
# Rebuild the dispatch function-type set now that PROCEDURE / FUNCTION /
# OBJECTFUNC are known — needed before we mint any Section probe below,
# because Section.__del__ dispatches `delete_section` (type FUN_BLTIN).
n._rebuild_function_types()
# SectionRef needs an accessed section to construct; mint a throwaway
# section purely for the probe.
_probe_sec = Section("_mn_types_probe_secref")
try:
    _probe_sr = n.SectionRef(sec=_probe_sec)
    _TYPES.probe_section_ref(_probe_sr._obj)
finally:
    del _probe_sec
_TYPES.validate()
