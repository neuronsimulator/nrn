Debugging and Internals Access
------------------------------

.. seealso::

    :hoc:func:`neuron.nrn_dll`


Namespace Related
~~~~~~~~~~~~~~~~~

.. hoc:function:: name_declared

    Syntax:
        ``type = name_declared("name")``

        ``type = name_declared("name", 1)``

        ``type = name_declared("name", 2)``

    Description:
        Return 0 if the name is not in the symbol table. The first form looks 
        for names in the top level symbol table. The second form looks in the 
        current object context. The last form also looks in the top level
        symbol table but is useful in Python to distinguish subtypes of
        variables which appear as doubles in HOC but internally are really
        not doubles and so cannot be pointed to by double*, eg. h.secondorder
        which is <type 'int'> or h.nseg which returns either
        ``TypeError: Section access unspecified`` or
        ``nseg  not a USERPROPERTY that can be pointed to``
         
        If the name exists return 
         
        2 if an objref
         
        3 if a Section 
         
        4 if a :ref:`strdef <hoc_keyword_strdef>`
         
        5 if a scalar or :ref:`double <hoc_keyword_double>` variable. (if second arg is not 2)

          if second arg is 2

          5 if a scalar double

          6 if a double array

          7 if an integer

          8 if a section property
         	         
        1 otherwise 

        Note that names can be (re)declared only if they do not already 
        exist or are already of the same type. 
        This is too useful to require the user to waste an objref in creating a 
        :hoc:class:`StringFunctions` class to use :hoc:meth:`~StringFunctions.is_name`.

        .. code-block::
            none

            name_declared("nrnmainmenu_") 
            {object_push(nrnmainmenu_) print name_declared("ldfile", 0) object_pop()} 
            {object_push(nrnmainmenu_) print name_declared("ldfile", 1) object_pop()} 


----

.. hoc:function:: symbols

    Name:
        symbols --- type the names of HOC functions and variables 

    Syntax:
        ``symbols()``

    Description:
        Types a list of functions and variable names defined in HOC.  Dimensions 
        of arrays are also indicated. 

    .. warning::
        No longer works. The nearest replacement is :hoc:func:`SymChooser` .





Object Related
~~~~~~~~~~~~~~


.. hoc:function:: object_id

    Syntax:
        ``object_id(objref)``

        ``object_id(objref, 1)``

    Description:
        Returns 0 if the object reference does not point to an object instance. 
        (Otherwise returns the pointer cast to a double, not a very useful number) 
         
        If the second argument is 1, it returns the index of the object name. Returns 
        -1 if the object is the NULLObject. 


----

.. hoc:function:: allobjectvars

    Syntax:
        ``allobjectvars()``

    Description:
        Prints all the object references (objref variables) that have been 
        declared along with the class type of the object they reference and the 
        number of references. 

    .. warning::
        Instead of printing the address of the object in hex format, it ought 
        also to print the object_id and/or the internal instance name. 

----

.. hoc:function:: allobjects

    Syntax:
        ``allobjects()``

        ``allobjects("templatename")``

        ``nref = allobjects(objectref)``

    Description:
        Prints the internal names of all class instances (objects) available 
        from the interpreter along with the number of references to them. 
         
        With a templatename the list is restricted to objects of that class. 
         
        With an object variable, nothing is printed but the reference count 
        is returned. The count is too large by one if the argument was of the 
        form templatename[index] since a temporary reference is created while 
        the object is on the stack during the call. 


----

.. hoc:function:: object_push

    Syntax:
        ``object_push(objref)``

    Description:
        Enter the context of the object referenced by objref. In this context you 
        can directly access any variables or call any functions, even those not 
        declared as :ref:`public <hoc_keyword_public>`. Do not attempt to create any new symbol names!
        This function is generally used by the object itself to save its state 
        in a session. 


----

.. hoc:function:: object_pop

    Syntax:
        ``object_pop()``

    Description:
        Pop the last object from an :hoc:func:`object_push` .

----

Miscellaneous
~~~~~~~~~~~~~

.. hoc:function:: hoc_pointer_

    Syntax:
        ``hoc_pointer_(&variable)``

    Description:
        A function used by c and c++ implementations to request a pointer to 
        the variable from its interpreter name. Not needed by the user. 

