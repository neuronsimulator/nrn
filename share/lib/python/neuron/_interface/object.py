"""Object, List, Vector wrappers — dynamic class system, refcounting, method dispatch."""
import array
import collections.abc
import contextlib
import ctypes
import functools
import numbers
import operator
import os
import sys
import types
import warnings

from ._docs import _ClassDoc, help_doc
from .api import (
    CALLS_PYTHON,
    SEGMENT_BASE,
    VECTOR_BASE,
    _Py_TPFLAGS_HEAPTYPE,
    PENDING_CALLBACKS,
    raise_callback_error,
    hoc_variable,
    _as_double,
    _to_double,
    _nrn_symbol_subtype,
    _VENDORED,
    HOC_CLASS_MODULE,
    HOC_LOCK,
    OBJECT_BASE,
    EPOCH,
    _PySequence_Check,
    _bump_model_epoch,
    _model_epoch,
    hoc_locked,
    _ERR_BUF,
    _ERR_BUF_SIZE,
    TYPES,
    _check_nrn_error,
    _hoc_unref_defer,
    _nrn_method_call_number,
    _nrn_mpiabort_on_error,
    _nrn_object_alias,
    _nrn_hoc_call,
    _nrn_method_call,
    _nrn_method_symbol,
    _nrn_object_ref,
    _nrn_pp_setpointer_pop,
    _nrn_prop_exists,
    _nrn_property_get,
    _nrn_property_object_get,
    _nrn_property_set,
    _nrn_section_is_active,
    _nrn_section_push,
    _nrn_section_ref,
    _nrn_section_unref,
    _nrn_sectionlist_data,
    _nrn_sectionlist_iterator_done,
    _nrn_sectionlist_iterator_free,
    _nrn_sectionlist_iterator_new,
    _nrn_sectionlist_iterator_next,
    _nrn_sectionlist_to_array,
    _nrn_symbol_array_length,
    _nrn_symbol_is_array,
    _nrn_vector_capacity,
    _nrn_vector_data,
    _property_array_get_checked,
    _property_array_set_checked,
    _property_get_checked,
    _property_set_checked,
    _ref_property_array_set_checked,
    _top_level_symbol,
    _nrn_object_new,
    _PACKAGE,
    symbol_array_dims,
)
from . import api as _api
from .nrnref import (
    NrnArrayPropertyRef,
    NrnHocTemplateStrRef,
    NrnHocTemplateVarRef,
    NrnObjectPropertyRef,
    NrnRef,
    NrnMatrixElementRef,
    NrnVectorElementRef,
    refuse_pickle,
    _array_index,
    _subscripts,
)

from .utils import (
    _is_python_object_arg,
    _NO_SEC,
    _check_sec_kwarg,
    _check_segment_arg,
    _name_error,
    _wrap_owned_object,
    FuncWrapper,
    READ_ONLY_METHODS,
    _drop_return_slot,
    _pop_pushed_section,
    _return_slot_pop,
    _try_wrap_point_nmodlrandom,
    class_symbol,
    is_located_point_process_class,
    _IS_POINT_PROCESS,
    is_point_process_class,
    list_functions,
    list_methods,
    _PythonObjectArg,
)
from . import utils as _utils


# ---------------------------------------------------------------------------
# Metaclass — _HocClassMeta
# ---------------------------------------------------------------------------


# NEURON's Python module reads the argument of these without a length check.
_ONE_ARG_COLLECTIVES = frozenset({"py_allgather", "py_gather", "py_broadcast"})


def _one_arg_collective(pc, name, method):
    """pc.<name>() with no argument: NEURON's CPython SystemError."""

    def collective(*args, **kwargs):
        if not args:
            error = SystemError(
                f"{pc}.{name}() returned a result with an exception set"
            )
            error.__cause__ = IndexError("tuple index out of range")
            raise error
        return method(*args, **kwargs)

    return collective


class _HocClassMeta(type):
    """Metaclass for HOC-wrapper classes.

    Provides ``Cls[i]`` indexing into the live HOC List of instances of
    the class: ``n.IClamp[0]`` is the first IClamp in the model,
    ``n.IClamp[-1]`` the most recent.

    This is a metaclass, not a proxy around the class, so ``n.IClamp``
    itself is the dynamic class: callable via ``type.__call__`` ->
    ``Object.__init__`` and subclassable in plain Python
    (``class MyIClamp(n.IClamp): ...``) without ``neuron.hclass()``.
    """

    def __new__(mcls, name, bases, namespace, **kwargs):
        hoc_name = namespace.get("_hoc_class_name")
        if hoc_name:
            namespace["__doc__"] = _ClassDoc(hoc_name, namespace.get("__doc__"))
        return super().__new__(mcls, name, bases, namespace, **kwargs)

    @property
    def __doc__(cls):
        # pydoc uses object.__getattribute__ on classes, bypassing the class's
        # own descriptor lookup. A metaclass property serves that path too.
        doc = vars(cls).get("__doc__")
        return doc.__get__(None, cls) if isinstance(doc, _ClassDoc) else doc

    @__doc__.setter
    def __doc__(cls, value):
        # Bypass this property's setter while updating the actual class dict,
        # so an explicit override is also visible on instances.
        type.__dict__["__doc__"].__set__(cls, value)

    def __getitem__(cls, index):
        # Base Object has no HOC counterpart, so it is not indexable.
        # NEURON.__getattr__ sets _hoc_class_name on dynamic classes; List,
        # Vector and SectionList set it explicitly.
        class_name = getattr(cls, "_hoc_class_name", None)
        if class_name is None:
            raise TypeError(
                f"class {cls.__name__!r} cannot be indexed — no HOC class bound"
            )

        # List(name) is allocation-ordered, not object_id-ordered. A negative
        # index sorts by object_id; a positive index matches object_id, like
        # real NEURON's h.IClamp[i].
        n = _PACKAGE._n
        lst = n.List(class_name)
        count = int(lst.count())
        if count == 0:
            raise IndexError(f"{class_name}[{index}]: no instances exist")
        if index < 0:
            ids = []
            for i in range(count):
                obj = lst.object(i)
                ids.append((int(n.object_id(obj, 1)), i))
            ids.sort()
            if -index > len(ids):
                raise IndexError(f"{class_name}[{index}] out of range")
            _, list_idx = ids[index]
            return lst.object(list_idx)
        # O(n) scan — object_ids are not contiguous positions.
        for i in range(count):
            obj = lst.object(i)
            if int(n.object_id(obj, 1)) == index:
                return obj
        raise IndexError(f"{class_name}[{index}] not found")


def _sectionlist_members(arg):
    """The Sections of a SectionList(iterable) argument, checked as NEURON does
    before the list exists. A HOC Object is not a Python iterable to NEURON."""

    if (
        isinstance(arg, (Object, str, bytes))
        or arg is None
        or not isinstance(arg, collections.abc.Iterable)
    ):
        raise RuntimeError("hoc_execerror: argument must be a Python iterable")
    members = list(arg)
    if not all(isinstance(member, _PACKAGE.sections.Section) for member in members):
        raise RuntimeError("hoc_execerror: iterable must contain only Section objects")
    return members


@hoc_locked
def _construct_hoc_object(class_name, args, sec=None):
    """Create the HOC object backing a user-side construction call.

    Returns ``(obj_ptr, post_init)``; call ``post_init`` with the initialized
    wrapper. Some classes take Python idioms that do not match the
    HOC constructor, such as Vector(iterable) and SectionList(iterable). Their
    args are rewritten here and the rest is finished by ``post_init``. All
    other args pass straight through.

    Called from ``Object.__init__`` for direct construction
    (``n.IClamp(seg)``, ``MyIClamp(seg)``). ``Object._wrap`` does not use it.
    """

    initing_to_list_arg = None
    initing_sectionlist = None
    rvp_ends = ()
    args_for_push = args

    if class_name == "RangeVarPlot" and len(args) > 1:
        # RangeVarPlot(expr, seg0, seg1): core reads each end as an x on the
        # accessed section, or a segment object through a hook only NEURON's
        # module installs. One section push cannot carry two ends, so set them
        # with begin()/end() after construction, as HOC code does.

        if all(isinstance(a, _PACKAGE.sections.Segment) for a in args[1:3]):
            rvp_ends = tuple(args[1:3])
            for seg in rvp_ends:
                _check_segment_arg(seg)
            args_for_push = args[:1]
            args = args_for_push
    if class_name == "SectionRef":
        # SectionRef(sec) reads its Section through nrn_secarg too.
        args, sec = _section_arg_as_sec(args, sec)
        args_for_push = args
    if (
        class_name == "Vector"
        and len(args) == 1
        and isinstance(args[0], collections.abc.Iterable)
        and not isinstance(args[0], str)
    ):
        # n.Vector([1, 2, 3]) → HOC Vector(3), then one copy into its data.
        initing_to_list_arg = _vector_values(args[0])
        args_for_push = (len(initing_to_list_arg),)
    elif (
        class_name == "Vector" and len(args) == 1 and _unreadable_vector_source(args[0])
    ):
        # Core reads a PythonObject argument through nrnpy_vec_from_python, a
        # hook only NEURON's module installs; without it core says "Python not
        # available". Give NEURON's error instead.
        raise RuntimeError(
            f"hoc_execerror: {type(args[0]).__name__} does not support the "
            "Python Sequence or Iterator protocol"
        )
    elif class_name == "SectionList" and args:
        # n.SectionList([s1, s2]) → empty HOC SectionList + later append().
        initing_sectionlist = _sectionlist_members(args[0])
        args_for_push = ()

    # Check liveness before any push: a section deleted through HOC (e.g.
    # n.IClamp(dead_seg)) makes nrn_object_new SIGABRT. This guards sec=;
    # a point process's location is checked by _check_location_arg, any
    # other Segment arg's section in utils._push_args.
    if sec is not None:
        _check_sec_kwarg(sec)
    if args_for_push and is_located_point_process_class(class_name):
        _check_location_arg(args_for_push[0])
    # Pushes are recorded before they happen, in *pushed* ([count, rollback],
    # see utils._push_args) and *pushed_sec*, because an interrupt can land
    # after any call returns, including a push or _push_args itself (gap-84).
    pushed = [0]
    pushed_sec = None
    result = ctypes.c_void_p()
    is_template = True  # an unknown class may run HOC too
    try:
        temp_strs, seg_sec = _utils._push_args(args_for_push, pushed)
        # A Segment arg's section wins over sec=: the arg is pushed after it.
        use_sec = seg_sec if seg_sec is not None else sec
        if use_sec is not None:
            pushed_sec = [use_sec._sec]
            _nrn_section_push(pushed_sec[0])
        sym = class_symbol(class_name)
        if not sym:
            raise LookupError(f"no HOC class named {class_name!r}")
        # Asked before the object exists: the first ask for a class runs HOC,
        # and an exception (a KeyboardInterrupt) after nrn_object_new would
        # leave the object with no owner.
        is_template = _is_hoc_template(class_name)
        _nrn_object_new(sym, len(args_for_push), result)
    except BaseException:
        if result.value:
            # An interrupt after the core took the arguments and returned the
            # object: release the object. Popping the arguments would throw.
            _api._nrn_object_unref(result.value)
        elif pushed[0]:
            # Not called yet, or nrn_object_new_nothrow failed and restored
            # the args. Pop them or repeated failures hit "Stack too deep"
            # (gap-56); again if an interrupt stops the first pass.
            try:
                _utils._pop_args(pushed)
            finally:
                if pushed[0]:
                    _utils._pop_args(pushed)

        raise_callback_error()
        raise
    finally:
        if pushed_sec:
            try:
                _pop_pushed_section(pushed_sec)
            finally:
                if pushed_sec:  # interrupted before the pop
                    _pop_pushed_section(pushed_sec)
        if is_template:
            # A template's init() is HOC: it can uninsert mechanisms or delete
            # sections, so the caches the epoch guards (allsec, Mechanism
            # wrappers) must check again. Only a class's first construction
            # moved it before (gap-68).

            _bump_model_epoch()

    def post_init(wrapper):
        if initing_to_list_arg is not None:
            _copy_into_vector(wrapper, initing_to_list_arg)
        if initing_sectionlist is not None:
            for s in initing_sectionlist:
                wrapper.append(sec=s)
        for method, seg in zip(("begin", "end"), rvp_ends):
            getattr(wrapper, method)(seg.x, sec=seg.sec)

    return result.value, post_init


# ---------------------------------------------------------------------------
# Base wrapper class
# ---------------------------------------------------------------------------


def _instance_hname(self):
    idx = int(_PACKAGE._n.object_id(self, 1))
    return f"{self._class_name}[{idx}]"


def _member_hides_helper(instance, name):
    """True if HOC member *name* of *instance* hides the Python helper of that
    name (same, hname, hocobjptr, baseattr, ref).

    NEURON looks an attribute of a template or point-process object up in HOC
    first, so a template's `public same` is that member. A Python subclass
    instance's attribute is looked up on its Python type first
    (hocobj_getattro), where NEURON's helpers are; ref is not one of them.
    """
    try:
        methods = instance._load_methods()
    except AttributeError:
        return False
    if name not in methods:
        return False
    return name == "ref" or "_hoc_class_name" in type(instance).__dict__


class _Helper:
    """A Python helper method of Object that a HOC member of the same name
    hides (_member_hides_helper)."""

    def __init__(self, func):
        functools.update_wrapper(self, func)
        self._func = func

    def __get__(self, instance, owner=None):
        if instance is None:
            return self._func
        name = self._func.__name__
        if _member_hides_helper(instance, name):
            return Object.__getattr__(instance, name)
        return types.MethodType(self._func, instance)


class _Hname:
    """``obj.hname()`` is ``"A[0]"``. On a template or point-process class,
    ``A.hname()`` is ``"A()"``: those classes are HocObject instances in NEURON
    (neuron.hclass names its class from it). Built-in C++ classes are Python
    types there, so ``Vector.hname()`` stays an unbound method.
    """

    def __get__(self, instance, owner):
        if instance is not None:
            if _member_hides_helper(instance, "hname"):
                return Object.__getattr__(instance, "hname")
            return types.MethodType(_instance_hname, instance)
        from ._facades import _is_python_type

        name = getattr(owner, "_hoc_class_name", None)
        if name is None or _is_python_type(None, owner):
            return _instance_hname
        return lambda: f"{name}()"


# Class name -> True if it is a HOC template, False if a C++ class (built in
# or from a MOD file, including one loaded later with nrn_load_dll).
_IS_HOC_TEMPLATE = {}


def _is_hoc_template(class_name):
    """True unless *class_name* is a C++ class (Vector, List, IClamp, ExpSyn).

    A HOC template's init() and its destruction run HOC code, which can
    remove mechanisms or delete sections. Decided by the class symbol's
    subtype; the class-id test (_object_is_builtin_class) counts a MOD class
    loaded after startup as a template.
    """
    known = _IS_HOC_TEMPLATE.get(class_name)
    if known is None:
        sym = class_symbol(class_name) if class_name else None
        if not sym:
            return True  # unknown class: assume it may
        known = int(_nrn_symbol_subtype(sym)) != TYPES.CPLUSOBJECT
        _IS_HOC_TEMPLATE[class_name] = known
    return known


def _may_own_sections(class_name):
    """True if freeing an object of *class_name* can delete sections: a HOC
    template, or a List, which can hold the last reference to template
    cells."""
    return class_name == "List" or _is_hoc_template(class_name)


def _other_hoc_pointer(other):
    """The HOC object address of another wrapper, or None."""
    return other._obj if isinstance(other, Object) else None


def _object_order(obj, other, compare):
    # NEURON's hocobj_richcmp: wrappers order by HOC object address, and
    # any other object by its own address; a reference is incomparable.
    if isinstance(other, (NrnRef, _utils.FuncWrapper)):
        raise TypeError("this comparison is undefined")
    pointer = _other_hoc_pointer(other)
    return compare(obj._obj or 0, id(other) if pointer is None else pointer)


class _UndefinedHocName(LookupError, AttributeError):
    """Assignment of a name that is not a member of a template or
    point-process object.

    NEURON raises LookupError ("'x' is not a defined hoc variable name."), and
    neuron.Wrapper.__setattr__ relies on it to keep such a name on the
    wrapper. It is also an AttributeError, which this interface raised before.
    """


def _unknown_name_error(obj, name):
    """What NEURON raises for ``obj.name = value`` when *name* is not a member.

    The built-in classes NEURON exposes as Python types (Vector, NetCon, ...)
    take the assignment through Python's generic setattr, which raises
    AttributeError; template and point-process objects are plain HocObjects
    there, which raise LookupError.
    """
    from ._facades import _is_python_type

    if _is_python_type(None, type(obj)):
        message = (
            f"'{HOC_CLASS_MODULE}.{obj._class_name}' object has no attribute '{name}'"
        )
        return _name_error(AttributeError, name, message)
    return _name_error(
        _UndefinedHocName, name, f"'{name}' is not a defined hoc variable name."
    )


class Object(OBJECT_BASE, metaclass=_HocClassMeta):
    """Wraps a HOC object (template instance) — IClamp, Vector, NetCon, etc.

    ``n.<ClassName>`` returns a dynamic subclass minted on first access.
    Construct by calling it (``n.IClamp(soma(0.5))``); Python subclassing
    works (``class MyClamp(n.IClamp): ...``). Attribute access goes to HOC
    template members via ``__getattr__`` / ``__setattr__``.
    """

    # The base has the other slots, and releases the HOC object. Core makes
    # wrappers too, with _methods None (_load_methods).
    __slots__ = ("__weakref__",)

    # Set on dynamic subclasses by NEURON.__getattr__; the hand-coded
    # subclasses below set it explicitly. Drives both metaclass indexing
    # (``cls[i]``) and the user-construction path in __init__.
    _hoc_class_name = None

    def __init__(self, *args, sec=_NO_SEC):
        # User construction only: _hoc_class_name names the template and
        # _construct_hoc_object pushes the args. Code holding a raw HOC obj*
        # wraps it with _wrap. A ctypes.c_void_p argument is a Python object
        # here, as in NEURON; reading it as an obj* segfaulted.
        class_name = type(self)._hoc_class_name
        if class_name is None:
            raise TypeError(
                f"{type(self).__name__} has no bound HOC class — "
                "instantiate via n.<ClassName>(...) or subclass a "
                "class returned by n.<ClassName>."
            )
        if sec is _NO_SEC:
            sec = None
        elif sec is None:
            raise TypeError("sec is not a Section")
        self._construct(class_name, args, sec)

    def _construct(self, class_name, args, sec=None):
        """Make the HOC object of class *class_name* for this wrapper."""
        obj, post_init = _construct_hoc_object(class_name, args, sec=sec)
        # Own the object before any step below can fail, as _wrap does: the
        # base type releases a partly built wrapper's object too.
        self._obj = obj
        self._init_from_ptr(obj, class_name)
        post_init(self)

    @classmethod
    def _wrap(cls, obj, class_name=None):
        """Wrap an existing HOC obj* without the user construction path.

        For internal callers that hold a raw pointer. Bypasses any
        user-defined __init__ on a subclass.
        """
        instance = cls.__new__(cls)
        # Establish ownership before initialization can fail. The base type
        # then releases the caller-provided ref of a partly initialized wrapper.
        instance._obj = obj
        instance._init_from_ptr(obj, class_name)
        return instance

    def _init_from_ptr(self, obj, class_name=None):
        """Populate all slots from an existing HOC obj*.

        Called by both ``__init__`` (user path) and ``_wrap`` (internal path).
        Takes ownership of the caller's reference (nrn_object_new returns
        refcount 1; likewise nrn_object_pop). The caller must not call
        nrn_object_ref, which would double-ref and leak the C object; the base
        class calls nrn_object_unref.
        """
        put = object.__setattr__
        put(self, "_obj", obj)
        put(
            self,
            "_class_name",
            class_name or _api._nrn_class_name(obj).decode("utf-8"),
        )
        put(self, "_methods", None)
        # Pre-set lazy slots so reads do not fall into __getattr__.
        put(self, "_plotshape_var", None)
        put(self, "_section_cache", None)

    def _load_methods(self):
        methods = self._methods
        if methods is None:
            methods = list_methods(self)
            object.__setattr__(self, "_methods", methods)
        return methods

    hname = _Hname()

    @_Helper
    def baseattr(self, name):
        """Look up *name* on the HOC side, ignoring Python-level overrides.

        Real NEURON's `HocObject.baseattr(name)` is literally
        `hocobj_getattr(self, name)` (9.0's nrnpy_hoc.cpp). It bypasses
        a Python subclass that shadows a HOC attribute with a property,
        descriptor or instance `__dict__` entry. HOC dispatch lives in
        `Object.__getattr__`, which Python only calls when normal lookup
        misses, so this calls it directly. It uses `Object.__getattr__`, not
        `type(self).__getattr__`, so subclasses can override `__getattr__`
        for non-HOC reasons.
        """
        if type(self).__dictoffset__:
            # A subclass instance usually stores the result (self.bp =
            # self.baseattr("p")). Bind it to a plain wrapper of the same HOC
            # object, as NEURON's result refers to the HOC object only:
            # bound to self it would be a reference cycle, and the object
            # would outlive its last Python reference until a collection.
            _nrn_object_ref(self._obj)
            return Object.__getattr__(_wrap_owned_object(self._obj), name)
        return Object.__getattr__(self, name)

    def __repr__(self):
        return _instance_hname(self)

    def __str__(self):
        return _instance_hname(self)

    @_Helper
    def same(self, other):
        """Return True if *other* wraps the same underlying HOC object."""
        pointer = _other_hoc_pointer(other)
        return pointer is not None and self._obj == pointer

    def __eq__(self, other):
        # Wrappers of the same HOC object compare equal, as in real NEURON's
        # tp_richcompare (a Vector round-tripped through a List is a new
        # wrapper). NotImplemented for non-Object lets Python try the other
        # side's __eq__, then fall back to False.
        pointer = _other_hoc_pointer(other)
        if pointer is None:
            return NotImplemented
        return self._obj == pointer

    def __ne__(self, other):
        result = self.__eq__(other)
        if result is NotImplemented:
            return result
        return not result

    def __hash__(self):
        # Must stay consistent with __eq__: same pointer => same hash.
        return hash(self._obj) if self._obj else 0

    def __lt__(self, other):
        return _object_order(self, other, operator.lt)

    def __le__(self, other):
        return _object_order(self, other, operator.le)

    def __gt__(self, other):
        return _object_order(self, other, operator.gt)

    def __ge__(self, other):
        return _object_order(self, other, operator.ge)

    def __copy__(self):
        """Reject copying HOC objects other than Vector.

        Real NEURON only exposes Python's copy protocol for Vector.  Letting
        ``copy.copy`` fall back to the wrapper's slots would duplicate the
        owned native pointer without taking a native reference, so the two
        wrappers would race to unref the same object.
        """
        raise TypeError("HocObject: Only Vector instance can be pickled")

    def __deepcopy__(self, memo):
        """Reject deepcopy for HOC objects other than Vector."""
        raise TypeError("HocObject: Only Vector instance can be pickled")

    def __reduce_ex__(self, protocol):
        """Reject pickling HOC objects other than Vector.

        The default would pickle the wrapper's slots, the native pointer
        among them, and the copy would unref an object it holds no
        reference to. A subclass that defines __reduce__ says how to rebuild
        itself, as neuron.HocBaseObject subclasses do.
        """
        if type(self).__reduce__ is not object.__reduce__:
            return self.__reduce__()
        raise TypeError("HocObject: Only Vector instance can be pickled")

    def _plotshape_variable(self, *args):
        """PlotShape.variable: a string goes to HOC, a Python object stays here."""
        if args and not isinstance(args[0], (str, int, float, Object, type(None))):
            self._plotshape_var = args[0]
            return 1.0
        self._plotshape_var = None
        return self._hoc_variable(*args)

    @_Helper
    def hocobjptr(self):
        """Return the raw C pointer to the underlying HOC object as an int."""
        return int(self._obj) if self._obj is not None else 0

    @_Helper
    def ref(self, name, idx=None):
        """Reference to variable member *name*, or to element *idx* of an
        array member (``obj.ref("amp", idx=0)``), as ``obj._ref_<name>``. A
        HOC member named ref (``VBox.ref(obj)``) hides it."""
        return self._property_ref(name, idx=idx)

    def _property_ref(self, name, idx=None):
        """``obj._ref_<name>``, or element *idx* of array member *name*.

        Only a double, a strdef or a StringFunctions alias has a reference,
        as in NEURON. The property-pointer functions would read any other
        name (a func, an objref, a name that is not a member) as a double and
        segfault.
        """
        methods = self._load_methods()
        if name == "delay" and "del" in methods:
            name = "del"  # Python keyword; NEURON's getsym maps it the same way
        member = methods.get(name)
        if member is None:
            # A StringFunctions alias resolves through HOC (see _alias_ref).
            alias = _alias_ref(self, name) if idx is None else None
            if alias is not None:
                return alias
            raise AttributeError(
                f"'{self._class_name}' object has no property or alias "
                f"'{name}' to reference"
            )
        _type = member[0]
        if _type == TYPES.STRING:
            if idx is not None:
                raise TypeError(f"strdef '{name}' does not take an index")
            return NrnHocTemplateStrRef(self, name)
        if _type not in (TYPES.VAR, TYPES.RANGEVAR):
            raise TypeError(
                f"Hoc pointer error, {name} is not a hoc variable or range "
                "variable or strdef"
            )
        self._check_located()
        name_encoded = name.encode("utf-8")
        sym = _nrn_method_symbol(self._obj, name_encoded)
        is_array = _nrn_symbol_is_array(sym)
        if not is_array and idx is not None:
            raise TypeError(f"scalar property '{name}' does not take an index")
        if is_array:
            rows = None
            if self._class_name == "Matrix" and name == "x":
                rows = _MatrixX(self, ref=True)
            elif _type == TYPES.VAR and len(symbol_array_dims(sym)) > 1:
                rows = _MemberArray(self, name, symbol_array_dims(sym), ref=True)
            if rows is not None:
                return rows if idx is None else rows[idx]
            if idx is None:
                # obj._ref_<array>: [i] gives element i, as NEURON's array pointer.
                return NrnArrayPropertyRef(self, name, self._array_size(name, sym))
            if self._class_name == "Vector" and name == "x":
                return NrnVectorElementRef(self, idx)
            size = self._array_size(name, sym)
            if not 0 <= idx < size:
                raise IndexError(f"index {idx} out of range [0, {size}) for {name}")
        if _type == TYPES.VAR:
            return NrnHocTemplateVarRef(self, name, idx=idx)
        return NrnObjectPropertyRef(self, name_encoded, idx)

    def _array_size(self, name, sym):
        """Length of array member *name* (symbol *sym*) of this object.

        The symbol keeps the length last set for it. A Vector's x has the
        Vector's size, and a NetCon's weight one element per NET_RECEIVE
        argument of its target (wcnt()), which differ between objects.
        """
        if self._class_name == "Vector" and name == "x":
            return int(_nrn_vector_capacity(self._obj))
        if self._class_name == "NetCon" and name == "weight":
            return int(self.wcnt())
        return int(_nrn_symbol_array_length(sym))

    def _template_section_entry(self, name):
        """The kept lookup of template section (or section array) *name*.

        NEURON reads the section with component() (9.0's nrnpy_hoc.cpp), which the
        C API lacks, so a lookup costs a HOC parse and its result is kept. A
        proc of the template can run `create` again (morphology loaders,
        Import3d's instantiate()), which deletes the sections and makes new
        ones; the kept section (for an array, its first element) is then no
        longer in the model, and the lookup runs again.
        """
        cache = self._section_cache
        if cache is None:
            cache = {}
            object.__setattr__(self, "_section_cache", cache)
        entry = cache.get(name)
        if entry is not None and entry.alive():
            return entry
        sym = _nrn_method_symbol(self._obj, name.encode("utf-8"))
        if _nrn_symbol_is_array(sym):
            # `create dend[N]`: subscriptable proxy (real NEURON returns a
            # HocObject array).
            entry = _SectionArrayEntry(self, name, sym)
        else:
            entry = _SectionEntry(_resolve_template_section(self, name, idx=None))
        cache[name] = entry
        return entry

    def __dir__(self):
        # `del` is a valid HOC field name (IClamp.del, AlphaSynapse.del)
        # but a Python keyword; expose it as `delay` in dir() and dispatch.
        methods = set(self._load_methods())
        if "del" in methods:
            methods.discard("del")
            methods.add("delay")
        # A template's own name is in its symbol list, and NEURON has no ref.
        methods.discard(self._class_name)
        # Include the Python-side helpers (hname, same, hocobjptr) and
        # subclass (Vector, List, SectionList) entries.
        for klass in type(self).__mro__:
            if klass is object:
                continue
            for name in klass.__dict__:
                # ref is this interface's helper, but a subclass may define it.
                if not name.startswith("_") and (klass is not Object or name != "ref"):
                    methods.add(name)
        methods.update(name for name in dir(type(self)) if name.startswith("__"))
        return sorted(methods)

    def _get_segment(self):
        """The Segment this point process is in, or None if it is in none."""

        n = _PACKAGE._n
        with HOC_LOCK:  # from has_loc to the pop of get_loc's push (gap-72)
            if not self.has_loc():
                return None
            x = self.get_loc()  # also makes the section the accessed one
            try:
                sec = n.cas()
            finally:
                n.pop_section()
        return _segment_of(sec, x)

    def _netcon_seg(self, loc_method):
        """NetCon.preseg()/postseg(): the Segment from preloc()/postloc().

        Core builds these through a hook only NEURON's own Python module
        installs (nrnpy_seg_from_sec_x); without it they return nil.
        """

        n = _PACKAGE._n
        with HOC_LOCK:  # from preloc's push to the pop (gap-72)
            x = self.__getattr__(loc_method)()
            if x == -1:  # no source/target section; nothing was pushed
                return None
            try:
                sec = n.cas()
            finally:
                n.pop_section()
        if x < 0:  # source is not a voltage
            return None
        return _segment_of(sec, x)

    def _check_located(self):
        """Raise TypeError, as NEURON does, before reading, writing or taking
        a reference to a variable of a point process that is in no section.

        Never placed (h.IClamp()), or its section was deleted: the property
        functions throw "point process not located in a section" through
        ctypes, which aborts. Core is asked each time, as loc() and deletion
        move it without this wrapper. Methods such as get_segment() and
        has_loc() still work. An ARTIFICIAL_CELL is in no section but has data.
        """
        name = self._class_name
        known = _IS_POINT_PROCESS.get(name)
        if known is None:
            known = is_point_process_class(name)
        if known and not _nrn_prop_exists(self._obj):
            raise TypeError("No value")

    def _assign_pointer(self, member, src_ref):
        """Wire this object's NMODL POINTER ``member`` to the address referenced
        by ``src_ref`` (an NrnRef), implementing ``obj._ref_PTR = src._ref_X``.

        The public ``nrn_pp_setpointer_pop`` API addresses the point process
        directly and consumes exactly one source handle.
        """
        member_encoded = member.encode("utf-8")
        err = _ERR_BUF()
        with HOC_LOCK:  # the pop must take this push (gap-72)
            src_ref._push()
            rc = _nrn_pp_setpointer_pop(self._obj, member_encoded, err, _ERR_BUF_SIZE)
        if rc:
            msg = err.value.decode("utf-8", errors="replace")
            raise RuntimeError(msg or f"setpointer {member} failed")

    def __setattr__(self, name, val):
        # POINTER assignment: obj._ref_PTR = src._ref_X wires an NMODL POINTER.
        # Must come before the generic underscore short-circuit below.
        if name.startswith("_ref_"):
            if isinstance(val, NrnRef):
                self._check_located()
                self._assign_pointer(name[5:], val)
                return
            member = name[5:]
            if member in self._load_methods():
                sym = _nrn_method_symbol(self._obj, member.encode("utf-8"))
                if (
                    sym
                    and int(_nrn_symbol_subtype(sym)) == _PACKAGE.sections._NRNPOINTER
                ):
                    raise ValueError("must be a hoc pointer")
                raise AttributeError(
                    " For assignment, only POINTER var can have a _ref_ prefix"
                )
        # Underscore names are internal bookkeeping, never HOC properties.
        if name.startswith("_"):
            super().__setattr__(name, val)
            return
        # A subclass with a per-instance __dict__ may carry ad-hoc Python
        # attributes (a.bp = ...) unless the name is a real HOC property.
        if type(self).__dictoffset__:  # also when a base class added the __dict__
            try:
                known = name in self._load_methods()
            except AttributeError:  # not constructed yet
                known = False
            if not known:
                object.__setattr__(self, name, val)
                return

        # `delay` → `del` alias — see __dir__ comment above.
        actual_name = name
        try:
            methods = self._load_methods()
        except AttributeError:  # not constructed yet
            methods = {}
        if name == "delay" and name not in methods and "del" in methods:
            actual_name = "del"

        # Allow setting scalar object properties (RANGEVAR or VAR doubles).
        # A user HOC template's VAR lives in its object dataspace, not behind
        # the C++ property `steer` callback used by nrn_property_set.
        member = methods.get(actual_name)
        if member is not None:
            _type, _ = member

            if _type == TYPES.STRING:
                _write_hoc_template_string(self, actual_name, val)
                return
            if _type == TYPES.OBJECTVAR:
                _write_object_member(self, actual_name, val)
                return
            if _type in (TYPES.RANGEVAR, TYPES.VAR):
                name_encoded = actual_name.encode("utf-8")
                sym = _nrn_method_symbol(self._obj, name_encoded)
                if not _nrn_symbol_is_array(sym):
                    if val.__class__ is not float and val.__class__ is not int:
                        # User code in the conversion may delete the section:
                        # convert, then check.
                        val = _to_double(val, TypeError)
                    self._check_located()
                    if _type == TYPES.VAR:
                        _write_hoc_template_var(self, actual_name, val)
                    else:
                        _property_set_checked(self._obj, name_encoded, val)
                    return
                if len(symbol_array_dims(sym)) > 1:
                    raise TypeError(f"'{name}' requires subscript for assignment")
                # Array property: a sequence of the right length sets each
                # element (same as the MATLAB interface).
                self._check_located()
                size = self._array_size(actual_name, sym)
                try:
                    seq = list(val)
                except TypeError:
                    raise TypeError(
                        f"Array property '{name}' (length {size}) requires a sequence, "
                        f"got {type(val).__name__}"
                    ) from None
                if len(seq) != size:
                    raise ValueError(
                        f"Array property '{name}' has length {size}; "
                        f"got {len(seq)} values"
                    )
                seq = [_to_double(v, TypeError) for v in seq]
                self._check_located()
                for i, v in enumerate(seq):
                    if _type == TYPES.VAR:
                        _write_hoc_template_var(self, actual_name, v, idx=i)
                    else:
                        _property_array_set_checked(self._obj, name_encoded, i, v)
                return
            if TYPES.RANGEOBJ is None or _type == TYPES.RANGEOBJ:
                sym = _nrn_method_symbol(self._obj, actual_name.encode("utf-8"))
                wrapped = _try_wrap_point_nmodlrandom(self._obj, sym, _type)
                if wrapped is not None:
                    raise TypeError(
                        f"NMODL RANDOM variable '{actual_name}' is not assignable"
                    )
        # StringFunctions alias write (obj.aliasname = v) — see __getattr__.
        if _write_object_alias(self, name, val):
            return
        if member is not None:
            raise TypeError("not assignable")  # NEURON's text for a method
        raise _unknown_name_error(self, name)

    def __getattr__(self, name):
        # _class_name is unset if a subclass __init__ skipped super().__init__().
        # Bail here, or the unset-slot AttributeError re-enters __getattr__
        # and recurses.
        methods = object.__getattribute__(self, "_methods")
        if methods is None:
            try:
                object.__getattribute__(self, "_class_name")
            except AttributeError:
                raise AttributeError(name) from None
            methods = self._load_methods()
        if name.startswith("_ref_"):
            return self._property_ref(name[5:])
        # NEURON's Python module adds get_segment() to every point process
        # that can be located (not an ARTIFICIAL_CELL). Ahead of the method
        # table: with NEURON's module loaded first, the table holds its native
        # get_segment, which returns NEURON's own Segment type.
        # A MOD file's FUNCTION get_segment comes first, as in NEURON.
        if (
            name == "get_segment"
            and is_located_point_process_class(self._class_name)
            and methods.get(name, (None,))[0] != TYPES.FUNCTION
        ):
            return self._get_segment
        if self._class_name == "NetCon" and name in _NETCON_SEG_METHODS:
            return functools.partial(self._netcon_seg, _NETCON_SEG_METHODS[name])
        # .plot() on RangeVarPlot / PlotShape — not a HOC method
        if name == "plot":
            if self._class_name == "RangeVarPlot":
                from .plotting import _RangeVarPlot

                return _RangeVarPlot(self)
            elif self._class_name == "PlotShape":
                from .plotting import _PlotShapePlot

                _PACKAGE._n.define_shape()
                return _PlotShapePlot(self)
        # PlotShape.variable also takes a Python object. Core keeps one only
        # through nrnpy_get_pyobj, which needs NEURON's Python module.
        if self._class_name == "PlotShape":
            if name == "variable":
                return self._plotshape_variable
            if name == "_hoc_variable":
                name = "variable"
        # `delay` → `del` alias — see __dir__ comment above.
        original_name = name
        if name == "delay" and name not in methods and "del" in methods:
            name = "del"
        member = methods.get(name)
        if member is not None:
            _type, _ = member
            qualified_name = f"{self._class_name}.{original_name}"

            # RANGEOBJ not yet discovered (a point-process-only MOD may be the
            # first RANGEOBJ seen, with no density symbol to learn the code
            # from): probe via the fail-closed adapter (see
            # api.TypeCodes.record_rangeobj). Free once RANGEOBJ is known.
            # Only a point process can have such a member, so every other
            # class (Vector, NetCon, a template) skips the probe: it was a
            # symbol lookup and a C call on each of its method accesses.
            if TYPES.RANGEOBJ is None and is_point_process_class(self._class_name):
                sym = _nrn_method_symbol(self._obj, name.encode("utf-8"))
                wrapped = _try_wrap_point_nmodlrandom(self._obj, sym, _type)
                if wrapped is not None:
                    return wrapped

            # --- type SECTION: template public section (cell.soma, cell.dend[i]) ---
            if _type == TYPES.SECTION:
                return self._template_section_entry(name).value(self, name)

            # --- type OBJECTVAR: template objref member (cell.all) ---
            if _type == TYPES.OBJECTVAR:
                return _object_member(self, name)

            # --- type STRING: user-template strdef member (obj.label) ---
            if _type == TYPES.STRING:
                return _read_hoc_template_string(self, name)

            # --- type RANGEVAR / VAR: scalar or array property (amp, tau1) ---
            if _type in (TYPES.RANGEVAR, TYPES.VAR):
                self._check_located()
                name_encoded = name.encode("utf-8")
                sym = _nrn_method_symbol(self._obj, name_encoded)
                if not _nrn_symbol_is_array(sym):
                    if _type == TYPES.VAR:
                        return _read_hoc_template_var(self, name)
                    return _property_get_checked(self._obj, name_encoded)
                else:
                    return _array_property(self, name, sym, _type, qualified_name)

            if TYPES.RANGEOBJ is not None and _type == TYPES.RANGEOBJ:
                sym = _nrn_method_symbol(self._obj, name.encode("utf-8"))
                wrapped = _try_wrap_point_nmodlrandom(self._obj, sym, _type)
                if wrapped is not None:
                    return wrapped

            elif _type == TYPES.METHOD_SECTIONREF:
                # NEURON: sr.sec/parent/root/trueparent are Sections (TypeError
                # "No value" when there is none, e.g. the parent of a root);
                # sr.child is an array, sr.child[i].
                if name == "child":
                    return _SectionRefChildren(self)
                try:
                    section = _steer_sectionref(self, name)
                except RuntimeError:
                    section = None  # HOC: the steering has no such section
                if section is None:
                    raise TypeError("No value")
                return section

            # --- all other callable types: FUNCTION, METHOD_OBFUNC/OBFUNCTION,
            #     METHOD_STRFUNC ---
            else:
                # NEURON's py_allgather/py_gather/py_broadcast read the raw
                # Python argument, so 5 stays an int; pass it as a
                # PythonObject instead of a HOC double.
                raw_first_arg = (
                    name in _PC_RAW_ARG_METHODS
                    and self._class_name == "ParallelContext"
                )

                # Python sections cannot be unnamed or renamed: core refuses for
                # Python-owned sections (secref.cpp), which myneuron's are not
                # in core until sections get a public owner (nrn fix 3).
                section_arg = name in _SECTION_ARG_METHODS.get(self._class_name, ())
                location_arg = name == "loc" and is_located_point_process_class(
                    self._class_name
                )
                pysec_name_change = self._class_name == "SectionRef" and name in (
                    "unname",
                    "rename",
                )

                bumps = name not in READ_ONLY_METHODS.get(self._class_name, ())

                @hoc_locked
                def method_func(*args, sec=_NO_SEC):
                    if sec is _NO_SEC:
                        sec = None
                    elif sec is None:
                        raise TypeError("sec is not a Section")
                    if pysec_name_change:
                        try:
                            ref_sec = _steer_sectionref(self, "sec")
                        except RuntimeError:
                            ref_sec = None

                        if (
                            ref_sec is not None
                            and ref_sec._sec in _PACKAGE.sections.PYTHON_SECTIONS
                        ):
                            return 0.0
                    if raw_first_arg and args:
                        args = (_PythonObjectArg(args[0]),) + args[1:]
                    if section_arg:
                        args, sec = _section_arg_as_sec(
                            args, sec, allow_object=name == "remove"
                        )

                    sym = _nrn_method_symbol(self._obj, name.encode("utf-8"))
                    # Liveness check before pushing (see utils._push_args).
                    if sec is not None:
                        _check_sec_kwarg(sec)
                    if location_arg and args:
                        _check_location_arg(args[0])
                    # Recorded before each push; see _construct_hoc_object.
                    pushed = [0]
                    pushed_sec = None
                    narg = len(args)
                    _err_buf = _ERR_BUF()
                    _err_buf[0] = b"\x01"  # the call clears it
                    try:
                        temp_strs, seg_sec = _utils._push_args(args, pushed)
                        use_sec = seg_sec if seg_sec is not None else sec
                        if use_sec is not None:
                            pushed_sec = [use_sec._sec]
                            _nrn_section_push(pushed_sec[0])
                        if _type == TYPES.FUNCTION:
                            number = ctypes.c_double()
                            code = _nrn_method_call_number(
                                self._obj,
                                sym,
                                narg,
                                number,
                                _err_buf,
                                _ERR_BUF_SIZE,
                            )
                            ret = code < 0
                        else:
                            ret = _nrn_method_call(
                                self._obj, sym, narg, _err_buf, _ERR_BUF_SIZE
                            )

                        if bumps:  # not utils.READ_ONLY_METHODS
                            _bump_model_epoch()
                        if ret:
                            # A failed nothrow call restores the args (gap-56).
                            # -2: the method ran and nothing of it is left.
                            if _type != TYPES.FUNCTION or code != -2:
                                _utils._pop_args(pushed)
                            failed, _err_buf = _err_buf, None  # no result
                            _check_nrn_error(ret, failed)
                        # Each branch marks the result popped right before its
                        # raw pop: on Python 3.10 a taken jump can take an
                        # interrupt, and a Python pop wrapper's entry can too.
                        if _type == TYPES.FUNCTION:
                            result = number.value
                            if code:
                                result = int(result) if code == 1 else bool(result)
                        elif _type == TYPES.METHOD_OBFUNC or _type == TYPES.OBFUNCTION:
                            _err_buf = None
                            result = _wrap_owned_object(_api._nrn_object_pop())
                        elif _type == TYPES.METHOD_STRFUNC:
                            _err_buf = None
                            result = _api._decode_hoc_string(_api._nrn_str_pop0())
                        elif _type == TYPES.PROCEDURE:
                            # A template proc: hoc_call_ob_proc drops its 0.0
                            # sentinel (hoc_nopop) and NEURON returns 0.0.
                            _err_buf = None
                            result = 0.0
                        else:
                            _err_buf = None
                            warnings.warn(f"Ignoring unknown return type {_type}")
                            result = None
                        # Raise a callback's pending exception here (e.g. a
                        # raising FInitializeHandler fired by finitialize()),
                        # after popping the return value, or failed callbacks
                        # leak operand-stack slots.
                        if PENDING_CALLBACKS:
                            raise_callback_error()
                        return result
                    except BaseException:
                        # A template proc leaves no result: core drops its sentinel.
                        # A number is off the stack: nrn_method_call_number.
                        _drop_return_slot(
                            _err_buf,
                            None
                            if _type == TYPES.PROCEDURE or _type == TYPES.FUNCTION
                            else _return_slot_pop(_type),
                            pushed,
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

                func = method_func
                if self._class_name == "ParallelContext" and not CALLS_PYTHON:
                    from .parallel import PYTHON_METHODS, python_method

                    if name in PYTHON_METHODS:
                        func = python_method(self, name, method_func)
                elif (
                    self._class_name == "ParallelContext"
                    and name in _ONE_ARG_COLLECTIVES
                ):
                    func = _one_arg_collective(self, name, method_func)
                doc = f"NEURON object method: {self._class_name}.{name}()"
                doc_key = f"{self._class_name}.{name}"
                if self._class_name == "Vector":
                    method = _VectorMethod(func, qualified_name, doc, doc_key=doc_key)
                    method._vector = self
                    return method
                return FuncWrapper(func, qualified_name, doc, doc_key=doc_key)
        # StringFunctions alias (obj.aliasname): not in the class symbol
        # table. Resolve via a HOC trampoline, since the native property
        # accessors SIGSEGV on a non-member name.
        if _has_alias(self, name):
            try:
                return _read_object_alias(self, name)
            except AttributeError:
                pass
        raise _name_error(
            AttributeError,
            name,
            f"'{self._class_name}' object has no attribute '{name}'",
        )


# Members of a HOC template object (begintemplate), public or not: NEURON's
# Python interface reaches every member, and HOC's `$o1.name` only public ones.
# nrn#3888 made nrn_property_get/set/array_get/array_set read a HOC template's
# doubles from the object's data, public or not; it also added
# nrn_property_object_get, which is what tells such a core apart. Without it
# those functions reach a member only through the steer function of a C++
# class, which a HOC template has none of. A statement run in the object's own
# scope, `execute(stmt, obj)`, reaches every member on any core, and strdefs
# have no C API at all. The value crosses in hoc_ac_, a built-in variable that
# every scope sees.
_HOC_AC = hoc_variable("hoc_ac_", ctypes.c_double)


_EXECUTE = None  # n.execute, the HOC built-in, looked up once


@hoc_locked
@contextlib.contextmanager
def _no_mpi_abort():
    """Keep a HOC error that is expected here from aborting MPI. The error is
    printed: silencing fd 2 would also lose what other threads write."""
    with HOC_LOCK:
        saved = _nrn_mpiabort_on_error(0)
        try:
            yield
        finally:
            _nrn_mpiabort_on_error(saved)


def _in_object_scope(owner, statement, value=None, element=None):
    """Run HOC *statement* in the scope of template object *owner* and return
    the value it left in hoc_ac_. *value*, if given, is in hoc_ac_ while it
    runs. hoc_ac_ is restored afterwards. The statement goes to HOC as UTF-8
    bytes; a str argument would have to be ASCII, and a strdef need not be.

    *element* names the array member the statement subscripts: a subscript past
    the object's own dimensions (the symbol's can differ) is an IndexError.
    """
    global _EXECUTE
    if _EXECUTE is None:
        _EXECUTE = _PACKAGE._n.execute
    saved = _HOC_AC.value
    try:
        if value is not None:
            _HOC_AC.value = value
        if element is None:
            _EXECUTE(statement.encode("utf-8"), owner)
        else:
            try:
                with _no_mpi_abort():
                    _EXECUTE(statement.encode("utf-8"), owner)
            except RuntimeError as exc:
                # Not one from HOC that the assignment ran (unref of the old
                # value, a callback).
                if f" of {element} out of range" not in str(exc):
                    raise
                raise IndexError(f"{owner._class_name}.{element}") from None
        return _HOC_AC.value
    finally:
        _HOC_AC.value = saved


def _member_expr(name, idx):
    return name + "".join(f"[{int(i)}]" for i in _subscripts(idx))


# The variables of a C++ class (NetCon.delay, NetCon.weight[i]) are public, so
# `$o1.name` reaches them; nrn_property_array_get could not until nrn#3888.
_HOC_TEMPLATE_VAR_TRAMPOLINES = set()


def _ensure_hoc_template_var_trampolines(name, array):
    key = (name, bool(array))
    if key in _HOC_TEMPLATE_VAR_TRAMPOLINES:
        return

    n = _PACKAGE._n
    kind = "a" if array else "s"
    get_name = f"_mn_hvget_{kind}_{name}"
    set_name = f"_mn_hvset_{kind}_{name}"
    if array:
        n(f"func {get_name}() {{ return $o1.{name}[$2] }}")
        n(f"proc {set_name}() {{ $o1.{name}[$2] = $3 }}")
    else:
        n(f"func {get_name}() {{ return $o1.{name} }}")
        n(f"proc {set_name}() {{ $o1.{name} = $2 }}")
    _HOC_TEMPLATE_VAR_TRAMPOLINES.add(key)


def _read_hoc_template_var(owner, name, idx=None):
    """Value of double member *name* (element *idx* of an array) of *owner*,
    a HOC template object or a C++ class instance (VAR members).

    The array accessors take a position in the object's storage, the subscript
    when there is one; with more it depends on the object's own dimensions,
    which only HOC's subscripting applies."""
    hoc_template = _is_hoc_template(owner._class_name)
    indices = _subscripts(idx)
    if _nrn_property_object_get is not None:
        if not indices:
            return _nrn_property_get(owner._obj, name.encode("utf-8"))
        if hoc_template and len(indices) == 1:
            return _property_array_get_checked(
                owner._obj, name.encode("utf-8"), indices[0]
            )
    if hoc_template:
        return _in_object_scope(
            owner,
            f"hoc_ac_ = {_member_expr(name, idx)}",
            element=name if indices else None,
        )

    array = idx is not None
    _ensure_hoc_template_var_trampolines(name, array)
    fn = f"_mn_hvget_{'a' if array else 's'}_{name}"
    args = (owner, idx) if array else (owner,)
    return getattr(_PACKAGE._n, fn)(*args)


def _write_hoc_template_var(owner, name, value, idx=None):
    value = _as_double(value)
    hoc_template = _is_hoc_template(owner._class_name)
    indices = _subscripts(idx)
    if _nrn_property_object_get is not None:
        if not indices:
            _nrn_property_set(owner._obj, name.encode("utf-8"), value)
            return
        if hoc_template and len(indices) == 1:
            _ref_property_array_set_checked(
                owner._obj, name.encode("utf-8"), indices[0], value
            )
            return
    if hoc_template:
        _in_object_scope(
            owner,
            f"{_member_expr(name, idx)} = hoc_ac_",
            value,
            element=name if indices else None,
        )
        return

    array = idx is not None
    _ensure_hoc_template_var_trampolines(name, array)
    fn = f"_mn_hvset_{'a' if array else 's'}_{name}"
    args = (owner, idx, value) if array else (owner, value)
    getattr(_PACKAGE._n, fn)(*args)


# HOC's lexer reads a backslash and one of b, f, n, r, t in a string literal as
# that control character, and a backslash and any other character as the
# character; a raw newline ends the literal with an error.
_HOC_STRING_ESCAPES = str.maketrans(
    {
        "\\": "\\\\",
        '"': '\\"',
        "\n": "\\n",
        "\r": "\\r",
        "\t": "\\t",
        "\b": "\\b",
        "\f": "\\f",
    }
)


def _read_hoc_template_string(owner, name):
    """Value of strdef member *name* of HOC template object *owner*, printed
    between the sentinels that _capture_hoc_string looks for."""
    statement = f'print "{_PACKAGE._STR_BEGIN}", {name}, "{_PACKAGE._STR_END}"'
    return _PACKAGE._capture_hoc_string(lambda: _in_object_scope(owner, statement))


def _write_hoc_template_string(owner, name, value):
    if not isinstance(value, str):
        raise TypeError(f"argument must be str, not {type(value).__name__}")
    if "\x00" in value:
        raise ValueError("embedded null character")
    literal = value.translate(_HOC_STRING_ESCAPES)
    _in_object_scope(owner, f'{name} = "{literal}"')


# StringFunctions alias support (obj.aliasname, obj.aliasname = v,
# obj._ref_aliasname). `StringFunctions().alias(obj, name, ref)` stores the
# alias in the object's runtime alias table, not the class symbol table, so
# nrn_property_get/set/push dereference garbage and SIGSEGV. HOC trampolines
# (`$o1.<name>`) consult the alias table as the interpreter does. One get/set
# pair per alias name serves every object (the proc takes $o1). Only a name
# NEURON's own lookup finds on the object gets trampolines (see _has_alias): a
# name that is not an alias runs no HOC.
_OBJECT_ALIAS_TRAMPOLINES = set()


def _has_alias(obj, name):
    """Whether *obj* has StringFunctions alias *name*. A plain identifier only:
    anything else is no alias, and no HOC trampoline is made for it."""
    if not name.isidentifier() or name.startswith("_"):
        return False
    return bool(_nrn_object_alias(obj._obj, name.encode("utf-8")))


def _ensure_alias_trampolines(name):
    if name not in _OBJECT_ALIAS_TRAMPOLINES:
        n = _PACKAGE._n
        n(f"func _mn_aliasget_{name}() {{ return $o1.{name} }}")
        n(f"proc _mn_aliasset_{name}() {{ $o1.{name} = $2 }}")
        n(f"obfunc _mn_aliasobj_{name}() {{ return $o1.{name} }}")
        _OBJECT_ALIAS_TRAMPOLINES.add(name)


def _segment_of(sec, x):
    """sec(x) through the base Section: NEURON builds the segment for
    get_segment(), preseg() and postseg() in C, so a subclass's __call__
    must not replace it."""

    return _PACKAGE.sections.Section.__call__(sec, x)


# NetCon.preseg/postseg rebuilt from the location methods (Object._netcon_seg).
_NETCON_SEG_METHODS = {"preseg": "preloc", "postseg": "postloc"}

_PC_RAW_ARG_METHODS = frozenset({"py_allgather", "py_gather", "py_broadcast"})

# Core reads a positional Section in these through nrn_secarg, which unwraps it
# with a hook that only NEURON's own Python module installs (nrnpy_o2sec_p_);
# without it, nrn_secarg silently uses the currently accessed section instead.
# Passing the Section as the accessed section (sec=) is what nrn_secarg then
# reads.
_SECTION_ARG_METHODS = {
    "SectionList": frozenset(
        {"append", "children", "contains", "remove", "subtree", "wholetree"}
    ),
}


def _section_arg_as_sec(args, sec, allow_object=False):
    """``(args, sec)`` with a lone Section argument moved to ``sec``.

    Any other lone argument is an error, as in NEURON, except a HOC object
    where ``allow_object`` (``SectionList.remove(other_list)``).
    """

    if len(args) != 1:
        return args, sec
    arg = args[0]
    if isinstance(arg, _PACKAGE.sections.Section):
        return (), arg
    if allow_object and isinstance(arg, Object):
        return args, sec
    raise RuntimeError("hoc_execerror: not a Python nrn.Section")


def _check_location_arg(arg):
    """Raise NEURON's error for a point-process location core cannot read.

    A core that does not call Python itself asserts on an object it cannot
    read as a location, and the process aborts. A Segment never gets that far
    (_push_args sends its x and pushes its section). Core checks numbers.
    """
    if CALLS_PYTHON:
        return
    from ._released_core import reads_locations

    if reads_locations():
        return

    if isinstance(arg, _PACKAGE.sections.Segment):
        if not _nrn_section_is_active(arg.sec._sec):
            raise RuntimeError(
                "hoc_execerror: nrn.Segment associated with deleted internal Section"
            )
    elif (
        arg is None
        or isinstance(arg, Object)
        or (
            not isinstance(arg, (int, float, str, bytes, NrnRef))
            and _is_python_object_arg(arg)
        )
    ):
        raise RuntimeError("hoc_execerror: not a Python nrn.Segment")


class _SuppressCStderr:
    """Keep the HOC error of an expected miss quiet for the wrapped call only.

    NEURON prints "'<name>' not a public member" to fd 2 on a benign alias
    miss even though the error is caught, and on more than one rank a HOC error
    aborts MPI. alias_list would avoid the probe but needs the String template,
    so probe and suppress both.

    fd 2 and nrn_mpiabort_on_error_ belong to the process, and dup2 releases
    the GIL. HOC_LOCK is held from enter to exit, and only the outermost of
    nested uses saves and restores them; otherwise two threads' saves and
    restores interleave and leave fd 2 on /dev/null and the flag 0 (gap-72).
    """

    __slots__ = ()

    def __enter__(self):
        global _stderr_quiet_depth, _stderr_quiet_saved
        HOC_LOCK.acquire()
        if _stderr_quiet_depth == 0:
            try:
                saved = os.dup(2)
                devnull = os.open(os.devnull, os.O_WRONLY)
                os.dup2(devnull, 2)
                os.close(devnull)
            except BaseException:
                HOC_LOCK.release()
                raise
            _stderr_quiet_saved = (saved, _nrn_mpiabort_on_error(0))
        _stderr_quiet_depth += 1
        return self

    def __exit__(self, *exc):
        global _stderr_quiet_depth
        _stderr_quiet_depth -= 1
        try:
            if _stderr_quiet_depth == 0:
                saved, abort_on_error = _stderr_quiet_saved
                _nrn_mpiabort_on_error(abort_on_error)
                os.dup2(saved, 2)
                os.close(saved)
        finally:
            HOC_LOCK.release()
        return False


# _SuppressCStderr nesting depth, and fd 2's copy and nrn_mpiabort_on_error_
# as the outermost level found them. Read and written under HOC_LOCK.
_stderr_quiet_depth = 0
_stderr_quiet_saved = None


def _read_object_alias(obj, name):
    """Return the value of StringFunctions alias *name* on *obj*; raise
    AttributeError if *obj* has no such alias (the HOC '... not a public member'
    error, surfaced as RuntimeError by the dispatch)."""

    if not _has_alias(obj, name):
        raise AttributeError(name)
    _ensure_alias_trampolines(name)
    n = _PACKAGE._n
    try:
        with _SuppressCStderr():
            return getattr(n, f"_mn_aliasget_{name}")(obj)
    except RuntimeError:
        pass
    # An object alias (sf.alias(obj, name, someobj)) only reads in an
    # object context.
    try:
        with _SuppressCStderr():
            return getattr(n, f"_mn_aliasobj_{name}")(obj)
    except RuntimeError:
        raise AttributeError(name) from None


def _write_object_alias(obj, name, value):
    """Write *value* to StringFunctions alias *name* on *obj*. Returns True if
    written, False if *obj* has no such alias (or *value* isn't numeric)."""

    if not _has_alias(obj, name):
        return False
    _ensure_alias_trampolines(name)
    try:
        fv = float(value)
    except (TypeError, ValueError):
        return False
    try:
        with _SuppressCStderr():
            getattr(_PACKAGE._n, f"_mn_aliasset_{name}")(obj, fv)
        return True
    except RuntimeError:
        return False


class _AliasRef:
    """``obj._ref_<alias>`` for a StringFunctions alias; ``[0]`` reads and
    writes through the alias trampolines (see the block comment above).
    Holds the object alive; an alias ref is lightweight."""

    __slots__ = ("_obj", "_name")

    def __init__(self, obj, name):
        self._obj = obj
        self._name = name

    def __getitem__(self, idx):
        if idx != 0:
            raise IndexError("Only index 0 is supported")
        return _read_object_alias(self._obj, self._name)

    def __setitem__(self, idx, value):
        if idx != 0:
            raise IndexError("Only index 0 is supported")
        if not _write_object_alias(self._obj, self._name, value):
            raise AttributeError(f"no alias '{self._name}' on {self._obj._class_name}")

    def __repr__(self):
        return f"<AliasRef to {self._obj._class_name}.{self._name}>"


def _alias_ref(obj, name):
    """Return an _AliasRef if *name* is a StringFunctions alias on *obj*, else
    None (confirmed by probing the read trampoline)."""
    try:
        _read_object_alias(obj, name)
    except AttributeError:
        return None
    return _AliasRef(obj, name)


# ---------------------------------------------------------------------------
# Container subclasses — List, Vector, SectionList
# ---------------------------------------------------------------------------


class List(Object):
    """A HOC ``List`` — ordered, indexable container of HOC objects.

    Create with ``n.List()``. Supports ``len(lst)``, ``lst[i]``, and any
    HOC ``List`` method (``append``, ``remove``, ``count``, ``object``,
    etc.) via attribute dispatch.
    """

    __slots__ = ()
    __module__ = HOC_CLASS_MODULE
    _hoc_class_name = "List"

    def __bool__(self):
        return int(self.count()) > 0

    def __len__(self):
        return int(self.count())

    def __getitem__(self, i):
        if i < 0:
            i += len(self)
        if i < 0 or i >= len(self):
            raise IndexError("List index out of range")
        return self.object(i)


_PyBUF_WRITE = 0x200
_memory_view = ctypes.PYFUNCTYPE(
    ctypes.py_object, ctypes.c_void_p, ctypes.c_ssize_t, ctypes.c_int
)(("PyMemoryView_FromMemory", ctypes.pythonapi))


def _vector_view(vec):
    """A view of a Vector's data. Good until the Vector is resized."""
    size = len(vec) * ctypes.sizeof(ctypes.c_double)
    if not size:
        return memoryview(b"").cast("d")
    return _memory_view(_nrn_vector_data(vec._obj), size, _PyBUF_WRITE).cast("d")


def _vector_values(seq):
    """Return *seq* as a buffer of doubles, with NEURON's error for a non-number."""
    if type(seq) is list or type(seq) is tuple:
        values = seq
    elif isinstance(seq, list):  # its own items, as PyList_GetItem
        values = list.copy(seq)
    elif (
        type(seq).__flags__ & _Py_TPFLAGS_HEAPTYPE
        and hasattr(seq, "__getitem__")
        and not isinstance(seq, dict)
    ):
        # A Python class, as PySequence_GetItem; a built-in's iterator is faster.
        values = list(map(seq.__getitem__, range(len(seq))))
    else:
        try:
            view = memoryview(seq)  # of a numpy array, for one
            if view.format == "d" and view.ndim == 1:
                return view
        except TypeError:
            pass
        values = list(seq)
    try:
        return array.array("d", values)
    except TypeError:
        # A size-1 array item, as the rows of an (n, 1) array, is a number to
        # NEURON, which reads the array's doubles.
        values = [v.item() if getattr(v, "size", 0) == 1 else v for v in values]
        for i, value in enumerate(values):
            try:
                float(value)
            except (TypeError, ValueError):
                raise RuntimeError(f"item {i} is not a valid number") from None
        return array.array("d", values)


def _unreadable_vector_source(value):
    """True if n.Vector(value) would hand HOC a new PythonObject that Python
    can neither iterate nor index, such as object() or a ctypes scalar."""
    if value is None or isinstance(value, (Object, NrnRef)):
        return False
    if not _is_python_object_arg(value):
        return False
    try:
        iter(value)
    except TypeError:
        return True
    return False


def _copy_into_vector(vec, values):
    """Copy what _vector_values returned into a Vector already sized to hold it."""
    if values:
        if type(values) is array.array:
            address, count = values.buffer_info()
            ctypes.memmove(_nrn_vector_data(vec._obj), address, count * values.itemsize)
        else:
            _vector_view(vec)[:] = values


class _VectorXAccessor:
    """Proxy for ``v.x[i]``, delegating to the Vector. Without it ``v.x`` would
    reach HOC dispatch and return a float."""

    __slots__ = ("_vec",)

    def __init__(self, vec):
        self._vec = vec

    def __getitem__(self, i):
        return self._vec[i]

    def __setitem__(self, i, val):
        self._vec[i] = val

    def __len__(self):
        return len(self._vec)

    def __iter__(self):
        return iter(self._vec)

    def __repr__(self):
        return f"<Vector.x ({len(self._vec)} elements)>"


def _pkl(arg):
    """Unpickling a Vector starts from an empty one (NEURON's neuron._pkl).

    Standalone pickles name this function. Inside NEURON they name
    neuron._pkl, but this one still loads the pickles that named it."""

    return _PACKAGE._n.Vector(0)


# A Vector's uncalled method. NEURON's is a HocObject of the Vector, so
# np.asarray(v.size) reads the Vector's data.
class _VectorMethod(FuncWrapper):
    __slots__ = ("_vector",)
    # A class body sets __doc__ itself; keep FuncWrapper's help-text property.
    __doc__ = FuncWrapper.__dict__["__doc__"]

    @property
    def __array_interface__(self):
        return self._vector.__array_interface__


_VECTOR_BASES = () if VECTOR_BASE is None else (VECTOR_BASE,)


class Vector(Object, *_VECTOR_BASES):
    """A HOC ``Vector`` — dynamic array of doubles with 100+ HOC methods.

    Create with ``n.Vector()``, ``n.Vector(size)``, or ``n.Vector([1, 2, 3])``.
    Supports indexing (``v[0]``, ``v[-1]``), slicing, iteration, ``len()``,
    and any HOC ``Vector`` method (``max``, ``min``, ``sum``, ``mean``,
    ``sort``, ``record``, ``dot``, etc.) via attribute dispatch.
    """

    __slots__ = ()
    __module__ = HOC_CLASS_MODULE
    _hoc_class_name = "Vector"

    def __copy__(self):
        """Return an independent native Vector, matching ``Vector.c()``."""
        return self.c()

    def __deepcopy__(self, memo):
        """Return an independent native Vector and memoize the wrapper."""
        copied = self.c()
        memo[id(self)] = copied
        return copied

    def __reduce_ex__(self, protocol):
        """Pickle as NEURON's hocpickle_reduce: a new Vector, then the state
        (version 1, the double 2.0 as an endianness sentinel, size, raw data).

        Inside NEURON the pickle names neuron._pkl, as NEURON's own Vector
        pickles do, so legacy NEURON and this interface load each other's."""
        data = array.array("d", self.to_python())
        sentinel = array.array("d", [2.0]).tobytes()
        rebuild = sys.modules["neuron"]._pkl if _VENDORED else _pkl
        return (rebuild, (0,), (1, sentinel, len(data), data.tobytes()))

    def __setstate__(self, state):
        version, sentinel, size, raw = state
        if not isinstance(raw, bytes) or not isinstance(sentinel, bytes):
            raise TypeError("pickle not returning string")
        if len(sentinel) != 8:
            raise ValueError("endian_data size is not sizeof(double)")
        if len(raw) != size * 8:
            raise ValueError("buffer size does not match array size")
        data = array.array("d")
        data.frombytes(raw)
        check = array.array("d")
        check.frombytes(sentinel)
        if check[0] != 2.0:  # written with the other byte order
            data.byteswap()
        self.from_python(data)

    @property
    def x(self):
        return _VectorXAccessor(self)

    # Element access reads and writes the data pointer directly, one ctypes
    # call instead of a HOC method call per element.
    def _index(self, i):
        size = _nrn_vector_capacity(self._obj)
        i = operator.index(i)
        if i < 0:
            i += size
        if i < 0 or i >= size:
            raise IndexError("Vector index out of range")
        return i

    def _getitem(self, i):
        if isinstance(i, slice):
            # A new Vector holding a copy, as NEURON's slice (new_vect).
            start, stop, step = i.indices(_nrn_vector_capacity(self._obj))
            count = len(range(start, stop, step))
            result = Vector(count)
            if count and step == 1:
                source = ctypes.cast(_nrn_vector_data(self._obj), ctypes.c_void_p)
                ctypes.memmove(
                    _nrn_vector_data(result._obj), source.value + 8 * start, 8 * count
                )
            elif count:
                values = array.array("d", _nrn_vector_data(self._obj)[start:stop:step])
                _copy_into_vector(result, values)
            return result
        return _nrn_vector_data(self._obj)[self._index(i)]

    def _setitem(self, i, val):
        if isinstance(i, slice):
            indices = range(*i.indices(len(self)))
            vals = list(val)
            if len(vals) != len(indices):
                raise IndexError(
                    f"attempt to assign sequence of size {len(vals)} "
                    f"to extended slice of size {len(indices)}"
                )
            data = _nrn_vector_data(self._obj)
            for idx, v in zip(indices, vals):
                data[idx] = v
            return
        _nrn_vector_data(self._obj)[self._index(i)] = val

    # The core's element access for an int index; it calls the two above for
    # anything else (prototype, nrnpy_fast.cpp).
    if VECTOR_BASE is None:
        __getitem__ = _getitem
        __setitem__ = _setitem
    else:
        __getitem__ = VECTOR_BASE.__getitem__
        __setitem__ = VECTOR_BASE.__setitem__
        __delitem__ = VECTOR_BASE.__delitem__

    def __iter__(self):
        # Live, as in NEURON: elements appended during the loop are visited.
        # Whatever resizes the Vector moves the model epoch.
        obj, current, epoch = self._obj, EPOCH, self  # no epoch
        i = size = 0
        while True:
            if current[0] != epoch:
                epoch = _model_epoch()
                size, data = _nrn_vector_capacity(obj), _nrn_vector_data(obj)
            if i >= size:
                return
            yield data[i]
            i += 1

    @property
    def __array_interface__(self):
        """Numpy array-interface protocol.

        `numpy.array(vec)` copies; `numpy.asarray(vec)` returns a
        **read-only** view, as real NEURON does. Use `vec.as_numpy()` for a
        mutable view, so a write through `np.asarray` cannot corrupt the
        Vector's storage.

        Empty vectors borrow a real empty array's interface. A NULL data
        pointer `(0, True)` is rejected by stricter numpy builds, which then
        fall back to the sequence protocol and fail in `float(vec)` with
        `TypeError: float() argument must be a string or a real number`.

        The dict is rebuilt on every access so resizes are seen. In tight
        loops, pin `numpy.asarray()` outside the loop or use `as_numpy()`.
        """
        size = len(self)
        if size == 0:
            # numpy guarantees its own empty-array data pointer is safe to expose.
            import numpy as _np

            return _np.empty(0, dtype=_np.float64).__array_interface__
        ptr = _nrn_vector_data(self._obj)
        addr = ctypes.cast(ptr, ctypes.c_void_p).value or 0
        return {
            "version": 3,
            "shape": (size,),
            "typestr": "<f8",  # little-endian float64
            "data": (addr, True),  # (address, read-only=True)
        }

    def as_numpy(self):
        """Return a numpy array view of this Vector's data (zero-copy)."""
        import numpy

        size = len(self)
        if size == 0:
            return numpy.array([], dtype=float)
        return numpy.ndarray((size,), float, _vector_view(self), order="C")

    @help_doc("Vector.to_python")
    def to_python(self, dest=None):
        """Return Vector contents as a Python list (or fill *dest*)."""
        size = len(self)
        if dest is not None:
            if not hasattr(dest, "__getitem__") or isinstance(dest, dict):
                raise RuntimeError(f"hoc_execerror: {self} is not a Python Sequence")
            if len(dest) != size:
                raise RuntimeError(
                    f"hoc_execerror: {self} Python Sequence not same size as Vector"
                )
        if dest is None or size <= 32:  # faster than a view
            data = _nrn_vector_data(self._obj)[:size] if size else []
            if dest is None:
                return data
            if type(dest) is list:
                dest[:] = data
                return dest
        else:
            data = _vector_view(self)
            if getattr(getattr(dest, "dtype", None), "hasobject", True):
                # A copy: the __del__ of what is replaced may resize the Vector.
                data = memoryview(data.tobytes()).cast("d")
        if isinstance(dest, list):
            for i in range(0, size, 4096):  # the next numbers reuse the replaced ones
                dest[i : i + 4096] = data[i : i + 4096]
        elif hasattr(dest, "dtype") and dest.ndim == 1:  # numpy
            dest[:] = data
        else:
            for i, value in enumerate(data):
                dest[i] = value
        return dest

    @help_doc("Vector.from_python")
    def from_python(self, seq):
        """Fill this Vector from a Python sequence. Resizes to match."""
        if isinstance(seq, collections.abc.Iterator) and not _PySequence_Check(seq):
            return self._append_values(seq)
        values = _vector_values(seq)
        self.resize(len(values))
        _copy_into_vector(self, values)
        return self

    def _append_values(self, iterator):
        """NEURON's from_python of an iterator: what it made before it failed
        stays."""
        values = array.array("d")
        try:
            for value in iterator:
                try:
                    values.append(value)
                except TypeError:
                    raise RuntimeError(
                        f"item {len(values)} is not a valid number"
                    ) from None
        finally:
            values = _vector_values(self.to_python()) + values
            self.resize(len(values))
            _copy_into_vector(self, values)
        return self

    def __len__(self):
        return _nrn_vector_capacity(self._obj)

    # --- Arithmetic (numeric protocol) ---
    # As legacy NEURON's nrnpy_vec_math (9.0's neuron/__init__.py), called
    # from py_hocobj_math (9.0's nrnpy_hoc.cpp). The other operand is a
    # number or a HOC object, which the HOC method rejects if it is not a
    # Vector; anything else is NotImplemented. Forward ops clone via `.c()`
    # then call the HOC method. Reversed sub/div use `mul(-1).add(x)` /
    # `pow(-1).mul(x)`; unary `-` uses `mul(-1)`. There are no in-place
    # operators, as in NEURON: `v += x` binds v to the new Vector that `v + x`
    # makes and leaves the old one (an alias, a recording Vector) alone.

    @staticmethod
    def _is_arith_operand(other):
        return isinstance(other, (Object, numbers.Number))

    def __add__(self, other):
        if not Vector._is_arith_operand(other):
            return NotImplemented
        return self.c().add(other)

    def __radd__(self, other):
        if not Vector._is_arith_operand(other):
            return NotImplemented
        return self.c().add(other)

    def __sub__(self, other):
        if not Vector._is_arith_operand(other):
            return NotImplemented
        return self.c().sub(other)

    def __rsub__(self, other):
        if not Vector._is_arith_operand(other):
            return NotImplemented
        return self.c().mul(-1).add(other)

    def __mul__(self, other):
        if not Vector._is_arith_operand(other):
            return NotImplemented
        return self.c().mul(other)

    def __rmul__(self, other):
        if not Vector._is_arith_operand(other):
            return NotImplemented
        return self.c().mul(other)

    def __truediv__(self, other):
        if not Vector._is_arith_operand(other):
            return NotImplemented
        return self.c().div(other)

    def __rtruediv__(self, other):
        if not Vector._is_arith_operand(other):
            return NotImplemented
        return self.c().pow(-1).mul(other)

    def __neg__(self):
        return self.c().mul(-1)

    def __pos__(self):
        return self.c()

    def __abs__(self):
        return self.c().abs()


if VECTOR_BASE is not None:  # only exact instances take the core's fast path
    SEGMENT_BASE._fast_vector_class = Vector


class SectionList(Object):
    """HOC SectionList — collection of sections with morphology operations.

    Iteration uses the C iterator API as allsec() does, with a fresh
    iterator per ``__iter__`` so nested loops work. Morphology methods
    (append, remove, children, subtree, ...) dispatch to HOC.

    Usage:
        sl = n.SectionList()                 # empty
        sl = n.SectionList([sec1, sec2])     # pre-populated
        sl.append(sec=soma)
        sl.wholetree(sec=soma)
        for sec in sl: ...
        soma in sl
        len(sl)
    """

    __slots__ = ()
    __module__ = HOC_CLASS_MODULE
    _hoc_class_name = "SectionList"

    def __iter__(self):

        # sections._section_for, inlined as in allsec(): a Python section is
        # its own object, a HOC one gets a new wrapper.
        owner_refs = _PACKAGE.sections.PYTHON_SECTIONS
        from_ptr = _PACKAGE.sections.Section._from_ptr
        items = _nrn_sectionlist_data(self._obj)
        it = _nrn_sectionlist_iterator_new(items)
        try:
            while not _nrn_sectionlist_iterator_done(it):
                sec_ptr = _nrn_sectionlist_iterator_next(it)
                # next() may discard dead entries and reach the sentinel.
                if not sec_ptr:
                    break
                owner_ref = owner_refs.get(sec_ptr)
                sec = owner_ref() if owner_ref is not None else None
                yield sec if sec is not None else from_ptr(sec_ptr)
        finally:
            _nrn_sectionlist_iterator_free(it)

    def __len__(self):
        items = _nrn_sectionlist_data(self._obj)
        return int(_nrn_sectionlist_to_array(items, None, 0))

    def __bool__(self):
        return len(self) != 0

    def __getitem__(self, index):
        # SectionList is backed by a C linked list, so indexing is O(n).
        # For repeated random access, materialize once with list(sl).
        if isinstance(index, slice):
            return list(self)[index]
        if not isinstance(index, int):
            raise TypeError(
                f"SectionList indices must be int or slice, not {type(index).__name__}"
            )
        if index < 0:
            secs = list(self)
            try:
                return secs[index]
            except IndexError:
                raise IndexError("SectionList index out of range") from None
        for i, sec in enumerate(self):
            if i == index:
                return sec
        raise IndexError("SectionList index out of range")


# ---------------------------------------------------------------------------
# Template section helpers — _resolve_template_section, _TemplateSectionArray
# ---------------------------------------------------------------------------


@hoc_locked
def _resolve_template_section(owner, name, idx=None):
    """Push a HOC section onto the stack via braces and snapshot it.

    Used for `cell.soma`, `cell.dend[i]`, and the top-level `n.soma` /
    `n.dend[i]`. Type-307 symbols are parsed as section references and
    `nrn_method_call` cannot dispatch them. `name { _mn_capture_cas() }`
    makes the section cas() inside the block and the proc records it; the
    braces pop the section when the block ends. The captured Section* stays
    valid because the template (or HOC top level) owns the section.

    `owner` is the template Object, or None for a top-level `create`d
    section (no `owner.` prefix in the command).
    """

    indices = _subscripts(idx)
    if owner is not None:
        owner_name = _instance_hname(owner)
        if len(indices) > 1:
            return _nd_component_section(owner_name, name, indices)
        sym = _nrn_method_symbol(owner._obj, name.encode("utf-8"))
        if indices and indices[0] >= int(_nrn_symbol_array_length(sym)):
            # HOC code checks an index against the length the array was last
            # declared with, by any object of the template; only the component
            # syntax (obj.dend[i]) checks it against the object's own length,
            # which can be longer. That syntax reaches public sections only.
            return _component_section(owner, owner_name, name, indices[0])
    # Save/restore the module-global slot and read into a local, so a nested
    # section resolution during the block cannot clobber the pending value.
    saved = _PACKAGE._template_section_capture.value
    _PACKAGE._template_section_capture.value = 0
    suffix = "".join(f"[{i}]" for i in indices)
    cmd = f"{name}{suffix} {{ _mn_capture_cas() }}"
    hoc_ac = _HOC_AC
    saved_ac = hoc_ac.value
    if owner is not None:
        # This hoc_ac_ is the address; braces: no echo. In the object's scope,
        # which reaches sections that are not public but not our top-level
        # proc.
        hoc_ac.value = 0.0
        cmd = f'{{execute("{name}{suffix} hoc_ac_ = this_section()", {owner_name})}}'
    try:
        ret = _nrn_hoc_call(cmd.encode("utf-8"))
        captured = _PACKAGE._template_section_capture.value
        if owner is not None:
            captured = int(hoc_ac.value)
    finally:
        _PACKAGE._template_section_capture.value = saved
        hoc_ac.value = saved_ac
    if ret != 0 or not captured:
        scope = owner._class_name + "." if owner is not None else ""
        raise AttributeError(f"Could not access section '{scope}{name}{suffix}'")
    return _PACKAGE.sections.Section._from_ptr(captured)


def _nd_component_section(owner_name, name, indices):
    """Element *indices* (several subscripts) of section array *name* of the
    object *owner_name*, by the component syntax, which checks them against the
    object's own dimensions; the object's scope checks the symbol's, and
    indexes outside the object's sections past them. It reaches public members
    only and section_exists takes one subscript, so a missing element and a
    private array alike are a HOC error, printed, and an IndexError.
    """
    where = f"{owner_name}.{_member_expr(name, indices)}"
    hoc_ac = _HOC_AC
    saved_ac = hoc_ac.value
    hoc_ac.value = 0.0
    try:
        with _no_mpi_abort():
            ret = _nrn_hoc_call(f"{{{where} hoc_ac_ = this_section()}}".encode())
        captured = int(hoc_ac.value)
    finally:
        hoc_ac.value = saved_ac
    if ret != 0 or not captured:
        raise IndexError(f"{where} is past the object's dimensions or not public")
    return _PACKAGE.sections.Section._from_ptr(captured)


# (class name, section array name) -> whether the component syntax reaches it
# (only a public member).
_COMPONENT_SECTION_REACHABLE = {}


def _component_section(owner, owner_name, name, idx):
    """Element *idx* of section array *name* of *owner*, past the length the
    array was last declared with, through the component syntax.

    That syntax refuses a member that is not public with a HOC error, which
    would print and, on more than one MPI rank, abort. Whether the member is
    reached is learned once per class, on an element that exists, with that
    error kept quiet (_SuppressCStderr); an element of a member that is not
    public then raises IndexError here without asking HOC, since nothing else
    reaches it.
    """

    where = f"{owner._class_name}.{name}[{idx}]"
    key = (owner._class_name, name)
    reachable = _COMPONENT_SECTION_REACHABLE.get(key)
    if reachable is False:
        raise IndexError(
            f"{where} is past the length '{name}' was last declared with, "
            f"and '{name}' is not public"
        )
    if not _PACKAGE._n.section_exists(name, idx, owner):
        raise AttributeError(f"Could not access section '{where}'")
    hoc_ac = _HOC_AC
    saved_ac = hoc_ac.value
    hoc_ac.value = 0.0
    cmd = f"{{{owner_name}.{name}[{idx}] hoc_ac_ = this_section()}}".encode()
    try:
        if reachable is None:
            with _SuppressCStderr():
                ret = _nrn_hoc_call(cmd)
            _COMPONENT_SECTION_REACHABLE[key] = ret == 0
        else:
            ret = _nrn_hoc_call(cmd)
        captured = int(hoc_ac.value)
    finally:
        hoc_ac.value = saved_ac
    if ret != 0 and _COMPONENT_SECTION_REACHABLE[key] is False:
        return _component_section(owner, owner_name, name, idx)
    if ret != 0 or not captured:
        raise AttributeError(f"Could not access section '{where}'")
    return _PACKAGE.sections.Section._from_ptr(captured)


# SECTIONREF steering trampolines (one proc per steering symbol). Cached so the
# proc is defined once. See _steer_sectionref.
_SECTIONREF_STEER_TRAMPOLINES = set()


def _steer_sectionref(sr_obj, which, index=None):
    """Resolve a SectionRef steering symbol to its Section via the HOC brace form.

    `sref.parent { ... }` is a parser steering construct that makes the
    relative section (parent/root/trueparent/child, or the ref's own `sec`)
    the accessed section for the block. ``nrn_method_call`` does not steer:
    ``nrn_cas()`` then returns the tree ROOT for sec/parent/root, which broke
    ``parentseg()`` and everything built on it (children, subtree, wholetree,
    trueparentseg). The brace form with ``_mn_capture_cas`` is the only
    correct path (mechanism: ``_resolve_template_section``). Returns the
    Section, or None when the ref has no such relative (e.g. parent of a
    root: the steering raises and nothing is captured).
    """

    n = _PACKAGE._n
    key = "child" if index is not None else which
    proc = f"_mn_steer_{key}"
    # Locked from the save to the restore: another thread's capture between
    # them would read or reset this one (gap-72).
    with HOC_LOCK:
        if proc not in _SECTIONREF_STEER_TRAMPOLINES:
            steer = f"{which}[$2]" if index is not None else which
            n(f"proc {proc}() {{ $o1.{steer} {{ _mn_capture_cas() }} }}")
            _SECTIONREF_STEER_TRAMPOLINES.add(proc)
            n._top_level = list_functions()
        # Same save/restore as _resolve_template_section.
        saved = _PACKAGE._template_section_capture.value
        _PACKAGE._template_section_capture.value = 0
        try:
            if index is not None:
                getattr(n, proc)(sr_obj, index)
            else:
                getattr(n, proc)(sr_obj)
            captured = _PACKAGE._template_section_capture.value
        finally:
            _PACKAGE._template_section_capture.value = saved
    if not captured:
        return None
    return _PACKAGE.sections._section_for(captured)


class _SectionRefChildren:
    """``sr.child``: ``sr.child[i]`` is the i-th child Section, len() is nchild."""

    __slots__ = ("_ref",)

    __reduce_ex__ = refuse_pickle

    def __init__(self, ref):
        self._ref = ref

    def __len__(self):
        return int(self._ref.nchild())

    def __getitem__(self, index):
        i = _array_index(index, len(self), "child index must be an integer")
        if i is None:
            raise IndexError("child index out of range")
        return _steer_sectionref(self._ref, "child", i)

    def __iter__(self):
        return (self[i] for i in range(len(self)))

    def __repr__(self):
        return f"{self._ref.hname()}.child"


@hoc_locked
def _read_object_member(owner, name, idx=None):
    """The Object in objref member *name* of template object *owner* (its
    element *idx* if an array), or None when nil.

    A core with nrn_property_object_get reads a scalar member directly. Any
    other read asks the object's own scope for the pointer, as object_id(),
    so a member that is not public is reached, and takes a reference for the
    wrapper, since the pointer is borrowed.
    """
    if idx is None and _nrn_property_object_get is not None:
        obj = _nrn_property_object_get(owner._obj, name.encode("utf-8"))
    else:
        obj = int(
            _in_object_scope(
                owner,
                f"hoc_ac_ = object_id({_member_expr(name, idx)})",
                element=None if idx is None else name,
            )
        )
    if not obj:
        return None
    _nrn_object_ref(obj)
    return _wrap_owned_object(obj)


def _array_property(owner, name, sym, _type, qualified_name):
    """``owner.name`` for an array of doubles: Matrix.x, a template member of
    more than one dimension, or an ArrayProperty."""
    if name == "x" and owner._class_name == "Matrix":
        return _MatrixX(owner)
    is_var = _type == TYPES.VAR
    if is_var and _is_hoc_template(owner._class_name):
        dims = symbol_array_dims(sym)
        if len(dims) > 1:
            return _MemberArray(owner, name, dims)
    return ArrayProperty(
        owner,
        name,
        owner._array_size(name, sym),
        qualified_name=qualified_name,
        hoc_template_var=is_var,
    )


def _object_member(owner, name):
    """``owner.name`` for an objref member: its Object, or the proxy of an
    objref array."""
    sym = _nrn_method_symbol(owner._obj, name.encode("utf-8"))
    if _nrn_symbol_is_array(sym):
        return _MemberArray(owner, name, symbol_array_dims(sym))
    return _read_object_member(owner, name)


@hoc_locked
def _write_object_member(owner, name, value, idx=None):
    """Assign *value*, a HOC object, to objref member *name* of template object
    *owner* (its element *idx* if an array), as NEURON's attribute assignment.

    `member = List[k].object(0)` runs in the object's scope, the value in a
    List of its own: HOC's assignment, of any member, with a HOC error coming
    back as an exception (nrn_property_object_set has no nothrow variant).
    A reference leaves the member nil, as for a top-level objref; None and
    other values are TypeErrors.
    """
    if name == "this":  # assigning it releases the object itself
        raise AttributeError(f"cannot assign '{name}'")
    if isinstance(value, NrnRef):
        value = None
    elif not isinstance(value, Object):
        raise TypeError(
            "argument must be hoc.HocObject, not "
            + ("None" if value is None else type(value).__name__)
        )
    indices = _subscripts(idx)
    if not indices and _nrn_symbol_is_array(
        _nrn_method_symbol(owner._obj, name.encode("utf-8"))
    ):
        raise TypeError(f"'{name}' requires subscript for assignment")
    holder = List()
    holder.append(value if value is not None else _nil_holder())
    source = f"{holder.hname()}.object(0)" + (".none" if value is None else "")
    _in_object_scope(
        owner,
        f"{_member_expr(name, idx)} = {source}",
        element=name if indices else None,
    )


def _nil_holder():
    """An object whose member `none` is nil: an object's scope sees templates
    and built-ins, not the top level's objrefs, and none of those is nil."""
    n = _PACKAGE._n
    if "_MnNil" not in n._top_level:
        n("begintemplate _MnNil\npublic none\nobjref none\nendtemplate _MnNil")
        n._top_level = list_functions()
    return n._MnNil()


class _MemberArray:
    """An array member of a HOC template object, of any number of dimensions:
    ``cell.ob[i][j]`` for ``objref ob[2][3]``, ``cell.xa[i][j]`` for
    ``double xa[2][3]`` and, with *ref*, ``cell._ref_xa[i][j]``.

    As NEURON's array it is partial until every subscript is given: ``cell.xa[i]``
    is a row, with its own len() and iteration, and cannot be assigned. A
    nil element of an objref array reads as None. Holds the owner Object, which
    keeps the HOC object alive. The dimensions are the member symbol's, which
    those of the object itself can differ from (`double xa[$1][$2]` in init()).
    TODO(gap-86): needs a public accessor for an object's own dimensions.
    """

    __slots__ = ("_owner", "_name", "_dims", "_prefix", "_ref")

    __reduce_ex__ = refuse_pickle

    def __init__(self, owner, name, dims, prefix=(), ref=False):
        self._owner = owner
        self._name = name
        self._dims = tuple(dims)
        self._prefix = tuple(prefix)
        self._ref = ref

    def _index(self, idx):
        i = _array_index(idx, len(self), "array indices must be integers, not {}")
        if i is None:
            raise IndexError(f"{self._owner._class_name}.{self._name}")
        return i

    def _is_row(self):
        return len(self._prefix) + 1 < len(self._dims)

    def __getitem__(self, idx):
        indices = self._prefix + (self._index(idx),)
        if self._is_row():
            return _MemberArray(self._owner, self._name, self._dims, indices, self._ref)
        owner, name = self._owner, self._name
        if self._ref:
            return NrnHocTemplateVarRef(owner, name, indices)
        if owner._load_methods()[name][0] == TYPES.OBJECTVAR:
            return _read_object_member(owner, name, indices)
        return _read_hoc_template_var(owner, name, indices)

    def __setitem__(self, idx, value):
        if self._ref or self._is_row():
            raise TypeError("Wrong number of subscripts")
        indices = self._prefix + (self._index(idx),)
        owner, name = self._owner, self._name
        if owner._load_methods()[name][0] == TYPES.OBJECTVAR:
            _write_object_member(owner, name, value, indices)
        else:
            _write_hoc_template_var(owner, name, value, indices)

    def __len__(self):
        return self._dims[len(self._prefix)]

    def __iter__(self):
        for i in range(len(self)):
            yield self[i]

    def __repr__(self):
        if self._ref:
            return f"<incomplete pointer to hoc array {self._name}>"
        subscripts = "".join(f"[{i}]" for i in self._prefix)
        return f"{_instance_hname(self._owner)}.{self._name}{subscripts}[?]"


class _HeldSection:
    """A reference to a section struct (nrn_section_ref), so that whether the
    section is still in the model can be asked safely: once the last
    reference to a deleted section goes, its struct is freed and its memory
    can hold a new section, which nrn_section_is_active would then answer
    for. Released when this goes."""

    __slots__ = ("ptr",)

    def __init__(self, ptr):
        _nrn_section_ref(ptr)
        self.ptr = ptr

    def alive(self):
        return bool(_nrn_section_is_active(self.ptr))

    def __del__(self):
        # Interpreter teardown: as in _released_base's __del__.
        if getattr(sys, "meta_path", None) is None:
            return
        try:
            ptr = self.ptr
        except AttributeError:  # nrn_section_ref raised in __init__
            return
        _nrn_section_unref(ptr)


class _SectionEntry:
    """``cell.soma``: the Section, kept while it is in the model."""

    __slots__ = ("_section", "_held")

    def __init__(self, section):
        self._section = section
        self._held = _HeldSection(section._sec)

    def alive(self):
        return self._held.alive()

    def value(self, owner, name):
        return self._section


class _SectionArrayEntry:
    """``cell.dend``: the object's own length of the array (the symbol's
    dimensions when there are several).

    Each object of a template has its own length (`create dend[$1]` in
    init()); the symbol has only the length declared last, for any object.
    The length is found with section_exists(name, i, obj), which answers for
    the object's own array without a HOC error, and kept while an element
    that existed then is still in the model (`create` again deletes every
    element). section_exists is also 0 for an element deleted on its own
    (`dend[2] delete_section()`), so such an element can end the count early,
    where NEURON counts it (and aborts on access to it).
    """

    __slots__ = ("dims", "_held")

    def __init__(self, owner, name, sym):
        self.dims = symbol_array_dims(sym)
        if len(self.dims) > 1:  # no section_exists for more than one subscript
            anchor = (0,) * len(self.dims)
        else:
            length, anchor = _section_array_length(owner, name, sym)
            self.dims = (length,)
        self._held = None
        if anchor is not None:
            # It exists, so resolving it raises no HOC error, except for an
            # element past the declared length that is not public.
            try:
                section = _resolve_template_section(owner, name, idx=anchor)
            except IndexError:
                return
            self._held = _HeldSection(section._sec)

    def alive(self):
        # Nothing kept (no element could be): look again each time.
        return self._held is not None and self._held.alive()

    def value(self, owner, name):
        return _TemplateSectionArray(owner, name)


def _section_array_length(owner, name, sym):
    """``(length, index)`` of section array *name* of template object *owner*:
    its length and the lowest index seen to exist, or None.

    A binary search over section_exists (see _SectionArrayEntry), after
    trying the length the symbol was last declared with, which is the usual
    answer. Element 0 always counts: an array has at least one element.
    """

    section_exists = _PACKAGE._n.section_exists
    seen = []

    def exists(i):
        if section_exists(name, i, owner):
            seen.append(i)
            return True
        return False

    declared = int(_nrn_symbol_array_length(sym))
    if declared > 0 and exists(declared - 1) and not exists(declared):
        return declared, declared - 1
    exists(0)
    low, step = 0, 1
    while exists(low + step):
        low += step
        step *= 2
    high = low + step  # not exists(high)
    while high - low > 1:
        mid = (low + high) // 2
        if exists(mid):
            low = mid
        else:
            high = mid
    return high, min(seen) if seen else None


class _TemplateSectionArray:
    """Subscriptable proxy for a HOC `create name[N]` section array, of any
    number of dimensions.

    Supports `cell.dend[i]`, `cell.dend[i][j]`, `len()` and iteration, like the
    HocObject real NEURON returns; a partial index is a row, `cell.dend[i]`.
    Each element resolves through _resolve_template_section. `owner` is None
    for a top-level `create dend[N]` (`n.dend[i]`). The dimensions are looked
    up on each use, as NEURON's array does: running `create dend[M]` again
    changes them. A template object's are its own (_SectionArrayEntry); the
    top level's are the symbol's.
    """

    __slots__ = ("_owner", "_name", "_prefix")

    __reduce_ex__ = refuse_pickle

    def __init__(self, owner, name, prefix=()):
        self._owner = owner
        self._name = name
        self._prefix = tuple(prefix)

    def _dims(self):
        owner = self._owner
        if owner is not None:
            entry = owner._template_section_entry(self._name)
            if isinstance(entry, _SectionArrayEntry):
                return entry.dims
        else:
            sym = _top_level_symbol(self._name.encode("utf-8"))
            if sym and _nrn_symbol_is_array(sym):
                return symbol_array_dims(sym)
        raise TypeError(f"'{self._name}' is no longer a section array")

    def __getitem__(self, idx):
        dims = self._dims()
        size = dims[len(self._prefix)]
        if isinstance(idx, slice):
            return [self[i] for i in range(*idx.indices(size))]
        i = _array_index(idx, size, "section array indices must be integers, not {}")
        if i is None:
            scope = f"{self._owner._class_name}." if self._owner is not None else ""
            raise IndexError(
                f"index {idx} out of range [0, {size}) for {scope}{self._name}"
            )
        indices = self._prefix + (i,)
        if len(indices) < len(dims):
            return _TemplateSectionArray(self._owner, self._name, indices)
        return _resolve_template_section(self._owner, self._name, idx=indices)

    def __len__(self):
        return self._dims()[len(self._prefix)]

    def __setitem__(self, idx, value):
        dims = self._dims()
        if len(self._prefix) + 1 < len(dims):
            raise TypeError("Wrong number of subscripts")
        self[idx]  # a bad index is an IndexError first
        raise TypeError("not assignable")  # NEURON's text for dend[1] = x

    def __iter__(self):
        for i in range(len(self)):
            yield self[i]

    def __repr__(self):
        # As NEURON: "dend[?]", or "C[0].d[1][?]" in a template (_owner is None
        # for a top-level array).
        scope = f"{_instance_hname(self._owner)}." if self._owner is not None else ""
        subscripts = "".join(f"[{i}]" for i in self._prefix)
        return f"{scope}{self._name}{subscripts}[?]"


# ---------------------------------------------------------------------------
# ArrayProperty
# ---------------------------------------------------------------------------


class _MatrixX:
    """``m.x`` of a Matrix: ``m.x[i][j]`` is element ``(i, j)``, as in NEURON.

    Matrix.x steers with two indices, which nrn_property_array_get/set (one
    index) cannot pass, so elements go through Matrix.getval/setval. With
    ``ref`` it is ``m._ref_x``, whose elements are pointers
    (NrnMatrixElementRef). Holds the Matrix.
    """

    __slots__ = ("_matrix", "_row", "_ref")

    __reduce_ex__ = refuse_pickle

    def __init__(self, matrix, row=None, ref=False):
        self._matrix = matrix
        self._row = row
        self._ref = ref

    def __len__(self):
        if self._row is None:
            return int(self._matrix.nrow())
        return int(self._matrix.ncol())

    def _index(self, idx):
        i = _array_index(idx, len(self), "indices must be integers, not {}")
        if i is None:
            raise IndexError(f"{self._matrix.hname()}.x")
        return i

    def __getitem__(self, idx):
        idx = self._index(idx)
        if self._row is None:
            return _MatrixX(self._matrix, idx, self._ref)
        if self._ref:
            return NrnMatrixElementRef(self._matrix, self._row, idx)
        return float(self._matrix.getval(self._row, idx))

    def __setitem__(self, idx, value):
        idx = self._index(idx)
        if self._row is None:
            raise TypeError("Wrong number of subscripts")
        if self._ref:
            self[idx][0] = value
            return
        self._matrix.setval(self._row, idx, float(value))

    def __iter__(self):
        for i in range(len(self)):
            yield self[i]

    def __repr__(self):
        row = "" if self._row is None else f"[{self._row}]"
        return f"{self._matrix.hname()}.x{row}[?]"


class ArrayProperty:
    """Subscriptable wrapper for an Object's array-valued property.

    Returned by Object.__getattr__ when the property's symbol is array-valued
    (detected via nrn_symbol_is_array + nrn_symbol_array_length).
    Example: VClamp has amp[3], dur[3]; scalar access (amp[0]) reads/writes
    the underlying double via nrn_property_array_get/set.
    """

    __slots__ = (
        "_owner",
        "_name",
        "_name_encoded",
        "_size",
        "_qualified_name",
        "_hoc_template_var",
    )

    __reduce_ex__ = refuse_pickle

    # Hold the owner Object wrapper, not the bare obj*. The wrapper's __del__
    # unrefs the HOC object, so a proxy holding only the pointer would read
    # freed memory and segfault (`a = vc.amp; del vc; a[0]`).
    def __init__(self, owner, name, size, qualified_name=None, hoc_template_var=False):
        self._owner = owner
        self._name = name
        self._name_encoded = name.encode("utf-8") if isinstance(name, str) else name
        self._size = size
        self._qualified_name = qualified_name or name
        self._hoc_template_var = hoc_template_var

    def _resolve(self, idx):
        i = _array_index(idx, self._size, "indices must be integers, not {}")
        if i is None:
            raise IndexError(
                f"index {idx} out of range [0, {self._size}) "
                f"for array property {self._qualified_name}"
            )
        return i

    def _check_owner(self):
        # As obj.<name> checks before every read: a point process that is in no
        # section has no data, and the property functions raise a HOC error
        # that aborts the process.
        self._owner._check_located()

    def __getitem__(self, idx):
        idx = self._resolve(idx)
        self._check_owner()
        if self._hoc_template_var:
            return float(_read_hoc_template_var(self._owner, self._name, idx))
        return float(
            _property_array_get_checked(self._owner._obj, self._name_encoded, idx)
        )

    def __setitem__(self, idx, value):
        value = _to_double(value, TypeError)
        idx = self._resolve(idx)
        self._check_owner()
        if self._hoc_template_var:
            _write_hoc_template_var(self._owner, self._name, value, idx)
            return
        _property_array_set_checked(
            self._owner._obj, self._name_encoded, idx, float(value)
        )

    def __len__(self):
        return self._size

    def __iter__(self):
        for i in range(self._size):
            yield self[i]

    def __repr__(self):
        try:
            vals = list(self)
        except Exception:
            vals = "?"
        return f"<ArrayProperty {self._qualified_name} {vals}>"
