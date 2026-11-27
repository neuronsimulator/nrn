"""myneuron — Python interface to NEURON via the C API and ctypes.

Provides the NEURON singleton (n/h), Section, and dynamic class dispatch.
"""
import builtins
import ctypes
import types

from ._docs import top_level_doc as _top_level_doc
from .api import (
    CALLS_PYTHON,
    HOC_LOCK,
    PENDING_CALLBACKS,
    raise_callback_error,
    EPOCH,
    _bump_model_epoch,
    _model_epoch,
    _as_double,
    _decode_hoc_string,
    _is_hoc_number,
    hoc_locked,
    symbol_array_dims,
    _ERR_BUF,
    _ERR_BUF_SIZE,
    TYPES,
    _call_at_top_level,
    _check_nrn_error,
    _ensure_topology,
    _nrn_allsec,
    _section_list_version,
    _nrn_cas,
    _nrn_distance,
    _nrn_double_pop,
    _nrn_double_push,
    _nrn_function_call,
    _nrn_hoc_call,
    _nrn_hoc_ret,
    _nrn_nseg_get,
    _nrn_object_pop,
    _nrn_register_function,
    _nrn_secname,
    _nrn_section_length_get,
    _nrn_section_push,
    _nrn_section_Ra_get,
    _nrn_section_rallbranch_get,
    _nrn_sectionlist_iterator_free,
    _nrn_sectionlist_iterator_new,
    _nrn_sectionlist_iterator_next,
    _nrn_section_is_active,
    _nrn_sectionlist_to_array,
    _nrn_str_pop0,
    _nrn_symbol,
    _nrn_symbol_dataptr,
    _nrn_symbol_is_array,
    _nrn_symbol_object_get,
    _nrn_symbol_str_get,
    _nrn_symbol_type,
    _top_level_symbol,
    install_stdout_redirect,
    _nrn_object_ref,
    _hoc_unref_defer,
)
from .nrnref import (
    NrnDoubleRef,
    NrnRef,
    NrnObjectPropertyRef,
    NrnObjectRef,
    NrnRangeVarRef,
    NrnStrRef,
    NrnTopStrRef,
    NrnVarRef,
    _array_index,
    _flat_index,
    numpy_element_ref,  # noqa: F401  (public: myneuron.numpy_element_ref)
    refuse_pickle,
    NrnArrayPropertyRef,
    NrnDoubleArrayElementRef,
    NrnVectorElementRef,
    _double_array_data,
)
from .object import (
    List,
    Object,
    SectionList,
    Vector,
    _TemplateSectionArray,
    _resolve_template_section,
)
from .sections import PYTHON_SECTIONS, Section, _section_for, Segment
from .utils import (
    _name_error,
    READ_ONLY_FUNCTIONS,
    TEMPLATE_CONVERTERS,
    FuncWrapper,
    _NO_SEC,
    _check_sec_kwarg,
    _is_python_object_arg,
    _pop_pushed_section,
    _PythonObjectArg,
    _Singleton,
    _wrap_owned_object,
    forget_class,
    hoc_class,
    list_functions,
    _check_segment_arg,
    _drop_return_slot,
    _return_slot_pop,
)
from .mechanism import Mechanism, density_mechanism
from . import utils as _utils

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

if CALLS_PYTHON:
    from . import _hooks as _python_hooks
else:
    from . import _released_core as _python_hooks
_python_hooks.install()
# neuron.rxd's
set_savestate_callbacks = _python_hooks.set_savestate_callbacks
set_rvp_converter = _python_hooks.set_rvp_converter

# Names of the per-name HOC helpers already defined for top-level objrefs
# (array element reads and writes, scalar writes), so each is defined once.
_OBJECTVAR_TRAMPOLINES = set()


def _read_hoc_objectvar(neuron_singleton, name):
    """Return the Object bound to the top-level objref ``name``, or None if nil.

    A scalar objref is read with nrn_symbol_object_get, as NEURON reads it, so
    no HOC statement runs: hoc_ac_, which HOC code uses as a scratch variable,
    keeps its value. HOC is switched to its top level for the lookup, so a call
    made while a template method runs finds the top-level objref and not a
    template member with the same name.
    """
    return _call_at_top_level(_read_top_level_objectvar, (neuron_singleton, name))


def _read_top_level_objectvar(neuron_singleton, name):
    sym = _nrn_symbol(name.encode("utf-8"))
    if not sym:
        raise AttributeError(f"objref '{name}' not found.")
    if _nrn_symbol_is_array(sym):
        # `objref g[N]`: subscriptable proxy like real NEURON's HocObject
        # array (g[i], len(), iteration); nil elements read as None.
        return _ObjrefArray(
            neuron_singleton,
            name,
            symbol_array_dims(sym),
        )
    obj = _nrn_symbol_object_get(sym)
    if not obj:
        return None  # a nil objref, as `h.x is None` in NEURON
    _nrn_object_ref(obj)  # borrowed; the wrapper releases its own reference
    return _wrap_owned_object(obj)


def _objref_trampolines(n, name, ndim):
    """Per-name, per-rank HOC helpers to read, test and write ``name[i]...``."""
    subs = "".join(f"[${k + 1}]" for k in range(ndim))
    write_subs = "".join(f"[${k + 2}]" for k in range(ndim))
    grab = f"_mn_objgrabidx{ndim}_{name}"
    ident = f"_mn_objididx{ndim}_{name}"
    put = f"_mn_setobjidx{ndim}_{name}"
    nil = f"_mn_setobjidxnil{ndim}_{name}"
    if grab not in _OBJECTVAR_TRAMPOLINES:
        n("objref _mn_nilref")  # a persistent nil objref
        n(f"obfunc {grab}() {{ return {name}{subs} }}")
        n(f"func {ident}() {{ return object_id({name}{subs}) }}")
        n(f"proc {put}() {{ {name}{write_subs} = $o1 }}")
        n(f"proc {nil}() {{ {name}{subs} = _mn_nilref }}")
        _OBJECTVAR_TRAMPOLINES.add(grab)
        n._top_level = list_functions()
    return grab, ident, put, nil


class _ObjrefArray:
    """Subscriptable proxy for a top-level HOC ``objref name[N]...`` array.

    Mirrors NEURON's array HocObject: ``n.name[i]``, ``n.name[i][j]`` for more
    dimensions, ``len()`` and iteration. Nil elements read as None. Elements
    take HOC objects, None, or any Python value (stored as a PythonObject and
    read back as the value); a row of a multi-dimensional array is not
    assignable.
    """

    __slots__ = ("_n", "_name", "_dims", "_prefix")

    __reduce_ex__ = refuse_pickle

    def __init__(self, neuron_singleton, name, dims, prefix=()):
        self._n = neuron_singleton
        self._name = name
        self._dims = tuple(dims)
        self._prefix = tuple(prefix)

    def _check_shape(self):
        # The name can be redeclared (`objref po` after `objref po[3]`); NEURON
        # then rejects the old array object.
        sym = _top_level_symbol(self._name.encode("utf-8"))
        if not sym or not _nrn_symbol_is_array(sym):
            raise TypeError(f"'{self._name}' is no longer an array")
        dims = symbol_array_dims(sym)
        if dims != self._dims:
            raise TypeError(f"'{self._name}' was redeclared with dimensions {dims}")

    def _index(self, idx):
        self._check_shape()
        size = self._dims[len(self._prefix)]
        i = _array_index(idx, size, "objref array indices must be integers, not {}")
        if i is None:
            raise IndexError(f"index {idx} out of range [0, {size}) for {self._name}")
        return i

    def _is_row(self):
        return len(self._prefix) + 1 < len(self._dims)

    def __getitem__(self, idx):
        if isinstance(idx, slice):
            return [self[i] for i in range(*idx.indices(len(self)))]
        idx = self._index(idx)
        indices = self._prefix + (idx,)
        if self._is_row():
            return _ObjrefArray(self._n, self._name, self._dims, indices)
        grab, ident, _, _ = _objref_trampolines(self._n, self._name, len(self._dims))
        if getattr(self._n, ident)(*indices) == 0.0:
            return None
        return getattr(self._n, grab)(*indices)

    def __setitem__(self, idx, value):
        idx = self._index(idx)
        if self._is_row():
            raise TypeError(
                f"'{self._name}' has {len(self._dims)} dimensions; assign an element"
            )

        indices = self._prefix + (idx,)
        _, _, put, nil = _objref_trampolines(self._n, self._name, len(self._dims))
        if value is None or isinstance(value, NrnRef):  # see __setattr__
            getattr(self._n, nil)(*indices)
            return
        if not isinstance(value, Object):
            value = _PythonObjectArg(value)
        getattr(self._n, put)(value, *indices)

    def __len__(self):
        self._check_shape()
        return self._dims[len(self._prefix)]

    def __bool__(self):
        self._check_shape()
        return True

    def __iter__(self):
        for i in range(len(self)):
            yield self[i]


class _DoubleArray:
    """Subscriptable proxy for a top-level HOC ``double name[N]...`` array.

    Mirrors NEURON's array HocObject: ``n.a[i]``, ``n.a[i][j]`` for more
    dimensions, ``len()`` and iteration; a row of a multi-dimensional array
    is not assignable. With ``ref`` it is ``n._ref_a``, whose elements are
    pointers (NrnDoubleArrayElementRef). Elements are read and written in the
    array's row-major storage through nrn_symbol_dataptr.
    """

    __slots__ = ("_name", "_dims", "_prefix", "_ref")

    __reduce_ex__ = refuse_pickle

    def __init__(self, name, dims, prefix=(), ref=False):
        self._name = name
        self._dims = tuple(dims)
        self._prefix = tuple(prefix)
        self._ref = ref

    def _index(self, idx):
        size = self._dims[len(self._prefix)]
        i = _array_index(idx, size, "indices must be integers, not {}")
        if i is None:
            raise IndexError(self._name)
        return i

    def _is_row(self):
        return len(self._prefix) + 1 < len(self._dims)

    def __getitem__(self, idx):
        if isinstance(idx, slice):
            return [self[i] for i in range(*idx.indices(len(self)))]
        indices = self._prefix + (self._index(idx),)
        if self._is_row():
            return _DoubleArray(self._name, self._dims, indices, self._ref)
        flat = _flat_index(indices, self._dims)
        if self._ref:
            return NrnDoubleArrayElementRef(self._name, self._dims, flat)
        return _double_array_data(self._name, self._dims)[flat]

    def __setitem__(self, idx, value):
        indices = self._prefix + (self._index(idx),)
        if self._is_row():
            raise TypeError("Wrong number of subscripts")
        if self._ref:
            self[idx][0] = value
            return
        _double_array_data(self._name, self._dims)[
            _flat_index(indices, self._dims)
        ] = _as_double(value)

    def __len__(self):
        return self._dims[len(self._prefix)]

    def __iter__(self):
        for i in range(len(self)):
            yield self[i]

    def __repr__(self):
        if self._ref:
            return f"<incomplete pointer to hoc array {self._name}>"
        return self._name + "".join(f"[{i}]" for i in self._prefix) + "[?]"


def _double_array(name, ref=False):
    """A _DoubleArray for top-level VAR ``name``, or None if it is a scalar."""
    sym = _top_level_symbol(name.encode("utf-8"))
    if not sym or not _nrn_symbol_is_array(sym):
        return None
    return _DoubleArray(name, symbol_array_dims(sym), ref=ref)


# Delimit the captured strdef value in HOC `print` output. Assumed not to
# collide with any plausible model output.
_STR_BEGIN = "\x01_mn_str_begin_\x01"
_STR_END = "\x01_mn_str_end_\x01"
_DISTANCE_MISSING = builtins.object()


def _distance_location(arg):
    """A distance() location as NEURON's o2loc2 reads it (9.0's nrnpy_nrn.cpp).

    An object with a ``segment`` (an rxd node), or a one-element list of one
    (an rxd NodeList), gives that Segment. Anything else is passed on, for
    HOC's distance to accept or reject in NEURON's order.
    """
    from numbers import Number

    if arg is _DISTANCE_MISSING or isinstance(arg, (Number, Segment)):
        return arg
    item = arg[0] if isinstance(arg, list) and len(arg) == 1 else arg
    segment = getattr(item, "segment", None)
    return segment if isinstance(segment, Segment) else arg


def _capture_hoc_string(action):
    """Run *action* while capturing one sentinel-delimited HOC string."""
    captured = []

    def sink(stream, msg):
        captured.append(msg)

    # The sink is the process's: another thread's HOC output or capture while
    # it is installed would land here or take this one (gap-72).
    with HOC_LOCK:
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


def _kept_wrappers(old_ptrs, old_wrappers, ptrs):
    """allsec()'s wrapper list for the gathered *ptrs*: the wrappers made for
    the last gather (*old_ptrs*, *old_wrappers*), for the sections still in
    the model, and None for the rest.

    A wrapper holds its section (Section._from_ptr), so a pointer in both
    gathers is still that section. A cold allsec() then makes and references
    only new sections' wrappers. On 300 HOC sections (2026-11-19), a cold
    allsec() took about 150 ns per section with this, 940 re-making every
    wrapper and 620 before wrappers held their section.
    """
    # any() is a None test: a wrapper is a plain Section, which defines no
    # __bool__ or __len__. A list of Python sections only has no wrappers.
    if not old_wrappers or not any(old_wrappers):
        return [None] * len(ptrs)
    # Most regathers find the same sections (the epoch moved, nothing else).
    if old_ptrs == ptrs:
        return old_wrappers
    return list(map(dict(zip(old_ptrs, old_wrappers)).get, ptrs))


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


class _AllSecIterator:
    """What n.allsec() returns: an iterator with a __len__ that raises, as
    NEURON's "hoc all section iterator". rxd.Region(n.allsec()) relies on
    hasattr(secs, "__len__") to take it as a collection of sections.

    As NEURON's, each iteration starts over: rxd.Region iterates it once to
    check the sections and again to build its SectionList."""

    __slots__ = ("_items", "_source")

    __reduce_ex__ = refuse_pickle

    def __init__(self, source):
        self._source = source
        self._items = source()

    def __iter__(self):
        return self._source()  # a fresh generator: no per-item overhead

    def __next__(self):
        return next(self._items)

    def __len__(self):
        raise TypeError("hoc all section iterator() has no len()")

    def __bool__(self):
        return True  # as NEURON's; truth would otherwise call __len__

    def __repr__(self):
        return "<all section iterator next>"  # NEURON's repr


# This interface's own HOC names (trampolines, probe template), not for dir().
_HIDDEN = (
    "_mn_capture_cas",
    "_mn_pyjob",
    "_mn_type_probe_",
    "_MNTypeProbe",
    "_MnNil",
    "_mn_nilref",
    "_mn_setobj_",
    "_mn_objgrabidx",
    "_mn_objididx",
    "_mn_setobjidx",
    "_mn_hvget_",
    "_mn_hvset_",
    "_mn_rv_",
    "_mn_steer_",
    "_mn_ptr_",
    "_mn_alias",
    "_mn_insert_",
    "_mn_set_nseg",
    "_mn_ret_",
)


def _top_level_getattribute(self, name):
    """``h.__getattribute__(name)`` and ``type(h).__getattribute__(h, name)``.

    NEURON's HocObject answers HOC names in its C getattro, so the explicit
    call finds them (ModelDB 182129 calls ``neuron.h.__getattribute__(name)``).
    Here those names live in ``__getattr__``, which an explicit call skips.
    The call is reached without touching ordinary ``h.X`` access: the instance
    carries it in its own ``__dict__`` and ``_TopLevelMeta`` answers the type
    form, so the type's MRO has no ``__getattribute__`` entry (gap-66).
    """
    if not isinstance(name, str):
        raise TypeError(
            "attribute name must be a string (Note: <class 'TypeError'>: "
            "Neither Unicode or PyBytes)"
        )
    return getattr(self, name)


class _TopLevelMeta(_Singleton):
    def __getattribute__(cls, name):
        if name == "__getattribute__":
            return _top_level_getattribute
        return super().__getattribute__(name)


class NEURON(metaclass=_TopLevelMeta):
    # h.__doc__ is neuron.doc's text for the top-level HocObject.
    __doc__ = property(lambda self: _top_level_doc())
    # The default pickled the instance dict; loading it raised RecursionError.
    __reduce_ex__ = refuse_pickle

    def __init__(self):
        # name -> (type code, FuncWrapper) of a top-level function; see __getattr__.
        self._func_cache = {}
        self._nrn_hoc_call = _nrn_hoc_call
        # Instance entry, not a class one: gap-66 (a class entry slows every
        # h.X). A bound method, as ``h.__getattribute__`` is in NEURON.
        # Set through object.__setattr__, not ``self.__dict__[...]``: reading
        # __dict__ materialises it and can cost every later self.X read.
        builtins.object.__setattr__(
            self, "__getattribute__", types.MethodType(_top_level_getattribute, self)
        )
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
        # wrappers, so later iterations skip `_from_ptr`; a regather keeps
        # those of sections still in the model (_kept_wrappers).
        self._allsec_ptrs = None
        self._allsec_wrappers = None
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
        # A fresh scan: HOC defined after import (load_file, h('x = 1')) is
        # listed, as in NEURON. A rare call, so the walk costs nothing hot.
        self._top_level = list_functions()
        names = {name for name in self._top_level if not name.startswith(_HIDDEN)}
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
        names.update(name for name in dir(type(self)) if name.startswith("__"))
        return sorted(names)

    def __getattr__(self, name):
        # Top-level fork of NEURON's dispatch (the component fork is
        # Object.__getattr__). Real NEURON's type-code switch is
        # 9.0's nrnpy_hoc.cpp. Covered here:
        #   263 (VAR, subtype 2) — global doubles (t, dt, celsius)
        #   264/270/271/280 — callable functions/procedures (FuncWrapper)
        #   284/296 — OBFUNCTION / OBJECTFUNC, return wrapped Object
        #   295 — string-returning function
        #   311 — density mechanism (DensityMechanism)
        #   324 (OBJECTVAR) — bound Object, read with nrn_symbol_object_get
        #   325 — HOC class (returned as the dynamic subclass of Object;
        #         subclassable and indexable via _HocClassMeta)
        # RANGEOBJ is segment/object scoped and runtime-probed after a
        # RANDOM-bearing MOD is loaded; see gap-22.

        # A function's wrapper depends on its name and type code alone (the
        # symbol is looked up at each call), so it is kept: building one costs
        # more than the lookup it follows. A changed type code rebuilds it.
        cached = self._func_cache.get(name)
        if cached is not None:
            info = self._top_level.get(name)
            if info is not None and info[0] == cached[0]:
                # A new wrapper each time, as NEURON's (h.sin is not h.sin);
                # only the closure it calls is kept.
                return FuncWrapper(
                    cached[1],
                    name,
                    f"NEURON top-level function: n.{name}()",
                    doc_key=name,
                    key=name,
                )

        # Two passes: the cached _top_level, then a fresh list_functions() scan
        # for HOC symbols defined after import.
        for _ in [0, 1]:
            if name.startswith("_ref_"):
                base_name = name[5:]
                if base_name in self._top_level:
                    _type, _subtype = self._top_level[base_name]
                    if _type == TYPES.VAR and _subtype in (0, TYPES.USERDOUBLE):
                        array = _double_array(base_name, ref=True)
                        if array is not None:
                            return array
                        return NrnVarRef(base_name)
                    if _type == TYPES.STRING:
                        return NrnTopStrRef(base_name)
                    if _type == TYPES.VAR and _subtype == 1:
                        # NEURON returns the integer itself, not a pointer.
                        return getattr(self, base_name)
                    if _type == TYPES.RANGEVAR:
                        raise TypeError(
                            f"Cannot access {name} (NEURON type {_type}) directly."
                        )
                    if _type != TYPES.VAR:
                        raise TypeError(
                            f"Hoc pointer error, {base_name} is not a hoc "
                            "variable or range variable or strdef"
                        )
            else:
                if name in self._top_level:
                    _type, _subtype = self._top_level[name]
                    if _type == TYPES.VAR and _subtype in (
                        TYPES.USERINT,
                        TYPES.USERDOUBLE,
                    ):
                        # subtype 2 = USERDOUBLE (t, dt, celsius), subtype 1 =
                        # USERINT (stoprun, use_mcell_ran4). Both live at the
                        # symbol's u.pval, read as double* or int*.

                        sym = _top_level_symbol(name.encode("utf-8"))
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
                        return getattr(self._cas_wrapper(), name)
                    if _type == TYPES.VAR and _subtype == 0:
                        # NOTUSER (hocdec.h:82): runtime-created HOC scalar
                        # (`n('newvar = 42')`), stored in
                        # `hoc_top_level_data[sym->u.oboff].pval`, not at
                        # `sym->u.pval`; nrn_symbol_dataptr gives it (nrn#3815).
                        # Otherwise it is read via the hoc_ac_ trampoline.

                        sym0 = _top_level_symbol(name.encode("utf-8"))
                        if sym0 and _nrn_symbol_is_array(sym0):
                            return _double_array(name)
                        if sym0:
                            dptr = _nrn_symbol_dataptr(sym0)
                            if dptr:
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

                        bumps = _type != TYPES.BLTIN and name not in READ_ONLY_FUNCTIONS

                        @hoc_locked
                        def nrn_func(*args, sec=_NO_SEC):
                            if sec is _NO_SEC:
                                sec = None
                            elif sec is None:
                                raise TypeError("sec is not a Section")
                            if _type == TYPES.BLTIN:
                                if len(args) != 1:
                                    raise RuntimeError(
                                        "hocobj_call error: hoc_execerror: must be one "
                                        f"argument for {name}"
                                    )
                                # Only a number is sure to be off the stack
                                # when a BLTIN raises; see the rollback below.
                                if not isinstance(
                                    args[0], (int, float)
                                ) and not _is_hoc_number(args[0]):
                                    raise RuntimeError(
                                        f"hocobj_call error: hoc_execerror: {name} takes "
                                        f"a number, not {type(args[0]).__name__}"
                                    )
                            if sec is not None:
                                for arg in args:  # a loop: a generator costs ~0.4 us
                                    if isinstance(arg, Segment):
                                        raise ValueError(
                                            "Cannot specify both 'sec' and a Segment argument."
                                        )
                            # Liveness check before any push; see utils._push_args.
                            if sec is not None:
                                _check_sec_kwarg(sec)
                            # Recorded before each push; see
                            # object._construct_hoc_object.
                            pushed = [0]
                            pushed_sec = None
                            narg = len(args)
                            _err_buf = _ERR_BUF()
                            _err_buf[0] = b"\x01"  # the call clears it
                            try:
                                temp_strs, seg_sec = _utils._push_args(args, pushed)
                                use_sec = sec if sec is not None else seg_sec
                                if use_sec is not None:
                                    pushed_sec = [use_sec._sec]
                                    _nrn_section_push(pushed_sec[0])
                                sym = _nrn_symbol(name.encode("utf-8"))
                                if _type == TYPES.BLTIN and not CALLS_PYTHON:
                                    # A released core's BLTIN pops its argument
                                    # even when it fails (gap-93).
                                    pushed[0] = 0
                                ret = _nrn_function_call(
                                    sym, narg, _err_buf, _ERR_BUF_SIZE
                                )
                                # Any call may change model structure
                                # (delete_section, insert, user procs), so bump
                                # the epoch as NEURON.__call__ does. This also
                                # catches a warm allsec() after
                                # n.delete_section(sec=s), which may not move the
                                # head-link fingerprint. Not a math built-in or
                                # a call in utils.READ_ONLY_FUNCTIONS.
                                if bumps:
                                    _bump_model_epoch()
                                converted = TEMPLATE_CONVERTERS.get(name)
                                if converted is not None and len(args) > converted:
                                    forget_class(str(args[converted]))
                                if ret and (_type != TYPES.BLTIN or CALLS_PYTHON):
                                    # Nothrow restores the stack to after the
                                    # arg push; release those slots (gap-56).
                                    # On a released core a failed BLTIN has
                                    # already popped its one argument, so the
                                    # restore cannot run ("oc_restore_code
                                    # cannot summon ...", gap-93) and nothing
                                    # is left to release.
                                    _utils._pop_args(pushed)
                                if ret:
                                    failed, _err_buf = _err_buf, None  # no result
                                    _check_nrn_error(ret, failed)
                                # Each branch marks the result popped right
                                # before its raw pop: on Python 3.10 a taken
                                # jump can take an interrupt, and a Python pop
                                # wrapper's entry can too (gap-84).
                                if _type in (
                                    TYPES.BLTIN,
                                    TYPES.FUNCTION,
                                    TYPES.FUN_BLTIN,
                                ):
                                    _err_buf = None
                                    result = _nrn_double_pop()
                                elif _type == TYPES.STRINGFUNC:
                                    _err_buf = None
                                    result = _decode_hoc_string(_nrn_str_pop0())
                                elif _type == TYPES.PROCEDURE:
                                    # hoc_procret pushes a sentinel 0.0
                                    # (oc/code.cpp:1540, "will be popped
                                    # immediately"). The HOC parser pops it for
                                    # statements; API-level calls must pop it
                                    # here. Otherwise every continuerun/run/
                                    # step/advance leaks a slot and the
                                    # 1000-deep operand stack (hoc_nstack,
                                    # oc/code.cpp:422) overflows after ~1000
                                    # calls. Covered by tests/stress/. Legacy
                                    # returns the popped 0.0.
                                    _err_buf = None
                                    result = _nrn_double_pop()
                                elif _type in (TYPES.OBJECTFUNC, TYPES.OBFUNCTION):
                                    # OBJECTFUNC (296) = C built-in obfunc.
                                    # OBFUNCTION (284) = HOC `obfunc` definition.
                                    # Both push the result onto the object stack.
                                    _err_buf = None
                                    result = _wrap_owned_object(_nrn_object_pop())
                                else:
                                    _err_buf = None
                                    result = None
                                # After the pop above, or a failed callback
                                # leaks its slot.
                                if PENDING_CALLBACKS:
                                    raise_callback_error()
                                return result
                            except BaseException:
                                _drop_return_slot(
                                    _err_buf, _return_slot_pop(_type), pushed
                                )
                                raise
                            finally:
                                if pushed_sec:
                                    try:
                                        _pop_pushed_section(pushed_sec)
                                    finally:
                                        if pushed_sec:  # interrupted before the pop
                                            _pop_pushed_section(pushed_sec)

                                if not CALLS_PYTHON:
                                    _hoc_unref_defer()

                        self._func_cache[name] = (_type, nrn_func)
                        return FuncWrapper(
                            nrn_func,
                            name,
                            f"NEURON top-level function: n.{name}()",
                            doc_key=name,
                            key=name,
                        )
                    elif _type == TYPES.TEMPLATE:
                        # Return the dynamic class: callable (``n.IClamp(seg)``
                        # runs Object.__init__'s construction path), indexable
                        # (``n.IClamp[0]`` via _HocClassMeta.__getitem__) and
                        # subclassable without neuron.hclass.
                        return hoc_class(name)

                    elif _type == TYPES.MECHANISM:
                        return density_mechanism(name)

                    elif _type == TYPES.OBJECTVAR:
                        # Real NEURON (9.0's nrnpy_hoc.cpp) uses the
                        # OBJECTVAR bytecode opcode, which ctypes cannot
                        # invoke. Use a cached per-name obfunc instead.
                        return _read_hoc_objectvar(self, name)

                    elif _type == TYPES.STRING:
                        # Not readable via _nrn_symbol_dataptr in NEURON 9
                        # (data_handle).
                        value = _nrn_symbol_str_get(
                            _top_level_symbol(name.encode("utf-8"))
                        )
                        return "" if value is None else value.decode("utf-8", "replace")

                    elif _type == TYPES.SECTION:
                        # `create soma` (scalar) or `create dend[N]` (array).
                        # Uses the brace capture of template sections with no
                        # owner prefix; see object._resolve_template_section.

                        sym = _top_level_symbol(name.encode("utf-8"))
                        if not sym:
                            raise AttributeError(f"Section '{name}' not found.")
                        if _nrn_symbol_is_array(sym):
                            return _TemplateSectionArray(None, name)
                        return _resolve_template_section(None, name, idx=None)

            # update the function list before giving up
            # TODO(gap-86): look the name up in the symbol table instead; this
            # walk makes hasattr(n, name) cost about half a millisecond.
            self._top_level = list_functions()

        # h is NEURON's HocObject; the second object n keeps its own type name.
        tname = "hoc.HocObject" if type(self) is NEURON else type(self).__name__
        raise _name_error(
            AttributeError,
            name,
            f"'{tname}' object has no attribute '{name}'",
        )

    def __setattr__(self, name, value):
        # Internal state (`Section`, `hclass`, every underscore-prefixed name)
        # is a real attribute. Every other name is a top-level HOC variable and
        # is written to HOC, as in real NEURON, rather than shadowed by a Python
        # attribute (`n.s = "x"` must set the strdef).
        if name.startswith("_") or name in ("Section", "hclass"):
            super().__setattr__(name, value)
            return

        if name in self.__dict__:
            # A name n holds as a Python attribute stays one when HOC declares
            # it later, as in NEURON (an underscore name is always one here;
            # NEURON writes one that HOC declared to HOC).
            super().__setattr__(name, value)
            return

        info = self._top_level.get(name)
        if info is None:
            # The var may have been declared after the cache was built;
            # refresh once (mirrors the two-pass lookup in __getattr__).
            self._top_level = list_functions()
            info = self._top_level.get(name)
        if info is None:
            return self._set_undeclared(name, value)

        _type, _subtype = info

        # Built-in double globals (dt, celsius, t): direct pointer write.
        if _type == TYPES.VAR and _subtype in (TYPES.USERINT, TYPES.USERDOUBLE):
            sym = _top_level_symbol(name.encode("utf-8"))
            if not sym:
                raise AttributeError(f"Variable '{name}' not found.")
            value_ptr = _nrn_symbol_dataptr(sym)
            if not value_ptr:
                raise AttributeError(f"Could not get data pointer for '{name}'.")
            if _subtype == TYPES.USERINT:
                ctypes.cast(value_ptr, ctypes.POINTER(ctypes.c_int))[0] = int(value)
            else:
                ctypes.cast(value_ptr, ctypes.POINTER(ctypes.c_double))[0] = _as_double(
                    value
                )
            return

        # Section-level property (L, nseg, Ra, rallbranch): write the accessed
        # section's value; cas() raises TypeError if none. Must precede the
        # generic VAR branch, which would assign a non-existent scalar via HOC.
        if _type == TYPES.VAR and _subtype == TYPES.USERPROPERTY:
            setattr(self._cas_wrapper(), name, value)
            return

        # Runtime double scalar (subtype 0, `n('x = ...')`).
        if _type == TYPES.VAR:
            sym0 = _top_level_symbol(name.encode("utf-8"))
            if sym0 and _nrn_symbol_is_array(sym0):
                # `double a[4]`: NEURON assigns elements only (h.a[i] = x).
                raise TypeError("Wrong number of subscripts")
            if sym0:
                dptr = _nrn_symbol_dataptr(sym0)
                if dptr:
                    dptr[0] = _as_double(value)
                    return
            # Fallback: assign through HOC. repr(float(...)) round-trips exactly.
            self("%s = %s" % (name, repr(_as_double(value))))
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
        # None re-nils it.
        if _type == TYPES.OBJECTVAR:
            _sym = _top_level_symbol(name.encode("utf-8"))
            if _sym and _nrn_symbol_is_array(_sym):
                # An objref array needs an index (n.g[i] = obj, via
                # _ObjrefArray). A scalar assign would write element 0
                # (n.g = obj) or collapse the whole array (n.g = None). Real
                # NEURON rejects it with a dimensionality error.
                raise IndexError(
                    f"'{name}' is an object array; assign an element "
                    f"(n.{name}[i] = ...), not the whole array"
                )
            # A reference is not a HOC object; NEURON leaves the objref nil.
            # Assigned, not `objref name`: that frees the storage of the objref,
            # which HOC may be using (name.method() calls Python).
            if value is None or isinstance(value, NrnRef):
                if "_mn_nilref" not in self._top_level:
                    self("objref _mn_nilref")
                    self._top_level = list_functions()
                self("%s = _mn_nilref" % name)
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

        # The entry may be older than the name's declaration.
        self._top_level = list_functions()
        if self._top_level.get(name) != info:
            setattr(self, name, value)
            return
        # Functions, mechanisms, templates, sections: not assignable.
        raise TypeError("not assignable")  # NEURON's text

    # NEURON's h == n, and their hashes agree.
    def __eq__(self, other):
        return isinstance(other, NEURON)

    def __hash__(self):
        return hash(NEURON)

    def _set_undeclared(self, name, value):
        # h raises, as NEURON's does, rather than shadow the name with a Python
        # attribute; n overrides this.
        raise _name_error(
            LookupError, name, f"'{name}' is not a defined hoc variable name."
        )

    def __repr__(self):
        return "<TopLevelHocInterpreter>"  # NEURON's name for h

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
            if isinstance(value, str) and not value.isascii():
                raise TypeError("string arg must have only ascii characters")
            return NrnStrRef(value)
        return NrnObjectRef(value)

    def setpointer(self, source_ref, pointer_name, target):
        """Assign a numeric NEURON reference to an NMODL POINTER target."""

        usage = (
            "setpointer(_ref_hocvar, 'POINTER_name', point_process or " "nrn.Mechanism)"
        )
        numeric_refs = (
            NrnArrayPropertyRef,
            NrnDoubleRef,
            NrnObjectPropertyRef,
            NrnRangeVarRef,
            NrnVarRef,
            NrnVectorElementRef,
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
            target._check_located()
        elif isinstance(target, Mechanism):
            target._seg._sec._check_alive()
        else:
            raise TypeError(usage)

        try:
            target._assign_pointer(pointer_name, source_ref)
        except (NameError, RuntimeError):
            raise TypeError(usage) from None
        return None

    @hoc_locked
    def __call__(self, cmd, *, sec=_NO_SEC, raise_on_error=False, _no_epoch_bump=False):
        # Returns True on success, False on a HOC execution error. Like real
        # NEURON's h(), HOC errors print to stderr and do not raise: ModelDB
        # init.hoc files trigger warnings real NEURON swallows.
        # raise_on_error=True raises RuntimeError. See docs/ARCHITECTURE.md
        # "Deliberate departures from h() behavior".

        if not isinstance(cmd, str):
            got = "None" if cmd is None else type(cmd).__name__
            raise TypeError(f"argument 1 must be str, not {got}")
        pushed_sec = None
        if sec is not _NO_SEC:
            _check_sec_kwarg(sec)  # Before pushing it; None is refused.
        try:
            if sec is not _NO_SEC:
                pushed_sec = [sec._sec]
                _nrn_section_push(pushed_sec[0])
            cmd = cmd.encode("utf-8")
            ret = self._nrn_hoc_call(cmd)
            # See api.EPOCH. Bump even on failure: the command may have
            # partly mutated. _no_epoch_bump is internal, for fixed-shape
            # commands that cannot change topology or mechanisms (the seg.diam
            # write); bumping there would invalidate allsec() caches on every
            # write (216 us vs 21 us over 300 sections).
            # Measurements: benchmarks/README.md#historical-implementation-measurements.
            if not _no_epoch_bump:
                _bump_model_epoch()
            # A Python callback that failed inside the statement is a HOC
            # error of the statement, as in NEURON: h() prints it and returns
            # False (NEURON's test_pycallobject), and raise_on_error raises it.
            try:
                if ret or PENDING_CALLBACKS:
                    raise_callback_error()
            except Exception as exc:
                if raise_on_error:
                    raise
                import traceback

                traceback.print_exception(exc)
                return False
            if ret != 0 and raise_on_error:
                raise RuntimeError(f"HOC execution failed: {cmd!r}")
            return ret == 0
        finally:
            if pushed_sec:
                try:
                    _pop_pushed_section(pushed_sec)
                finally:
                    if pushed_sec:  # interrupted before the pop
                        _pop_pushed_section(pushed_sec)

    def cas(self):
        """Return the currently accessed Section.

        Matches real NEURON's `h.cas()`: raises ``TypeError`` if no
        section is on the access stack
        (``9.0's nrnpy_nrn.cpp``).
        """
        # Not between another thread's section push and pop (gap-72).
        with HOC_LOCK:
            sec_ptr = _nrn_cas()
        if not sec_ptr:
            raise TypeError("Section access unspecified")
        return _section_for(sec_ptr)

    def _cas_wrapper(self):
        """The accessed section as a plain wrapper, for the interface's own
        reads and writes: a Section subclass's attributes, properties and
        __setattr__ can't change them (NEURON does these in C)."""
        with HOC_LOCK:  # as in cas()
            sec_ptr = _nrn_cas()
        if not sec_ptr:
            raise TypeError("Section access unspecified")
        return Section._from_ptr(sec_ptr)

    @hoc_locked
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

        A location may also be anything with a ``segment``, such as an rxd
        node, or a one-element list of one (an rxd NodeList), as in NEURON.

        The two-segment form calls nrn_distance directly because _push_args
        overwrites seg_sec and would lose the first segment's section.
        """

        if kwargs:
            raise RuntimeError("invalid keyword argument")
        arg1 = _distance_location(arg1)
        arg2 = _distance_location(arg2)
        explicit_sec = sec is not _DISTANCE_MISSING
        if explicit_sec:
            _check_sec_kwarg(sec)

        if (
            arg1 is not _DISTANCE_MISSING
            and arg2 is not _DISTANCE_MISSING
            and isinstance(arg1, Segment)
            and isinstance(arg2, Segment)
        ):
            # The direct nrn_distance path skips HOC dispatch's deleted-section
            # checks: a segment of a section deleted through HOC would
            # dereference freed nodes and SIGABRT. Raise NEURON's HOC error.
            _check_segment_arg(arg1)
            _check_segment_arg(arg2)
            # Its topol_distance sets the tree up at any x (solve.cpp:180):
            # node_exact does at one of a section's two ends.
            try:
                for end in (0.0, 1.0):
                    _ensure_topology(arg1.sec._sec, end)
            except RuntimeError as exc:
                # NEURON's h.distance is a HOC call, whose errors hocobj_call
                # prefixes.
                raise RuntimeError(f"hocobj_call error: {exc}") from None
            return float(_nrn_distance(arg1.sec._sec, arg1.x, arg2.sec._sec, arg2.x))

        # Legacy shapes call the HOC `distance` function directly, avoiding a
        # second __getattr__ lookup on a hot path.
        args = tuple(arg for arg in (arg1, arg2) if arg is not _DISTANCE_MISSING)
        sym = _nrn_symbol(b"distance")
        if not sym:
            raise RuntimeError("HOC 'distance' symbol not found")
        # Recorded before each push; see object._construct_hoc_object.
        pushed = [0]
        pushed_sec = None
        narg = len(args)
        err = _ERR_BUF()
        err[0] = b"\x01"  # the call clears it
        try:
            temp_strs, seg_sec = _utils._push_args(args, pushed)
            call_sec = (
                seg_sec if seg_sec is not None else (sec if explicit_sec else None)
            )
            if call_sec is not None:
                pushed_sec = [call_sec._sec]
                _nrn_section_push(pushed_sec[0])
            ret = _nrn_function_call(sym, narg, err, _ERR_BUF_SIZE)
            if ret:
                # Release the arg slots nothrow left on the stack (gap-56).
                _utils._pop_args(pushed)
                failed, err = err, None
                _check_nrn_error(ret, failed)
            err = None
            return _nrn_double_pop()
        except BaseException:
            _drop_return_slot(err, _nrn_double_pop, pushed)
            raise
        finally:
            if pushed_sec:
                try:
                    _pop_pushed_section(pushed_sec)
                finally:
                    if pushed_sec:  # interrupted before the pop
                        _pop_pushed_section(pushed_sec)

            if not CALLS_PYTHON:
                _hoc_unref_defer()

    def _gather_section_ptrs(self):
        """Return the list of Section pointers of every live section.

        Runs only on a cache miss. ``nrn_sectionlist_to_array`` (nrn#3834)
        snapshots the list in two FFI crossings (count, then fill). The
        c_void_p restype already yields Python ints. The list-vs-array reason
        is in NEURON.__init__.
        """
        items = _nrn_allsec()
        total = _nrn_sectionlist_to_array(items, None, 0)
        if total <= 0:
            return []
        buf = (ctypes.c_void_p * total)()
        # Under the held GIL (PyDLL), nothing mutates the list between calls.
        got = _nrn_sectionlist_to_array(items, buf, total)
        return buf[: min(got, total)]

    def allsec(self):
        """Iterate over all sections currently in the model.

        Yields non-owning Section wrappers. A cached list of section pointers
        is reused while the topology is stable and regathered when it changes.

        NEURON ends the model epoch when a section is made or deleted, and so
        does everything that can change the model unseen (api.EPOCH): the list
        is good while that has not moved. (A released core does not, so its
        section list itself is looked at too.)

        Sections that the loop makes before its last are handed out too, as
        NEURON's list has them. A section deleted during the loop (by the loop, by HOC, or
        by the garbage collector freeing a Python section or a template object)
        is left out: once the epoch moves, each pointer is checked before it is
        handed out.
        """
        return _AllSecIterator(self._allsec_items)

    def _allsec_pointers(self):
        """(section pointers, wrappers of them, epoch) as of now, cached."""
        epoch = _model_epoch()
        version = epoch if CALLS_PYTHON else (epoch, _section_list_version())
        ptrs = self._allsec_ptrs
        if ptrs is None or self._allsec_version != version:
            new_ptrs = self._gather_section_ptrs()
            self._allsec_wrappers = _kept_wrappers(
                ptrs, self._allsec_wrappers, new_ptrs
            )
            self._allsec_ptrs = ptrs = new_ptrs
            self._allsec_version = version
        return ptrs, self._allsec_wrappers, epoch

    def _allsec_items(self):
        _from_ptr = Section._from_ptr
        owner_refs = PYTHON_SECTIONS
        ptrs, wrappers, epoch = self._allsec_pointers()
        # Once the epoch moves during the loop, a later pointer may be a
        # deleted section's, so it is checked before it is handed out. NEURON's
        # own iterator reads the freed list item there and can crash. A loop
        # that changes nothing pays one comparison.
        current = EPOCH
        handed_out = set()  # when the loop has made sections

        while True:
            made, last = None, len(ptrs) - 1
            for idx, ptr in enumerate(ptrs):
                if current[0] != epoch:
                    if idx == last:
                        # NEURON's iterator is at the list's end when it hands
                        # out the last: what was made before comes after it.
                        all_ptrs, _, epoch = self._allsec_pointers()
                        handed_out.update(ptrs)
                        made = [p for p in all_ptrs if p not in handed_out]
                    if not _nrn_section_is_active(ptr):
                        continue

                # A Python section's own object, as NEURON gives: a wrapper
                # would not keep a section that is otherwise garbage alive for
                # the loop.
                owner_ref = owner_refs.get(ptr)
                w = owner_ref() if owner_ref is not None else None
                if w is None:
                    w = wrappers[idx]
                    if w is None:
                        # A HOC call still running may have deleted it without
                        # moving the epoch; its struct must not be referenced.
                        if not _nrn_section_is_active(ptr):
                            continue
                        w = _from_ptr(ptr)
                        wrappers[idx] = w
                yield w
            if not made:
                return
            # This loop's own: its positions are not the cache's.
            ptrs, wrappers = made, [None] * len(made)

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


class _NEURON_INTERFACE(NEURON):
    # NEURON's ``n``: a second top-level object, as ``neuron.n`` is there. Its
    # repr differs, and an undeclared name is stored as a plain Python attribute
    # of n (not a HOC variable, not visible through h). It has h's __doc__;
    # NEURON's n has its own text.
    __doc__ = NEURON.__dict__["__doc__"]

    def _set_undeclared(self, name, value):
        super(NEURON, self).__setattr__(name, value)

    def __repr__(self):
        return "<TopLevelNEURONInterface>"


# _n is the package's own reference: a user can rebind n or h (n = something).
n = _NEURON_INTERFACE()
h = _n = NEURON()


# ── Module initialization sequence ──
# discover() ran at api.py import for top-level codes. This fills in the codes
# that need a live HOC session (PROCEDURE, SECTION, OBFUNCTION, METHOD_OBFUNC,
# METHOD_STRFUNC, via a probe template and classes).
TYPES.discover_template_scoped(h)
h._rebuild_function_types()
n._rebuild_function_types()
TYPES.probe_section_ref()
TYPES.validate()


_SUBMODULES = []  # this package's submodule names, filled on first use


def __getattr__(name):
    """``myneuron.<HOC class>``, which is how pickle finds a HOC class.

    Standalone, a HOC class's ``__module__`` names this package (in NEURON it
    is ``hoc``, whose facade finds the class). Python calls this only for a
    name the package does not have, so a HOC class named like one of the
    package's globals or submodules never replaces it (and cannot be pickled
    by name). A submodule not imported yet is not a HOC class either: ``from
    . import plotting`` must import it.
    """
    if not name.startswith("__"):
        if not _SUBMODULES:
            import pkgutil

            _SUBMODULES.extend(module.name for module in pkgutil.iter_modules(__path__))
        sym = None if name in _SUBMODULES else _nrn_symbol(name.encode("utf-8"))
        if sym and _nrn_symbol_type(sym) == TYPES.TEMPLATE:
            value = getattr(n, name)
            if isinstance(value, type):
                return value
    raise AttributeError(f"module {__name__!r} has no attribute {name!r}")
