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

----

Debugging
~~~~~~~~~~~

.. function:: nrn_digest

    Syntax:
        ``h.nrn_digest()``

        ``h.nrn_digest(tid, i)``

        ``h.nrn_digest(tid, i, "abort")``

        ``h.nrn_digest(filename)``

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

    Syntax:
        ``h.use_exp_pow_precision(istyle)``

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
