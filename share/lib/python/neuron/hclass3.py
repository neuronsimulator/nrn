# Python 3 only
# ------------------------------------------------------------------------------
# class factory for subclassing h.anyclass
# h.anyclass methods may be overridden. If so the base method can be called
# using the idiom self.basemethod = self.baseattr('methodname')
# ------------------------------------------------------------------------------

__all__ = ["hclass", "HocBaseObject"]

from . import h, hoc
import nrn
import sys


def assert_not_hoc_composite(cls):
    """
    Asserts that a class is not directly composed of multiple HOC types.
    """
    hoc_bases = set(
        b._hoc_type
        for b in cls.__bases__
        if issubclass(b, HocBaseObject) and b is not HocBaseObject
    )
    if len(hoc_bases) > 1:
        bases = ", ".join(b.__name__ for b in cls.__bases__)
        raise TypeError(f"Composition of {bases} HocObjects with different HOC types.")


def _overrides(cls, base, method_name):
    return getattr(cls, method_name) is not getattr(base, method_name)


def hclass(hoc_type, module_name=None, name=None):
    """
    Class factory for subclassing HOC types.

    Example:

    ..code-block:: python

        import neuron

        myClassTemplate = neuron.hclass(
            neuron.h.Vector,
            module_name=__name__,
            name="MyVector",
        )

        class MyVector(myClassTemplate):
            pass

    :param hoc_type: HOC types/classes such as ``h.List``, ``h.NetCon``, ``h.Vector``, ...
    :type hoc_type: :class:`hoc.HocObject`
    :param module_name: Name of the module where the class will be stored
        (usually ``__name__``)
    :type module_name: str
    :param name: Name of the module level variable that the class will be stored in. When
        omitted the name of the HOC type is used.
    :deprecated: Inherit from :class:`~neuron.HocBaseObject` instead.
    """
    if hoc_type == h.Section:
        return nrn.Section
    if module_name is None:
        module_name = __name__
    if name is None:
        name = hoc_type.hname()[:-2]
    try:
        hc = type(name, (HocBaseObject,), {}, hoc_type=hoc_type)
    except TypeError:
        raise TypeError("Argument is not a valid HOC type.") from None
    hc.__module__ = module_name
    hc.__name__ = name
    hc.__qualname__ = name
    return hc


class HocBaseObject(hoc.HocObject):
    """
    The base class for inheriting from HOC types.

    .. code-block:: python

        import neuron

        class MyVector(neuron.HocBaseObject, hoc_type=neuron.h.Vector):
            pass

    The ``__new__`` method passes the hoc type to the builtin ``hoc.HocObject`` and
    returns a new instance with the correct mapping_proxy based on the hoc type template.
    """

    def __init_subclass__(cls, hoc_type=None, **kwargs):
        if hoc_type is not None:
            if not isinstance(hoc_type, hoc.HocObject):
                raise TypeError(
                    f"Class's `hoc_type` {hoc_type} is not a valid HOC type."
                )
            cls._hoc_type = hoc_type
        elif not hasattr(cls, "_hoc_type"):
            raise TypeError(
                "Class keyword argument `hoc_type` is required for HocBaseObjects."
            )
        assert_not_hoc_composite(cls)
        hobj = hoc.HocObject
        hbase = HocBaseObject
        if _overrides(cls, hobj, "__init__") and not _overrides(cls, hbase, "__new__"):
            # Subclasses that override `__init__` must also implement `__new__` to deal
            # with the arguments that have to be passed into `HocObject.__new__`.
            # See https://github.com/neuronsimulator/nrn/issues/1129
            raise TypeError(
                f"`{cls.__qualname__}` implements `__init__` but misses `__new__`. "
                + "Class must implement `__new__`"
                + " and call `super().__new__` with the arguments required by HOC"
                + f" to construct the underlying h.{cls._hoc_type.hname()} HOC object."
            )
        super().__init_subclass__(**kwargs)

    def __new__(cls, *args, **kwds):
        # To construct HOC objects within NEURON from the Python interface, we use the
        # C-extension module `hoc`. `hoc.HocObject.__new__` both creates an internal
        # representation of the object in NEURON, and hands us back a Python object that
        # is linked to that internal representation. The `__new__` functions takes the
        # arguments that HOC objects of that type would take, and uses the `hocbase`
        # keyword argument to determine which type of HOC object to create. The `sec`
        # keyword argument can be passed along in case the construction of a HOC object
        # requires section stack access.
        kwds2 = {"hocbase": cls._hoc_type}
        if "sec" in kwds:
            kwds2["sec"] = kwds["sec"]
        return hoc.HocObject.__new__(cls, *args, **kwds2)
