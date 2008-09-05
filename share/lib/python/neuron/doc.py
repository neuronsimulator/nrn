"""
neuron.doc
===================

This module is used behind the scenes to generate docstrings of all HocObjects
of various types

To get general help on the neuron module try:

>>> import neuron
>>> help(neuron)

or in ipython

In []: import neuron
In []: neuron ?

From there, you can get help on the various objects in the hoc world:

In []: fom neuron import h
In []: v = h.Vector()
In []: ? v.to_python

a feature whose implementation is based on the neuron.doc module.

"""


doc_h = """

neuron.h
========

neuron.h is the top-level HocObject, allowing interaction between python and hoc.

It is callable like a function, and takes hoc code as an argument to be exectuted.

The top-level hoc namespace is exposed as attributes to the h object.

Ex:

    >>> h = neuron.h
    >>> h("objref myvec")
    >>> h("myvec = new Vector(10)")
    >>> h.myvec.x[0]=1.0
    >>> h.myvec.printf()
    1       0       0       0       0
    0       0       0       0       0


Hoc classes are defined in the h namespace and can be constructed as follows:

>>> v = h.Vector(10)
>>> soma = h.Section()
>>> input = h.IClamp(soma(0.5))

More help on individual classes defined in hoc and exposed in python
is available using python's online help feature

>>> help(h.Section)

or in ipython:

In []: ? h.Section

For a list of symbols defined in neuron.h try:

>>> dir(neuron.h)

Several hoc symbols are not useful in python, and thus raise an exception when accesed, for example:

In []: h.objref
---------------------------------------------------------------------------
TypeError                                 Traceback (most recent call last)

/home/emuller/hg/nrn_neurens_hg/<ipython console> in <module>()

TypeError: no HocObject interface for objref (hoc type 322)

In []: h.objref ?
Object `h.objref` not found.


"""



def get_docstring(objtype, symbol):
    """ Get the docstring for object-type and symbol.

    Ex:
    get_docstring('Vector','sqrt')

    returns a string

    """


    if (objtype,symbol)==('',''):

        return doc_h

    else:
        return "to appear"
