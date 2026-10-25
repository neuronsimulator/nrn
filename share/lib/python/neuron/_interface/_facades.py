"""The hoc, nrn and _neuron_section modules, for NEURON's own package.

The legacy extension created these modules at runtime and registered them in
sys.modules (nrnpy_nrn.cpp:3060-3108); the neuron package, rxd and user code
import them alongside ``h``. Vendored as ``neuron._interface``, the package
installs equivalents backed by this implementation's types. They are module
objects in sys.modules, not files: a compiled ``hoc`` module next to a
``hoc.py`` would win the import, so the build no longer installs one.
"""
import sys
import types

_NAMES = ("hoc", "nrn", "_neuron_section")


_CPLUSOBJECT = 16  # Symbol.subtype of a class registered from C++ (hocdec.h)


def _is_python_type(h, cls):
    """True for the classes NEURON's own interface exposes as Python types.

    Built-in C++ classes (Vector, List, NetCon, ...) are types there. Point
    processes and HOC templates (NetStim, IClamp, begintemplate classes) are
    HocObject instances, which neuron.HocBaseObject subclasses take as
    ``hoc_type``. Here every h.<class> is a type; this tells them apart.
    """
    from .api import _nrn_symbol_subtype
    from .utils import class_symbol, is_point_process_class

    name = getattr(cls, "_hoc_class_name", None)
    if name is None:
        return False
    sym = class_symbol(name)
    if not sym or _nrn_symbol_subtype(sym) != _CPLUSOBJECT:
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
    from .object import Object, _construct_hoc_object, _HocClassMeta
    from .utils import FuncWrapper

    class _HocObjectMeta(_HocClassMeta):
        def __instancecheck__(cls, instance):
            if cls is not HocObject:
                return super().__instancecheck__(instance)
            if isinstance(instance, (Object, NrnRef, FuncWrapper)) or instance is h:
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
            obj, host, post_init, abort_init = _construct_hoc_object(
                class_name, args, sec=sec
            )
            instance = object.__new__(cls)
            instance._obj = obj
            try:
                instance._init_from_ptr(obj, host)
                post_init(instance)
            except BaseException:
                abort_init()
                raise
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

    hoc.__getattr__ = __getattr__
    return hoc


def _nrn_module():
    from .mechanism import Mechanism
    from .sections import OpaquePointer, Section, Segment

    nrn = types.ModuleType("nrn", "Section, segment and mechanism types.")
    nrn.Section = Section
    nrn.Segment = Segment
    nrn.Mechanism = Mechanism
    nrn.OpaquePointer = OpaquePointer
    psection = []

    def set_psection(function):
        # Section.psection is implemented here; keep the hook for callers.
        psection[:] = [function]

    nrn.set_psection = set_psection
    return nrn


def _neuron_section_module(h, nrn):
    # NEURON's _neuron_section holds the same Section/Segment types as nrn,
    # plus cas() and allsec() (ModelView's realcell.hoc calls cas()).
    module = types.ModuleType("_neuron_section", "Section and segment helpers.")
    module.Section = nrn.Section
    module.Segment = nrn.Segment
    module.set_psection = nrn.set_psection

    def cas():
        """Return the currently accessed section."""
        return h.cas()

    def allsec():
        """Iterate over all sections."""
        return h.allsec()

    module.cas = cas
    module.allsec = allsec
    return module


def install(interface, embedded):
    """Register the facades and return (hoc, nrn, _neuron_section).

    ``embedded`` means nrniv -python already created the legacy modules; they
    are returned unchanged so the session keeps one set of them.
    """
    if embedded:
        return tuple(sys.modules.get(name) for name in _NAMES)
    present = [name for name in _NAMES if name in sys.modules]
    if present:
        raise ImportError(
            f"{', '.join(present)} already imported; the legacy NEURON extension "
            "cannot share a process with this interface"
        )
    nrn = _nrn_module()
    modules = (
        _hoc_module(interface.h),
        nrn,
        _neuron_section_module(interface.h, nrn),
    )
    for name, module in zip(_NAMES, modules):
        sys.modules[name] = module
    sys.modules["neuron.hoc"] = modules[0]
    return modules
