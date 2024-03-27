NEURON Python Classes and Objects
=================================

NEURON exposes its internal objects and hoc templates as Python objects via an automatic
conversion layer, effectively making all entities from the HOC stack available to a Python program.

There are basically two main objects which expose most Neuron entities. The first is `hoc` which
exposes a number of internal established classes and functions.

.. code-block::
    python

    >>> from neuron import hoc
    >>> hoc.
    hoc.List(
    hoc.SectionList(
    hoc.SectionRef(
    hoc.Vector(
        ...

However, for *dynamic* entities NEURON provides the `h` gateway object. It gives access to internal
classes (templates) and objects, even if they were just created. E.g.:

.. code-block::
    python

    >>> from neuron import h
    >>> # Create objects in the hoc stack
    >>> h("objref vec")
    >>> h("vec = new Vector(5, 1)")
    >>> # Access to objects
    >>> h.vec.as_numpy()
    array([1., 1., 1., 1., 1.])
    >>>
    >>> # Access to exposed types
    >>> vv = h.Vector(5, 2)
    >>> vv.as_numpy()
    array([1., 1., 1., 1., 1.])

This is particularly useful as NEURON can dynamically load libraries with more functions and classes.

Class Hierarchy
---------------

All NEURON's internal interpreter objects are instances of a global top-level type: `HocObject`.
Until very recently they were considered direct instances, without any intermediate hierarchy.

With #1858 Hoc classes are now associated with actual Python types, created dynamically. Such
change enables type instances to be properly recognized as such, respecting e.g. `isinstance()`
predicates and subclassing.

.. code-block::
    python

    >>> isinstance(hoc.Vector, type)
    True
    >>> v = h.Vector()
    >>> isinstance(v, hoc.HocObject)
    True
    >>> isinstance(v, hoc.Vector)
    True
    >>> type(v) is hoc.Vector  # direct subclass
    True
    >>>isinstance(v, hoc.Deck)  # Not instance of other class
    False

Subclasses are also recognized properly. For creating them please inherit from `HocBaseObject`
with `hoc_type` given as argument. E.g.:

.. code-block::
    python

    >>> class MyStim(neuron.HocBaseObject, hoc_type=h.NetStim):
        pass
    >>> issubclass(MyStim, hoc.HocObject)
    True
    >>> issubclass(MyStim, neuron.HocBaseObject)
    True
    >>> MyStim._hoc_type == h.NetStim
    True
    >>> stim = MyStim()
    >>> isinstance(stim, MyStim)
    True
    >>> isinstance(stim, h.NetStim)
    True
    >>> isinstance(stim, h.HocObject)
    True
