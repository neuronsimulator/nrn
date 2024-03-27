Strings
-------

.. seealso::

    .. toctree::
        :maxdepth: 3
        
        strfun.rst
        sscanf.rst

----

.. function:: sprint

    Syntax:
        ``h.sprint(strdef, "format", args)``

    Description:
        Prints to a NEURON (i.e. not Python) string. See :func:`printf` for the description of the format.

    Example:

        from neuron import h

        strdef = h.ref('')
        h.sprint(strdef, 'There are %d %s.', 3, 'cows')
        print(strdef[0])

    .. note::

        Similar functionality is available for Python strings using the ``%`` operator or (for Python 2.6+) a
        string object's ``format`` method or (for Python 3.6+) with f-strings. 
        As Python strings are immutable, these approaches each create a new string.



----

.. function:: strcmp

    Syntax:
        ``x = h.strcmp("string1", "string2")``

    Description:
        return negative, 0, or positive value 
        depending on how the strings compare lexicographically. 
        0 means they are identical. 


