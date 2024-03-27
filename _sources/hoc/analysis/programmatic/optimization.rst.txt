Optimization
============


.. hoc:function:: fit_praxis


    Syntax:
        ``min = fit_praxis(n, "funname", &x[0])``

        ``min = fit_praxis(n, "funname", Vector)``

        ``min = fit_praxis(..., ..., ..., "after quad statement")``

        ``min = fit_praxis(efun_as_python_callable, hoc_vector)``


    Description:
        This is the principal axis method for minimizing a function. See praxis.c 
        in the scopmath library. 
         


        ``1 <= n < 20`` 
            is the number of parameters to vary (number 
            of arguments to *funname*). 

        *funname* 
            the name of the function to minimize, eg. least square difference between model and data. 
            The funname must take two arguments, the first arg, ``$1``, 
            is the number of elements in second arg vector, ``$&2``. The ith index of the 
            vector is given by ``$&2[i]``. 

        *x* 
            is a double vector of at least length *n*. Prior to the call set 
            it to a guess of the parameter values. On return it contains the 
            values of the args that minimize ``funname()``. 

         
        *funname* may be either 
        an interpreted hoc function or a compiled NMODL function. 
         
        If the variable stoprun is set to 1 during a call to fit_praxis, it will 
        return immediately (when the current call to  funname returns) with 
        a return value and varx values set to the best minimum found so far. Use 
        :hoc:func:`stop_praxis` to stop after finishing the current principal axis calculation.
         
        The fourth argument, if present, specifies a statement to be executed at 
        the end of each principal axis evaluation. 
         
        If the third argument is a Vector, then that style is used to specify 
        the initial starting point and return the final value. However the 
        function is still called with second arg as a pointer into a double array. 
         
        The Python callable form uses a Python Callable as the function to 
        minimize and it must take a single hoc Vector argument specifying the 
        values of the parameters for use in evaluation the function. On entry to 
        fit_praxis the Vector specifies the number of parameters and the 
        parameter starting values. On return the vector contains the values of 
        parameters which generated the least minimum found so far. 
         
        Hoc example: minimize ``(x+y - 5)^2 + 5*((x-y) - 15)^2``

        .. code-block::
            none

            objref vec 
            vec = new Vector(2) // vec.x[0] is x, vec.x[1] is y 
            func efun() {local x, y 
              x = $&2[0]  y = $&2[1] 
              return (x+y - 5)^2 + 5*(x-y - 15)^2 
            } 
            attr_praxis(1e-5, .5, 0) 
            e = fit_praxis(vec.size(), "efun", vec) 
            printf("e=%g x=%g y=%g\n", e, vec.x[0], vec.x[1]) 
             
            objref paxis 
            paxis = new Vector() 
            for i=0, 1 { 
              pval = pval_praxis(i, paxis) 
              printf("%d  %10g      %10g %10g\n", i, pval, paxis.x[0], paxis.x[1]) 
            } 

         
        Python example: 

        .. code-block::
            none

            from neuron import h 
            v = h.Vector(2) 
            def efun(v): 
              return (v.x[0]+v.x[1] - 5)**2 + 5*(v.x[0]-v.x[1] - 15)**2 
            h.attr_praxis(1e-5, .5, 0) 
            e = h.fit_praxis(efun, v) 
            print "e=%g x=%g y=%g\n"%(e, v.x[0], v.x[1]) 


    .. warning::
        Up to version 4.0.1, the arguments to *funname* were an explicit 
        list of *n* arguments. ie ``numarg()==n``. 

    .. seealso::
        :hoc:func:`attr_praxis`, :hoc:func:`stop_praxis`, :hoc:func:`pval_praxis`

         

----



.. hoc:function:: attr_praxis


    Syntax:
        ``attr_praxis(tolerance, maxstepsize, printmode)``

        ``previous_index = attr_praxis(mcell_ran4_index)``


    Description:
        Set the attributes of the praxis method. This must be called before 
        the first call to :hoc:func:`fit_praxis`.


        tolerance 
            praxis attempt to return f(x) such that if x0 is the true 
            local minimum then ``norm(x-x0) < tolerance`` 

        maxstepsize 
            should be set to about the maximum distance from 
            initial guess to the minimum. 

        printmode=0 
            	no printing 

        printmode=1,2,3 
            more and more verbose 

        The single argument form causes praxis to pick its random numbers from 
        the the mcellran4 generator beginning at the specified index. This 
        allows reproducible fitting. The return value is the previously picked 
        index. (see :hoc:func:`mcell_ran4`)

         

----



.. hoc:function:: pval_praxis


    Syntax:
        ``pval = pval_praxis(i)``

        ``pval = pval_praxis(i, &paxis[0])``

        ``pval = pval_praxis(i, Vector)``


    Description:
        Return the ith principal value. If the second argument is present, ``pval_praxis`` also fills 
        the vector with the ith principal axis. 

         

----



.. hoc:function:: stop_praxis


    Syntax:
        ``stop_praxis()``

        ``stop_praxis(i)``


    Description:
        Set a flag in the praxis function that will cause it to stop after 
        it finishes the current (or ith subsequent) 
        principal axis calculation. If this function 
        is called before :hoc:func:`fit_praxis`, then praxis will do a single
        (or i) principal axis calculation and then exit. 

         


