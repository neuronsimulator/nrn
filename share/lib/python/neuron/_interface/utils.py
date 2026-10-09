"""Helpers: FuncWrapper, symbol table iteration, argument pushing."""
import ctypes

from ._docs import get_doc
from .api import (
    TYPES,
    _ensure_pyobj_to_hoc,
    _nrn_double_pop,
    _nrn_double_push,
    _nrn_global_symbol_table,
    _nrn_object_push,
    _nrn_pntproc_nmodlrandom_get,
    _nrn_segment_nmodlrandom_get,
    _nrn_str_pop,
    _nrn_str_push,
    _nrn_symbol,
    _nrn_symbol_name,
    _nrn_symbol_subtype,
    _nrn_symbol_table,
    _nrn_symbol_table_iterator_done,
    _nrn_symbol_table_iterator_free,
    _nrn_symbol_table_iterator_new,
    _nrn_symbol_table_iterator_next,
    _nrn_symbol_type,
    _nrn_top_level_symbol_table,
    _object_pop_safe,
    _owned_pythonobject_to_python,
    _py_pyobj_to_hoc_cfunctype,
)
from .nrnref import (
    NrnRef,
)


class FuncWrapper:
    # __doc__ is a property backed by the _doc slot (a class variable and a
    # same-named slot would clash). Lookup is deferred to avoid help I/O
    # during core probes. FuncWrapper.__doc__ on the class is the property
    # object, not help text.
    __slots__ = ("_func", "_name", "_doc", "_doc_key", "_fallback_doc")

    def __init__(self, func, name, doc="", *, doc_key=None):
        self._func = func
        self._name = name
        self._doc = doc if doc_key is None else None
        self._doc_key = doc_key
        self._fallback_doc = doc

    @property
    def __doc__(self):
        if self._doc is None and self._doc_key is not None:
            self._doc = get_doc(self._doc_key, self._fallback_doc)
        return self._doc

    @__doc__.setter
    def __doc__(self, value):
        self._doc = value
        self._doc_key = None

    def __call__(self, *args, **kwargs):
        return self._func(*args, **kwargs)

    def __repr__(self):
        return f"{self._name}()"

    def __str__(self):
        return f"{self._name}()"


class _Singleton(type):
    # Standard singleton metaclass. Used by NEURON (see __init__.py).
    _instances = {}

    def __call__(cls, *args, **kwargs):
        if cls not in cls._instances:
            cls._instances[cls] = super(_Singleton, cls).__call__(*args, **kwargs)
        return cls._instances[cls]


def _list_symbol_table(symtab):
    """
    Return a dict of all symbols in the given symbol table.
    Keys are symbol names, values are (type, subtype) tuples.
    """
    entries = {}
    it = _nrn_symbol_table_iterator_new(symtab)
    while not _nrn_symbol_table_iterator_done(it):
        sym = _nrn_symbol_table_iterator_next(it)
        name = _nrn_symbol_name(sym)
        if isinstance(name, bytes):
            name = name.decode("utf-8")
        type_ = _nrn_symbol_type(sym)
        subtype = _nrn_symbol_subtype(sym)
        entries[name] = (int(type_), int(subtype))
    _nrn_symbol_table_iterator_free(it)
    return entries


# Object method-table cache, keyed by HOC class name.
#
# WHY: real NEURON caches the equivalent table in its CPython type objects.
# Walking the symbol table on every Object construction was the largest
# cost in IClamp/Vector creation. Historical numbers:
# benchmarks/README.md#historical-implementation-measurements.
# INVALIDATES: only the late-definition case below. An already-loaded
# template can't be redefined (NEURON raises an execerror on redefinition),
# so its method-table entry is stable for the process lifetime.
# TODO(gap-46): if a template name is first queried before the template is
# defined, the cache holds a stale empty entry for it. In practice templates
# are fully defined before first use.
_CLASS_METHODS_CACHE = {}


def list_methods(obj):
    """
    Return a dict of all methods for the given NEURON object/class.
    Keys are method names, values are (type, subtype) tuples.
    """
    # Accept either a class name (str) or an Object instance
    if isinstance(obj, str):
        class_name = obj
    else:
        class_name = obj._class_name
    cached = _CLASS_METHODS_CACHE.get(class_name)
    if cached is not None:
        return cached
    sym = _nrn_symbol(class_name.encode("utf-8"))
    table = _nrn_symbol_table(sym)
    methods = _list_symbol_table(table)
    _CLASS_METHODS_CACHE[class_name] = methods
    return methods


def list_functions():
    """
    Return a dict of all top-level NEURON functions and their types.
    Keys are function/variable names, values are (type, subtype) tuples.
    """
    entries = {}
    for symtab_func in (_nrn_global_symbol_table, _nrn_top_level_symbol_table):
        symtab = symtab_func()
        entries.update(_list_symbol_table(symtab))
    return entries


class _PythonObjectArg:
    """Force object-valued marshaling when the destination is a HOC objref."""

    __slots__ = ("value",)

    def __init__(self, value):
        self.value = value


def _new_python_object(value):
    """Return one creator-owned PythonObject reference from the active provider."""
    obj_ptr = _py_pyobj_to_hoc_cfunctype(value)
    if obj_ptr is None:
        po2ho = _ensure_pyobj_to_hoc()
        if po2ho is None:
            raise TypeError(
                "Cannot push Python object as HOC Object: "
                "no matching PythonObject provider available"
            )
        obj_ptr = po2ho(value)
    return obj_ptr


def _is_python_object_arg(value):
    # Preserve numeric coercion for scalar protocols, but not containers
    # (notably singleton numpy arrays, which float() can otherwise collapse).
    kind = type(value)
    return (
        isinstance(value, _PythonObjectArg)
        or callable(value)
        or hasattr(kind, "__len__")
        or not (hasattr(kind, "__float__") or hasattr(kind, "__index__"))
    )


def _push_args(args, return_rollback=False):
    """Push positional args onto the HOC stack in order.

    Two passes. Pass 1 resolves every argument into a typed (kind, payload)
    descriptor. It runs every conversion that can fail (``float()``,
    ``.encode()``, PythonObject wrapping) and raises on a dead Segment
    section, with no HOC-stack side effects. Pass 2 pushes the descriptors.
    If a reference's liveness guard rejects its push (liveness can change
    between passes), the earlier pushes are rolled back.

    Conversions must finish before pushing, or a failure would orphan
    operand-stack slots. Repeated leaks overflow HOC ("Stack too deep"),
    whose ``neuron::oc::runtime_error`` can cross ctypes and
    ``std::terminate`` the process.

    Returns (temp_strs, seg_sec), or (temp_strs, seg_sec, rollback) when
    ``return_rollback`` is true:
      temp_strs — ctypes string objects that must stay alive until the call completes.
      seg_sec   — Section extracted from any Segment argument (for section-context
                  calls); None if no Segment was in args.
      rollback  — pop the caller arguments restored by a failed nothrow call,
                  releasing their object references. Never call after success:
                  the callee consumed the arguments, even if a pending Python
                  callback exception is raised afterwards.
    """
    from .api import _nrn_object_unref
    from .object import Object
    from .sections import Segment

    temp_strs = []
    seg_sec = None
    descriptors = []  # (kind, payload); object kinds are borrowed or owned

    # PASS 1: resolve/validate only, no pushes.
    try:
        for arg in args:
            if isinstance(arg, Segment):
                # A Segment contributes its x and supplies section context.
                arg.sec._check_alive()
                seg_sec = arg.sec
                descriptors.append(("double", float(arg.x)))
            elif arg is None:
                descriptors.append(("object", None))
            elif isinstance(arg, (int, float)):
                # bool is an int subclass, so it follows HOC's double path.
                descriptors.append(("double", float(arg)))
            elif isinstance(arg, Object):
                descriptors.append(("object", arg._obj))
            elif isinstance(arg, NrnRef):
                descriptors.append(("ref", arg))
            elif isinstance(arg, (str, bytes)):
                b = arg.encode("utf-8") if isinstance(arg, str) else arg
                c_arg = ctypes.c_char_p(b)
                temp_strs.append(c_arg)
                descriptors.append(("str", ctypes.pointer(c_arg)))
            elif _is_python_object_arg(arg):
                if isinstance(arg, _PythonObjectArg):
                    arg = arg.value
                descriptors.append(("owned_object", _new_python_object(arg)))
            else:
                # numpy integer scalars, Decimal, Fraction, and other numeric
                # values reach this coercion path; HOC stores them as doubles.
                try:
                    d = float(arg)
                except (TypeError, ValueError):
                    raise TypeError(
                        f"Cannot pass argument of type {type(arg).__name__} to a "
                        "HOC function: numeric scalar conversion to double failed."
                    ) from None
                descriptors.append(("double", d))
    except BaseException:
        for kind, payload in descriptors:
            if kind == "owned_object" and payload:
                _nrn_object_unref(payload)
        raise

    # PASS 2: push the validated descriptors. Rollback pops completed
    # pushes newest-first by recorded kind; the failing ref pushed nothing.
    def rollback(count):
        for kind, payload in reversed(descriptors[:count]):
            if kind == "double":
                _nrn_double_pop()
            elif kind in ("object", "owned_object"):
                obj = _object_pop_safe()
                if obj:
                    _nrn_object_unref(obj)
            elif kind == "str":
                _nrn_str_pop()
            else:
                payload._pop()

    pushed = 0
    released_owned = 0
    try:
        for kind, payload in descriptors:
            if kind == "double":
                _nrn_double_push(payload)
            elif kind in ("object", "owned_object"):
                _nrn_object_push(payload)
                if kind == "owned_object" and payload:
                    _nrn_object_unref(payload)
                    released_owned += 1
            elif kind == "str":
                _nrn_str_push(payload)
            else:  # ref
                payload._push()
            pushed += 1
    except BaseException:
        rollback(pushed)
        to_release = released_owned
        for kind, payload in descriptors:
            if kind != "owned_object" or not payload:
                continue
            if to_release:
                to_release -= 1
            else:
                _nrn_object_unref(payload)
        raise
    if return_rollback:
        return temp_strs, seg_sec, lambda: rollback(len(descriptors))
    return temp_strs, seg_sec


def _wrap_owned_object(obj):
    """Consume an owned HOC ``Object*`` into its typed Python wrapper.

    The reference is consumed even if class resolution or wrapper
    initialization fails. Once ``cls._wrap`` starts, the partial wrapper owns
    the reference and its finalizer releases it.
    """
    from .api import _nrn_class_name, _nrn_object_unref

    if not obj:
        return None
    try:
        name = _nrn_class_name(obj).decode("utf-8")
    except BaseException:
        _nrn_object_unref(obj)
        raise

    if name == "PythonObject":
        # The converter consumes ownership even on failure; keep it outside
        # the class-wrapper cleanup so the reference cannot be released twice.
        return _owned_pythonobject_to_python(obj)

    try:
        # Mint the class if no one has asked for it via n.<ClassName> yet.
        from . import _dynamic_classes
        from .object import Object

        cls = _dynamic_classes.get(name)
        if cls is None:
            if name == "":
                cls = Object
            else:
                cls = type(
                    name,
                    (Object,),
                    {
                        "__module__": __package__,
                        "__slots__": (),
                        "_hoc_class_name": name,
                    },
                )
                _dynamic_classes[name] = cls
                from . import object as _obj_mod

                setattr(_obj_mod, name, cls)
                import sys

                setattr(sys.modules[__package__], name, cls)
    except BaseException:
        _nrn_object_unref(obj)
        raise
    return cls._wrap(obj)


def _try_wrap_density_nmodlrandom(sec, x, sym, sym_type):
    """Return an NMODLRandom wrapper, or None when ``sym`` is not one.

    See api.TypeCodes.record_rangeobj for why this is probed at runtime.
    """
    raw = _nrn_segment_nmodlrandom_get(sec, x, sym)
    if not raw:
        return None
    TYPES.record_rangeobj(sym_type)
    return _wrap_owned_object(raw)


def _try_wrap_point_nmodlrandom(obj, sym, sym_type):
    """Return a point-process NMODLRandom wrapper, or None if not applicable."""
    raw = _nrn_pntproc_nmodlrandom_get(obj, sym)
    if not raw:
        return None
    TYPES.record_rangeobj(sym_type)
    return _wrap_owned_object(raw)


def _object_pop():
    """Pop the top HOC Object from the stack and return a typed Python wrapper.

    Returns ``None`` for a nil HOC object (an uninitialized ``objref``, or an
    obfunc/method that yields nil, e.g. ``NetCon(None, syn).precell()``), as
    real NEURON does.

    ``nrn_object_pop`` returns an owned reference for a non-nil object;
    ``_init_from_ptr`` takes ownership of it.
    """
    obj = _object_pop_safe()
    return _wrap_owned_object(obj)
