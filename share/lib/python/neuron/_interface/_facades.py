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


def _hoc_module(h):
    from .object import Object, _HocClassMeta

    hoc = types.ModuleType("hoc", "HOC object types for the neuron package.")
    hoc.HocObject = Object
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
    modules = (
        _hoc_module(interface.h),
        _nrn_module(),
        types.ModuleType("_neuron_section"),
    )
    for name, module in zip(_NAMES, modules):
        sys.modules[name] = module
    sys.modules["neuron.hoc"] = modules[0]
    return modules
