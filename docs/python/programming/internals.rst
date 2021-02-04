Debugging and Internals Access
------------------------------

.. seealso::

    :func:`neuron.nrn_dll`


Namespace Related
~~~~~~~~~~~~~~~~~

.. function:: name_declared

    Syntax:
        ``type = h.name_declared("name")``

        ``type = h.name_declared("name", 1)``

        ``type = h.name_declared("name", 2)``

    Description:
        Return 0 if the name is not in the NEURON/HOC symbol table. The first form looks 
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
         
        4 if a :ref:`strdef <keyword_strdef>` 
         
        5 if a scalar or :ref:`double <keyword_double>` variable. (if second arg is not 2)

          if second arg is 2

          5 if a scalar double

          6 if a double array

          7 if an integer

          8 if a section property
         	         
        1 otherwise 

        .. ::

            A HOC work-around that is not relevant in Python.

            Note that names can be (re)declared only if they do not already 
            exist or are already of the same type. 
            This is too useful to require the user to waste an objref in creating a 
            :class:`StringFunctions` class to use :meth:`~StringFunctions.is_name`. 

            .. code-block::
                none

                name_declared("nrnmainmenu_") 
                {object_push(nrnmainmenu_) print name_declared("ldfile", 0) object_pop()} 
                {object_push(nrnmainmenu_) print name_declared("ldfile", 1) object_pop()} 

    .. note::

        This function checks the NEURON/HOC symbol table; Python objects are handled separately.

        To test if a simple name is a local variable in Python, use:

        .. code-block::
            python

            if 'soma' in locals():
                # do something

        Checking against ``globals()`` and ``dir()`` are also often useful.

        If the name is known in advance, use a ``try``/``except`` block and catch NameError and AttributeError:

        .. code-block::
            python

            try:
                h.soma.squiggle
            except (NameError, AttributeError):
                print('Name does not exist')

        Combining this with an ``eval`` can allow testing arbitrary names, but is potentially unsafe as it allows
        execution of arbitrary code.


----

.. function:: symbols

    Name:
        symbols --- type the names of HOC functions and variables 

    Syntax:
        ``h.symbols()``

    Description:
        Types a list of functions and variable names defined in HOC.  Dimensions 
        of arrays are also indicated. 

    .. warning::
        No longer works. The nearest replacement is :func:`SymChooser` . 





Object Related
~~~~~~~~~~~~~~


.. function:: object_id

    Syntax:
        ``h.object_id(objref)``

        ``h.object_id(objref, 1)``

    Description:
        Returns 0 if the object reference does not point to an object instance. 
        (Otherwise returns the pointer cast to a double, not a very useful number,
        except that this is equal to the value returned by Python's ``hash`` function.) 
         
        If the second argument is 1, it returns the index of the object name. Returns 
        -1 if the object is the NULLObject. 

    Example:

        .. code-block::
            python

            from neuron import h

            a, b, c = h.List(), h.List(), h.Vector()

            print(h.object_id(a))       # displays a double; equal to hash(a)
            print(h.object_id(a, 1))    # 0 since a == h.List[0]
            print(h.object_id(b, 1))    # 1 since b == h.List[1]
            print(h.object_id(c, 1))    # 0 since c == h.Vector[0]

----

.. function:: allobjectvars

    Syntax:
        ``h.allobjectvars()``

    Description:
        Prints all the HOC object references (objref variables) that have been 
        declared along with the class type of the object they reference and the 
        number of references. Objects created via Python and not assigned to a 
        HOC objref

    Example:

        .. code-block::
            python

            >>> h('objref foo')
            1
            >>> h.foo = h.Vector()
            >>> h.allobjectvars()
            obp hoc_obj_[0] -> NULL
            obp hoc_obj_[1] -> NULL
            obp foo[0] -> Vector[0] with 1 refs.
            0.0
            >>> banana = h.foo
            >>> h.allobjectvars()
            obp hoc_obj_[0] -> NULL
            obp hoc_obj_[1] -> NULL
            obp foo[0] -> Vector[0] with 2 refs.
            0.0

----

.. function:: allobjects

    Syntax:
        ``h.allobjects()``

        ``h.allobjects("templatename")``

        ``nref = h.allobjects(objectref)``

    Description:
        Prints the internal names of all class instances (objects) available 
        from the interpreter along with the number of references to them. 
         
        With a templatename the list is restricted to objects of that class. 
         
        With an object variable, nothing is printed but the reference count 
        is returned. The count is too large by one if the argument was of the 
        form templatename[index] since a temporary reference is created while 
        the object is on the stack during the call. 

    Example:

        .. code-block::
            python

            >>> v = h.Vector()
            >>> foo = h.List()
            >>> h.allobjects()
            List[0] with 1 refs
            Vector[0] with 1 refs
            0.0
            >>> h.allobjects('Vector')
            Vector[0] with 1 refs
            0.0
            >>> h.allobjects(foo)
            2.0

----

.. function:: object_push

    Syntax:
        ``h.object_push(objref)``

    Description:
        Enter the context of the object referenced by objref. In this context you 
        can directly access any variables or call any functions, even those not 
        declared as :ref:`public <keyword_public>`. Do not attempt to create any new symbol names! 
        This function is generally used by the object itself to save its state 
        in a session. 


----

.. function:: object_pop

    Syntax:
        ``h.object_pop()``

    Description:
        Pop the last object from an :func:`object_push` . 

----

Miscellaneous
~~~~~~~~~~~~~

.. function:: hoc_pointer_

    Syntax:
        ``h.hoc_pointer_(&variable)``

    Description:
        A function used by c and c++ implementations to request a pointer to 
        the variable from its interpreter name. Not needed by or useful for the user; returns 1.0 on
        success.

