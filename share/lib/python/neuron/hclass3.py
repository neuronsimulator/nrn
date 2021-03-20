#Python 3 only
# ------------------------------------------------------------------------------
# class factory for subclassing h.anyclass
# h.anyclass methods may be overridden. If so the base method can be called
# using the idiom self.basemethod = self.baseattr('methodname')
# ------------------------------------------------------------------------------

__all__ = ["hclass", "nonlocal_hclass", "HocBaseObject"]

from . import h, hoc
import nrn
import sys

def assert_not_hoc_composite(cls):
    """
    Asserts that a class is not directly composed of multiple HOC types.
    """
    hoc_bases = [b for b in cls.__bases__ if issubclass(b, hoc.HocObject)]
    if len(hoc_bases) > 1:
        bases = ", ".join(b.__name__)
        cname = cls.__name__
        raise TypeError(f"Composition of {bases} HocObjects not allowed in {cname}")

def hclass(hoc_type):
    """
    Class factory for subclassing HOC types.

    Example:

    ..code-block:: python

        import neuron

        class MyVector(neuron.hclass(neuron.h.Vector)):
            pass

    :param hoc_type: HOC types/classes such as ``h.List``, ``h.NetCon``, ``h.Vector``, ...
    :type hoc_type: :class:`hoc.HocObject`
    :deprecated: Inherit from :class:`~neuron.HocBaseObject` instead.
    """
    if hoc_type == h.Section:
        return nrn.Section
    try:
        hc = type("hc", (HocBaseObject,), {}, hoc_type=hoc_type)
    except TypeError:
        raise TypeError("Argument is not a valid HOC type.") from None
    return hc

def nonlocal_hclass(hoc_type, module_name, name=None):
    """
    Analogous to :func:`~neuron.hclass` but adds the class's module information.

    Example:

    .. code-block:: python

        import neuron

        MyVector = nonlocal_hclass(neuron.h.Vector, __name__, name="MyVector")

    :param hoc_type: HOC types/classes such as ``h.List``, ``h.NetCon``, ``h.Vector``, ...
    :type hoc_type: :class:`hoc.HocObject`
    :param module_name: Name of the module where the class will be stored
        (usually ``__name__``)
    :type module_name: str
    :param name: Name of the module level variable that the class will be stored in. When
        omitted the name of the HOC type is used.
    """

    module = sys.modules[module_name]
    local_hclass = hclass(hoc_type)
    if name is None:
        name = hoc_type.hname()[:-2]
    setattr(module, name, local_hclass)
    local_hclass.__module__ = module_name
    local_hclass.__name__ = name
    local_hclass.__qualname__ = name
    return local_hclass


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
        assert_not_hoc_composite(cls)
        if hoc_type is not None:
            if not isinstance(hoc_type, hoc.HocObject):
                raise TypeError(f"Class's `hoc_type` {hoc_type} is not a valid HOC type.")
            else:
                cls._hoc_type = hoc_type
        elif not hasattr(cls, "_hoc_type"):
            raise TypeError("Class keyword argument `hoc_type` is required for HocBaseObjects.")
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
        kwds2 = {'hocbase': cls._hoc_type}
        if 'sec' in kwds:
            kwds2['sec'] = kwds['sec']
        return hoc.HocObject.__new__(cls, *args, **kwds2)
