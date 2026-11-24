"""Object, List, Vector wrappers — dynamic class system, refcounting, method dispatch."""
import array
import collections.abc
import ctypes
import functools
import numbers
import operator
import os
import sys
import types
import warnings
import weakref

from ._docs import _ClassDoc, help_doc
from .api import (
    _as_double,
    _nrn_symbol_subtype,
    _VENDORED,
    HOC_CLASS_MODULE,
    HOC_LOCK,
    libnrniv,
    hoc_locked,
    _ERR_BUF_SIZE,
    TYPES,
    _check_nrn_error,
    _hoc_return_type_code,
    _hoc_unref_defer,
    _nrn_double_pop,
    _nrn_hoc_call,
    _nrn_method_call,
    _nrn_method_symbol,
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
    _nrn_str_pop,
    _nrn_symbol_array_length,
    _nrn_symbol_is_array,
    _nrn_vector_capacity,
    _nrn_vector_data,
    _object_is_builtin_class,
    _property_array_get_checked,
    _property_array_set_checked,
    _property_get_checked,
    _property_set_checked,
    _ref_property_array_set_checked,
    _top_level_symbol,
)
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
)
from .utils import (
    _is_python_object_arg,
    _check_sec_kwarg,
    _check_segment_arg,
    _name_error,
    FuncWrapper,
    _object_pop,
    _pop_pushed_section,
    _try_wrap_point_nmodlrandom,
    class_symbol,
    is_located_point_process_class,
    is_point_process_class,
    list_functions,
    list_methods,
)

# ---------------------------------------------------------------------------
# Metaclass — _HocClassMeta
# ---------------------------------------------------------------------------


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
        from . import NEURON

        # List(name) is allocation-ordered, not object_id-ordered. A negative
        # index sorts by object_id; a positive index matches object_id, like
        # real NEURON's h.IClamp[i].
        n = NEURON()
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
    from .sections import Section

    if (
        isinstance(arg, (Object, str, bytes))
        or arg is None
        or not isinstance(arg, collections.abc.Iterable)
    ):
        raise RuntimeError("hoc_execerror: argument must be a Python iterable")
    members = list(arg)
    if not all(isinstance(member, Section) for member in members):
        raise RuntimeError("hoc_execerror: iterable must contain only Section objects")
    return members


@hoc_locked
def _construct_hoc_object(class_name, args, sec=None):
    """Create the HOC object backing a user-side construction call.

    Returns ``(obj_ptr, host_section, post_init)``; call ``post_init`` with the
    initialized wrapper. Some classes take Python idioms that do not match the
    HOC constructor, such as Vector(iterable) and SectionList(iterable). Their
    args are rewritten here and the rest is finished by ``post_init``. All
    other args pass straight through.

    Called from ``Object.__init__`` for direct construction
    (``n.IClamp(seg)``, ``MyIClamp(seg)``). ``Object._wrap`` does not use it.
    """
    from .api import _nrn_object_new, _nrn_object_unref
    from .utils import _push_args

    initing_to_list_arg = None
    initing_sectionlist = None
    rvp_ends = ()
    args_for_push = args

    if class_name == "RangeVarPlot" and len(args) > 1:
        # RangeVarPlot(expr, seg0, seg1): core reads each end as an x on the
        # accessed section, or a segment object through a hook only NEURON's
        # module installs. One section push cannot carry two ends, so set them
        # with begin()/end() after construction, as HOC code does.
        from .sections import Segment

        if all(isinstance(a, Segment) for a in args[1:3]):
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
    temp_strs, seg_sec, rollback_args = _push_args(args_for_push, return_rollback=True)
    # A Segment arg's section wins over sec=: the arg is pushed after it.
    use_sec = seg_sec if seg_sec is not None else sec
    if use_sec is not None:
        _nrn_section_push(use_sec._sec)
    result = ctypes.c_void_p()
    is_template = True  # an unknown class may run HOC too
    try:
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
            _nrn_object_unref(result.value)
        else:
            # nrn_object_new_nothrow restores the caller-owned args on failure.
            # Pop them or repeated failures hit "Stack too deep" (gap-56).
            rollback_args()
        from . import _raise_pending_callback_exc

        _raise_pending_callback_exc()
        raise
    finally:
        if use_sec is not None:
            _pop_pushed_section(use_sec._sec)
        if is_template:
            # A template's init() is HOC: it can uninsert mechanisms or delete
            # sections, so the caches the epoch guards (allsec, Mechanism
            # wrappers) must check again. Only a class's first construction
            # moved it before (gap-68).
            from . import _bump_model_epoch

            _bump_model_epoch()

    def post_init(wrapper):
        if initing_to_list_arg is not None:
            _copy_into_vector(wrapper, initing_to_list_arg)
        if initing_sectionlist is not None:
            for s in initing_sectionlist:
                wrapper.append(sec=s)
        for method, seg in zip(("begin", "end"), rvp_ends):
            getattr(wrapper, method)(seg.x, sec=seg.sec)

    return result.value, use_sec, post_init


# ---------------------------------------------------------------------------
# Base wrapper class
# ---------------------------------------------------------------------------


def _instance_hname(self):
    from . import NEURON

    idx = int(NEURON().object_id(self, 1))
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
        methods = object.__getattribute__(instance, "_methods")
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


# Under nrniv -python the legacy module still makes its own HocObjects for HOC
# objects it hands to Python (a HOC call of a Python method passes them as
# arguments). _facades.install sets this to that type so they compare equal to
# this interface's wrappers of the same HOC object, as two legacy wrappers do.
_LEGACY_OBJECT_TYPE = None

# HOC class name -> whether freeing an instance can delete sections (user
# templates can, built-in classes cannot). Filled by Object.__del__.
_MAY_OWN_SECTIONS = {}
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
    """The HOC object address of a wrapper from either module, or None."""
    if isinstance(other, Object):
        return other._obj
    if _LEGACY_OBJECT_TYPE is not None and isinstance(other, _LEGACY_OBJECT_TYPE):
        try:
            return other.hocobjptr()
        except Exception:  # h, references and other non-objects
            return None
    return None


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


class Object(metaclass=_HocClassMeta):
    """Wraps a HOC object (template instance) — IClamp, Vector, NetCon, etc.

    ``n.<ClassName>`` returns a dynamic subclass minted on first access.
    Construct by calling it (``n.IClamp(soma(0.5))``); Python subclassing
    works (``class MyClamp(n.IClamp): ...``). Attribute access goes to HOC
    template members via ``__getattr__`` / ``__setattr__``.
    """

    # __weakref__: allows weakref.ref(vector) etc.
    # _section_cache is lazy; declared here so the slot descriptor exists
    # before any read.
    __slots__ = (
        "_obj",
        "_class_name",
        "_methods",
        "_host_section",
        "_host_section_ref",
        "_section_cache",
        "_plotshape_var",
        "__weakref__",
    )

    # Set on dynamic subclasses by NEURON.__getattr__; the hand-coded
    # subclasses below set it explicitly. Drives both metaclass indexing
    # (``cls[i]``) and the user-construction path in __init__.
    _hoc_class_name = None

    def __init__(self, *args, host_section=None, sec=None):
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
        obj, host_from_args, post_init = _construct_hoc_object(
            class_name, args, sec=sec
        )
        # Own the object before any step below can fail, as _wrap does: a
        # partly built wrapper releases it in __del__. The first point-process
        # check for a class runs HOC, so even that step can be interrupted.
        self._obj = obj

        if host_section is None and is_point_process_class(class_name):
            # Only a point process lives in the section it was made in;
            # SectionRef(sec=s) keeps working after s is deleted.
            host_section = host_from_args
        self._init_from_ptr(obj, host_section)
        post_init(self)

    @classmethod
    def _wrap(cls, obj, host_section=None):
        """Wrap an existing HOC obj* without the user construction path.

        For internal callers that hold a raw pointer. Bypasses any
        user-defined __init__ on a subclass.
        """
        instance = cls.__new__(cls)
        # Establish ownership before initialization can fail. A partially
        # initialized wrapper then releases the caller-provided ref in __del__.
        instance._obj = obj
        instance._init_from_ptr(obj, host_section)
        return instance

    def _init_from_ptr(self, obj, host_section):
        """Populate all slots from an existing HOC obj*.

        Called by both ``__init__`` (user path) and ``_wrap`` (internal path).
        Takes ownership of the caller's reference (nrn_object_new returns
        refcount 1; likewise nrn_object_pop). The caller must not call
        nrn_object_ref, which would double-ref and leak the C object;
        ``__del__`` calls nrn_object_unref.
        """
        from .api import _nrn_class_name

        self._obj = obj
        self._class_name = _nrn_class_name(obj).decode("utf-8")

        self._methods = list_methods(self)
        # A HOC/template-owned Section wrapper is non-owning and may be a
        # temporary (cell.soma[0]). Keep that wrapper alive with its dependent;
        # the template owns the native Section, so wrapper GC says nothing
        # about native liveness (gap-50). Python-owned Sections keep the weak
        # hold required by real NEURON's last-wrapper-deletes-section contract.
        self._host_section = None
        self._plotshape_var = None
        self._host_section_ref = None
        if host_section is not None and host_section.is_pysec():
            self._host_section_ref = weakref.ref(host_section)
        elif host_section is not None:
            self._host_section = host_section
        # Pre-set lazy slots so reads do not fall into __getattr__.
        self._section_cache = None

    def __del__(self):
        # Reference ownership: see _init_from_ptr.
        # During interpreter teardown meta_path/modules can be None, and
        # importing from .api would crash.
        if (
            getattr(sys, "meta_path", None) is None
            or getattr(sys, "modules", None) is None
        ):
            return
        # The guard above covers the common case; libnrniv may still be torn
        # down in pathological atexit orders.
        # A partially initialised Object may have unset slots, which raise
        # AttributeError.
        try:
            obj = self._obj
        except AttributeError:
            obj = None
        if obj is not None:
            from .api import _nrn_object_unref

            # Freeing a template object deletes its sections, which allsec()
            # would not otherwise notice, so that bumps the model epoch. A
            # built-in class (Vector, IClamp, ...) owns none, and bumping for
            # every one would make each allsec() regather. Decided per class,
            # before the unref (it reads the object's template).
            try:
                class_name = self._class_name
            except AttributeError:
                class_name = None
            may_own = _MAY_OWN_SECTIONS.get(class_name)
            if may_own is None:
                if class_name is None:
                    may_own = not _object_is_builtin_class(obj)
                else:
                    may_own = _may_own_sections(class_name)
                    _MAY_OWN_SECTIONS[class_name] = may_own
            _nrn_object_unref(obj)
            # _hoc_unref_defer: prompt deletion, as when NEURON releases a
            # HocObject wrapper (api.py has the reason).
            _hoc_unref_defer()
            if may_own:
                from . import _bump_model_epoch

                _bump_model_epoch()

    hname = _Hname()

    @_Helper
    def baseattr(self, name):
        """Look up *name* on the HOC side, ignoring Python-level overrides.

        Real NEURON's `HocObject.baseattr(name)` is literally
        `hocobj_getattr(self, name)` (nrnpy_hoc.cpp:1410-1416). It bypasses
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
            from .api import _nrn_object_ref
            from .utils import _wrap_owned_object

            _nrn_object_ref(self._obj)
            twin = _wrap_owned_object(self._obj)
            if self._host_section is not None or self._host_section_ref is not None:
                object.__setattr__(twin, "_host_section", self._host_section)
                object.__setattr__(twin, "_host_section_ref", self._host_section_ref)
            return Object.__getattr__(twin, name)
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
        methods = self._methods
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
        sym = _nrn_method_symbol(self._obj, name.encode("utf-8"))
        is_array = bool(_nrn_symbol_is_array(sym))
        if not is_array and idx is not None:
            raise TypeError(f"scalar property '{name}' does not take an index")
        if is_array:
            if self._class_name == "Matrix" and name == "x":
                rows = _MatrixX(self, ref=True)
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
        return NrnObjectPropertyRef(self, name, idx=idx)

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

        NEURON reads the section with component() (nrnpy_hoc.cpp), which the
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
        methods = set(self._methods)
        if "del" in methods:
            methods.discard("del")
            methods.add("delay")
        # Include the Python-side helpers (hname, ref, same, hocobjptr) and
        # subclass (Vector, List, SectionList) entries.
        for klass in type(self).__mro__:
            if klass is object:
                continue
            for name in klass.__dict__:
                if not name.startswith("_"):
                    methods.add(name)
        return sorted(methods)

    def _check_host_alive(self):
        """Raise if this point process's host section has been deleted.

        No-op for non-point-process objects (both host slots are None).

        Python-owned Sections are weakly held so their last owning wrapper can
        delete the native section; a stale weakref raises ``ReferenceError``.
        HOC/template-owned wrappers are strongly held because their native
        Section has an independent owner. In either case an explicit native
        ``delete_section`` leaves the point process in no section, and this
        raises NEURON's ``TypeError("No value")`` before property dispatch can
        dereference freed state.
        """
        host = self._host_section
        if host is None:
            ref = self._host_section_ref
            if ref is None:
                return
            host = ref()
            if host is None:
                if not self._follow_relocation():
                    raise ReferenceError(
                        f"{self._class_name}: host section has been deleted"
                    )
                return
        if not _nrn_section_is_active(host._sec):
            if not self._follow_relocation():
                raise TypeError("No value")

    def _follow_relocation(self):
        """Record the section the point process is in now, after loc() moved it
        off a deleted host. False if it is in no section."""
        seg = self._get_segment()  # has_loc/get_loc skip the host check
        if seg is None:
            return False
        self._host_section = None
        self._host_section_ref = None
        if seg.sec.is_pysec():
            self._host_section_ref = weakref.ref(seg.sec)
        else:
            self._host_section = seg.sec
        return True

    def _get_segment(self):
        """The Segment this point process is in, or None if it is in none."""
        from . import NEURON

        n = NEURON()
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
        from . import NEURON

        n = NEURON()
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

    def _is_unlocated(self):
        """True for a point process in no section, whose variables have no
        data.

        Never placed (h.IClamp()) or wrapped from HOC after its section was
        deleted (h.List("IClamp").o(0)): no host section is known here, and
        the property functions throw "point process not located in a
        section" through ctypes, which aborts. Methods such as get_segment()
        and has_loc() still work. An ARTIFICIAL_CELL is in no section but has
        data.
        """
        return (
            self._host_section is None
            and self._host_section_ref is None
            and is_point_process_class(self._class_name)
            and not _nrn_prop_exists(self._obj)
        )

    def _check_located(self):
        """Raise TypeError, as NEURON does, before reading, writing or taking
        a reference to a variable of a point process that is in no section."""
        if self._is_unlocated():
            raise TypeError("No value")

    def _assign_pointer(self, member, src_ref):
        """Wire this object's NMODL POINTER ``member`` to the address referenced
        by ``src_ref`` (an NrnRef), implementing ``obj._ref_PTR = src._ref_X``.

        The public ``nrn_pp_setpointer_pop`` API addresses the point process
        directly and consumes exactly one source handle.
        """
        member_encoded = member.encode("utf-8")
        err = ctypes.create_string_buffer(_ERR_BUF_SIZE)
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
                self._check_host_alive()
                self._check_located()
                self._assign_pointer(name[5:], val)
                return
            member = name[5:]
            methods = getattr(self, "_methods", None) or {}
            if member in methods:
                from .api import _nrn_symbol_subtype
                from .sections import _NRNPOINTER

                sym = _nrn_method_symbol(self._obj, member.encode("utf-8"))
                if sym and int(_nrn_symbol_subtype(sym)) == _NRNPOINTER:
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
            methods = getattr(self, "_methods", None)
            if methods is None or name not in methods:
                object.__setattr__(self, name, val)
                return
        self._check_host_alive()

        # `delay` → `del` alias — see __dir__ comment above.
        actual_name = name
        if (
            hasattr(self, "_methods")
            and name not in self._methods
            and name == "delay"
            and "del" in self._methods
        ):
            actual_name = "del"

        # Allow setting scalar object properties (RANGEVAR or VAR doubles).
        # A user HOC template's VAR lives in its object dataspace, not behind
        # the C++ property `steer` callback used by nrn_property_set.
        if hasattr(self, "_methods") and actual_name in self._methods:
            _type, _ = self._methods[actual_name]

            if _type == TYPES.STRING:
                _write_hoc_template_string(self, actual_name, val)
                return
            if _type in (TYPES.RANGEVAR, TYPES.VAR):
                self._check_located()
                name_encoded = actual_name.encode("utf-8")
                sym = _nrn_method_symbol(self._obj, name_encoded)
                if not _nrn_symbol_is_array(sym):
                    if _type == TYPES.VAR:
                        _write_hoc_template_var(self, actual_name, val)
                    else:
                        _property_set_checked(self._obj, name_encoded, val)
                    return
                # Array property: a sequence of the right length sets each
                # element (same as the MATLAB interface).
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
        if (
            name.isidentifier()
            and not name.startswith("_")
            and _write_object_alias(self, name, val)
        ):
            return
        if actual_name in (getattr(self, "_methods", None) or ()):
            raise AttributeError(
                f"Cannot set '{name}': not a settable HOC property (RANGEVAR/VAR) "
                f"of {self._class_name}"
            )
        raise _unknown_name_error(self, name)

    def __getattr__(self, name):
        # _methods is unset if a subclass __init__ skipped super().__init__().
        # Bail here, or the unset-slot AttributeError re-enters __getattr__
        # and recurses.
        try:
            object.__getattribute__(self, "_methods")
        except AttributeError:
            raise AttributeError(name) from None
        # Skipped for underscore names so the weakref bookkeeping does not
        # recurse.
        if not name.startswith("_") and name not in _LOCATION_METHODS:
            self._check_host_alive()
        if name.startswith("_ref_"):
            return self._property_ref(name[5:])
        # NEURON's Python module adds get_segment() to every point process
        # that can be located (not an ARTIFICIAL_CELL). Ahead of the method
        # table: with NEURON's module loaded first, the table holds its native
        # get_segment, which returns NEURON's own Segment type.
        if name == "get_segment" and is_located_point_process_class(self._class_name):
            return self._get_segment
        if self._class_name == "NetCon" and name in _NETCON_SEG_METHODS:
            return functools.partial(self._netcon_seg, _NETCON_SEG_METHODS[name])
        # .plot() on RangeVarPlot / PlotShape — not a HOC method
        if name == "plot":
            if self._class_name == "RangeVarPlot":
                from .plotting import _RangeVarPlot

                return _RangeVarPlot(self)
            elif self._class_name == "PlotShape":
                from . import NEURON
                from .plotting import _PlotShapePlot

                NEURON().define_shape()
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
        if name not in self._methods and name == "delay" and "del" in self._methods:
            name = "del"
        if name in self._methods:
            _type, _ = self._methods[name]
            qualified_name = f"{self._class_name}.{original_name}"

            # RANGEOBJ not yet discovered (a point-process-only MOD may be the
            # first RANGEOBJ seen, with no density symbol to learn the code
            # from): probe via the fail-closed adapter (see
            # api.TypeCodes.record_rangeobj). Free once RANGEOBJ is known.
            if TYPES.RANGEOBJ is None:
                sym = _nrn_method_symbol(self._obj, name.encode("utf-8"))
                wrapped = _try_wrap_point_nmodlrandom(self._obj, sym, _type)
                if wrapped is not None:
                    return wrapped

            # --- type SECTION: template public section (cell.soma, cell.dend[i]) ---
            if _type == TYPES.SECTION:
                return self._template_section_entry(name).value(self, name)

            # --- type OBJECTVAR: template objref member (cell.all) ---
            if _type == TYPES.OBJECTVAR:
                # Not dispatchable via nrn_method_call: the generic method
                # path below would misread it as a FuncWrapper and warn
                # "unknown return type 271". Read via a per-member obfunc
                # trampoline (_read_object_member). Import3d SectionLists
                # (cell.all, somatic, ...) work this way.

                sym = _nrn_method_symbol(self._obj, name.encode("utf-8"))
                if _nrn_symbol_is_array(sym):
                    return _ObjectMemberArray(
                        self, name, int(_nrn_symbol_array_length(sym))
                    )
                return _read_object_member(self, name)

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
                    if name == "x" and self._class_name == "Matrix":
                        return _MatrixX(self)
                    return ArrayProperty(
                        self,
                        name,
                        self._array_size(name, sym),
                        qualified_name=qualified_name,
                        hoc_template_var=(_type == TYPES.VAR),
                    )

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

                @hoc_locked
                def method_func(*args, sec=None):
                    from .utils import _push_args, _PythonObjectArg

                    if pysec_name_change:
                        try:
                            ref_sec = _steer_sectionref(self, "sec")
                        except RuntimeError:
                            ref_sec = None
                        from .sections import PYTHON_SECTIONS

                        if ref_sec is not None and ref_sec._sec in PYTHON_SECTIONS:
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
                    temp_strs, seg_sec, rollback_args = _push_args(
                        args, return_rollback=True
                    )
                    use_sec = seg_sec if seg_sec is not None else sec
                    if use_sec is not None:
                        _nrn_section_push(use_sec._sec)
                    try:
                        _err_buf = ctypes.create_string_buffer(_ERR_BUF_SIZE)
                        # Reset the return-type hint so an earlier call's
                        # value cannot leak in (real NEURON's fcall does too).
                        _hoc_return_type_code.value = 0
                        ret = _nrn_method_call(
                            self._obj, sym, len(args), _err_buf, _ERR_BUF_SIZE
                        )
                        from . import _bump_model_epoch

                        _bump_model_epoch()
                        if ret:
                            # A failed nothrow call restores the args (gap-56).
                            rollback_args()
                        _check_nrn_error(ret, _err_buf)
                        if _type == TYPES.FUNCTION:
                            val = _nrn_double_pop()
                            # A built-in method returning int/bool set the hint.
                            # Trust it only for built-in classes; then reset.
                            code = _hoc_return_type_code.value
                            _hoc_return_type_code.value = 0
                            if code and _object_is_builtin_class(self._obj):
                                if code == 1:
                                    result = int(val)
                                elif code == 2:
                                    result = bool(val)
                                else:
                                    result = val
                            else:
                                result = val
                        elif _type == TYPES.METHOD_OBFUNC or _type == TYPES.OBFUNCTION:
                            result = _object_pop()
                        elif _type == TYPES.METHOD_STRFUNC:
                            result = _nrn_str_pop()
                        elif _type == TYPES.PROCEDURE:
                            # A template proc: hoc_call_ob_proc drops its 0.0
                            # sentinel (hoc_nopop) and NEURON returns 0.0.
                            result = 0.0
                        else:
                            warnings.warn(f"Ignoring unknown return type {_type}")
                            result = None
                        # Raise a callback's pending exception here (e.g. a
                        # raising FInitializeHandler fired by finitialize()),
                        # after popping the return value, or failed callbacks
                        # leak operand-stack slots.
                        from . import _raise_pending_callback_exc

                        _raise_pending_callback_exc()
                        return result
                    finally:
                        if use_sec is not None:
                            _pop_pushed_section(use_sec._sec)
                        from .api import _hoc_unref_defer

                        _hoc_unref_defer()

                func = method_func
                if self._class_name == "ParallelContext":
                    from .parallel import PYTHON_METHODS, python_method

                    if name in PYTHON_METHODS:
                        func = python_method(self, name, method_func)
                return FuncWrapper(
                    func,
                    qualified_name,
                    f"NEURON object method: {self._class_name}.{name}()",
                    doc_key=f"{self._class_name}.{name}",
                )
        # StringFunctions alias (obj.aliasname): not in the class symbol
        # table. Resolve via a HOC trampoline, since the native property
        # accessors SIGSEGV on a non-member name. Plain identifiers only.
        if name.isidentifier() and not name.startswith("_"):
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
_HOC_AC = ctypes.c_double.in_dll(libnrniv, "hoc_ac_")


_EXECUTE = None  # n.execute, the HOC built-in, looked up once


@hoc_locked
def _in_object_scope(owner, statement, value=None):
    """Run HOC *statement* in the scope of template object *owner* and return
    the value it left in hoc_ac_. *value*, if given, is in hoc_ac_ while it
    runs. hoc_ac_ is restored afterwards. The statement goes to HOC as UTF-8
    bytes; a str argument would have to be ASCII, and a strdef need not be."""
    global _EXECUTE
    if _EXECUTE is None:
        from . import NEURON

        _EXECUTE = NEURON().execute
    saved = _HOC_AC.value
    try:
        if value is not None:
            _HOC_AC.value = value
        _EXECUTE(statement.encode("utf-8"), owner)
        return _HOC_AC.value
    finally:
        _HOC_AC.value = saved


def _member_expr(name, idx):
    return name if idx is None else f"{name}[{int(idx)}]"


# The variables of a C++ class (NetCon.delay, NetCon.weight[i]) are public, so
# `$o1.name` reaches them; nrn_property_array_get could not until nrn#3888.
_HOC_TEMPLATE_VAR_TRAMPOLINES = set()


def _ensure_hoc_template_var_trampolines(name, array):
    key = (name, bool(array))
    if key in _HOC_TEMPLATE_VAR_TRAMPOLINES:
        return
    from . import NEURON

    n = NEURON()
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
    a HOC template object or a C++ class instance (VAR members)."""
    hoc_template = _is_hoc_template(owner._class_name)
    if _nrn_property_object_get is not None:
        if idx is None:
            return _nrn_property_get(owner._obj, name.encode("utf-8"))
        if hoc_template:
            return _property_array_get_checked(owner._obj, name.encode("utf-8"), idx)
    elif hoc_template:
        return _in_object_scope(owner, f"hoc_ac_ = {_member_expr(name, idx)}")
    from . import NEURON

    array = idx is not None
    _ensure_hoc_template_var_trampolines(name, array)
    fn = f"_mn_hvget_{'a' if array else 's'}_{name}"
    args = (owner, idx) if array else (owner,)
    return getattr(NEURON(), fn)(*args)


def _write_hoc_template_var(owner, name, value, idx=None):
    value = _as_double(value)
    hoc_template = _is_hoc_template(owner._class_name)
    if _nrn_property_object_get is not None:
        if idx is None:
            _nrn_property_set(owner._obj, name.encode("utf-8"), value)
            return
        if hoc_template:
            _ref_property_array_set_checked(
                owner._obj, name.encode("utf-8"), idx, value
            )
            return
    elif hoc_template:
        _in_object_scope(owner, f"{_member_expr(name, idx)} = hoc_ac_", value)
        return
    from . import NEURON

    array = idx is not None
    _ensure_hoc_template_var_trampolines(name, array)
    fn = f"_mn_hvset_{'a' if array else 's'}_{name}"
    args = (owner, idx, value) if array else (owner, value)
    getattr(NEURON(), fn)(*args)


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
    from . import _STR_BEGIN, _STR_END, _capture_hoc_string

    statement = f'print "{_STR_BEGIN}", {name}, "{_STR_END}"'
    return _capture_hoc_string(lambda: _in_object_scope(owner, statement))


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
# pair per alias name serves every object (the proc takes $o1). A non-alias
# name makes HOC raise "... not a public member", converted to AttributeError.
_OBJECT_ALIAS_TRAMPOLINES = set()


def _ensure_alias_trampolines(name):
    """Define the per-name get/set alias trampolines. Returns False for names
    that aren't usable HOC identifiers (dunders / private probes), so callers
    skip the alias path for them."""
    if not (name.isidentifier() and not name.startswith("_")):
        return False
    if name not in _OBJECT_ALIAS_TRAMPOLINES:
        from . import NEURON

        n = NEURON()
        n(f"func _mn_aliasget_{name}() {{ return $o1.{name} }}")
        n(f"proc _mn_aliasset_{name}() {{ $o1.{name} = $2 }}")
        n(f"obfunc _mn_aliasobj_{name}() {{ return $o1.{name} }}")
        _OBJECT_ALIAS_TRAMPOLINES.add(name)
    return True


def _segment_of(sec, x):
    """sec(x) through the base Section: NEURON builds the segment for
    get_segment(), preseg() and postseg() in C, so a subclass's __call__
    must not replace it."""
    from .sections import Section

    return Section.__call__(sec, x)


# NetCon.preseg/postseg rebuilt from the location methods (Object._netcon_seg).
_NETCON_SEG_METHODS = {"preseg": "preloc", "postseg": "postloc"}

_PC_RAW_ARG_METHODS = frozenset({"py_allgather", "py_gather", "py_broadcast"})

# Point-process methods that work after the host section is deleted, as in
# NEURON: core checks the location itself (get_loc raises a HOC error).
_LOCATION_METHODS = frozenset({"get_segment", "has_loc", "get_loc", "loc"})

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
    from .sections import Section

    if len(args) != 1:
        return args, sec
    arg = args[0]
    if isinstance(arg, Section):
        return (), arg
    if allow_object and isinstance(arg, Object):
        return args, sec
    raise RuntimeError("hoc_execerror: not a Python nrn.Section")


_o2loc_hook = ctypes.c_void_p.in_dll(libnrniv, "nrnpy_o2loc_p_")


def _check_location_arg(arg):
    """Raise NEURON's error for a point-process location core cannot read.

    Core reads an object in a location argument (nrn_seg_or_x_arg) through
    nrnpy_o2loc_p_, a hook only NEURON's own Python module installs. Without
    it core asserts and the process aborts; a hook has no way to report an
    error either, as raising would throw through ctypes. A Segment never
    reaches it (_push_args sends its x and pushes its section), so reject
    everything else _push_args would send as an object, as NEURON's o2loc
    does. Core checks numbers itself; it rejects strings and refs as NEURON
    does. Where NEURON's module is loaded its hook is installed and reads
    the arguments itself, so they are left alone.
    """
    if _o2loc_hook.value:
        return
    from .sections import Segment

    if isinstance(arg, Segment):
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


_mpiabort_on_error = ctypes.c_int.in_dll(libnrniv, "nrn_mpiabort_on_error_")


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
            _stderr_quiet_saved = (saved, _mpiabort_on_error.value)
            _mpiabort_on_error.value = 0
        _stderr_quiet_depth += 1
        return self

    def __exit__(self, *exc):
        global _stderr_quiet_depth
        _stderr_quiet_depth -= 1
        try:
            if _stderr_quiet_depth == 0:
                saved, abort_on_error = _stderr_quiet_saved
                _mpiabort_on_error.value = abort_on_error
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
    from . import NEURON

    if not _ensure_alias_trampolines(name):
        raise AttributeError(name)
    n = NEURON()
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
    from . import NEURON

    if not _ensure_alias_trampolines(name):
        return False
    try:
        fv = float(value)
    except (TypeError, ValueError):
        return False
    try:
        with _SuppressCStderr():
            getattr(NEURON(), f"_mn_aliasset_{name}")(obj, fv)
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


def _vector_values(seq):
    """Return *seq* as an array of doubles, with NEURON's error for a non-number."""
    values = seq if isinstance(seq, (list, tuple)) else list(seq)
    try:
        return array.array("d", values)
    except TypeError:
        for i, value in enumerate(values):
            try:
                float(value)
            except (TypeError, ValueError):
                raise RuntimeError(f"item {i} is not a valid number") from None
        raise


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
    """Copy an array of doubles into a Vector already sized to hold it."""
    if values:
        address, count = values.buffer_info()
        ctypes.memmove(_nrn_vector_data(vec._obj), address, count * values.itemsize)


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
    from . import n

    return n.Vector(0)


class Vector(Object):
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

    def __getitem__(self, i):
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

    def __setitem__(self, i, val):
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

    def __iter__(self):
        # Live, as in NEURON: elements appended during the loop are visited.
        obj = self._obj
        i = 0
        while i < _nrn_vector_capacity(obj):
            yield _nrn_vector_data(obj)[i]
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
        ptr = _nrn_vector_data(self._obj)
        buf_from_mem = ctypes.pythonapi.PyMemoryView_FromMemory
        buf_from_mem.restype = ctypes.py_object
        # Py_ssize_t length so vectors over 2^31 doubles are not truncated;
        # flags 0x200 = PyBUF_WRITE.
        buf_from_mem.argtypes = (ctypes.c_void_p, ctypes.c_ssize_t, ctypes.c_int)
        cbuffer = buf_from_mem(ptr, size * numpy.dtype(float).itemsize, 0x200)
        return numpy.ndarray((size,), float, cbuffer, order="C")

    @help_doc("Vector.to_python")
    def to_python(self, dest=None):
        """Return Vector contents as a Python list (or fill *dest*)."""
        size = len(self)
        result = [] if size == 0 else _nrn_vector_data(self._obj)[:size]
        if dest is not None:
            dest[:] = result
            return dest
        return result

    @help_doc("Vector.from_python")
    def from_python(self, seq):
        """Fill this Vector from a Python sequence. Resizes to match."""
        values = _vector_values(seq)
        self.resize(len(values))
        _copy_into_vector(self, values)
        return self

    def __len__(self):
        return _nrn_vector_capacity(self._obj)

    # --- Arithmetic (numeric protocol) ---
    # As NEURON's nrnpy_vec_math (share/lib/python/neuron/__init__.py), called
    # from py_hocobj_math (src/nrnpython/nrnpy_hoc.cpp). The other operand is a
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
        from .sections import PYTHON_SECTIONS, Section

        # sections._section_for, inlined as in allsec(): a Python section is
        # its own object, a HOC one gets a new wrapper.
        owner_refs = PYTHON_SECTIONS.data
        from_ptr = Section._from_ptr
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
    from . import _template_section_capture
    from .sections import Section

    # Save/restore the module-global slot and read into a local, so a nested
    # section resolution during the block cannot clobber the pending value.
    saved = _template_section_capture.value
    _template_section_capture.value = 0
    suffix = f"[{int(idx)}]" if idx is not None else ""
    cmd = f"{name}{suffix} {{ _mn_capture_cas() }}"
    hoc_ac = ctypes.c_double.in_dll(libnrniv, "hoc_ac_")
    saved_ac = hoc_ac.value
    if owner is not None:
        # This hoc_ac_ is the address; braces: no echo.
        hoc_ac.value = 0.0
        owner_name = _instance_hname(owner)
        sym = _nrn_method_symbol(owner._obj, name.encode("utf-8"))
        if idx is not None and idx >= int(_nrn_symbol_array_length(sym)):
            # HOC code checks an index against the length the array was last
            # declared with, by any object of the template; only the component
            # syntax (obj.dend[i]) checks it against the object's own length,
            # which can be longer. That syntax reaches public sections only.
            return _component_section(owner, owner_name, name, int(idx))
        else:
            # In the object's scope, which reaches sections that are not
            # public but not our top-level proc.
            cmd = (
                f'{{execute("{name}{suffix} hoc_ac_ = this_section()", '
                f"{owner_name})}}"
            )
    try:
        ret = _nrn_hoc_call(cmd.encode("utf-8"))
        captured = _template_section_capture.value
        if owner is not None:
            captured = int(hoc_ac.value)
    finally:
        _template_section_capture.value = saved
        hoc_ac.value = saved_ac
    if ret != 0 or not captured:
        scope = owner._class_name + "." if owner is not None else ""
        raise AttributeError(f"Could not access section '{scope}{name}{suffix}'")
    return Section._from_ptr(captured)


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
    from . import NEURON
    from .sections import Section

    where = f"{owner._class_name}.{name}[{idx}]"
    key = (owner._class_name, name)
    reachable = _COMPONENT_SECTION_REACHABLE.get(key)
    if reachable is False:
        raise IndexError(
            f"{where} is past the length '{name}' was last declared with, "
            f"and '{name}' is not public"
        )
    if not NEURON().section_exists(name, idx, owner):
        raise AttributeError(f"Could not access section '{where}'")
    hoc_ac = ctypes.c_double.in_dll(libnrniv, "hoc_ac_")
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
    return Section._from_ptr(captured)


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
    from . import NEURON, _template_section_capture
    from .sections import _section_for

    n = NEURON()
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
        saved = _template_section_capture.value
        _template_section_capture.value = 0
        try:
            if index is not None:
                getattr(n, proc)(sr_obj, index)
            else:
                getattr(n, proc)(sr_obj)
            captured = _template_section_capture.value
        finally:
            _template_section_capture.value = saved
    if not captured:
        return None
    return _section_for(captured)


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


# Per-member obfunc trampolines that return a template's objref member
# (type 324, OBJECTVAR). `$o1.<name>` resolves the member on whichever object
# is pushed, so one trampoline serves every template with that member name.
# Caching avoids redefining the obfunc on every access. Mirrors
# __init__._OBJECTVAR_TRAMPOLINES (top-level objrefs).
_MEMBER_OBJ_TRAMPOLINES = set()


def _read_object_member(owner, name):
    """Return the Object bound to template objref member ``owner.name``.

    ``nrn_method_call`` cannot dispatch OBJECTVAR members, so this defines
    ``obfunc _mn_memgrab_<name>() { return $o1.<name> }`` once and calls it
    with ``owner`` as the argument. Used by the OBJECTVAR branch of
    ``Object.__getattr__`` (e.g. an Import3d cell's ``cell.all``).
    """
    from . import NEURON
    from .api import _nrn_object_ref, _nrn_property_object_get
    from .utils import _wrap_owned_object

    if _nrn_property_object_get is not None:
        # Reads non-public members too, as NEURON's Python module does.
        obj = _nrn_property_object_get(owner._obj, name.encode("utf-8"))
        if not obj:
            return None
        _nrn_object_ref(obj)  # borrowed; the wrapper owns one reference
        return _wrap_owned_object(obj)
    n = NEURON()
    fn = f"_mn_memgrab_{name}"
    idfn = f"_mn_memid_{name}"
    if fn not in _MEMBER_OBJ_TRAMPOLINES:
        n(f"obfunc {fn}() {{ return $o1.{name} }}")
        n(f"func {idfn}() {{ return object_id($o1.{name}) }}")
        _MEMBER_OBJ_TRAMPOLINES.add(fn)
        # Refresh the dispatch table so the new obfunc is callable.
        n._top_level = list_functions()
    # Same object_id nil guard as __init__._read_hoc_objectvar: an unset
    # member pops a nil Object* that SIGSEGVs nrn_class_name.
    if getattr(n, idfn)(owner) == 0.0:
        return None
    return getattr(n, fn)(owner)


def _read_object_member_index(owner, name, idx):
    """Return the Object at ``owner.name[idx]`` for an objref-array member, or None.

    Array analog of _read_object_member, with the same nil guard.
    """
    from . import NEURON

    n = NEURON()
    fn = f"_mn_memgrabidx_{name}"
    idfn = f"_mn_memidx_id_{name}"
    if fn not in _MEMBER_OBJ_TRAMPOLINES:
        n(f"obfunc {fn}() {{ return $o1.{name}[$2] }}")
        n(f"func {idfn}() {{ return object_id($o1.{name}[$2]) }}")
        _MEMBER_OBJ_TRAMPOLINES.add(fn)
        n._top_level = list_functions()
    if getattr(n, idfn)(owner, idx) == 0.0:
        return None
    return getattr(n, fn)(owner, idx)


class _ObjectMemberArray:
    """Subscriptable proxy for a template's ``objref name[N]`` member array.

    Mirrors real NEURON: ``cell.name[i]``, ``len()``, iteration; nil elements
    read as None. Holds the owner Object (which keeps the HOC object alive).
    """

    __slots__ = ("_owner", "_name", "_size")

    __reduce_ex__ = refuse_pickle

    def __init__(self, owner, name, size):
        self._owner = owner
        self._name = name
        self._size = size

    def __getitem__(self, idx):
        if isinstance(idx, slice):
            return [self[i] for i in range(*idx.indices(self._size))]
        i = _array_index(
            idx, self._size, "objref array indices must be integers, not {}"
        )
        if i is None:
            raise IndexError(
                f"index {idx} out of range [0, {self._size}) for "
                f"{self._owner._class_name}.{self._name}"
            )
        return _read_object_member_index(self._owner, self._name, i)

    def __len__(self):
        return self._size

    def __iter__(self):
        for i in range(self._size):
            yield self[i]


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
        # Interpreter teardown: as in Object.__del__.
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
    """``cell.dend``: the object's own length of the array.

    Each object of a template has its own length (`create dend[$1]` in
    init()); the symbol has only the length declared last, for any object.
    The length is found with section_exists(name, i, obj), which answers for
    the object's own array without a HOC error, and kept while an element
    that existed then is still in the model (`create` again deletes every
    element). section_exists is also 0 for an element deleted on its own
    (`dend[2] delete_section()`), so such an element can end the count early,
    where NEURON counts it (and aborts on access to it).
    """

    __slots__ = ("length", "_held")

    def __init__(self, owner, name, sym):
        self.length, anchor = _section_array_length(owner, name, sym)
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
    from . import NEURON

    section_exists = NEURON().section_exists
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
    """Subscriptable proxy for a HOC `create name[N]` section array.

    Supports `cell.dend[i]`, `len()` and iteration, like the HocObject real
    NEURON returns. Each index resolves through _resolve_template_section.
    `owner` is None for a top-level `create dend[N]` (`n.dend[i]`). The length
    is looked up on each use, as NEURON's array does: running `create dend[M]`
    again changes it. A template object's is its own (_SectionArrayEntry); the
    top level's is the symbol's. *size* is not used.
    """

    __slots__ = ("_owner", "_name")

    __reduce_ex__ = refuse_pickle

    def __init__(self, owner, name, size=None):
        self._owner = owner
        self._name = name

    def _length(self):
        owner = self._owner
        if owner is not None:
            entry = owner._template_section_entry(self._name)
            if isinstance(entry, _SectionArrayEntry):
                return entry.length
        else:
            sym = _top_level_symbol(self._name.encode("utf-8"))
            if sym and _nrn_symbol_is_array(sym):
                return int(_nrn_symbol_array_length(sym))
        raise TypeError(f"'{self._name}' is no longer a section array")

    def __getitem__(self, idx):
        size = self._length()
        if isinstance(idx, slice):
            return [self[i] for i in range(*idx.indices(size))]
        i = _array_index(idx, size, "section array indices must be integers, not {}")
        if i is None:
            scope = f"{self._owner._class_name}." if self._owner is not None else ""
            raise IndexError(
                f"index {idx} out of range [0, {size}) for {scope}{self._name}"
            )
        return _resolve_template_section(self._owner, self._name, idx=i)

    def __len__(self):
        return self._length()

    def __setitem__(self, idx, value):
        raise TypeError("not assignable")  # NEURON's text for dend[1] = x

    def __iter__(self):
        for i in range(len(self)):
            yield self[i]

    def __repr__(self):
        # As NEURON: "dend[?]", or "C[0].d[?]" in a template (_owner is None
        # for a top-level array).
        scope = f"{_instance_hname(self._owner)}." if self._owner is not None else ""
        return f"{scope}{self._name}[?]"


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
        # As obj.<name> checks before every read: a point process whose
        # section was deleted, or that is in no section, has no data, and the
        # property functions raise a HOC error that aborts the process.
        self._owner._check_host_alive()
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
