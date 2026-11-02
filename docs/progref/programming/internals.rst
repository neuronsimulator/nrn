Debugging and Internals Access
------------------------------

.. seealso::

    :func:`neuron.nrn_dll`


Namespace Related
~~~~~~~~~~~~~~~~~

.. function:: name_declared

    .. tab:: Python

        Syntax:
            ``type = n.name_declared("name")``

            ``type = n.name_declared("name", 1)``

            ``type = n.name_declared("name", 2)``

        Description:
            Return 0 if the name is not in the NEURON/HOC symbol table. The first form looks 
            for names in the top level symbol table. The second form looks in the 
            current object context. The last form also looks in the top level
            symbol table but is useful in Python to distinguish subtypes of
            variables which appear as doubles in HOC but internally are really
            not doubles and so cannot be pointed to by ``double*``, e.g., ``n.secondorder``
            which is ``<type 'int'>`` or ``n._ref_nseg`` which raises either
            ``TypeError: Section access unspecified`` or
            ``TypeError: Cannot be a reference``
            
            If the name exists return:

            .. list-table::
                :header-rows: 1

                * - Return Value
                  - Description
                * - 2
                  - If an objref
                * - 3
                  - If a Section
                * - 4
                  - If a :ref:`strdef <keyword_strdef>`
                * - 5
                  - If a scalar or :ref:`double <keyword_double>` variable (if second arg is not 2)

            If the second argument is 2:

            .. list-table::
                :header-rows: 1

                * - Return Value
                  - Description
                * - 5
                  - If a scalar double
                * - 6
                  - If a double array
                * - 7
                  - If an integer
                * - 8
                  - If a Section property

            If none of the above apply, return 1.

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

            If the name is known in advance, use a ``try``/``except`` block and catch ``NameError`` and ``AttributeError``:

            .. code-block::
                python

                try:
                    n.soma.squiggle
                except (NameError, AttributeError):
                    print('Name does not exist')

            Combining this with an ``eval`` can allow testing arbitrary names, but is potentially unsafe as it allows
            execution of arbitrary code.

    .. tab:: HOC

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
            not doubles and so cannot be pointed to by double*, eg. n.secondorder
            which is <type 'int'> or n.nseg which returns either
            ``TypeError: Section access unspecified`` or
            ``nseg  not a USERPROPERTY that can be pointed to``
            
            If the name exists return:

            .. list-table::
                :header-rows: 1

                * - Return Value
                  - Description
                * - 2
                  - If an objref
                * - 3
                  - If a Section
                * - 4
                  - If a :ref:`strdef <keyword_strdef>`
                * - 5
                  - If a scalar or :ref:`double <keyword_double>` variable (if second arg is not 2)

            If the second argument is 2:

            .. list-table::
                :header-rows: 1

                * - Return Value
                  - Description
                * - 5
                  - If a scalar double
                * - 6
                  - If a double array
                * - 7
                  - If an integer
                * - 8
                  - If a Section property

            If none of the above apply, return 1.

            Note that names can be (re)declared only if they do not already 
            exist or are already of the same type. 
            This is too useful to require the user to waste an objref in creating a 
            :class:`StringFunctions` class to use :meth:`~StringFunctions.is_name`.

            .. code-block::
                none

                name_declared("nrnmainmenu_") 
                {object_push(nrnmainmenu_) print name_declared("ldfile", 0) object_pop()} 
                {object_push(nrnmainmenu_) print name_declared("ldfile", 1) object_pop()} 


----

.. function:: symbols

    .. tab:: Python

        Name:
            symbols --- type the names of HOC functions and variables 

        Syntax:
            ``n.symbols()``

        Description:
            Types a list of functions and variable names defined in HOC.  Dimensions 
            of arrays are also indicated. 

        .. warning::
            No longer works. The nearest replacement is ``dir(h)`` or :func:`SymChooser` . 

    .. tab:: HOC

        Name:
            symbols --- type the names of HOC functions and variables 

        Syntax:
            ``symbols()``

        Description:
            Types a list of functions and variable names defined in HOC.  Dimensions 
            of arrays are also indicated. 

        .. warning::
            No longer works. The nearest replacement is :func:`SymChooser` .

Object Related
~~~~~~~~~~~~~~


.. function:: object_id

    .. tab:: Python

        Syntax:
            ``n.object_id(objref)``

            ``n.object_id(objref, 1)``

        Description:
            Returns 0 if the object reference does not point to an object instance. 
            (Otherwise returns the pointer cast to a double, not a very useful number,
            except that this is equal to the value returned by Python's ``hash`` function.) 
            
            If the second argument is 1, it returns the index of the object name. Returns 
            -1 if the object is the ``NULLObject``. 

        Example:

            .. code-block::
                python

                from neuron import n

                a, b, c = n.List(), n.List(), n.Vector()

                print(n.object_id(a))       # displays a double; equal to hash(a)
                print(n.object_id(a, 1))    # 0 since a == n.List[0]
                print(n.object_id(b, 1))    # 1 since b == n.List[1]
                print(n.object_id(c, 1))    # 0 since c == n.Vector[0]

    .. tab:: HOC

        Syntax:
            ``object_id(objref)``

            ``object_id(objref, 1)``

        Description:
            Returns 0 if the object reference does not point to an object instance. 
            (Otherwise returns the pointer cast to a double, not a very useful number,
            except that this is equal to the value returned by Python's ``hash`` function.) 
            
            If the second argument is 1, it returns the index of the object name. Returns 
            -1 if the object is the ``NULLObject``. 

        Example:

            .. code-block::
                C++

                objref a, b, c
                a = new List()
                b = new List()
                c = new Vector()

                print object_id(a)       // displays a double; equal to hash(a)
                print object_id(a, 1)    // 0 since a == List[0]
                print object_id(b, 1)    // 1 since b == List[1]
                print object_id(c, 1)    // 0 since c == Vector[0]


----

.. function:: allobjectvars

    .. tab:: Python

        Syntax:
            ``n.allobjectvars()``

        Description:
            Prints all the HOC object references (objref variables) that have been 
            declared along with the class type of the object they reference and the 
            number of references. Objects created via Python and not assigned to a 
            HOC objref will not appear.

        Example:

            .. code-block::
                python

                >>> n('objref foo')
                1
                >>> n.foo = n.Vector()
                >>> n.allobjectvars()
                obp hoc_obj_[0] -> NULL
                obp hoc_obj_[1] -> NULL
                obp foo[0] -> Vector[0] with 1 refs.
                0.0
                >>> banana = n.foo
                >>> n.allobjectvars()
                obp hoc_obj_[0] -> NULL
                obp hoc_obj_[1] -> NULL
                obp foo[0] -> Vector[0] with 2 refs.
                0.0

    .. tab:: HOC

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

.. function:: allobjects

    .. tab:: Python

        Syntax:
            ``n.allobjects()``

            ``n.allobjects("templatename")``

            ``nref = n.allobjects(objectref)``

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

                >>> v = n.Vector()
                >>> foo = n.List()
                >>> n.allobjects()
                List[0] with 1 refs
                Vector[0] with 1 refs
                0.0
                >>> n.allobjects('Vector')
                Vector[0] with 1 refs
                0.0
                >>> n.allobjects(foo)
                2.0
        
        .. seealso::

            Use a :class:`List` to programmatically loop over all instances of a
            template.

    .. tab:: HOC

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

.. function:: object_push

    .. tab:: Python

        Syntax:

        .. code-block:: python

            n.object_push(objref)

        Description:
            Enter the context of the object referenced by objref. In this context you 
            can directly access any variables or call any functions, even those not 
            declared as :ref:`public <keyword_public>`. Do not attempt to create any new symbol names! 
            This function is generally used by the object itself to save its state 
            in a session. 

    .. tab:: HOC

        Syntax:

        .. code-block:: C++

            object_push(objref)

        Description:
            Enter the context of the object referenced by objref. In this context you 
            can directly access any variables or call any functions, even those not 
            declared as :ref:`public <keyword_public>`. Do not attempt to create any new symbol names! 
            This function is generally used by the object itself to save its state 
            in a session. 

----

.. function:: object_pop

    .. tab:: Python

        Syntax:
            ``n.object_pop()``

        Description:
            Pop the last object from an :func:`object_push` . 

    .. tab:: HOC

        Syntax:
            ``object_pop()``

        Description:
            Pop the last object from an :func:`object_push` . 

----

Miscellaneous
~~~~~~~~~~~~~

.. function:: hoc_pointer_

    .. tab:: Python

        Syntax:
            ``n.hoc_pointer_(variable_ref)``

        Description:
            A function used by C and C++ implementations to request a pointer to 
            the variable from its interpreter name. Not needed by or useful for the user; returns 1.0 on
            success.

    .. tab:: HOC

        Syntax:
            ``hoc_pointer_(&variable)``

        Description:
            A function used by C and C++ implementations to request a pointer to 
            the variable from its interpreter name. Not needed by or useful for the user; returns 1.0 on
            success.


----

Debugging
~~~~~~~~~~~

.. function:: nrn_digest

    .. tab:: Python

        Syntax:
            ``n.nrn_digest()``

            ``n.nrn_digest(tid, i)``

            ``n.nrn_digest(tid, i, "abort")``

            ``n.nrn_digest(filename)``

        Description:
            Available when configured with the cmake option ``-DNRN_ENABLE_DIGEST=ON``

            If the same simulation gives different results on different machines,
            this function can help isolate the statement that generates the
            first difference during the simulation.
            I think :meth:`ParallelContext.prcellstate` is generally better, but in rare
            situations, nrn_digest can be very helpful.

            The first three forms begin digest gathering. The last form
            prints the gathered digest information to the filename.
            With just the two ``tid, i`` arguments, the i gathered item of the
            tid thread is printed (for single thread simulations, use ``tid = 0``),
            to the terminal as well as the individual values of the array
            for that digest item. With the third ``"abort"`` argument, the
            ith gathered item is printed and ``abort()`` is called (dropping
            into gdb if that is being used so that one can observe the backtrace).

            Lines are inserted into the digest by calling the C function declared
            in ``src/oc/nrndigest.h``.
                ``void nrn_digest_dbl_array(const char* msg, int tid, double t, double* array, size_t sz);``
            at the moment, such lines are present in ``src/nrncvode/occvode.cpp``
            to instrument the cvode callbacks that compute ``y' = f(y, t)`` and the
            approximate jacobian matrix solver ``M*x = b``. I.e in part

            .. code-block::

                #include "nrndigest.h"
                ...
                void Cvode::fun_thread(neuron::model_sorted_token const& sorted_token,
                        double tt,
                        double* y,
                        double* ydot,
                        NrnThread* nt) {
                    CvodeThreadData& z = CTD(nt->id);
                #if NRN_DIGEST
                    if (nrn_digest_) {
                        nrn_digest_dbl_array("y", nt->id, tt, y, z.nvsize_);
                    }
                #endif
                ...
                #if NRN_DIGEST
                    if (nrn_digest_ && ydot) {
                        nrn_digest_dbl_array("ydot", nt->id, tt, ydot, z.nvsize_);
                    }
                #endif

            Note: when manually adding such lines, the conditional compilation and
            nrn\_digest\_ test are not needed. The arguments to
            ``nrn_digest_dbl_array`` determine the line added to the digest.
            The 5th arg is the size of the 4th arg double array. The double array
            is processed by SHA1 and the first 16 hex digits are appended to the line.
            An example of the first few lines of output in a digest file is
            .. code-block::

                tid=0 size=1344
                y 0 0 0 e1f6a372856b45e6
                y 0 1 0 e1f6a372856b45e6
                ydot 0 2 0 523c9694c335e458
                y 0 3 4.7121609153871379e-09 fabb4bc469447404
                ydot 0 4 4.7121609153871379e-09 60bcff174645fc29

            The first line is thread id and number of lines for that thread.
            Other thread groups, if any, follow the end of each thread group.
            The digest lines consist of thread id, line identifier (start from 0
            for each group), double value of the 3rd arg, hash of the array.

----

.. function:: use_exp_pow_precision

    .. tab:: Python

        Syntax:
            ``n.use_exp_pow_precision(istyle)``

        Description:
            Works when configured with the cmake option
            ``-DNRN_ENABLE_ARCH_INDEP_EXP_POW=ON`` and otherwise does nothing.

            * istyle = 1
                All calls to :func:`exp` and :func:`pow` as well as their use
                internally, in mod files, and by cvode, are computed on mac, linux,
                windows so that double precision floating point results are
                cross platform consistent. (Makes use of a
                multiple precision floating-point computation library.)

            * istyle = 2
                exp and pow are rounded to 32 bits of mantissa

            * istyle = 0
                Default.
                exp and pow calcualted natively (cross platform values can have
                round off error differences.)

                When using clang (eg. on a mac) cross platform floating point
                identity is often attainable with  C and C++ flag option
                ``"-ffp-contract=off"``.
