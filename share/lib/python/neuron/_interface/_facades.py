"""The hoc, nrn and _neuron_section modules, for NEURON's own package.

The legacy extension created these modules at runtime and registered them in
sys.modules (9.0's nrnpy_nrn.cpp); the neuron package, rxd and user code
import them alongside ``h``. Vendored as ``neuron._interface``, the package
installs equivalents backed by this implementation's types. They are module
objects in sys.modules, not files: a compiled ``hoc`` module next to a
``hoc.py`` would win the import, so the build no longer installs one.
"""
import sys
import types

_NAMES = ("hoc", "nrn", "_neuron_section")


def _is_python_type(h, cls):
    """True for the classes NEURON's own interface exposes as Python types.

    Built-in C++ classes (Vector, List, NetCon, ...) are types there. Point
    processes and HOC templates (NetStim, IClamp, begintemplate classes) are
    HocObject instances, which neuron.HocBaseObject subclasses take as
    ``hoc_type``. Here every h.<class> is a type; this tells them apart.
    """
    from .api import TYPES, _nrn_symbol_subtype
    from .utils import class_symbol, is_point_process_class

    name = getattr(cls, "_hoc_class_name", None)
    if name is None:
        return False
    sym = class_symbol(name)
    if not sym or _nrn_symbol_subtype(sym) != TYPES.CPLUSOBJECT:
        return False
    return not is_point_process_class(name)


def _hoc_object_class(h):
    """hoc.HocObject: isinstance/issubclass answers match NEURON's interface.

    isinstance(x, hoc.HocObject) is true for HOC objects, references, h and
    its functions, and for point-process and template classes (instances in
    NEURON). issubclass(C, hoc.HocObject) is true for the built-in C++ classes
    and for real subclasses such as neuron.HocBaseObject.
    hoc.HocObject.__new__(cls, *args, hocbase=C, sec=None) creates an instance
    of cls backed by a new HOC object of class C, as hclass3 needs.
    """
    from .nrnref import NrnRef
    from .object import Object, _HocClassMeta
    from .utils import FuncWrapper

    class _HocObjectMeta(_HocClassMeta):
        def __instancecheck__(cls, instance):
            if cls is not HocObject:
                return super().__instancecheck__(instance)
            if isinstance(instance, (Object, NrnRef, FuncWrapper, type(h))):
                return True
            return (
                isinstance(instance, _HocClassMeta)
                and getattr(instance, "_hoc_class_name", None) is not None
                and not _is_python_type(h, instance)
            )

        def __subclasscheck__(cls, subclass):
            if super().__subclasscheck__(subclass):
                return True
            if cls is not HocObject:
                return False
            return isinstance(subclass, _HocClassMeta) and _is_python_type(h, subclass)

    class HocObject(Object, metaclass=_HocObjectMeta):
        __slots__ = ()

        def __new__(cls, *args, hocbase=None, sec=None):
            if hocbase is None:
                if cls is HocObject and not args and sec is None:
                    return h  # hoc.HocObject() is the top-level interpreter
                raise TypeError("HocObject() needs hocbase=, the HOC class to create")
            class_name = getattr(hocbase, "_hoc_class_name", None)
            if class_name is None:
                raise TypeError(f"hocbase {hocbase!r} is not a HOC class")
            instance = super().__new__(cls)
            instance._construct(class_name, args, sec=sec)
            return instance

        def __init__(self, *args, **kwargs):
            # __new__ built the HOC object; nothing is left to construct.
            pass

    HocObject.__module__ = "hoc"
    HocObject.__qualname__ = "HocObject"
    return HocObject


def _hoc_module(h):
    from .object import _HocClassMeta

    hoc = types.ModuleType("hoc", "HOC object types for the neuron package.")
    hoc.HocObject = _hoc_object_class(h)
    hoc.HocClass = _HocClassMeta

    def __getattr__(name):
        # hoc.Vector and friends are the same classes h exposes.
        if name.startswith("__"):
            raise AttributeError(name)
        try:
            value = getattr(h, name)
        except Exception:
            raise AttributeError(f"module 'hoc' has no attribute {name!r}") from None
        if isinstance(value, type):
            return value
        raise AttributeError(f"module 'hoc' has no attribute {name!r}")

    def execute(command):
        """Execute a hoc command, return True on success, False on failure."""
        return h(command)

    def hoc_ac(value=None):
        """Get (or set) the scalar hoc_ac_."""
        if value is not None:
            h.hoc_ac_ = value
        return h.hoc_ac_

    hoc.__getattr__ = __getattr__
    hoc.ref_address = _ref_address
    hoc.execute = execute
    hoc.hoc_ac = hoc_ac
    return hoc


_STACK_IS_POINTER = 2  # nrn_stack_type


def _ref_address(ref):
    """hoc.ref_address: the address of the number a reference holds.

    rxd's C code takes plain double pointers; it gets them from references such
    as h._ref_t and seg._ref_v through this, as from NEURON's hoc module.
    """
    import ctypes

    from .api import HOC_LOCK, _nrn_double_ptr_pop, _nrn_stack_type
    from .nrnref import NrnArrayPropertyRef, NrnRangeArrayRef, NrnRef

    if not isinstance(ref, NrnRef) or isinstance(
        ref, (NrnArrayPropertyRef, NrnRangeArrayRef)
    ):
        raise TypeError("not a reference to a number")
    with HOC_LOCK:  # the pop must take this push (gap-72)
        try:
            ref._push()
        except NotImplementedError:  # a RuntimeError, but not a dead reference
            raise
        except (RuntimeError, ReferenceError) as exc:
            raise ValueError("the reference is no longer valid") from exc
        if _nrn_stack_type() != _STACK_IS_POINTER:
            ref._pop()
            raise TypeError("not a reference to a number")
        address = ctypes.cast(_nrn_double_ptr_pop(), ctypes.c_void_p).value
    if not address:
        raise ValueError("the reference is no longer valid")
    return address


def _nrn_module(h):
    from .api import TYPES
    from .mechanism import Mechanism
    from .sections import OpaquePointer, Section, Segment
    from .utils import list_functions

    nrn = types.ModuleType("nrn", "Section, segment and mechanism types.")
    nrn.Section = Section
    nrn.Segment = Segment
    nrn.Mechanism = Mechanism
    nrn.OpaquePointer = OpaquePointer
    psection = []

    def set_psection(function):
        # Section.psection is implemented here; keep the hook for callers.
        psection[:] = [function]

    def __getattr__(name):
        # nrn.hh, nrn.pas, ...: every density mechanism's type is Mechanism.
        # Not by h.<name>'s class: NEURON's package makes its own. Not the two
        # that are no density mechanism of nrn's.
        if (
            not name.startswith("__")
            and name not in ("capacitance", "morphology")
            and list_functions().get(name, (None,))[0] == TYPES.MECHANISM
        ):
            return Mechanism
        raise AttributeError(f"module 'nrn' has no attribute {name!r}")

    def cas():
        """Return the currently accessed section."""
        return h.cas()

    def allsec():
        """Iterate over all sections."""
        return h.allsec()

    nrn.set_psection = set_psection
    nrn.__getattr__ = __getattr__
    nrn.cas = cas
    nrn.allsec = allsec
    return nrn


def _neuron_section_module(nrn):
    # NEURON's _neuron_section holds the same Section/Segment types as nrn,
    # plus cas() and allsec() (ModelView's realcell.hoc calls cas()).
    module = types.ModuleType("_neuron_section", "Section and segment helpers.")
    module.Section = nrn.Section
    module.Segment = nrn.Segment
    module.set_psection = nrn.set_psection
    module.cas = nrn.cas
    module.allsec = nrn.allsec
    return module


_installed = ()


def install(interface):
    """Register the facades and return (hoc, nrn, _neuron_section)."""
    global _installed
    present = [name for name in _NAMES if name in sys.modules]
    if present:
        if tuple(sys.modules.get(name) for name in _NAMES) == _installed:
            return _installed  # importlib.reload(neuron)
        # Our own stay when neuron is removed from sys.modules and imported again.
        if not all(hasattr(sys.modules[name], "_neuron_facade") for name in present):
            raise ImportError(
                f"{', '.join(present)} already imported, from another NEURON or not "
                "NEURON's"
            )
    nrn = _nrn_module(interface.h)
    modules = (
        _hoc_module(interface.h),
        nrn,
        _neuron_section_module(nrn),
    )
    for name, module in zip(_NAMES, modules):
        module._neuron_facade = True
        sys.modules[name] = module
    sys.modules["neuron.hoc"] = modules[0]
    _installed = modules
    return modules
