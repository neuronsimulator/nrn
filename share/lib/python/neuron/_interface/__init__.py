"""myneuron — Python interface to NEURON via the C API and ctypes.

Provides the NEURON singleton (n/h), Section, and dynamic class dispatch.
"""
import builtins
import ctypes
import sys
import threading

from .api import (
    _ERR_BUF_SIZE,
    _METHODS_HOC_NRNPYTHON_OFFSET,
    _MIN_VALID_DATAPTR,
    TYPES,
    _call_at_top_level,
    _check_nrn_error,
    _install_methods_slot,
    _nrn_allsec,
    _nrn_cas,
    _nrn_distance,
    _nrn_double_pop,
    _nrn_double_push,
    _nrn_function_call,
    _nrn_gargstr,
    _nrn_getarg,
    _nrn_hoc_call,
    _nrn_hoc_ret,
    _nrn_nseg_get,
    _nrn_register_function,
    _nrn_secname,
    _nrn_section_length_get,
    _nrn_section_pop,
    _nrn_section_push,
    _nrn_section_Ra_get,
    _nrn_section_rallbranch_get,
    _nrn_sectionlist_iterator_free,
    _nrn_sectionlist_iterator_new,
    _nrn_sectionlist_iterator_next,
    _nrn_sectionlist_to_array,
    _nrn_str_pop,
    _nrn_symbol,
    _nrn_symbol_array_length,
    _nrn_symbol_dataptr,
    _nrn_symbol_is_array,
    _structure_change_cnt,
    install_stdout_redirect,
)
from .nrnref import (
    NrnDoubleRef,
    NrnObjectPropertyRef,
    NrnObjectRef,
    NrnRangeVarRef,
    NrnStrRef,
    NrnVarRef,
)
from .object import List, Object, SectionList, Vector
from .sections import Section
from .utils import (
    FuncWrapper,
    _is_python_object_arg,
    _PythonObjectArg,
    _Singleton,
    list_functions,
)

# Slot that _mn_capture_cas writes cas() into; object._resolve_template_section
# and object._steer_sectionref read it back. Module-global because a C callback cannot capture closure state.
_template_section_capture = ctypes.c_void_p(0)


def _register_template_section_callback():
    """Register `_mn_capture_cas()`, which stores cas() in _template_section_capture.

    The brace-capture mechanism that uses it is described in
    object._resolve_template_section.
    """

    def _capture_cas():
        _template_section_capture.value = _nrn_cas() or 0
        # Registered as 280: the dispatcher expects hoc_ret, then a pushed double.
        _nrn_hoc_ret()
        _nrn_double_push(0.0)

    cb_type = ctypes.CFUNCTYPE(None)
    # Keep-alive: NEURON stores only the raw pointer, so the trampoline must
    # not be garbage-collected.
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

# Python callbacks invoked from HOC (FInitializeHandler(type, python_callable)).
# Each callable gets an integer id; the HOC function `_mn_py_callback(id)`
# dispatches by id. Same scheme as the MATLAB interface's callback registry.
#
# Object.__del__ (object.py) evicts an entry via _unregister_py_callback when
# the FInitializeHandler wrapper is collected.
# TODO(gap-46): that is the only eviction path. An id never bound to a wrapper
# (construction raises after _register_py_callback) or a wrapper in a reference
# cycle leaks its entry. A weakref scheme would auto-evict but breaks the
# "registry keeps the callable alive" contract the FIH wrapper relies on.
_PY_CALLBACKS = {}
_PY_CALLBACK_NEXT_ID = 0
# A wrapper can be constructed on the main thread while another is evicted from
# the gui timer thread's GC, and the id read-inc-store is not atomic, so
# registry mutations take this lock. RLock: tolerate re-entry.
_PY_CALLBACK_LOCK = threading.RLock()

# Set when a Python callback (e.g. an FInitializeHandler) raises inside the HOC
# frame. The exception cannot unwind through the ctypes trampoline (C++ frame;
# SIGSEGV), so the trampoline stashes it here and the dispatch chokepoints
# re-raise it after the HOC call (finitialize / run / continuerun) returns.
# Otherwise the simulation would continue on half-initialized state and give a
# plausible but wrong trace.
_pending_callback_exc = None


def _stash_callback_exc(exc):
    """Stash *exc* for _raise_pending_callback_exc. The first writer wins.

    Called by the FInitializeHandler trampoline and the gap-41 py2n_component
    bridge (api.py).
    """
    global _pending_callback_exc
    if _pending_callback_exc is None:
        _pending_callback_exc = exc


def _raise_pending_callback_exc():
    """Re-raise (once) any exception stashed by a HOC-frame Python callback.

    Called at the dispatch chokepoints after a HOC call returns, so a failed
    FInitializeHandler or py2n_component read surfaces like real NEURON's
    hoc_execerror. Callers must pop the HOC return/sentinel slot first, or the
    operand stack leaks one slot per failed call.
    """
    global _pending_callback_exc
    if _pending_callback_exc is not None:
        exc = _pending_callback_exc
        _pending_callback_exc = None
        raise exc


def _register_py_callback_dispatcher():
    """Register `_mn_py_callback(id)` as a HOC function.

    FInitializeHandler is constructed with the command `_mn_py_callback(N)`,
    where N selects the callable in _PY_CALLBACKS. Returns 0 so it stays
    type 280 (FUN_BLTIN, returning double).
    """

    def _dispatch():
        # Bound before the try: if _nrn_getarg raises, the except handler still
        # uses cb_id, and a NameError would mask the real cause.
        global _pending_callback_exc
        cb_id = None
        try:
            cb_id = int(_nrn_getarg(1)[0])
            # After an earlier handler failed, run no more user code on the
            # inconsistent state (real NEURON aborts the init on the first error).
            if _pending_callback_exc is None:
                cb = _PY_CALLBACKS.get(cb_id)
                if cb is not None:
                    cb()
        except Exception as exc:
            # Stash for re-raise after the HOC call; also print for visibility.
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

    In real NEURON, `nrnpython("code")` goes through
    `neuron::python::methods.hoc_nrnpython`, which libnrnpython.so fills with a
    `PyRun_SimpleString` handler. Without libnrnpython or an `import neuron`
    side effect that pointer stays NULL and `nrnpython(...)` is a no-op.

    This handler is registered on every import. `nrn_register_function` ->
    `hoc_install` always emallocs a new symbol in hoc_top_level_symlist
    (symbol.cpp:77) and does not overwrite an existing entry. Ours wins because
    `hoc_lookup` checks hoc_top_level_symlist before hoc_built_in_symlist
    (symbol.cpp:63-75), so our type-280 (FUN_BLTIN) entry shadows the
    libnrnpython one even after `import neuron`. Same pattern as nrnmatlab
    (MATLAB/neuron_api.cpp:495-526). It also fills methods.hoc_nrnpython when
    empty so `nrnpython()` works at template scope.

    Semantics matched against real NEURON:
      * single string argument, executed in `__main__` (matches
        `PyRun_SimpleString`),
      * returns 1.0 on success, 0.0 on error (matches
        `(PyRun_SimpleString == 0)`),
      * Python exceptions are printed to stderr but do not propagate
        into the HOC frame.
    """

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


# Per-name HOC obfunc trampolines that return the Object bound to a top-level
# objref. Cached so `n.foo` does not redefine the obfunc on every access.
_OBJECTVAR_TRAMPOLINES = set()


def _read_hoc_objectvar(neuron_singleton, name):
    """Return the Object bound to the top-level objref ``name``, or None if nil.

    Defines `_mn_objgrab_<name>() { return <name> }` on first use.

    Nil guard: an unset `objref` (the `if h.x == None` idiom, or an objref
    array before assignment) is a nil HOC object. A nil `Object*` is a non-NULL
    sentinel, so popping it makes `nrn_class_name` dereference it and SIGSEGV.
    `object_id(nil) == 0` is the safe test: check it before the pop and return
    None, as real NEURON's `h.x is None` does.
    """
    sym = _nrn_symbol(name.encode("utf-8"))
    if sym and _nrn_symbol_is_array(sym):
        # `objref g[N]`: subscriptable proxy like real NEURON's HocObject
        # array (g[i], len(), iteration); nil elements read as None.
        return _ObjrefArray(neuron_singleton, name, int(_nrn_symbol_array_length(sym)))

    fn = f"_mn_objgrab_{name}"
    if fn not in _OBJECTVAR_TRAMPOLINES:
        neuron_singleton(f"obfunc {fn}() {{ return {name} }}")
        _OBJECTVAR_TRAMPOLINES.add(fn)
        # Refresh _top_level so the new obfunc is dispatchable.
        neuron_singleton._top_level = list_functions()
    # object_id takes a HOC object, not a Python wrapper, so evaluate it by name
    # through the hoc_ac_ trampoline.
    neuron_singleton(("hoc_ac_ = object_id(" + name + ")").encode("utf-8"))
    if neuron_singleton.hoc_ac_ == 0.0:
        return None
    return getattr(neuron_singleton, fn)()


def _read_hoc_objectvar_index(neuron_singleton, name, idx):
    """Return the Object at ``name[idx]`` for a top-level objref array, or None.

    Same nil guard as `_read_hoc_objectvar`.
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
        # Writes `n.name[i] = obj` through a per-name proc trampoline
        # `NAME[$2] = $o1`; None nils the element. Same as the scalar path in
        # NEURON.__setattr__.
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


# Delimit the captured strdef value in HOC `print` output. Assumed not to
# collide with any plausible model output.
_STR_BEGIN = "\x01_mn_str_begin_\x01"
_STR_END = "\x01_mn_str_end_\x01"
_DISTANCE_MISSING = builtins.object()


def _capture_hoc_string(action):
    """Run *action* while capturing one sentinel-delimited HOC string."""
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
        # The value sits exactly between the sentinels: HOC's `print a, b, c`
        # inserts no separator and the newline follows the END sentinel. Do NOT
        # .strip(): it would remove the value's own leading/trailing whitespace.
        return text.split(_STR_BEGIN, 1)[1].rsplit(_STR_END, 1)[0]
    except IndexError:
        return ""


def _read_hoc_string(neuron_singleton, expr):
    """Read the value of a top-level HOC strdef or string expression."""
    return _capture_hoc_string(
        lambda: neuron_singleton(f'print "{_STR_BEGIN}", {expr}, "{_STR_END}"')
    )


# Model-mutation epoch. Bumped on raw HOC commands (`n(cmd)`), top-level and
# object FuncWrapper dispatch, and direct C mutation paths such as
# `Section.insert()`. Python-side invalidation only clears the wrapper that made
# a change, but HOC-side changes and alias wrappers of the same native Section
# need a shared signal. The Section inserted-mech cache, Mechanism validity and
# the allsec pointer cache compare this epoch and rebuild when it moves.
# structure_change_cnt is not sufficient: it does not bump on insert, and some
# interior deletes evade the cheap head-link check. Over-invalidation (a
# harmless `n("print x")` also bumps) is intentional; classify commands only if
# it ever matters.
_MODEL_EPOCH = 0


def _model_epoch():
    return _MODEL_EPOCH


def _bump_model_epoch():
    global _MODEL_EPOCH
    _MODEL_EPOCH += 1


def _allsec_head_links():
    """Cheap freshness probe for the C section_list.

    Returns the (next, prev) pointers of the list sentinel from
    ``nrn_allsec()`` plus the ``element`` pointer of the nodes they point to.
    ``hoc_Item`` layout on 64-bit: element @0, next @8, prev @16, itemtype @24
    (nrn/src/oc/hoclist.h:34). An append or remove at either end changes
    ``next`` or ``prev``.

    The element pointers are needed because NEURON's hoc_Item allocator reuses
    freed nodes from a pool, so one hoc_Item address can wrap a different
    Section*. Two extra ``from_address`` reads (~200 ns total) make the
    fingerprint alias-resistant and still cheap enough for every ``allsec()``.

    Limit: it does not cover the whole list. Deleting or reordering strictly
    interior sections may go unnoticed until ``finitialize`` (or anything that
    bumps ``structure_change_cnt``).
    """
    head = _nrn_allsec()
    addr = ctypes.cast(head, ctypes.c_void_p).value or 0
    if not addr:
        return (0, 0, 0, 0)
    nxt = ctypes.c_void_p.from_address(addr + 8).value or 0
    prv = ctypes.c_void_p.from_address(addr + 16).value or 0
    # Empty list: sentinel.next == sentinel itself; no element to read.
    nxt_elem = (
        ctypes.c_void_p.from_address(nxt).value or 0 if nxt and nxt != addr else 0
    )
    prv_elem = (
        ctypes.c_void_p.from_address(prv).value or 0 if prv and prv != addr else 0
    )
    return (nxt, prv, nxt_elem, prv_elem)


def hclass(cls):
    """Return *cls* unchanged (source-level parity shim).

    Real NEURON needs ``class Sub(neuron.hclass(h.IClamp))`` because
    ``h.IClamp`` is a HocObject proxy, not a class. In myneuron ``n.IClamp`` is
    the dynamic class itself (``_HocClassMeta`` in object.py), so this shim
    only lets the same code run under both:

        # works in both real NEURON and myneuron
        class MyCell(n.hclass(n.IClamp)):
            pass
    """
    return cls


class NEURON(metaclass=_Singleton):
    def __init__(self):
        self._nrn_hoc_call = _nrn_hoc_call
        self._top_level = list_functions()
        # Placeholder; _rebuild_function_types() fills it after template-scoped
        # type-code discovery (PROCEDURE and FUNCTION codes come from there).
        self._function_types = frozenset()
        self.Section = Section
        # Identity shim for ``n.hclass(n.IClamp)``; see hclass() above.
        self.hclass = hclass
        # allsec() cache; freshness rules are in NEURON.allsec.
        # `_allsec_ptrs` is a list of raw section pointer ints, not a ctypes
        # c_void_p array: a list is faster to index and much faster to iterate
        # (a ctypes array boxes a fresh int on every step). A ctypes array
        # would cut storage to ~1/4 (~14 KiB at 500 sections), which is
        # negligible next to the warm-hit iteration cost. See
        # ALLSEC_WARM_RATIO_VS_REAL in benchmarks/python/canonical_constants.py.
        # `_allsec_wrappers` is a parallel list of lazily built Section
        # wrappers, so later iterations skip `_from_ptr`.
        self._allsec_ptrs = None
        self._allsec_wrappers = None
        self._allsec_count = 0
        self._allsec_version = None

    def _rebuild_function_types(self):
        """Repopulate the function-type set from the now-complete TYPES."""
        self._function_types = frozenset(
            t
            for t in (
                TYPES.BLTIN,
                TYPES.FUNCTION,
                TYPES.PROCEDURE,
                TYPES.FUN_BLTIN,
                TYPES.STRINGFUNC,
                TYPES.OBJECTFUNC,
                TYPES.OBFUNCTION,
            )
            if t is not None
        )

    def __dir__(self):
        # HOC symbols plus Python-side helpers. _Singleton's metaclass does not
        # expose these via dict(type(self)), so list them explicitly.
        names = set(self._top_level)
        names.update(
            name
            for name in type(self).__dict__
            if not name.startswith("_") and name not in ("Section",)
        )
        names.update(
            [
                "Section",
                "ref",
                "cas",
                "allsec",
                "hclass",
                "allsec_names",
                "allsec_data",
            ]
        )
        return sorted(names)

    def __getattr__(self, name):
        # Top-level fork of NEURON's dispatch (the component fork is
        # Object.__getattr__). Real NEURON's type-code switch is
        # nrnpython/nrnpy_hoc.cpp:1302-1407. Covered here:
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

        # Two passes: the cached _top_level, then a fresh list_functions() scan
        # for HOC symbols defined after import.
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
                        # subtype 2 = USERDOUBLE (t, dt, celsius), subtype 1 =
                        # USERINT (stoprun, use_mcell_ran4). Both live at the
                        # symbol's u.pval, read as double* or int*.

                        sym = _nrn_symbol(name.encode("utf-8"))
                        if not sym:
                            raise AttributeError(f"Variable '{name}' not found.")
                        value_ptr = _nrn_symbol_dataptr(sym)
                        if not value_ptr:
                            raise AttributeError(
                                f"Could not get data pointer for '{name}'."
                            )
                        if _subtype == TYPES.USERINT:
                            return int(
                                ctypes.cast(value_ptr, ctypes.POINTER(ctypes.c_int))[0]
                            )
                        return ctypes.cast(value_ptr, ctypes.POINTER(ctypes.c_double))[
                            0
                        ]
                    if _type == TYPES.VAR and _subtype == TYPES.USERPROPERTY:
                        # L, nseg, Ra, rallbranch read the accessed section's
                        # value. With none accessed, real NEURON raises
                        # TypeError; cas() raises the same, so route through it.
                        return getattr(self.cas(), name)
                    if _type == TYPES.VAR and _subtype == 0:
                        # NOTUSER (hocdec.h:82): runtime-created HOC scalar
                        # (`n('newvar = 42')`), stored in
                        # `hoc_top_level_data[sym->u.oboff].pval`, not at
                        # `sym->u.pval`. See api._MIN_VALID_DATAPTR (nrn#3815):
                        # dereference only a real address; otherwise read via
                        # the hoc_ac_ trampoline.

                        sym0 = _nrn_symbol(name.encode("utf-8"))
                        if sym0:
                            dptr = _nrn_symbol_dataptr(sym0)
                            addr = ctypes.cast(dptr, ctypes.c_void_p).value
                            if addr and addr >= _MIN_VALID_DATAPTR:
                                return dptr[0]

                        # Fallback: assign `hoc_ac_ = <name>` in HOC and read
                        # hoc_ac_'s USERDOUBLE dataptr.
                        ac_sym = _nrn_symbol(b"hoc_ac_")
                        if not ac_sym:
                            raise AttributeError(
                                f"Cannot read '{name}': hoc_ac_ trampoline missing."
                            )
                        if (
                            self._nrn_hoc_call(("hoc_ac_ = " + name).encode("utf-8"))
                            != 0
                        ):
                            raise AttributeError(
                                f"Failed to read HOC variable '{name}'."
                            )
                        ac_ptr = _nrn_symbol_dataptr(ac_sym)
                        return ctypes.cast(ac_ptr, ctypes.POINTER(ctypes.c_double))[0]

                    elif _type in self._function_types:

                        def nrn_func(*args, sec=None):
                            from .sections import Segment
                            from .utils import _push_args

                            if sec is not None and any(
                                isinstance(arg, Segment) for arg in args
                            ):
                                raise ValueError(
                                    "Cannot specify both 'sec' and a Segment argument."
                                )
                            # Liveness check before any push; see utils._push_args.
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
                                ret = _nrn_function_call(
                                    sym, len(args), _err_buf, _ERR_BUF_SIZE
                                )
                                # Any call may change model structure
                                # (delete_section, insert, user procs), so bump
                                # the epoch as NEURON.__call__ does. This also
                                # catches a warm allsec() after
                                # n.delete_section(sec=s), which may not move the
                                # head-link fingerprint.
                                _bump_model_epoch()
                                if ret:
                                    # Nothrow restores the stack to after the
                                    # arg push; release those slots (gap-56).
                                    rollback_args()
                                _check_nrn_error(ret, _err_buf)
                                if _type in (
                                    TYPES.BLTIN,
                                    TYPES.FUNCTION,
                                    TYPES.FUN_BLTIN,
                                ):
                                    result = _nrn_double_pop()
                                elif _type == TYPES.STRINGFUNC:
                                    result = _nrn_str_pop()
                                elif _type == TYPES.PROCEDURE:
                                    # hoc_procret pushes a sentinel 0.0
                                    # (oc/code.cpp:1540, "will be popped
                                    # immediately"). The HOC parser pops it for
                                    # statements; API-level calls must pop it
                                    # here. Otherwise every continuerun/run/
                                    # step/advance leaks a slot and the
                                    # 1000-deep operand stack (hoc_nstack,
                                    # oc/code.cpp:422) overflows after ~1000
                                    # calls. Covered by tests/stress/.
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
                                # After the pop above; see
                                # _raise_pending_callback_exc.
                                _raise_pending_callback_exc()
                                return result
                            finally:
                                if use_sec is not None:
                                    from .api import _nrn_section_is_active

                                    if _nrn_section_is_active(use_sec._sec):
                                        _nrn_section_pop()
                                    else:
                                        # The call deleted the pushed section;
                                        # pop via HOC pop_section (see
                                        # Section.__del__).
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
                        # Return the dynamic class: callable (``n.IClamp(seg)``
                        # runs Object.__init__'s construction path), indexable
                        # (``n.IClamp[0]`` via _HocClassMeta.__getitem__) and
                        # subclassable without neuron.hclass.
                        if name not in _dynamic_classes:
                            # Empty __slots__: no per-instance __dict__, like
                            # List, Vector and SectionList. _hoc_class_name
                            # binds the class to its HOC template for
                            # construction and metaclass indexing.
                            cls = type(
                                name,
                                (Object,),
                                {
                                    "__module__": __name__,
                                    "__slots__": (),
                                    "_hoc_class_name": name,
                                },
                            )
                            _dynamic_classes[name] = cls
                            from . import object as _obj_mod

                            setattr(_obj_mod, name, cls)
                            setattr(sys.modules[__name__], name, cls)
                        return _dynamic_classes[name]

                    elif _type == TYPES.MECHANISM:
                        from .mechanism import DensityMechanism

                        return DensityMechanism(name)

                    elif _type == TYPES.OBJECTVAR:
                        # Real NEURON (nrnpy_hoc.cpp:1346-1357) uses the
                        # OBJECTVAR bytecode opcode, which ctypes cannot
                        # invoke. Use a cached per-name obfunc instead.
                        return _read_hoc_objectvar(self, name)

                    elif _type == TYPES.STRING:
                        # A strdef is not readable via _nrn_symbol_dataptr in
                        # NEURON 9 (data_handle). Capture HOC `print` output
                        # through the stdout redirect: slow but reliable.
                        return _read_hoc_string(self, name)

                    elif _type == TYPES.SECTION:
                        # `create soma` (scalar) or `create dend[N]` (array).
                        # Uses the brace capture of template sections with no
                        # owner prefix; see object._resolve_template_section.
                        from .object import (
                            _resolve_template_section,
                            _TemplateSectionArray,
                        )

                        sym = _nrn_symbol(name.encode("utf-8"))
                        if not sym:
                            raise AttributeError(f"Section '{name}' not found.")
                        if _nrn_symbol_is_array(sym):
                            size = int(_nrn_symbol_array_length(sym))
                            return _TemplateSectionArray(None, name, size)
                        return _resolve_template_section(None, name, idx=None)

            # update the function list before giving up
            self._top_level = list_functions()

        raise AttributeError(f"'NEURON' object has no attribute '{name}'")

    def __setattr__(self, name, value):
        # Internal state (`Section`, `hclass`, every underscore-prefixed name)
        # is a real attribute. Every other name is a top-level HOC variable and
        # is written to HOC, as in real NEURON, rather than shadowed by a Python
        # attribute (`n.s = "x"` must set the strdef).
        if name.startswith("_") or name in ("Section", "hclass"):
            super().__setattr__(name, value)
            return

        info = self._top_level.get(name)
        if info is None:
            # The var may have been declared after the cache was built;
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
            sym = _nrn_symbol(name.encode("utf-8"))
            if not sym:
                raise AttributeError(f"Variable '{name}' not found.")
            value_ptr = _nrn_symbol_dataptr(sym)
            if not value_ptr:
                raise AttributeError(f"Could not get data pointer for '{name}'.")
            if _subtype == TYPES.USERINT:
                ctypes.cast(value_ptr, ctypes.POINTER(ctypes.c_int))[0] = int(value)
            else:
                ctypes.cast(value_ptr, ctypes.POINTER(ctypes.c_double))[0] = float(
                    value
                )
            return

        # Section-level property (L, nseg, Ra, rallbranch): write the accessed
        # section's value; cas() raises TypeError if none. Must precede the
        # generic VAR branch, which would assign a non-existent scalar via HOC.
        if _type == TYPES.VAR and _subtype == TYPES.USERPROPERTY:
            setattr(self.cas(), name, value)
            return

        # Runtime double scalar (subtype 0, `n('x = ...')`). Writing through
        # sym->u.pval segfaults on a core without nrn#3815; see
        # api._MIN_VALID_DATAPTR. Otherwise assign through HOC.
        if _type == TYPES.VAR:
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
                # A NUL ends the C string mid-literal, leaving an unterminated
                # quote; the assignment would fail silently and the strdef read
                # back empty. Reject like real NEURON.
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

            _sym = _nrn_symbol(name.encode("utf-8"))
            if _sym and _nrn_symbol_is_array(_sym):
                # An objref array needs an index (n.g[i] = obj, via
                # _ObjrefArray). A scalar assign would write element 0
                # (n.g = obj) or collapse the whole array (n.g = None). Real
                # NEURON rejects it with a dimensionality error.
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
        raise TypeError(f"Cannot assign to HOC symbol '{name}' (symbol type {_type}).")

    def __repr__(self):
        return "<NEURON>"

    def ref(self, value):
        """Return a fresh anonymous reference cell, like real NEURON's ``h.ref(x)``.

        ``n.ref(3.0)`` returns a settable double cell, ``n.ref('')`` /
        ``n.ref('s')`` a settable string cell. The returned object is indexed
        with ``[0]`` and can be passed to a HOC function expecting a pointer
        (``$&1``) or string (``$s1``) argument, which may write through it
        (gap-37). Other values create an object cell: ``n.ref({})`` retains
        the dictionary's identity and supports HOC ``$o1`` writeback. As in
        real NEURON, ``h.ref('t')`` is the string ``'t'``, not a reference to
        the global ``t``.

        For a pointer to an existing global use ``n._ref_t`` (double-valued
        globals only); range variables use ``seg._ref_v`` and object properties
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
        from .nrnref import NrnDoubleRef, NrnVarRef
        from .object import Object

        usage = (
            "setpointer(_ref_hocvar, 'POINTER_name', point_process or " "nrn.Mechanism)"
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
            raise TypeError("POINTER name can contain only ascii characters") from None

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

    def __call__(self, cmd, *, sec=None, raise_on_error=False, _no_epoch_bump=False):
        # Returns True on success, False on a HOC execution error. Like real
        # NEURON's h(), HOC errors print to stderr and do not raise: ModelDB
        # init.hoc files trigger warnings real NEURON swallows.
        # raise_on_error=True raises RuntimeError. See docs/ARCHITECTURE.md
        # "Deliberate departures from h() behavior".

        if sec is not None:
            sec._check_alive()  # Reject a deleted section before pushing it.
            _nrn_section_push(sec._sec)
        try:
            if isinstance(cmd, str):
                cmd = cmd.encode("utf-8")
            ret = self._nrn_hoc_call(cmd)
            # See _MODEL_EPOCH. Bump even on failure: the command may have
            # partly mutated. _no_epoch_bump is internal, for fixed-shape
            # commands that cannot change topology or mechanisms (the seg.diam
            # write); bumping there would invalidate allsec() caches on every
            # write (216 us vs 21 us over 300 sections).
            # Measurements: benchmarks/README.md#historical-implementation-measurements.
            if not _no_epoch_bump:
                _bump_model_epoch()
            # The n(cmd) no-raise contract (CLAUDE.md rule 9) covers HOC errors
            # only; a callback's Python exception is raised even by default.
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

        Supports four call shapes:

            n.distance(seg1, seg2)      -> distance between two segments
            n.distance(0, seg)          -> set origin (legacy)
            n.distance(seg)             -> distance from current origin
            n.distance(x, sec=sec)      -> location in an explicit section

        The two-segment form calls nrn_distance directly because _push_args
        overwrites seg_sec and would lose the first segment's section.
        """
        from .sections import Section, Segment
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
            # The direct nrn_distance path skips HOC dispatch's deleted-section
            # checks: a segment of a section deleted through HOC would
            # dereference freed nodes and SIGABRT. _check_alive raises
            # RuntimeError, as real NEURON does.
            arg1.sec._check_alive()
            arg2.sec._check_alive()
            return float(_nrn_distance(arg1.sec._sec, arg1.x, arg2.sec._sec, arg2.x))

        # Legacy shapes call the HOC `distance` function directly, avoiding a
        # second __getattr__ lookup on a hot path.
        args = tuple(arg for arg in (arg1, arg2) if arg is not _DISTANCE_MISSING)
        sym = _nrn_symbol(b"distance")
        if not sym:
            raise RuntimeError("HOC 'distance' symbol not found")
        temp_strs, seg_sec, rollback_args = _push_args(args, return_rollback=True)
        sec_pushed = False
        call_sec = seg_sec if seg_sec is not None else (sec if explicit_sec else None)
        if call_sec is not None:
            _nrn_section_push(call_sec._sec)
            sec_pushed = True
        try:
            err = ctypes.create_string_buffer(_ERR_BUF_SIZE)
            ret = _nrn_function_call(sym, len(args), err, _ERR_BUF_SIZE)
            if ret:
                # Release the arg slots nothrow left on the stack (gap-56).
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

        Runs only on a cache miss. ``nrn_sectionlist_to_array`` (nrn#3834)
        snapshots the list in two FFI crossings (count, then fill). The
        c_void_p restype already yields Python ints. The list-vs-array reason
        is in NEURON.__init__.
        """
        items = _nrn_allsec()
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

        Yields non-owning Section wrappers. A cached list of section pointers
        is reused while the topology is stable and regathered when it changes.

        Freshness is checked at the start of each iteration with the
        (head-link, ``structure_change_cnt``, model epoch) tuple:
          * head-link probes the section_list sentinel's (next, prev)
            pointers — catches every append and every removal at either
            end of the C list, including the empty ↔ non-empty transition.
          * ``structure_change_cnt`` catches ``finitialize()`` and other
            global topology rebuilds.
          * the model epoch catches HOC/Python mutation paths that do not
            move the C-list endpoints or bump ``structure_change_cnt``.

        ``structure_change_cnt`` is also re-read at every yield to catch a
        rebuild mid-iteration (see the in-loop comment).

        Known limit of the cheap probe: strictly-interior native reordering
        that also skips the model epoch may not invalidate. ``finitialize``
        resets the cache.
        """
        _from_ptr = Section._from_ptr

        link = _allsec_head_links()
        sc = _structure_change_cnt.value
        # The epoch covers interior deletions the head-link probe misses,
        # e.g. n("delete_section()") and n.delete_section(sec=s).
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
                # Topology rebuilt mid-iteration: regather and resume at the
                # section about to be yielded (or the next position if deleted).
                current_ptr = ptrs[idx]
                new_link = _allsec_head_links()
                ptrs, count = self._gather_section_ptrs()
                self._allsec_ptrs = ptrs
                self._allsec_count = count
                self._allsec_version = (new_link, new_sc, _model_epoch())
                self._allsec_wrappers = [None] * count
                wrappers = self._allsec_wrappers
                sc = new_sc

                # Search forward from the old index first (parents-before-
                # children order keeps positions close), then from zero.
                # TODO(gap-33): O(n) scan on every mid-iteration rebuild; use a
                # ptr->index dict once allsec_count routinely exceeds ~5000.
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
                    # Section was deleted: resume at the same index (clamped).
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
    # Bulk accessors that return raw values and skip Section wrappers, so the
    # allsec() cache does not apply. Opt-in fast paths for analysis loops.

    def allsec_names(self):
        """Return a list of section names without constructing wrappers.

        Same result as ``[s.name() for s in n.allsec()]``, but faster: it calls
        ``nrn_secname`` once per section pointer and creates no Section objects.
        """
        iterator = _nrn_sectionlist_iterator_new(_nrn_allsec())
        out = []
        push = out.append
        try:
            while True:
                ptr = _nrn_sectionlist_iterator_next(iterator)
                if not ptr:
                    break
                raw = _nrn_secname(ptr).decode("utf-8")
                # Strip the "_pysec." prefix nrn_secname adds, as Section.name() does.
                push(raw[7:] if raw.startswith("_pysec.") else raw)
        finally:
            _nrn_sectionlist_iterator_free(iterator)
        return out

    def allsec_data(self, *names):
        """Bulk-fetch one or more section properties without wrappers.

        ``n.allsec_data('L')`` returns ``[L0, L1, ...]``; with multiple
        names ``n.allsec_data('L', 'nseg')`` returns a list of tuples.
        Skips Section construction, so it beats the list-comprehension form and
        real NEURON's equivalent, which also builds a wrapper per step.

        Supported names: ``L``, ``Ra``, ``nseg``, ``rallbranch``, ``name``.
        Other names raise ``ValueError``.
        """
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
        iterator = _nrn_sectionlist_iterator_new(_nrn_allsec())
        try:
            if single:
                fn, cast = getters[0]
                while True:
                    ptr = _nrn_sectionlist_iterator_next(iterator)
                    if not ptr:
                        break
                    push(cast(fn(ptr)))
            else:
                while True:
                    ptr = _nrn_sectionlist_iterator_next(iterator)
                    if not ptr:
                        break
                    push(tuple(cast(fn(ptr)) for fn, cast in getters))
        finally:
            _nrn_sectionlist_iterator_free(iterator)
        return out


n = h = NEURON()


# ── Module initialization sequence ──
# discover() ran at api.py import for top-level codes. This fills in the codes
# that need a live HOC session (PROCEDURE, SECTION, OBFUNCTION, METHOD_OBFUNC,
# METHOD_STRFUNC, via a probe template and classes).
TYPES.discover_template_scoped(n)
# Rebuild the function-type set before minting the probe Section below, because
# Section.__del__ dispatches `delete_section` (type FUN_BLTIN).
n._rebuild_function_types()
# SectionRef needs an accessed section; use a throwaway one for the probe.
_probe_sec = Section("_mn_types_probe_secref")
try:
    _probe_sr = n.SectionRef(sec=_probe_sec)
    TYPES.probe_section_ref(_probe_sr._obj)
finally:
    del _probe_sec
TYPES.validate()
