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

        .. code-block::
            python

            from neuron import h

            strdef = h.ref('')
            h.sprint(strdef, 'There are %d %s.', 3, 'dendrites')
            print(strdef[0])

    .. note::

        Similar functionality is available for Python strings using the ``%`` operator or (for Python 2.6+) a
        string object's ``format`` method or (for Python 3.6+) with f-strings. 
        As Python strings are immutable, these approaches each create a new string.

        We can also use f-strings to update NEURON strings by writing to ``strdef[0]``; for example:

        .. code-block::
            python

            from neuron import h

            strdef = h.ref('')
            num_parts = 3
            part = "dendrites"
            strdef[0] = f'There are {num_parts} {part}.'
            print(strdef[0])



----

.. function:: strcmp

    Syntax:
        ``x = h.strcmp("string1", "string2")``

    Description:
        Returns negative, 0, or positive value 
        depending on how the strings compare lexicographically. 
        0 means they are identical. A positive value indicates
        ``string1`` comes after ``string2``. This is a thin wrapper around
        the C standard library function ``strcmp`` and returns the
        same values it would return.

        Note: ``string1`` and ``string2`` must contain *only* ASCII characters.
        All strings (whether or not they contain unicode) may be compared
        directly in Python via ``<``, ``>``, ``==``, etc.

        For example ``"apical" < "basal"`` would return ``True``.

        Note that accented characters are not treated as equal to their
        non-accented counterparts. For example, ``"soma" == "sóma"`` would
        return ``False``.
        If you wish for accented and non-accented characters to be compared
        equal, one solution would be to use the third-party ``unidecode`` module,
        available via ``pip install unidecode``, to remove accents before running
        the comparison.

