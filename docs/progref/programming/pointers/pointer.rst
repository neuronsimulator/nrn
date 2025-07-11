.. _pointer:


.. warning::

    Use this class only when writing HOC code or (in Python) when interacting with legacy code. 
    
    In Python, NEURON pointers can be assigned to a variable
    and manipulated directly; this class was created as a work-around for HOC which does not permit assigning
    pointers to a variable.

Pointer Class
-------------


.. class:: Pointer

    .. tab:: Python
    
    
        Syntax:
            ``n.Pointer(_ref_x)``

            ``n.Pointer("variable")``

            ``n.Pointer(_ref_x, "stmt that may contain $1")``

            ``n.Pointer("variable", "stmt that may contain $1 or variable name")``

        Example:

            .. code-block::
                python

                >>> p = n.Pointer(n._ref_t)
                >>> p.val
                0.0
                >>> p.assign(42)
                42.0
                >>> n.t
                42.0
                >>> p.val = 13
                >>> n.t
                13.0

        Description:
            Holds a reference to a variable. When memory for the variable is freed, 
            the Pointer will return an error if used. 
         
            The optional second arg is used by :meth:`Pointer.assign` and is described
            there.

        .. warning::

            The string variable arguments refer to HOC variable names not Python variable names. See
            :meth:`Pointer.s` for an example.

    .. tab:: HOC


        Syntax:
            ``Pointer(&x)``
        
        
            ``Pointer("variable")``
        
        
            ``Pointer(&x, "stmt that may contain $1")``
        
        
            ``Pointer("variable", "stmt that may contain $1 or variable name")``
        
        
        Description:
            Holds a reference to a variable. When memory for the variable is freed, 
            the Pointer will return an error if used. 
        
        
            See :meth:`Pointer.assign` for the meaning of the optional second arg.
        
----



.. attribute:: Pointer.val

    .. tab:: Python

        Syntax:
            ``x = ptr.val``

            ``ptr.val = expr``


        Description:
            Returns the value of the variable pointed to by ptr or, if the left 
            hand side of an assignment, sets the value of the variable. See the example in the constructor.

    .. tab:: HOC

        Syntax:
            ``x = ptr.val``

            ``ptr.val = expr``


        Description:
            Returns the value of the variable pointed to by ptr or, if the left 
            hand side of an assignment, sets the value of the variable. 


         

----



.. method:: Pointer.s

    .. tab:: Python
    
    
        Syntax:
            ``str = ptr.s()``


        Description:
            If the Pointer was constructed with the name of a variable, that name 
            can be retrieved as a string. 

        Example:

            .. code-block::
                python

                >>> n('create soma')
                1
                >>> p = n.Pointer('soma.v(0.5)')
                >>> p.s()
                'soma.v(0.5)'
         

    .. tab:: HOC


        Syntax:
            ``str = ptr.s``
        
        
        Description:
            If the Pointer was constructed with the name of a variable, that name 
            can be retrieved as a strdef. 
        
----



.. method:: Pointer.assign

    .. tab:: Python
    
    
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


    .. tab:: HOC


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
            the Pointer is constructed. Thus ptr.assign(val) is marginally 
            faster than execute("stmt with val"). 
        
        
            (following not implemented) And if the stmt is a variablename 
            then the pointer is used and all interpreter overhead is avoided. 
            Also note that on construction, the second arg variable is executed with the 
            value of the first arg pointer. So 
        
        
            Returns val. 
        
