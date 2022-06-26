
.. _hoc_pointer:

Pointer Class
-------------



.. hoc:class:: Pointer


    Syntax:
        ``Pointer(&x)``

        ``Pointer("variable")``

        ``Pointer(&x, "stmt that may contain $1")``

        ``Pointer("variable", "stmt that may contain $1 or variable name")``


    Description:
        Holds a reference to a variable. When memory for the variable is freed, 
        the Pointer will return an error if used. 
         
        See :hoc:meth:`Pointer.assign` for the meaning of the optional second arg.

----



.. hoc:method:: Pointer.val


    Syntax:
        ``x = ptr.val``

        ``ptr.val = expr``


    Description:
        Returns the value of the variable pointed to by ptr or, if the left 
        hand side of an assignment, sets the value of the variable. 

         

----



.. hoc:method:: Pointer.s


    Syntax:
        ``str = ptr.s``


    Description:
        If the Pointer was constructed with the name of a variable, that name 
        can be retrieved as a strdef. 

         

----



.. hoc:method:: Pointer.assign


    Syntax:
        ``x = ptr.assign(val)``


    Description:
        Sets the value of the pointer variable to val. If  prt was constructed 
        with a second arg then the execution depends on its form. If the 
        second arg string contains one or more $1 tokens, then the tokens 
        are replaced by :hoc:data:`hoc_ac_`, :hoc:data:`hoc_ac_` is set to the val and the resulting
        statement is executed. Otherwise the second arg string is assumed to 
        be a variable name and a statement of the form 
        variablename = :hoc:data:`hoc_ac_` is executed.
        Note that the compiling of these statements takes place just once when 
        the Pointer is constructed. Thus ptr.assign(val) is marginally 
        faster than execute("stmt with val"). 
         
        (following not implemented) And if the stmt is a variablename 
        then the pointer is used and all interpreter overhead is avoided. 
        Also note that on construction, the second arg variable is executed with the 
        value of the first arg pointer. So 
         
        Returns val. 


