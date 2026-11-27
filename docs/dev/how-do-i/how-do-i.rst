==============
How-Do-I Guide
==============

Working with Object and PyObject
================================

NEURON does not include :file:`Python.h`. The CPython functions it calls are looked up in the running
interpreter; all of them are in the stable ABI. They are listed in :file:`src/nrniv/nrnpy_capi.h`; code that calls
them belongs in :file:`src/nrniv/nrnpy_object.cpp`, in ``namespace neuron::python``, with the GIL held (``Gil``) and
each new reference in a ``Ref``.

Code elsewhere calls it through a function pointer declared in :file:`src/nrniv/nrnpy.h` that
``register_python_object()`` sets, and must first check that the pointer is not NULL: it is in a build without Python.
Python itself starts when one of these functions first needs it.


To check if a NEURON object is wrapping a PyObject
--------------------------------------------------

To see if an Object* obj is wrapping a PyObject, check if

    .. code-block:: c

        obj->ctemplate->sym == nrnpy_pyobj_sym_

(See object types below for more about detecting the type of an ``Object*``.)


To encapsulate a NEURON Object in a PyObject
--------------------------------------------

Use ``to_python``. If the NEURON object is wrapping a ``PyObject`` it returns the original ``PyObject``.
Otherwise it asks the Python interface for a wrapper (the ``object_class`` hook of :c:func:`nrn_python_hook_set`).


To encapsule a PyObject in a NEURON Object
------------------------------------------

Use ``to_hoc``. If the object is wrapping a NEURON Object, it increments the NEURON object's reference count by 1 and returns the original
NEURON object. Otherwise it returns a new NEURON Object* wrapping the ``PyObject``, even for
strings and floats that would be more naturally represented in NEURON as their respective datatypes.

To check if a PyObject is a number
----------------------------------

Use ``is_number``. Unlike the CPython API ``PyNumber_Check`` it detects things that NEURON cannot treat directly as numbers,
like complex numbers.

To reference a NEURON Object
----------------------------

Use ``hoc_obj_ref(obj)`` where ``obj`` is an ``Object*``. This is defined in :file:`src/oc/hoc_oop.c`. Include :file:`src/oc/oc_ansi.h` to define the function prototype.

To dereference a NEURON Object
------------------------------

Use ``hoc_obj_unref(obj)`` where ``obj`` is an ``Object*``. This is defined in :file:`src/oc/hoc_oop.c`. Include :file:`src/oc/oc_ansi.h` to define the function prototype.

Arguments
=========

Only positional arguments (and sec=) are allowed in NEURON *except* for functions with no HOC
equivalent that are handled separately.

Note: NEURON argument indices are always numbered from 1.

Check for the existence of an argument
--------------------------------------

Use ``ifarg(n)`` to check to see if there is an ``n``\ th argument (the arguments are numbered from 1).

Many existing functions and methods currently do not check for too many arguments, however doing
so is recommended as it indicates a probable user error.

Check the type of an argument
-----------------------------

Relevant functions include:
- ``hoc_is_object_arg(n)``
- ``hoc_is_pdouble_arg(n)``
- ``hoc_is_str_arg(n)``
- ``hoc_is_double_arg(n)``

Get the value of an argument
----------------------------

Relevant functions include:
- ``hoc_objgetarg(n)`` -- returns a ``Object**``

- ``vector_arg(n)`` -- returns a ``IvocVect*``
- ``hoc_pgetarg(n)`` -- returns a ``double**``
- ``gargstr(n)`` -- returns a ``char*``
- ``getarg(n)`` -- returns a ``double*``. Python bools, ints, and floats are all valid inputs.

Note: attempting to get the wrong type of an argument displays a "bad stack access" message and
a Python ``RuntimeError`` exception gets raised. If multiple types of arguments are possible,
you must check the type of the argument first.

Classes
=======

Declaring classes
-----------------

Classes are declared using the ``class2oc`` function, e.g.

    .. code-block:: c
    
        class2oc("ClassName", cons, destruct, members, NULL, retobj_members, NULL)
  
Here ``cons`` is the constructor, which must take an ``Object*`` and return a ``void*``.

``destruct`` is the destructor, which takes a ``void*`` and has no return.

``members`` is a null-terminated array of ``Member_func`` of methods that in Python could return float, 
integer, or bool. In HOC, these all return doubles.
- To specify the return type as seen by Python, set ``hoc_return_type_code``.
  Set it to the appropriate ``HocReturnType``.
- Each of these methods must take a ``void*`` and return a double.

``retobj_methods`` is a null-terminated array of ``Member_ret_obj_func`` of methods that return objects.
(The actual functions implementing them take a ``void*`` and return an ``Object**``.)

Object types
------------

The type of every NEURON ``Object* obj`` is determined by it's ``ctemplate->sym``. This is a ``Symbol*``.
The pointers can be directly compared to see if two objects are of the same type. In particular,
a ``Symbol`` has a ``char*`` field ``name``. That is, to print the name of the type
that ``obj`` is an instance of, one can use:

    .. code-block:: c
    
        printf("The type of obj is: %s\n", obj->ctemplate->sym->name);

The ``hoc_lookup`` function takes a NEURON class name and returns the associated ``Symbol*``.
For example:

    .. code-block:: c
    
        Symbol* vector_sym = hoc_lookup("Vector");

NEURON provides internal convenience functions ``is_obj_type(Object* obj, const char* type_name)``
and ``check_obj_type(Object* obj, const char* type_name)`` that check to see if ``obj`` is of the
type specified by ``type_name``. The former returns a 1 (true) or 0 (false); the latter has no
return and raises an error if the type is wrong. These work by doing a ``strcmp``. If the
``Symbol*`` is known, it is more efficient to directly compare the ``Symbol*``.

For example, to see if ``obj`` is an instance of ``Vector`` and the ``Symbol*`` is not already
known, use ``is_obj_type(obj, "Vector")``.

These convenience functions are defined in ``src/oc/hoc_oop.cpp``.

HocCommand objects
==================

The ``HocCommand`` class, defined in ``src/ivoc/objcmd.cpp``, provides a consistent interface
for calling Python or HOC code. The constructor accepts a ``const char*`` for HOC, or a
``const char*`` and an ``Object*`` also for HOC (in this case the HOC string is executed in
the context of the object), or an ``Object*`` that wraps a Python callable.

Each ``HocCommand`` object has a ``pyobject()`` method that returns the underlying Python
object if any, else NULL. This can be used to distinguish between HOC and Python calls.

The ``execute()`` method runs the underlying HOC or Python code. No value is returned in
this case.

The ``func_call(int narg, int* perr)`` method returns a double from invoking the HOC or Python. 
The value pointed to by ``perr`` is set to 1 if the ``HocCommand`` is to run Python but running
Python failed. Otherwise ``perr`` is unchanged. In particular, note that if ``perr`` originally
points to a 1, then it will still point to a 1 even upon success. The number of arguments is
indicated with ``narg``. The arguments themselves must have already been pushed onto NEURON's
stack, e.g. with ``pushx`` for doubles, ``hoc_push_object`` for ``Object*``, ``hoc_push_str``
for ``char**``, or ``hoc_pushpx`` for pointers to doubles (stack manipulation functions are
defined in ``src/oc/code.cpp``).


Miscellaneous tips
==================

Raising a NEURON error
----------------------

Use ``hoc_execerror`` which takes two ``char*`` arguments (which can be NULL). e.g.

    .. code-block:: c
    
        hoc_execerror("Message part 1", "Message part 2");

Note: all NEURON errors currently are received by Python as a ``RuntimeError`` exception, and all errors
print their error messages before returning to Python, meaning that they will always print out, even
inside a try/except block.

Checking if the name of an internal symbol
------------------------------------------

``hoc_table_lookup(name, hoc_built_in_symlist)`` returns NULL if ``name`` not in the symlist; otherwise
it returns the ``Symbol*``
