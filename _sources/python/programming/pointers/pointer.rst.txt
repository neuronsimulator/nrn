.. _pointer:


.. warning::

    Use this class only when interacting with legacy code. In Python, NEURON pointers can be assigned to a variable
    and manipulated directly; this class was created as a work-around for HOC which did not permit assigning
    pointers to a variable.

Pointer Class
-------------


.. class:: Pointer


    Syntax:
        ``h.Pointer(_ref_x)``

        ``h.Pointer("variable")``

        ``h.Pointer(_ref_x, "stmt that may contain $1")``

        ``h.Pointer("variable", "stmt that may contain $1 or variable name")``

    Example:

        .. code-block::
            python

            >>> p = h.Pointer(h._ref_t)
            >>> p.val
            0.0
            >>> p.assign(42)
            42.0
            >>> h.t
            42.0
            >>> p.val = 13
            >>> h.t
            13.0

    Description:
        Holds a reference to a variable. When memory for the variable is freed, 
        the Pointer will return an error if used. 
         
        The optional second arg is used by :meth:`Pointer.assign` and is described
        there.

    .. warning::

        The string variable arguments refer to HOC variable names not Python variable names. See
        :meth:`Pointer.s` for an example.

----



.. attribute:: Pointer.val


    Syntax:
        ``x = ptr.val``

        ``ptr.val = expr``


    Description:
        Returns the value of the variable pointed to by ptr or, if the left 
        hand side of an assignment, sets the value of the variable. See the example in the constructor.

         

----



.. method:: Pointer.s


    Syntax:
        ``str = ptr.s()``


    Description:
        If the Pointer was constructed with the name of a variable, that name 
        can be retrieved as a string. 

    Example:

        .. code-block::
            python

            >>> h('create soma')
            1
            >>> p = h.Pointer('soma.v(0.5)')
            >>> p.s()
            'soma.v(0.5)'
         

----



.. method:: Pointer.assign


    Syntax:
        ``x = ptr.assign(val)``


    Description:
        Sets the value of the pointer variable to val. If  prt was constructed 
        with a second arg then the execution depends on its form. If the 
        second arg string contains one or more $1 tokens, then the tokens 
        are replaced by :data:`hoc_ac_`, :data:`hoc_ac_` is set to the val and the resulting 
        statement is executed. Otherwise the second arg string is assumed to 
        be a variable name and a statement of the form 
        variablename = :data:`hoc_ac_` is executed. 
        Note that the compiling of these statements takes place just once when 
        the Pointer is constructed. Thus ``ptr.assign(val)`` is marginally 
        faster than execute("stmt with val"). 
         
        ..
            (following not implemented) And if the stmt is a variablename 
            then the pointer is used and all interpreter overhead is avoided. 
            Also note that on construction, the second arg variable is executed with the 
            value of the first arg pointer. So 
         
        Returns val. 


