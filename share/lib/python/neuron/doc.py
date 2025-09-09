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

In []: from neuron import n
In []: v = n.Vector()
In []: ? v.to_python

a feature whose implementation is based on the neuron.doc module.

"""

import pydoc, sys, inspect


header = """
NEURON+Python Online Help System
================================

"""


# override basic helper functionality to give proper help on HocObjects
class NRNPyHelper(pydoc.Helper):
    def __call__(self, request=None):
        from . import n, hoc

        if (
            isinstance(request, type(n))
            or isinstance(request, hoc.HocClass)
            or isinstance(type(request), hoc.HocClass)
            or isinstance(request, hoc.HocObject)
        ):
            pydoc.pager(header + request.__doc__)
        else:
            pydoc.Helper.__call__(self, request)


help = NRNPyHelper(sys.stdin, sys.stdout)


def doc_asstring(thing, title="Python Library Documentation: {}", forceload=0):
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
        return title.format(desc) + "\n\n" + pydoc.text.document(object, name)
    except (ImportError, pydoc.ErrorDuringImport) as value:
        print(value)


# override systemwide behaviour
pydoc.help = help


doc_h = """

neuron.h
========

neuron.h is the top-level HocObject, allowing interaction between Python and Hoc.

Starting in NEURON 9, neuron.n is also available for constructing NEURON objects
and for accessing HOC.

neuron.h is callable like a function, and takes Hoc code as an argument to be
executed.

The top-level Hoc namespace is exposed as attributes to the h object.

Ex:

    >>> from neuron import h
    >>> h("objref myvec")
    >>> h("myvec = new Vector(10)")
    >>> h.myvec.x[0]=1.0
    >>> h.myvec.printf()
    1       0       0       0       0
    0       0       0       0       0


NEURON classes are defined in the h namespace and can be constructed as follows:

>>> v = h.Vector(10)
>>> soma = h.Section("soma")
>>> input = h.IClamp(soma(0.5))

More help on individual classes defined in Hoc and exposed in Python
is available using Jupyter's online help feature

In []: ? h.Section

or in standard Python by Python's help system

>>> help(h.Vector)

For a list of symbols defined in neuron.h try:

>>> dir(neuron.h)



NOTE: Several Hoc symbols are not useful in Python, and thus raise an exception when accessed, for example:

>>> h.objref
Traceback (most recent call last):
  File "<stdin>", line 1, in <module>
TypeError: Cannot access objref (NEURON type 326) directly.
>>> help(h.objref)
Traceback (most recent call last):
  File "<stdin>", line 1, in <module>
TypeError: Cannot access objref (NEURON type 326) directly.


"""


default_class_doc_template = """
No docstring available for class '%s'

Try checking the online documentation at:
https://nrn.readthedocs.org/
"""


default_object_doc_template = """
No docstring available for object type '%s'

Try checking the online documentation at:
https://nrn.readthedocs.org/
"""


default_member_doc_template = """
No docstring available for the class member '{}.{}'

Try checking the online documentation at:
https://nrn.readthedocs.org/

==================================================

%s
    
"""


def _get_from_help_dict(name):
    return _help_dict.get(name, "")


def _get_class_from_help_dict(name):
    from . import n

    result = _get_from_help_dict(name)
    if not result:
        return ""
    methods = dir(getattr(n, name))
    for m in methods:
        if f"{name}.{m}" in _help_dict:
            result += f"\n\n\n{name}.{m}:\n\n{_help_dict[f'{name}.{m}']}"
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
            return default_object_doc_template % objtype

    # are we asking for help on a member of an object, e.g. h.Vector.size
    full_name = f"{objtype}.{symbol}"
    if full_name in _help_dict:
        return _get_from_help_dict(full_name)
    else:
        return default_member_doc_template.format(
            objtype,
            symbol,
            get_docstring(objtype, ""),
        )
