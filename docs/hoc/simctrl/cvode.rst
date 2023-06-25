
.. _hoc_cvode:

         
CVode
-----

.. hoc:class:: CVode


    Syntax:
        ``objref cvode``

        ``cvode = new CVode()``


    Description:
        Multi order variable time step integration method which may be used in place 
        of the default staggered fixed time step method.  The performance benefits 
        can be substantial (factor of more than 10) for problems in which all states 
        vary slowly for long periods of time between fast spikes. 
         
        Although for historical reasons, this class is called CVode at the hoc level, 
        in fact it is an interface to a family of methods which are implemented on 
        top of the CVODES and IDA integrators of the SUNDIALS package, 

        .. code-block::
            none

              SUite of Nonlinear and DIfferential/ALgebraic equation Solvers 
                               Release 2.0.1, January 2005 
                 Peter Brown, Aaron Collier, Keith Grant, Alan Hindmarsh, 
                  Steve Lee, Radu Serban, Dan Shumaker, Carol Woodward 
                      Center for Applied Scientific Computing, LLNL 

        (see :hoc:meth:`CVode.use_local_dt` and :hoc:meth:`CVode.use_daspk`)
         
        When this class is :hoc:meth:`CVode.active` the finitialize/fadvance calls use the CVode
        integrator. 
        In the default variable step context, the integrator 
        chooses the time step and fadvance returns after one step. Local accuracy 
        is determined by :hoc:meth:`CVode.atol` and :hoc:meth:`CVode.rtol`.
         
        The two major energy barriers to 
        using the method are the requirement that hh type models be 
        expressed in a DERIVATIVE block (instead of the explicit 
        exponential integration step commonly implemented in a PROCEDURE) 
        and that the solver be explicitly notified of 
        the exact time of any discontinuity 
        such as step changes, pulses, and synaptic conductance 
        onset's. These issues are discussed in :ref:`hoc_Channels <hoc_ModelDescriptionIssues_Channels>`
        and :ref:`hoc_Events <hoc_ModelDescriptionIssues_Events>`.
         
        After your mod files are generalized it will probably be 
        convenient to compare the default method with CVode by 
        toggling the Use variable dt checkbox in the :ref:`hoc_VariableStepControl`
        panel
        :menuselection:`NEURON Main Menu --> Tools --> VariableStepControl`.
         
         

    .. seealso::
        :hoc:func:`fadvance`, :hoc:func:`finitialize`, :hoc:data:`secondorder`, :hoc:data:`dt`

    .. warning::
        The consequences of solving continuous equations can be sometimes 
        surprising when one is used to discrete fixed time step simulations. 
        For example if one records an action potential (with either fixed or 
        variable time steps) and plays it back into a voltage clamp; the clamp 
        potential is not a discrete function but an exact step function. 
         
        Only the SEClamp works with CVode. VClamp cannot be used with this method. 
         
        Also .mod authors must take measures to handle step changes in parameters 
        (:ref:`hoc_Events <hoc_ModelDescriptionIssues_Events>`)
         

    .. warning::
        Alternative variable step methods such as :hoc:meth:`CVode.use_local_dt`
        and :hoc:meth:`CVode.use_daspk` have been folded into this class and it has become
        a catchall class for invoking any of the numerical methods. For example, 
        :hoc:meth:`CVode.use_mxb` is used to switch between the tree structured matrix solver
        and the general sparse matrix solver. Not all components work together, see 
        :hoc:meth:`CVode.current_method` for acceptable mixing.

         

----



.. hoc:method:: CVode.solve


    Syntax:
        ``cvode.solve()``

        ``cvode.solve(tout)``


    Description:
        With no argument integrates for one step. All states and assigned variables 
        are consistent at time t. dt is set to the size of the step. 
        With the tout argument, cvode integrates til its step passes tout. Internally 
        cvode returns the interpolated values of the states (at exactly tout) 
        and the CVode class calls the functions necessary to update the assigned variables. 
        Note that cvode.solve(tout) may be called for any value of tout greater than 
        t-dt where dt is the size of its last single step. 
         
        For backward compatibility with finitialize/fadvance 
        it is better to use the :hoc:meth:`CVode.active` method instead of calling
        solve directly. 
         


----



.. hoc:method:: CVode.statistics


    Syntax:
        ``cvode.statistics()``


    Description:
        Prints information about the number of integration steps, function evaluations, 
        newton iterations, etc. 

    .. seealso::
        :hoc:meth:`CVode.spike_stat`

         

----



.. hoc:method:: CVode.spike_stat


    Syntax:
        ``cvode.spike_stat(vector)``


    Description:
        Similar to :hoc:meth:`CVode.statistics` but returns statistics information in the
        passed :hoc:class:`Vector` argument. The vector will be resized to length
        11 and the elements are: 

        .. code-block::
            none

              0  total number of equations (0 unless cvode has been active). 
              1  number of NetCon objects. 
              2  total number of events delivered. 
              3  number of NetCon events delivered. 
              4  number of PreSyn events put onto queue. 
              5  number of SelfEvents delivered. 
              6  number of SelfEvents put onto queue (net_send from mod files). 
              7  number of SelfEvents moved (net_move from mod files). 
              8  number of items inserted into event queue. 
              9  number of items moved to a new time in the event queue. 
             10  number of items removed from event queue. 


         

----



.. hoc:method:: CVode.print_event_queue


    Syntax:
        ``cvode.print_event_queue()``

        ``cvode.print_event_queue(Vector)``


    Description:
        With no arg, prints information on the event queue. 
        It should only be called after an finitialize and before changing any 
        aspect of the model structure. Many types of structure changes invalidate 
        pointers used in the event queue. 
         
        With a vector argument, the delivery times are copied to the Vector in 
        proper monotonically increasing order. 


----



.. hoc:method:: CVode.event_queue_info


    Syntax:
        ``cvode.event_queue_info(2, tvec, list)``

        ``cvode.event_queue_info(3, tvec, flagvec, list)``


    Description:
        Returns NetCon (2) or SelfEvent (3) information currently on the event queue. 
        If the type is 2,  NetCon information currently on the event queue 
        is returned: delivery times are returned in tvec and the corresponding 
        NetCon objects are returned in the List arg. If the type is 3, 
        SelfEvent information is returned: delivery times are returned in tvec, 
        the flags are returned in flagvec, and the SelfEvent targets 
        (ArtificialCells are PointProcesses) returned in the List arg. 
         
        It should only be called after an finitialize and before changing any 
        aspect of the model structure. Many types of structure changes invalidate 
        pointers used in the event queue. 
         
        The delivery times are copied to the Vector in 
        proper monotonically increasing order. 


----



.. hoc:method:: CVode.free_event_queues


    Syntax:
        ``cvode.free_event_queues()``


    Description:
        This function takes cares of clearing and free all the event queues allocated in NEURON.
        More specifically, it frees the `TQItemPool`, `SelfEventPool` and `SelfQueue` members of
        the `NetCvodeThreadData`.
        This method should be called only after the end of the NEURON simulation since calling it
        will clear all the Event Queues and it should only be used for freeing up memory.

----



.. hoc:method:: CVode.poolshrink


    Syntax:
        ``cvode.poolshrink()``

        ``cvode.poolshrink(1)``


    Description:
        This function is used to either print or free the `DoubleArrayPool` s and `DatumArrayPool` s
        used by the mechanisms' data.
        If the function is called with argument `1` it deletes the pools if the number of items used
        is 0.
        If the function is called without arguments or with argument `0` it prints current number of
        items used and number of items allocated for double arrays and Datum arrays.
        This method should be called only after the end of the NEURON simulation for freeing up
        memory.

----



.. hoc:method:: CVode.rtol


    Syntax:
        ``x = cvode.rtol()``

        ``x = cvode.rtol(relative)``


    Description:
        Returns the local relative error tolerance. With arg, set the relative 
        tolerance. The default relative tolerance is 0. 
         
        The solver attempts to use a step size so that the local error for each 
        state is less than 

        .. code-block::
            none

            	rtol*|state| + atol*atolscale_for_state 

        The error test passes if the error in each state, e[i], is such that 
        e[i]/state[i] < rtol OR e[i] < atol*atolscale_for_state 
        (the default atolscale_for_state is 1, see :hoc:meth:`CVode.atolscale` )
         

----



.. hoc:method:: CVode.atol


    Syntax:
        ``x = cvode.atol()``

        ``x = cvode.atol(absolute)``


    Description:
        Returns the default local absolute error tolerance. With args, set the 
        default absolute tolerance. 
        The default absolute tolerance is 1e-2. A multiplier for 
        specific states may be set with the :hoc:meth:`CVode.atolscale` function and also may be
        specified in model descriptions. 
         
        The solver attempts to use a step size so that the local error for each 
        state is less than 

        .. code-block::
            none

            	rtol*|state| + atol*atolscale_for_state 

        The error test passes if the error in each state, e[i], is such that 
        e[i]/state[i] < rtol OR e[i] < atol*atolscale_for_state 
         
        Therefore states should be scaled (or the absolute tolerance reduced) 
        so that when the value is close to 0, the error is not too large. 
         
        (See :hoc:func:`atolscale` for how to set distinct absolute multiplier
        tolerances for different states.) 
         
        Either rtol or atol may be set to 0 but not both. (pure absolute tolerance 
        or pure relative tolerance respectively). 

         

----



.. hoc:method:: CVode.atolscale


    Syntax:
        ``tol = cvode.atolscale(&var, toleranceMultiplier)``

        ``tol = cvode.atolscale(&var)``

        ``tol = cvode.atolscale("basename" [, toleranceMultiplier])``


    Description:
        Specifies the absolute tolerance scale multiplier (default is 1.0) 
        for all STATE's of which the address 
        of var is an instance. Eg.  \ ``cvode.atolscale(&soma.v(.5), 1e-8)`` sets 
        the absolute tolerance multiplier for all membrane potentials everywhere. 
        (The syntax for merely specifying a name is admittedly cumbersome but 
        the function is not often needed and it avoids the necessity of 
        explicitly having to parse strings such as "TrigKSyn.G".) 
        The currently specified multiplier for that state name 
        is returned by the function call. 
         
        Specification of a particular STATEs absolute tolerance multiplier 
        is only needed 
        if its scale is extremely small or large and is best indicated within the 
        model description file itself using the STATE declaration syntax:n 

        .. code-block::
            none

            	state (units) <tolerance> 

        See nrn/demo/release/cabpump.mod for an example of a model which needs 
        a specific scaling of absolute tolerances (ie, calcium concentration 
        and pump density). 
         
        The "basename" form is simpler than the pointer form and was added to 
        simplify the implementation of the AtolTool. The pointer form required 
        the state to actually exist at the specified location. Base names are 
        "v", "vext", state_suffix such as m_hh, and PointProcessName.state such 
        as ExpSyn.g . 

         

----



.. hoc:method:: CVode.re_init


    Syntax:
        ``cvode.re_init()``


    Description:
        Initializes the integrator. This is done by :hoc:func:`finitialize` when cvode
        is :hoc:meth:`~CVode.active`.

         

----



.. hoc:method:: CVode.stiff


    Syntax:
        ``x = cvode.stiff()``

        ``x = cvode.stiff(0-2)``


    Description:
        2 is the default. All states computed implicitly. 
         
        1 only membrane potential computed implicitly. 
         
        0 Adams-Bashforth integration. 

         

----



.. hoc:method:: CVode.active


    Syntax:
        ``x = cvode.active()``

        ``x = cvode.active(0)``

        ``x = cvode.active(1)``

        ``following two not yet implemented``

        ``x = cvode.active(1, dt)``

        ``x = cvode.active(tvec)``


    Description:
        When CVode is active then :hoc:func:`finitialize`
        calls :hoc:meth:`CVode.re_init` and  :hoc:func:`fadvance` calls :hoc:meth:`CVode.solve`.
         
        This function allows one to toggle between the normal integration 
        method and the CVode method with no changes to existing interpreter 
        code. The return value is whether CVode is active. 
         
        With only a single 1 arg, the fadvance calls CVode to do a single 
        variable time step. 
         
        With the dt arg, fadvance returns at t+dt. 
         
        With a Vector tvec argument, CVode is made active and a sequence of 
        calls to fadvance returns at the times given by the elements of 
        tvec. After the last tvec element, fadvance returns after each 
        step. 

         

----



.. hoc:method:: CVode.maxorder


    Syntax:
        ``x = cvode.maxorder()``

        ``x = cvode.maxorder(0 - 12)``


    Description:
        Default maximum order for implicit methods is 5. It is usually best to 
        let cvode determine the order. 12 for Adams. 

         

----



.. hoc:method:: CVode.jacobian


    Syntax:
        ``x = cvode.jacobian()``

        ``x = cvode.jacobian(0 - 2)``


    Description:
        0 is the default. Linear solvers supplied by NEURON. 
        1 use dense matrix 
        2 use diagonal matrix 

         

----



.. hoc:method:: CVode.states


    Syntax:
        ``objref dest_vector``

        ``dest_vector = new Vector()``

        ``cvode.states(dest_vector)``


    Description:
        Fill the destination :hoc:class:`Vector` with the values of the states.
        On return dest_vector.size will be the number of states. 

         

----



.. hoc:method:: CVode.dstates


    Syntax:
        ``cvode.dstates(dest_vector)``


    Description:
        Fill the destination :hoc:class:`Vector` with the values of d(state)/dt.

         

----



.. hoc:method:: CVode.f


    Syntax:
        ``cvode.f(t, yvec, ypvec)``


    Description:
        returns f(yvec, t) in ypvec. f is the existing model. 
        Size of yvec must be equal to the number of states ( ie vector size 
        returned by :hoc:meth:`CVode.states`). ypvec will be resized to the proper size.
        Note that the order of the states in the vector is indicated by the 
        names returned by :hoc:meth:`CVode.statename`

    .. warning::
        Works only for global variable time step method. 
        Works only with single thread. 

         

----



.. hoc:method:: CVode.yscatter


    Syntax:
        ``cvode.yscatter(yvec)``


    Description:
        Fills the state variables with the values specified in yvec. 
        Size of yvec must be equal to the number of states ( ie vector size 
        returned by :hoc:meth:`CVode.states`). Note that active CVode requires a subsequent
        :hoc:meth:`CVode.re_init` if one wishes to integrate from the yvec state point.

    .. warning::
        Works only for global variable time step method. 
        Works only with single thread. 

         

----



.. hoc:method:: CVode.ygather


    Syntax:
        ``cvode.ygather(yvec)``


    Description:
        Fills yvec with the state variables (will be resized to the number of 
        states). This is analogous to :hoc:meth:`CVode.states` after a :hoc:meth:`CVode.re_init`.

    .. warning::
        Works only for global variable time step method. 
        Works only with single thread. 

         

----



.. hoc:method:: CVode.fixed_step


    Syntax:
        ``cvode.fixed_step()``


    Description:
        Uses the fixed step method to advance the simulation by :hoc:data:`dt` .
        The initial condition is whatever state values are present (eg subsequent 
        to a previous integration step or :hoc:meth:`CVode.yscatter` or :hoc:meth:`CVode.f` or explicitly
        user modified state values). The model state values are those after the 
        fixed step integration (but are NOT the same as the current state defined 
        by CVode and returned by :hoc:meth:`CVode.states` (that would be the case only after
        a subsequent :hoc:meth:`CVode.re_init`)) To get the new current states in CVode
        vector order, use :hoc:meth:`CVode.ygather`.
         
        Valid under all circumstances. This is basically an :hoc:func:`fadvance` using
        the fixed step method and avoids the overhead of 

        .. code-block::
            none

            cvode.active(0) 
            fadvance() 
            cvode.active(1) 

        in order to allow the use of the CVode functions assigning state and 
        evaluating states and dstates/dt 

    .. warning::
        :hoc:meth:`CVode.dstates` are invalid and should be determined by a call to
        :hoc:meth:`CVode.f` using the current state from :hoc:meth:`CVode.ygather` .

         

----



.. hoc:method:: CVode.error_weights


    Syntax:
        ``cvode.error_weights(dest_vector)``


    Description:

        Fill the destination :hoc:class:`Vector` with the values of the weights used
        to compute the norm of the local error in cvodes and ida.

----



.. hoc:method:: CVode.acor


    Syntax:
        ``cvode.acor(dest_vector)``

    Description:
        Fill the destination :hoc:class:`Vector` with the values of the local errors
        on the last step.
         

----



.. hoc:method:: CVode.statename


    Syntax:
        ``cvode.statename(i, dest_string)``

        ``cvode.statename(i, dest_string, style)``


    Description:
        Return the hoc name of the i'th string in dest_string 
        The default style, 0, is to attempt to specify the name in terms of 
        object references such as cell[3].syn[2].g. Style 1 specifies the name 
        in terms of the object id, eg. ExpSyn[25].g or Cell[25].soma.v(.5). 
        Style 2 returns the basename, e.g. v, or ExpSyn.g . 

         
         

----



.. hoc:method:: CVode.netconlist


    Syntax:
        ``List = cvode.netconlist(precell, postcell, target)``

        ``List = cvode.netconlist(precell, postcell, target, list)``


    Description:
        Returns a new :hoc:class:`List` (or appends to the list in the 4th argument
        position and returns a reference to that) of :hoc:class:`NetCon` object
        references whose precell (or pre), postcell, and target match the pattern 
        specified in the first three arguments. These arguments may each be either 
        an object reference or a string. If an object, then each NetCon 
        appended to the list will match that object exactly. String arguments 
        are regular expressions 
        and the NetCon will match if the name of the object has a substring that 
        is accepted by the regular expression. 
        (Object names are the 
        internal names consisting of the template name followed by an index). 
        An empty string, "", is equivalent to ".*" and 
        matches everything in that field. A template 
        name will match all the objects of that particular class. Note that 
        some of the useful special regular expression characters are ".*+^$<>". 
        The "<>" is used instead of the the standard special characters "[]" to specify 
        a character range and obviates escaping the square bracket characters 
        when attempting to match an array string. ie square brackets are not 
        special and only match themselves. 

    Example:
        A compact method of iterating over a set of NetCon objects is 
        to create the list iterator 

        .. code-block::
            none

            iterator ltr() {local i, cnt  localobj nil 
            	for i = 0, $o2.count - 1 { 
            		$o1 = $o2.object(i) 
            		iterator_statement 
            	} 
            	$o1 = nil 
            } 

        and then take advantage of the automatic creation and destruction 
        of lists with, for example, to print all the postcells that the 
        given precell connects to: 

        .. code-block::
            none

            objref xo 
            for ltr(xo, cvode.netconlist(precell, "", "")) { 
            	print xo.postcell 
            } 


         

----



.. hoc:method:: CVode.record


    Syntax:
        ``cvode.record(&rangevar, yvec, tvec)``

        ``cvode.record(&rangevar, yvec, tvec, 1)``


    Description:
        Similar to the Vector :hoc:meth:`~Vector.record` function but also works correctly with
        the local variable time step method. Limited to recording only range variables 
        of density mechanisms and point processes. 
         
        During a run, record the stream of values in the specified range 
        variable into the yvec Vector along with time values into the tvec Vector. 
        Note that each recorded range variable must have a separate tvec which 
        will be different for different cells. On initialization 
        the yvec and tvec Vectors are resized to 1 and the initial value of the 
        range variable and time is stored in the Vectors. 
         
        To stop recording into a particular vector, remove all the references 
        either to tvec or yvec or call :hoc:func:`record_remove` .
         
        If the fourth argument is present and equal to 1, the yvec is recorded 
        only at the existing t values in tvec. This option may slow integration 
        since it requires calculation of states at those particular times. 

         

----



.. hoc:method:: CVode.record_remove


    Syntax:
        ``cvode.record_remove(yvec)``


    Description:
        Remove yvec (and the corresponding xvec) 
        from the list of recorded vectors. See :hoc:func:`record`.

         

----



.. hoc:method:: CVode.event


    Syntax:
        ``cvode.event(t)``

        ``cvode.event(t, "statement")``

        ``cvode.event(t, "statement", pointprocess, re_init)``


    Description:
        With no argument, an event without a source or target 
        is inserted into the event queue 
        for "delivery" at time t. This has the side effect of causing a return 
        from :hoc:func:`fadvance` (or :hoc:meth:`CVode.solve` or :hoc:meth:`ParallelContext.psolve` or :hoc:func:`batch_run`
        exactly at time t. This is used by the stdrun.hoc file 
        to make sure a simulation stops at tstop or after the appropriate 
        time on pressing "continuerun" or "continuefor". When :hoc:meth:`CVode.use_local_dt`
        is active, all cells are interpolated to the event time. 
         
        If the hoc statement argument is present, the statement is executed (in 
        the object context of the call to cvode.event) when 
        the event time arrives. 
        This statement is normally a call to a procedure 
        which may send another cvode.event. Note that since the event queue 
        is cleared upon :hoc:func:`finitialize` the cvode.event must be sent after that.
         
        Multiple threads and/or the local variable time step method, sometimes require 
        a bit of extra thought about the purpose of the statement. Should it be executed 
        only in the context of a single thread, should it be executed only in the 
        context of a single cell, and should only the integrator associated with that 
        cell be initialized due to a state change caused by the statement? 
        When the third arg is absent, then before the statement is executed, all cells 
        of all threads are interpolated to time t, all threads 
        join at time t, and the statement is executed by the main thread. A call to 
        :hoc:meth:`CVode.re_init` is allowed. If the third arg (a POINT_PROCESS object) is
        present, then, the integrator of the cell  (if lvardt) containing the POINT_PROCESS 
        is interpolated to time t, and the statement is executed by the thread 
        containing the POINT_PROCESS. Meanwhile, the other threads keep executing. 
        The statement should only access states and parameters associated with the 
        cell containing the POINT_PROCESS. If any states or parameters are changed, 
        then the fourth arg should be set to 1 to cause a re-initialization of only 
        the integrator managing the cell (CVode.re_init is nonsense in this context). 

         

----



.. hoc:method:: CVode.minstep


    Syntax:
        ``hmin = cvode.minstep()``

        ``hmin = cvode.minstep(hmin)``


    Description:
        Gets (and sets in the arg form) the minimum time step allowed for 
        a CVODE step. Default is 0.0 . An error message is printed if a time step less 
        than the minimum step is used. 

    .. warning::
        Not very useful. What we'd really like is a minimum first order implicit step. 

         

----



.. hoc:method:: CVode.maxstep


    Syntax:
        ``hmax = cvode.maxstep()``

        ``hmax = cvode.maxstep(hmax)``


    Description:
        Gets (and sets in the arg form) the maximum value of the step size 
        allowed for a CVODE step. CVODE will not choose a step size larger than this. 
        The default value is 0 and in this case means infinity. 

         

----



.. hoc:method:: CVode.use_local_dt


    Syntax:
        ``boolean = cvode.use_local_dt()``

        ``boolean = cvode.use_local_dt(boolean)``


    Description:
        Gets (and sets) the local variable time step method flag. 
        When CVODE is :hoc:meth:`~CVode.active`, this implies a separate CVODE
        instance for every cell in the simulation. :hoc:meth:`CVode.record` is the only way
        at present that variables can be properly obtained when this method is used. 

    .. warning::
        Not well integrated with the existing standard run system graphics 
        because cells are 
        generally at different times and an fadvance only changes the variables 
        for the earliest time cell. 
         
        :hoc:meth:`CVode.use_daspk` and use_local_dt cannot both be 1 at present. Toggling one
        on will toggle the other off. 

         

----



.. hoc:method:: CVode.debug_event


    Syntax:
        ``cvode.debug_event(1)``

        ``cvode.debug_event(2)``


    Description:
        Prints information whenever an event is generated or delivered. When the 
        argument is true, information is printed at every integration step as well. 

         

----



.. hoc:method:: CVode.use_long_double


    Syntax:
        ``boolean = cvode.use_long_double()``

        ``booelan = cvode.use_long_double(boolean)``


    Description:
        When true, vector methods involving sums over the elements are accumulated 
        in a long double variable. This is useful in debugging when the 
        global variable time step method gives different results for different 
        :hoc:meth:`ParallelContext.nthread` or numbers of processes. It may be the case that the difference is
        due to differences in round-off error due to the non-associativity of 
        computer addition. I.e when threads are used each thread adds up its own 
        group of numbers and then the group results are added together. When 
        a long double is used as the accumulator for addition, the round off error 
        is much more likely to be the same regardless of the order of addition. Note that 
        this DOES NOT make the simulation more accurate --- just more likely to be identical for 
        different numbers of threads or processes (if the difference without it was due to 
        round off errors during summation). 

         

----



.. hoc:method:: CVode.order


    Syntax:
        ``order = cvode.order()``

        ``order = cvode.order(i)``


    Description:
        CVODE method order used on the last step. The arg form is for the ith 
        cell instance with the local step method. 

         

----



.. hoc:method:: CVode.use_daspk


    Syntax:
        ``boolean = cvode.use_daspk()``

        ``boolean = cvode.use_daspk(boolean)``


    Description:
        Gets (sets for the arg form) the internal flag with regard to whether to 
        use the IDA method when CVode is :hoc:meth:`~CVode.active`. If CVode is active
        and the simulation involves :hoc:func:`LinearMechanism` or :hoc:func:`extracellular` mechanisms
        then the IDA method is automatic and required. 
         
        Daspk refers to the Differential Algebraic Solver with the Preconditioned 
        Krylov method. The SUNDIALS package now calls this the IDA (Integrator 
        for Differential-Algebraic problems) integrator but it is really the same 
        thing. 

         

----



.. hoc:method:: CVode.condition_order


    Syntax:
        ``order = cvode.condition_order()``

        ``order = cvode.condition_order(1or2)``


    Description:
        When condition_order is 1 then :hoc:func:`NetCon` threshold detection takes place at a time
        step boundary. This is the default. When condition_order is 2 then 
        NetCon threshold detection times  are linearly interpolated within the 
        integration step interval for which the threshold occurred. Second order 
        threshold is limited to variable step methods and is ignored for the 
        fixed step methods. Note that second order threshold detection time may change 
        due to synaptic events within the interval or even be abandoned. 
        It is useful for cells with approach threshold very slowly or with large 
        time steps. 

         

----



.. hoc:method:: CVode.dae_init_dteps


    Syntax:
        ``eps = cvode.dae_init_dteps()``

        ``eps = cvode.dae_init_dteps(eps)``

        ``eps = cvode.dae_init_dteps(eps, style)``


    Description:
        The size of the "infinitesimal" fixed fully implicit step used for 
        initialization of the DAE solver, see :hoc:func:`use_daspk` , in order to
        meet the the initial condition requirement of f(y',y,t)=0. The default 
        is 1e-9 ms. 
         
        The default heuristic for meeting the initial condition requirement based 
        on the pre-initialization value of all the states and an initialization time 
        of t0 is: 
         
        t = t0 Vector.play continuous. 
         
        Two dteps voltage solve steps. (does not change t, or membrane mechanism 
        states but changes v,vext). 
        The initial value of  y is the present value of the 
        states. 
         
        t = t0 + dteps Vector.play continuous 
         
        One dteps step without changing y but it does determine dy/dt of the 
        v, vext portion of states. 
         
        t = t0 determine the dy/dt of the membrane mechanism states. 
        (note: membrane mechanism states are all derivative or kinetic 
        scheme states) 
         

    .. warning::
        A number of things can go wrong with the heuristics used to provide 
        the integrator with a consistent initial condition. When this happens 
        the default behavior is to stop. However one can modify the error 
        handling and/or choose a second 
        initialization heuristic that might work by setting the style method. 
         
        The working values of style are 0,1,2, 8,9,10. the latter style group 
        (010 bit set) chooses the alternative heuristic. This alternative 
        is very similar to the default except the third dteps step that determines 
        y' also is allowed to change y. This may be more reliable when the user 
        is not using Vector.play continuous. 
         
        If the 1 or 2 bit is 
        set, a warning is printed instead of an error and the sim continues. 
        If the 2 bit is set, then for the next 1e-6 ms, the integrator solves the 
        equation f(y', y, t)*(1 - exp(-1e-7(t - t0)) where t0 is the initialization 
        time. I call this parasitic since it is supposed to be 
        analogous to every voltage having a small capacitance to ground. 
        It has not been determined if the parasitic 
        heuristic has a reliable mathematical basis and the user should investigate 
        the state change patterns in the neighborhood of the initialization time. 
         

         

----



.. hoc:method:: CVode.simgraph_remove


    Syntax:
        ``cvode.simgraph_remove()``


    Description:
        Removes all items from the list of Graph lines recorded during 
        a local variable step simulation. Graph lines would have been added to this 
        list with :ref:`hoc_gui_graph`.

         

----



.. hoc:method:: CVode.state_magnitudes


    Syntax:
        ``cvode.state_magnitudes(integer)``

        ``cvode.state_magnitudes(Vector, integer)``

        ``maxstate = cvode.state_magnitudes("basename", &maxacor)``


    Description:
         
        cvode.state_magnitudes(1) activates the calculation of the 
        running maximum magnitudes of states and acor. 0 turns it off. 
         
        cvode.state_magnitudes(2) creates an internal 
        list of the maximum of the maximum states and acors 
        according to the state basename currently in the model. Statenames not 
        in use have a maximum magnitude state and acor value of -1e9. 
         
        maxstate = cvode.state_magnitudes("basename", &maxacor) 
        returns the maxstate and maxacor for the state type, e.g. "v" or 
        "ExpSyn.g", or "m_hh". Note: state type names can be determined from 
        MechanismType and MechanismStandard 
         
        cvode.state_magnitudes(Vector, 0) returns all the maximum magnitudes for 
        each state in the Vector. This is analogous to cvode.states(Vector). 
        cvode.state_magnitudes(Vector, 1) returns the maximum magnitudes for 
        each acor in the Vector. 
         

         

----



.. hoc:method:: CVode.current_method


    Syntax:
        ``method = cvode.current_method()``


    Description:
        A value that indicates 
         
        modeltype + 10*use_sparse13 + 100*methodtype + 1000*localtype 
         
        where modeltype has the value: 
        0 if there are no sections or LinearMechanisms (i.e. empty model) 
        2 if the extracellular mechanism or LinearMechanism is present. (in this 
        case the fully implicit fixed step or daspk methods are required and cvode 
        cannot be used. 
        1 otherwise 
         
        use_sparse13 is 0 if the tree structured matrix solver is used and 1 
        if the general sparse matrix solver is used. The latter is required for 
        daspk and not allowed for cvode. The fixed step methods can use either. 
        The latter takes about twice as much time as the former. 
         
        methodtype = :hoc:data:`secondorder` if CVode is not active. It equals 3 if CVODE is
        being used and 4 is DASPK is used. 
         
        localtype = 1 if the local step method is used. This implies methodtype==3 

         

----



.. hoc:method:: CVode.use_mxb


    Syntax:
        ``boolean = cvode.use_mxb()``

        ``boolean = cvode.use_mxb(boolean)``


    Description:
        Switch between the tree structured matrix solver (0) and the general 
        sparse matrix solver (1). Either is acceptable for fixed step methods. 
        For CVODE only the tree structured solver is allowed. For DASPK only the 
        general sparse solver is allowed. 

         

----


.. hoc:method:: CVode.use_fast_imem


    Syntax:
        ``boolean = cvode.use_fast_imem()``

        ``boolean = cvode.use_fast_imem(boolean)``


    Description:
        When true, compute i_membrane\_ for all segments during a simulation.
        This is closely related to i_membrane which is computed when the
        extracellular mechanism is inserted. However, i_membrane\_ (note
        the trailing '\_'), has dimensions of nA instead of mA/cm2 (ie. total
        membrane current out of the segment), is available
        at 0 area nodes (locations 0 and 1 of every section), does not require
        that extracellular be inserted (and so is much faster), and works
        during parallel simuations with variable step methods. (ie. does not
        require IDA which is currently not available in parallel).
        i_membrane\_ exists as a range variable only when this function has
        been called with an argument of 1.

         

----



.. hoc:method:: CVode.store_events


    Syntax:
        ``cvode.store_events(vec)``


    Description:
        Accumulates all the sent events as adjacent pairs in the vector. 
        The pairs are the time at which the event was sent and the time it 
        is to be delivered. The user should do a vec.resize(0) before starting 
        a run. Cvode will stop storing with cvode.store_event(). 
        This is primarily for gathering data to design more efficient priority 
        queues. It may be eliminated when the tq-exper branch is merged back to 
        the main branch. Notice that there is no info about event type or where the 
        event is coming from or going to. 

         

----



.. hoc:method:: CVode.queue_mode


    Syntax:
        ``mode = cvode.queue_mode(boolean use_fixed_step_bin_queue, boolean use_self_queue)``


    Description:
        Normally, there is one event queue for all pending events. However, for the 
        fixed step method one can obtain marginally better queue performance through 
        the use of a bin queue for NetCon events. This utilizes a queue with 
        bins of size dt which has a very fast insertion time and every time step 
        all the events in a bin are delivered to their targets. Note that the 
        numerics of the simulation will differ compared to the default splay 
        tree queue (which stores double precision delivery times) if 
        NetCon.delay values are not integer multiples of dt. Also, even with 
        the fixed step method and and delays as integer multiples of dt, results 
        can differ at the double precision round off level due to the different order 
        that same time events can be received by the NET_RECEIVE block. 
         
        The optional "use_self_queue" (default 0) argument can only be used if the 
        the simulation is run with :hoc:meth:`~ParallelContext.psolve` method
        of the :hoc:class:`ParallelContext` and must be selected prior to a call of
        :hoc:meth:`ParallelContext.set_maxstep`  since this special technique requires a
        computation of the global minimum :hoc:meth:`NetCon.delay` (not just the
        minimum interprocessor NetCon delay) and that delay must be 
        greater than 0. The technique avoids the use of the  normal splay tree queue 
        for self events for ARTIFICIAL_CELLs (events initiated by the net_send call 
        and which may be manipulated by the net_move call in the NET_RECEIVE block). 
        It may thus be considerably faster. However, every minimum NetCon delay interval, 
        all the ARTIFICIAL_CELLS must be iterated to see if there are any outstanding 
        net_send events that need to be handled. Thus it is likely to have a beneficial 
        performance impact only for large numbers of ARTIFICIAL_CELLs which receive 
        many external input events per reasonable minimum delay interval. This method 
        has not receive much testing and the results should be compared with the 
        default queuing method. 
         
        Returns ``2*use_self_queue + use_fixed_step_bin_queue``. 

    .. seealso::
        :hoc:meth:`ParallelContext.spike_compress`

         

----



.. hoc:method:: CVode.cache_efficient


    Syntax:
        ``mode = cache_efficient(0or1)``


    Description:
        Deprecated method.
        This used to cause the G*v = R matrix and vectors to be reallocated in
        tree order so that all the elements of each type are contiguous in
        memory.
        This is no longer required because this scheme is now used all the time
        and cannot be disabled.
        Pointers to these elements used by the GUI, Vector, Pointer, etc. are updated.


----



ModelDescriptionIssues
======================

        The following aspects of model descriptions (.mod files) 
        are relevant to their use with CVode. 
         
        KINETIC block - No changes required. 
         
        DERIVATIVE block - No changes required. 
        The Jacobian is approximated as a diagonal matrix. 
        If the states are linear in state' = f(state) the diagonal elements 
        are calculated analytically, otherwise the 
        diagonal elements are calculated using the numerical 
        derivative (f(s+.01) - f(s))/.001 . 
         
        LINEAR, NONLINEAR blocks - No changes required. 
        However, at this 
        time they can only be SOLVED from a PROCEDURE or FUNCTION, not 
        from the BREAKPOINT block. The nrn/src/nrnoc/vclmp.mod file 
        gives an example of correct usage in which the function 
        icur is called from the BREAKPOINT block and in turn SOLVE's 
        a LINEAR block. If desired, it will be a simple matter to 
        allow these blocks to be solved from the BREAKPOINT block. 
         
        SOLVE PROCEDURE within a BREAKPOINT block - Changes probably required. 
        Such a procedure is called once after each return from 
        CVode.solve(). 
         



.. _hoc_ModelDescriptionIssues_Channels:

Channels
~~~~~~~~

The SOLVE PROCEDURE form was often used to implement 
the exponential integration method for HH like states and was 
very efficient in the context of the Crank-Nicolson like
staggered time step approach historically used by NEURON. 
Furthermore the exponential integration often used tables 
of rates which were calculated under the assumption of 
a fixed time step, dt. Although it can still be used under some 
circumstances, the usage to integrate states 
should be considered obsolete and converted to 
a DERIVATIVE form. To do this, 

1)  replace the PROCEDURE block with a DERIVATIVE block, eg. 

    .. code-block::
        none
        
        DERIVATIVE states { 
        m' = (minf - m)/mtau 
        ... 
        } 
2)  replace the SOLVE statement in the BREAKPOINT block with 
    ``SOLVE states METHOD cnexp``
3)  if using tables, store mtau instead of :math:`(1 -\exp(-dt/m_{tau}))`
    The nmodl translator will emit c code for both the staggered 
    time step and high order variable time step methods. The only 
    downside is slightly less efficiency with the staggered time 
    step method since the exp(-dt...) is calculated instead of 
    looked up in tables. 
 
In summary, no model should anymore depend on :hoc:data:`dt`.
         


Concentrations
~~~~~~~~~~~~~~

         





.. _hoc_ModelDescriptionIssues_Events:

Events
~~~~~~

 
How does one handle events?  This is really the only serious 
difficulty in writing models that work properly in the 
context of a variable time step method. All models which involve 
discontinuous functions of time, eg steps, pulses, synaptic 
onset, require special provision to notify the integrator that 
an event has occurred within this time step, ie between t-dt and t. 
If this is not done, the time step may be so large that it 
completely misses a pulse or synaptic event. And if it does see 
the effect of the event, there is a huge inefficiency involved in the 
variable step method's search for the location of the event and the 
concomitant tremendous reduction in size of dt. 
 
So, if you change any variable discontinuously in the model 
at some time tevent, call 
call 

.. code-block::
    none

            at_time(tevent) 

The user may check the return value of this function to decide 
if something needs changing. Examples of the two styles of usage are: 
 
1) Just notify and do the logic separately. 

    .. code-block::
        none

        	at_time(del) 
        	at_time(del + dur) 
        	if (t >= del && t <= del + dur) { 
        		istim = on_value 
        	}else{ 
        		istim = 0 
        	} 

 
2) Use the at_time return value to do the logic. 

    .. code-block::
        none

        INITIAL { 
        	istim = 0 
        } 
        ... 
        	if (at_time(del)) { 
        		istim = on_value 
        	} 
        	if (at_time(del + dur)) { 
        		istim = 0 
        	} 

Notice the requirement of initialization or else if the previous 
run was stopped before del + dur the value of istim would be on_value 
at the beginning of the next run. 
 
What happens internally when at_time(tevent) is called? 
 
The interesting case (t-dt < tevent <= t) --- 
First, at_time returns 0. Then 
CVode changes its step size to (tevent - (t-dt) - epsilon) and redoes 
the step starting at t-dt. Note that this should be safely prior 
to the event (so at_time still returns 0), 
but if not then the above process will repeat 
until a step size is found for which there is no event. 
CVode then re-initializes it's internal state and 
restarts from a new initial condition at tevent+epsilon. 
Now when at_time is called, it returns 1. 
Note that in its single step mode, CVode.solve() will return 
at t = tevent-epsilon, the subsequent call will start the 
initial condition at t = tevent + epsilon and return after a normal 
step (usually quite small). 
 
The case (tevent <= t - dt) --- at_time returns 0. 
 
The case (tevent > t) --- at_time returns 0. 
 
Note that 
an action potential model with 
axonal delay delivering a "message" to a synaptic model may or 
may not think it worthwhile to call at_time at the time of threshold 
(I would just do my own interpolation to set t_threshold) 
but will certainly call at_time(t_threshold + delay)  (and possibly not 
allow t_threshold to change again until it returns a 1); 
 
I am sorry that the variable time step method requires that the 
model author take careful account of events but I see no way 
to have them automatically taken care of. 
 

 

