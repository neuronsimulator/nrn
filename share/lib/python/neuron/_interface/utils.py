"""Helpers: FuncWrapper, symbol table iteration, argument pushing."""
import ctypes
import operator
from collections.abc import Mapping

from ._docs import get_doc
from .api import (
    _ERR_BUF,
    _ERR_BUF_SIZE,
    CALLS_PYTHON,
    HOC_CLASS_MODULE,
    TYPES,
    _check_nrn_error,
    _new_python_object,
    _ensure_topology,
    _nrn_class_name,
    _nrn_double_pop,
    _nrn_double_push,
    _nrn_function_call,
    _nrn_global_symbol_table,
    _nrn_method_symbol,
    _nrn_object_push,
    _nrn_pntproc_nmodlrandom_get,
    _nrn_section_is_active,
    _nrn_section_pop,
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
    _PACKAGE,
)
from . import api as _api
from .nrnref import (
    NrnRef,
    refuse_pickle,
)


# NEURON's error for a deleted section (nrnpy_sec_referr, 9.0's nrnpy_nrn.cpp). An
# assignment through a Segment, Mechanism or RangeVar puts the type in front:
# "nrn.Segment can't access a deleted section".
_DELETED_SECTION = "can't access a deleted section"


def _check_segment_arg(seg):
    """Raise NEURON's HOC error for a Segment of a deleted section passed to a
    HOC call (9.0's nrnpy_hoc.cpp), before anything is pushed: the core would read
    the freed section and abort."""
    if not _nrn_section_is_active(seg._sec._sec):
        raise RuntimeError("nrn.Segment associated with deleted internal Section")


# The keywords a HOC call takes (the others are refused by its signature).
_CALL_KEYWORDS = frozenset({"sec"})


class FuncWrapper:
    # __doc__ is a property backed by the _doc slot (a class variable and a
    # same-named slot would clash). Lookup is deferred to avoid help I/O
    # during core probes. FuncWrapper.__doc__ on the class is the property
    # object, not help text.
    __slots__ = ("_func", "_name", "_doc", "_doc_key", "_fallback_doc", "_key")

    __reduce_ex__ = refuse_pickle

    def __init__(self, func, name, doc="", *, doc_key=None, key=None):
        self._func = func
        self._name = name
        self._doc = doc if doc_key is None else None
        self._doc_key = doc_key
        self._fallback_doc = doc
        # Wrappers with the same key are the same HOC function: NEURON's
        # h.xvalue == h.xvalue. Without a key (bound methods), identity.
        self._key = key

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
        try:
            return self._func(*args, **kwargs)
        except TypeError:
            # The function's signature refused a keyword. NEURON's call
            # checks its keywords first and raises RuntimeError.
            if kwargs and kwargs.keys() - _CALL_KEYWORDS:
                raise RuntimeError("invalid keyword argument") from None
            raise

    def __repr__(self):
        return f"{self._name}()"

    def __str__(self):
        return f"{self._name}()"

    def __eq__(self, other):
        if not isinstance(other, FuncWrapper):
            return NotImplemented
        if self._key is None:
            return self is other
        return self._key == other._key

    def __hash__(self):
        if self._key is None:
            return object.__hash__(self)
        return hash(self._key)

    def __lt__(self, other):
        return _function_order(self, other, operator.lt)

    def __le__(self, other):
        return _function_order(self, other, operator.le)

    def __gt__(self, other):
        return _function_order(self, other, operator.gt)

    def __ge__(self, other):
        return _function_order(self, other, operator.ge)


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
    if not symtab:
        # A class without members (PythonObject on a core without Python) has
        # no table, and nrn_symbol_table_iterator_new(NULL) segfaults.
        return entries
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


def _find_in_table(symtab, name, type_):
    if not symtab:
        return None
    it = _nrn_symbol_table_iterator_new(symtab)
    try:
        while not _nrn_symbol_table_iterator_done(it):
            sym = _nrn_symbol_table_iterator_next(it)
            if _nrn_symbol_name(sym) == name and _nrn_symbol_type(sym) == type_:
                return sym
    finally:
        _nrn_symbol_table_iterator_free(it)
    return None


def class_symbol(class_name):
    """Return the symbol of the HOC class ``class_name``, or None.

    nrn_symbol searches the current symbol table, not the top level, so while
    HOC runs inside an object's method (a callback from Import3d_GUI's
    instantiate, say) it misses top-level templates; the caller would then
    read the symbol table of NULL. Fall back to the top-level and built-in
    tables.
    """
    encoded = class_name.encode("utf-8")
    sym = _nrn_symbol(encoded)
    if sym and _nrn_symbol_type(sym) == TYPES.TEMPLATE:
        return sym
    for symtab_func in (_nrn_top_level_symbol_table, _nrn_global_symbol_table):
        sym = _find_in_table(symtab_func(), encoded, TYPES.TEMPLATE)
        if sym:
            return sym
    return None


# Point-process status per HOC class name. The MechanismType scan runs once
# per class; make_pointprocess turns a HOC template into one (forget_class).
# MechanismType is seeded: the scan's own attribute reads on it come back
# through this check.
_IS_POINT_PROCESS = {"MechanismType": False}
# ARTIFICIAL_CELL point processes, recorded by the same scan.
_IS_ARTIFICIAL = set()


def is_point_process_class(class_name):
    """True if the HOC class ``class_name`` is a point process (IClamp, ExpSyn)."""
    known = _IS_POINT_PROCESS.get(class_name)
    if known is not None:
        return known

    n = _PACKAGE._n
    mt = n.MechanismType(1)
    selected = n.ref("")
    for i in range(int(mt.count())):
        mt.select(i)
        mt.selected(selected)
        _IS_POINT_PROCESS[selected[0]] = True
        if mt.is_artificial(i):
            _IS_ARTIFICIAL.add(selected[0])
    return _IS_POINT_PROCESS.setdefault(class_name, False)


def is_located_point_process_class(class_name):
    """True for a point process that sits in a section (not an ARTIFICIAL_CELL)."""
    return is_point_process_class(class_name) and class_name not in _IS_ARTIFICIAL


# Object method-table cache, keyed by HOC class name.
#
# WHY: real NEURON caches the equivalent table in its CPython type objects.
# Walking the symbol table on every Object construction was the largest
# cost in IClamp/Vector creation. Historical numbers:
# benchmarks/README.md#historical-implementation-measurements.
# INVALIDATES: make_pointprocess / make_mechanism add methods to a HOC
# template (loc, get_loc, has_loc, ...), so they drop its entry
# (forget_class); otherwise only the late-definition case below. An
# already-loaded template can't be redefined (NEURON raises an execerror on
# redefinition). A KSChan point process has no entry (_KSChanMembers).
# TODO(gap-46): if a template name is first queried before the template is
# defined, the cache holds a stale empty entry for it. In practice templates
# are fully defined before first use.
_CLASS_METHODS_CACHE = {}


# HOC functions that turn a template into a mechanism, and the position of
# the template name among their arguments.
TEMPLATE_CONVERTERS = {"make_pointprocess": 0, "make_mechanism": 1}

# Calls that leave the model epoch where it is. Every other HOC call moves it
# (api._bump_model_epoch), because HOC can run user code and edit the
# model. These cannot: each only reads, in C, from the section or the vector
# it is given (READ: nrnoc/cabcode.cpp, nrnoc/treeset.cpp, ivoc/ivocvect.cpp,
# ivoc/oclist.cpp), runs no HOC or user code, and creates, deletes and changes
# nothing. The one Python they can reach is secname's owner-name hook
# (sections._owner_name) for a section a Python cell owns: a dict lookup of the
# name stored when the section was made (str(cell) ran then, not now).
# A caller that holds a name for a function listed here gets the built-in one:
# HOC refuses to redefine a built-in function or template. A new entry needs
# that argument for itself; apply, fit, record, play, execute and every user
# function must stay off the list. Math built-ins (HOC type BLTIN,
# double -> double) are exempt by type, in __init__.NEURON.__getattr__.
READ_ONLY_FUNCTIONS = frozenset(
    {
        # a lookup in the section's property list, or in its 3-D point array
        "ismembrane",
        "secname",
        "section_orientation",
        "n3d",
        "x3d",
        "y3d",
        "z3d",
        "arc3d",
        "diam3d",
    }
)
# By HOC class name: Vector and List are built-in, and cannot be redefined.
READ_ONLY_METHODS = {
    "Vector": frozenset(
        {
            "size",
            "sum",
            "sumsq",
            "mean",
            "var",
            "stdev",
            "min",
            "max",
            "contains",
            "dot",
            "mag",
        }
    ),
    "List": frozenset({"count"}),
}


def forget_class(class_name):
    """Drop the cached method table and point-process status of a HOC class."""
    _CLASS_METHODS_CACHE.pop(class_name, None)
    if class_name != "MechanismType":
        _IS_POINT_PROCESS.pop(class_name, None)
        _IS_ARTIFICIAL.discard(class_name)


class _KSChanMembers(Mapping):
    """The members of a KSChan point process object, looked up in HOC each time.

    Its KSChan renames its variables (iv_type makes gmax pmax) and adds and
    frees them (states, ion), from Python, HOC or the ChannelBuilder, also
    while the object exists. No callback or counter in the core says so.
    """

    __slots__ = ("_obj",)

    def __init__(self, obj):
        self._obj = obj

    def __getitem__(self, name):
        sym = _nrn_method_symbol(self._obj, name.encode("utf-8"))
        if not sym:
            raise KeyError(name)
        return int(_nrn_symbol_type(sym)), int(_nrn_symbol_subtype(sym))

    def __contains__(self, name):
        return bool(_nrn_method_symbol(self._obj, name.encode("utf-8")))

    def _names(self):
        # The class's present name: KSChan.name renames it.
        sym = class_symbol(_nrn_class_name(self._obj).decode("utf-8"))
        return _list_symbol_table(_nrn_symbol_table(sym))

    def __iter__(self):
        return iter(self._names())

    def __len__(self):
        return len(self._names())


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
    if is_point_process_class(class_name) and _nrn_method_symbol(obj._obj, b"nsingle"):
        # Only a KSChan point process has nsingle (member_func in kschan.cpp);
        # no public flag says "this class is a KSChan". Another class with a
        # member of that name is read the same way, only slower.
        return _KSChanMembers(obj._obj)
    cached = _CLASS_METHODS_CACHE.get(class_name)
    if cached is not None:
        return cached
    sym = class_symbol(class_name)
    if not sym:
        raise LookupError(f"no HOC class named {class_name!r}")
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


def _push_args(args, state):
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

    Returns (temp_strs, seg_sec):
      temp_strs — ctypes string objects that must stay alive until the call completes.
      seg_sec   — Section extracted from any Segment argument (for section-context
                  calls); None if no Segment was in args.
    *state*, a list the caller made before the call, becomes
    [count, descriptors]: ``_pop_args(state)`` pops the arguments still on the
    stack (before the call, or restored by a failed nothrow call), releasing
    their object references. Never call it after a successful call: the callee
    consumed the arguments, even if a pending Python callback exception is
    raised afterwards.
    """

    # Only plain numbers: the two passes and their descriptors are not needed.
    # Exact types only; a bool or a subclass takes the slow path. The conversion
    # runs before any push (an int too large to convert raises there). The
    # count in *state* rises right before each push, as in pass 2 below, so
    # the caller's ``_pop_args(state)`` rolls back whatever was pushed.
    for arg in args:
        if type(arg) is not float and type(arg) is not int:
            break
    else:
        values = [float(arg) for arg in args]
        state[:] = [0, (("double", None),) * len(values)]
        try:
            for value in values:
                state[0] += 1
                _nrn_double_push(value)
        except BaseException:
            _pop_args(state)
            raise
        return [], None

    temp_strs = []
    seg_sec = None
    descriptors = []  # (kind, payload); object kinds are borrowed or owned

    # PASS 1: resolve/validate only, no pushes.
    try:
        for arg in args:
            if isinstance(arg, (int, float)):
                # bool is an int subclass, so it follows HOC's double path.
                descriptors.append(("double", float(arg)))
            elif isinstance(arg, _PACKAGE.sections.Segment):
                # A Segment contributes its x and supplies section context.
                _check_segment_arg(arg)
                seg_sec = arg.sec
                descriptors.append(("double", float(arg.x)))
            elif arg is None:
                descriptors.append(("object", None))
            elif isinstance(arg, _PACKAGE.object.Object):
                descriptors.append(("object", arg._obj))
            elif isinstance(arg, NrnRef):
                descriptors.append(("ref", arg))
            elif isinstance(arg, (str, bytes)):
                if isinstance(arg, str) and not arg.isascii():
                    # NEURON's hocobj_pushargs: HOC string arguments are ASCII.
                    raise RuntimeError(
                        "hoc_execerror: python string arg cannot decode into c_str"
                    )
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
                _api._nrn_object_unref(payload)
        raise

    # PASS 2: push the validated descriptors. Each push is counted in *state*
    # before its ctypes call: an interrupt is raised once a C call returns,
    # never before it starts (a C callable has no eval-breaker check on
    # entry), so the count covers every pushed slot. A ref's _push raises it
    # itself, just before its own ctypes push. The caller holds *state* from
    # before this call and rolls back with ``_pop_args(state)`` even when an
    # interrupt lands before this function returns.
    st = state
    st[:] = [0, descriptors]
    released_owned = 0
    try:
        for kind, payload in descriptors:
            if kind == "double":
                st[0] += 1
                _nrn_double_push(payload)
            elif kind in ("object", "owned_object"):
                st[0] += 1
                _nrn_object_push(payload)
                if kind == "owned_object" and payload:
                    released_owned += 1
                    _api._nrn_object_unref(payload)
            elif kind == "str":
                st[0] += 1
                _nrn_str_push(payload)
            else:  # ref
                payload._push(st)
    except BaseException:
        _pop_args(st)
        to_release = released_owned
        for kind, payload in descriptors:
            if kind != "owned_object" or not payload:
                continue
            if to_release:
                to_release -= 1
            else:
                _api._nrn_object_unref(payload)
        raise
    return temp_strs, seg_sec


def _pop_args(pushed):
    """Pop the arguments *pushed* (see _push_args) still counts, newest first
    by recorded kind, with the raw ctypes pops. The count drops right before
    each pop (a taken jump can take an interrupt on Python 3.10), so a second
    call after an interrupt pops the rest, never a slot twice."""
    while pushed[0]:
        kind, payload = pushed[1][pushed[0] - 1]
        if kind == "ref":
            kind = payload._slot
        if kind == "double":
            pushed[0] -= 1
            _nrn_double_pop()
        elif kind == "str":
            pushed[0] -= 1
            _api._nrn_str_pop0()
        elif kind == "double_ptr":
            pushed[0] -= 1
            _api._nrn_double_ptr_pop()
        else:  # object, owned_object, an objref pointer
            pushed[0] -= 1
            obj = _api._nrn_object_pop()
            if obj:
                _api._nrn_object_unref(obj)


def _function_order(func, other, compare):
    # NEURON's hocobj_richcmp: top-level functions order by symbol address;
    # bound methods of different objects, and other HOC objects, are
    # undefined; a plain object compares by its address against the
    # function's object (none at top level).
    if isinstance(other, FuncWrapper):
        if func._key is None or other._key is None:
            raise TypeError("this comparison is undefined")
        symbol = _api._nrn_symbol
        return compare(
            symbol(func._key.encode("utf-8")) or 0,
            symbol(other._key.encode("utf-8")) or 0,
        )
    if isinstance(other, (NrnRef, _PACKAGE.object.Object)):
        raise TypeError("this comparison is undefined")
    if func._key is None:
        return NotImplemented  # a bound method's object address is unknown here
    return compare(0, id(other))


# The default of a ``sec=`` keyword, so an explicit ``sec=None`` is told apart
# and refused as NEURON does.
_NO_SEC = object()


def _check_sec_kwarg(sec):
    """Reject a ``sec=`` that is not a live Section, before anything is pushed."""

    if not isinstance(sec, _PACKAGE.sections.Section):
        raise TypeError("sec is not a Section")
    sec._check_alive()


def _pop_pushed_section(pushed):
    """Pop the section pushed as a HOC call's section context, which the call
    may have deleted (``delete_section()``, a template proc or ``init()``).

    *pushed* is the caller's ``[sec_ptr]``, set before the push and emptied
    here just before the pop. An interrupt can land at this function's entry
    or after the liveness check, so a caller's ``finally`` calls it again from
    an inner ``finally`` while *pushed* is not empty; the pop itself happens
    once.

    ``nrn_section_pop`` runs ``chk_access`` first. With the top of the stack
    deleted it falls back to another live section, leaving the deleted one's
    reference behind, or with none left its "Accessing a deleted section"
    error throws through ctypes and aborts. HOC's ``pop_section()`` pops
    without the check (cabcode.cpp:2165), as NEURON's hocobj_call does with
    ``nrn_popsec``. The stack's reference keeps the struct readable here.
    """
    if _nrn_section_is_active(pushed[0]):
        del pushed[0]
        _nrn_section_pop()
        return
    sym = _nrn_symbol(b"pop_section")
    err = _ERR_BUF()
    del pushed[0]
    ret = _nrn_function_call(sym, 0, err, _ERR_BUF_SIZE)
    _check_nrn_error(ret, err)
    _nrn_double_pop()  # pop_section's hoc_retpushx(1.)


def _name_error(exc_type, name, message):
    """The error for a name HOC does not know: TypeError for a non-ASCII name,
    as NEURON (HOC names are ASCII, and hasattr must not swallow it)."""
    if isinstance(name, str) and not name.isascii():
        return TypeError("attribute name must be a string")
    return exc_type(message)


def _wrap_owned_object(obj):
    """Consume an owned HOC ``Object*`` into its typed Python wrapper.

    The reference is consumed even if class resolution or wrapper
    initialization fails. Once ``cls._wrap`` starts, the partial wrapper owns
    the reference and its finalizer releases it.
    """
    if not obj:
        return None
    if CALLS_PYTHON:
        try:
            return _api._python_object_get(obj)
        finally:
            _api._nrn_object_unref(obj)
    try:
        name = _api._nrn_class_name(obj)
        cls = _WRAPPER_CLASSES.get(name)
        if cls is None and name != b"PythonObject":
            if name:
                cls = _WRAPPER_CLASSES[name] = hoc_class(name.decode("utf-8"))
            else:
                cls = _PACKAGE.object.Object
    except BaseException:
        _api._nrn_object_unref(obj)
        raise
    if cls is None:
        # The converter consumes ownership even on failure; it is outside the
        # cleanup above so the reference cannot be released twice.
        return _owned_pythonobject_to_python(obj)
    return cls._wrap(obj, cls._hoc_class_name)


# Class name, as NEURON gives it, -> the class of its wrappers.
_WRAPPER_CLASSES = {}


def hoc_class(name):
    """The Python class of HOC class ``name``, made on first use and cached.

    The class is not stored as an attribute of any module: a HOC class named
    like a module global or submodule (api, ctypes, NEURON, n) would replace
    it. pickle finds the class through its ``__module__``: the package's
    module ``__getattr__`` (standalone) or the ``hoc`` facade (in NEURON).
    """

    cls = _PACKAGE._dynamic_classes.get(name)
    if cls is None:

        # Empty __slots__: no per-instance __dict__, like List, Vector and
        # SectionList. _hoc_class_name binds the class to its HOC template
        # for construction and metaclass indexing.
        cls = type(
            name,
            (_PACKAGE.object.Object,),
            {
                "__module__": HOC_CLASS_MODULE,
                "__slots__": (),
                "_hoc_class_name": name,
            },
        )
        _PACKAGE._dynamic_classes[name] = cls
    return cls


def _try_wrap_density_nmodlrandom(sec, x, sym, sym_type):
    """Return an NMODLRandom wrapper, or None when ``sym`` is not one.

    See api.TypeCodes.record_rangeobj for why this is probed at runtime.
    """
    if not 0.0 < x < 1.0:  # the core finds the node with node_exact
        _ensure_topology(sec, x)
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


_SLOT_POPS = {}


def _return_slot_pop(call_type):
    """What pops the result slot a call of *call_type* leaves, or None.

    A BLTIN is covered too: a signal that arrives in the Python code before
    any call is raised when the next call returns, however short."""
    if not _SLOT_POPS:  # the codes are known once the package is imported
        for codes, pop in (
            (
                (TYPES.BLTIN, TYPES.FUNCTION, TYPES.FUN_BLTIN, TYPES.PROCEDURE),
                _nrn_double_pop,
            ),
            ((TYPES.METHOD_STRFUNC, TYPES.STRINGFUNC), _nrn_str_pop),
            (
                (TYPES.METHOD_OBFUNC, TYPES.OBFUNCTION, TYPES.OBJECTFUNC),
                _object_pop,
            ),
        ):
            _SLOT_POPS.update(dict.fromkeys(codes, pop))
    return _SLOT_POPS.get(call_type)


def _drop_return_slot(err_buf, pop, pushed=None):
    """After a KeyboardInterrupt: a signal handler runs once the ctypes call has
    returned, with its result still on the stack (gap-84). The call cleared the
    buffer's first byte, which the caller set before it. A failed call left a
    message there and a result of none, and restored the arguments, which are
    still on the stack too when the interrupt came before the call: *pushed*
    (see _push_args) pops them. *err_buf* is None once the result is popped."""
    if err_buf is None:
        return
    if err_buf[0] == b"\x00":
        if pop:
            pop()
    elif pushed is not None and pushed[0]:
        _pop_args(pushed)


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
