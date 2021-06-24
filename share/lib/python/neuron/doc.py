"""
neuron.doc
===================

This module is used behind the scenes to generate docstrings of all HocObjects
of various types

To get general help on the neuron module try:

>>> import neuron
>>> help(neuron)

or in Jupyter

In []: import neuron
In []: neuron ?

From there, you can get help on the various objects in the hoc world:

In []: fom neuron import h
In []: v = h.Vector()
In []: ? v.to_python

a feature whose implementation is based on the neuron.doc module.

"""

import pydoc, sys, io, inspect
from neuron import h


header = """
NEURON+Python Online Help System
================================

"""


# override basic helper functionality to give proper help on HocObjects
class NRNPyHelper(pydoc.Helper):
    def __call__(self, request=None):
        if type(request) == type(h):
            pydoc.pager(header + request.__doc__)
        else:
            pydoc.Helper.__call__(self, request)


help = NRNPyHelper(sys.stdin, sys.stdout)


def doc_asstring(thing, title="Python Library Documentation: %s", forceload=0):
    """return text documentation as a string, given an object or a path to an object."""
    try:
        object, name = pydoc.resolve(thing, forceload)
        desc = pydoc.describe(object)
        module = inspect.getmodule(object)
        if name and "." in name:
            desc += " in " + name[: name.rfind(".")]
        elif module and module is not object:
            desc += " in module " + module.__name__
        if not (
            inspect.ismodule(object)
            or inspect.isclass(object)
            or inspect.isroutine(object)
            or inspect.isgetsetdescriptor(object)
            or inspect.ismemberdescriptor(object)
            or isinstance(object, property)
        ):
            # If the passed object is a piece of data or an instance,
            # document its available methods instead of its value.
            object = type(object)
            desc += " object"
        return title % desc + "\n\n" + pydoc.text.document(object, name)
    except (ImportError, ErrorDuringImport) as value:
        print(value)


# override systemwide behaviour
pydoc.help = help


doc_h = """

neuron.h
========

neuron.h is the top-level HocObject, allowing interaction between Python and Hoc.

It is callable like a function, and takes Hoc code as an argument to be executed.

The top-level Hoc namespace is exposed as attributes to the h object.

Ex:

    >>> h = neuron.h
    >>> h("objref myvec")
    >>> h("myvec = new Vector(10)")
    >>> h.myvec.x[0]=1.0
    >>> h.myvec.printf()
    1       0       0       0       0
    0       0       0       0       0


NEURON classes are defined in the h namespace and can be constructed as follows:

>>> v = h.Vector(10)
>>> soma = h.Section()
>>> input = h.IClamp(soma(0.5))

More help on individual classes defined in Hoc and exposed in Python
is available using Jupyter's online help feature

In []: ? h.Section

or in standard Python by python help system

>>> help(h.Vector)

For a list of symbols defined in neuron.h try:

>>> dir(neuron.h)



NOTE: Several Hoc symbols are not useful in Python, and thus raise an exception when accessed, for example:

In []: h.objref
---------------------------------------------------------------------------
TypeError                                 Traceback (most recent call last)

/home/emuller/hg/nrn_neurens_hg/<ipython console> in <module>()

TypeError: Cannot access objref (NEURON type 323) directly.

In []: h.objref ?
Object `h.objref` not found.


"""


default_class_doc_template = """
No docstring available for class '%s'

Try checking the online documentation at:
https://www.neuron.yale.edu/neuron/static/py_doc/index.html
"""


default_object_doc_template = """
No docstring available for object type '%s'

Try checking the online documentation at:
https://www.neuron.yale.edu/neuron/static/py_doc/index.html
"""


default_member_doc_template = """
No docstring available for the class member '%s.%s'

Try checking the online documentation at:
https://www.neuron.yale.edu/neuron/static/py_doc/index.html

==================================================

%s
    
"""


def _get_from_help_dict(name):
    return _help_dict.get(name, "")


def _get_class_from_help_dict(name):
    result = _get_from_help_dict(name)
    if not result:
        return ""
    methods = dir(h.__getattribute__(name))
    for m in methods:
        if name + "." + m in _help_dict:
            result += "\n\n\n%s.%s:\n\n%s" % (name, m, _help_dict[name + "." + m])
    return result


_help_dict = None


def get_docstring(objtype, symbol):
    """Get the docstring for object-type and symbol.

    Ex:
    get_docstring('Vector','sqrt')

    returns a string

    """
    global _help_dict
    if _help_dict is None:
        import neuron
        import os
        import zlib
        import pickle

        f = open(os.path.join(os.path.split(neuron.__file__)[0], "help_data.dat"), "rb")
        _help_dict = pickle.loads(zlib.decompress(f.read()))
        f.close()

    if (objtype, symbol) == ("", ""):

        return doc_h

    # are we asking for help on a class, e.g. h.Vector
    if objtype == "":
        if symbol in _help_dict:
            return _get_class_from_help_dict(symbol)
        else:
            return default_class_doc_template % symbol

    # are we asking for help on a object, e.g. h.Vector()
    if symbol == "":
        if objtype in _help_dict:
            return _get_class_from_help_dict(objtype)
        else:
            return default_object_doc_template % symbol

    # are we asking for help on a member of an object, e.g. h.Vector.size
    full_name = "%s.%s" % (objtype, symbol)
    if full_name in _help_dict:
        return _get_from_help_dict(full_name)
    else:

        return default_member_doc_template % (
            objtype,
            symbol,
            get_docstring(objtype, ""),
        )
