.. _impedanc:

         
Impedance
---------



.. class:: Impedance

        For calculating input and transfer impedances at an instant of time 
        Usage involves first defining a location either 
        for the current stimulus or else the voltage measuring electrode, then 
        computing the global transfer and input impedance function 
        at a particular	frequency, then retrieving values of the complex 
        transfer and input impedance at particular locations. 
         
        The default calculation (the only calculation prior to version 5.3) 
        is defined by di/dv only. i.e it assumes conductances depend only 
        locally on v and does not take into account the impedance contributions of gating state 
        differential equations. Specifically, the cable equation, 
        c*dv/dt = i(v), 
        where the d2v/dx2 compartmental terms are in i, yields the linearized impedance 
        matrix 
        [(jwc - di/dv)v = i0 ] * exp(jwt) 
        The di/dv terms, apart from the axial terms, 
        are defined by the current calculation in the BREAKPOINT 
        blocks of the membrane mechanisms. 
         
        In version 5.3 the calculation was extended to take into account effects of 
        other gating states. The calculation is currently limited to systems that can 
        be solved with the CVode method but can be extended to systems solvable by 
        the DASPK method. ie. currently one cannot deal with the extracellular mechanism 
        or :class:`LinearMechanism`. It would be easy to implement the LinearMechanism extension 
        and that would be the only way one could handle non-local interactions such 
        as gap junctions. The extension takes into account not only di/dv but also 
        di/ds, ds'/dv and ds'/ds contributions to the impedance where s are all the 
        other states of the membrane mechanisms. i.e the system can be written 

        .. code-block::
            none

            	|c 0| * d/dt |v| = |i(v,s)| 
            	|0 1|        |s|   |f(v,x)| 

        which is formally similar to the original. 
        E.g. the hh mechanism has a sodium 
        channel defined by 

        .. code-block::
            none

            	ina = gnabar*m^3*h*(v - ena) 
            	m' = (minf - m)/mtau 
            	h' = (hinf - h)/htau 

        the only thing contributed (one compartment) by the default method is 

        .. code-block::
            none

            	(jwc + gnabar*m^3*h) * v = i0 

        but the full linearized method contributes a matrix of terms like 

        .. code-block::
            none

            	(jwc + gnabar*m^3*h)   gnabar*3*m^2*h*(v-ena)   gnabar*m^3*(v-ena) 
            	-d((minf - m)/mtau)/dv (jw - 1/mtau) 
            	-d((hinf - h)/htau)/dv                          (jw - 1/htau) 

        associated with the vector of states (v, m, h) 
         
        The extended full impedance calculation is invoked with an extra argument 
        to the :meth:`Impedance.compute` function. One should also review the 
        :meth:`Impedance.deltafac` method which defines the accuracy of the calculation. 
         

----



.. method:: Impedance.loc


    Syntax:
        ``.loc(x, sec=section)``


    Description:
        A fixed current stimulus or voltage electrode location 
        at position 0<=x<=1 of the specified section. 
        This is needed for the transfer impedance calculation. Note that 
        transfer impedances obey the relation 
        \ ``v(x)/i(loc) == v(loc)/i(x)`` where *loc* is the fixed location and 
        x ranges over every position of every section. 

         

----



.. method:: Impedance.compute


    Syntax:
        ``.compute(freq)``

        ``.compute(freq, 1)``
        
        ``.compute(freg, 1, maxiter=500)``


    Description:
        Transfer impedance between location specified above and any other 
        location is computed. Also the input impedance at all locations 
        is computed -- \ ``v(x)/i(x)`` 
        Frequency specified in Hz. 
        All membrane conductances are computed and used in the 
        calculation as if \ :func:`fcurrent()` was called. 
        The compute call is expensive but as a rule of thumb is not 
        as expensive as \ :func:`fadvance()`. 
         
        Since version 5.3, when the second argument is 1, an extended impedance 
        calculation is performed which takes into account the effect of 
        differential gating states. ie. the linearized cy' = f(y) system is used 
        where y is all the membrane potentials plus all the states in KINETIC and 
        DERIVATIVE blocks of membrane mechanisms. Currently, the system must 
        be computable with the Cvode method, i.e.extracellular and 
        LinearMechanism are not allowed. See :meth:`Impedance.deltafac` 
         
        Note that the extended impedance calculation may involve a singular matrix 
        because of the negative resistance contributions of excitable channels. 

        If the extended impedance calculation has been chosen (second arg = 1)
        then parallel gap junction effects will be taken into account.
        But for parallel gap junctions, there are several qualifications:

        One and only one rank can have a stimulus location. :meth:`Impedance.loc`
        can be used with an arg of -1 to remove the stimulus location on
        a rank.

        Every rank must participate in the call to compute (because of the use of
	MPI collective calls to carry out the impedance calculation). Note that only the
        freq arg value on the rank that has a location matters. If not all ranks have the
        second arg value of 1, the machine will hang in an MPI collective call.

        Not more than 5 types of gap junction POINT_PROCESS mechanisms can be instantiated.
        If any POINT_PROCESS instance participates in a gap junction
        (via :meth:`ParallelContext.target_var`) then all instances of that type
        must participate in gap junctions.

        Only :meth:`Impedance.transfer` and :meth:`Impedance.transfer_phase` can be used
        to access the impedance values.
        Ranks do not have to participate in the calls to the those two
        methods since no MPI collective calls are involved. After
        :meth:`Impedance.compute` is called, the transfer impedance is available at any
        cell location and multiple calls from a rank are allowed. Note that if the stimulus
        location is at location x and the transfer impedance is obtained at location x and
        y, the input impedance is known only at location x (equal to the transfer impedance)
        and the voltage ratio is known only at x and y. Note that the voltage ratio at
        x is trivially 1.0, and the voltage at y, given that x is voltage clamped to a 1mV
        sine wave with freq, is transfer(y)/transfer(x) . Unfortunately this is the opposite
        of the definition given for :meth:`Impedance.ratio` which voltage clamped y
        and recorded at x. I regret
        the original convention which was an artifact of
        :meth:`Impedance.compute` with args (freq, 0) calculating at one time, not only all the transfer
        impedances, but also all the input impedances at every location.  The problem with
        the original convention for :meth:`Impedance.ratio`, and also with
        :meth:`Impedance.input`, when the second :meth:`Impedance.compute` arg is 1,
        is that their use necessitates a solve with a moved input stimulus location
        specified by their argument. This is very inconvenient in a parallel context, as
        that solve would require the participation of all the ranks where all the args except
        one would have to be -1.  An error message will be generated if one attempts to use the
        ratio or input methods in the context of parallel gap junctions when nhost > 1.

        Impedance calculations with parallel gap junctions use the
        Jacobi iterative method to solve the linear matrix equation.
        This method converges linearly and the number of iterations
        required is proportional to the gap junction strength. Up to 500 iterations
        are allowed before an error message is generated. Iteration stops when no state
        changes more than 1e-9 after an iteration. It is expected that the number of
        iterations will be quite modest with realistic gap junction conductances (a dozen
        or so). A third argument to .compute specifies the maximum number of iterations
        (default 500).


    .. warning::
         
        There are many limitations to the extended linearization of the 
        complete system. It basically handles only voltage sensitive 
        density channels where the gating states are defined by 
        DERIVATIVE or KINETIC blocks. Prominent limitations are: 
         
        extracellular mechanism not allowed. 
         
        LinearMechanism not allowed. 
         
        Because we are not doing the complete full df/dy calculation, there 
        may be interactions between states that are not computed.
        An example is  where ion concentration 
        equations are voltage sensitive in one mechanism and then the ionic 
        current is concentration sensitive in another mechanism. ie. the 
        typical way NEURON deals with ionic concentration coupling to current 
        is not handled. 
         

         

----



.. method:: Impedance.transfer


    Syntax:
        ``.transfer(x, sec=section)``


    Description:
        absolute amplitude of the transfer impedance between the position 
        specified in the \ ``loc(x)`` call above and 0<=x<=1 of the
        specified section at the freq specified by a previous 
        compute(freq). The value returned can be thought of as either 
        \ ``|v(loc)/i(x)| or |v(x)/i(loc)|`` 
        Probably the more useful way of thinking about it is to assume 
        a current stimulus of 1nA injected at x and the voltage in mV 
        recorded at loc. 
         
        Return value has the units of 
        Megohms and can be thought of as the amplitude of the voltage (mV) 
        at one location	that would result from the injection of 1nA at the 
        other. 

         

----



.. method:: Impedance.input


    Syntax:
        ``.input(x, sec=section)``


    Description:
        absolute amplitude of \ ``v(x)/i(x)`` of the specified section 

         

----



.. method:: Impedance.ratio


    Syntax:
        ``.ratio(x)``


    Description:
        \ ``|v(loc)/v(x)|`` Think of it as voltage clamping to 1mV at x at some 
        frequency and recording the voltage at loc. 

         

----



.. method:: Impedance.transfer_phase


    Syntax:
        ``.transfer_phase(x)``


    Description:
        phase of transfer impedance. The phase is modulo 2Pi in the range 
        -Pi to +Pi so as one moves away from the loc remember that the 
        actual phase can become less than -Pi. If the amplitude is very 
        small the phase may be inaccurate and cannot be computed at all 
        if the amplitude is 0. 

         

----



.. method:: Impedance.input_phase


    Syntax:
        ``.input_phase(x)``


    Description:
        phase of input impedance. 
         
        Note: Impedance makes heavy use of memory since four complex 
        vectors are allocated with size equal to the total number of 
        segments. After compute is called two of these vectors holds 
        the input and transfer impedance for a given loc, freq, and 
        neuron state. Because 
        of the way results of calculations are stored it is very efficient 
        to access amp and phase; reasonably efficient to change freq or loc, 
        and inefficient to vary neuron state, eg, diameters. The last case 
        implies at least the overhead of a call like \ :func:`fcurrent()`.(actually 
        the present implementation calls \ :func:`fcurrent()` on every \ ``compute()`` call 
        but that could be fixed if increased performance was needed). 

         

----



.. method:: Impedance.deltafac


    Syntax:
        ``fac = imp.deltafac()``

        ``fac = imp.deltafac(fac)``


    Description:
        Gets or sets and gets the factor used in computing the numerical derivatives 
        during calculation of the extended full impedance. Jacobian elements are 
        calculated via the formula ``f(s+delta) - f(s))/delta`` where 
        delta is defined by fac * the state tolerance scale factor for cvode. 
        Note that default state tolerance scale factors are 1.0 except when 
        specifically declared in mod files or changed by calling 
        :meth:`CVode.atolscale`. The default delta factor is 0.001 which is consistent 
        with the factor used by the default impedance calculation. Note that the 
        factor for the default impedance calculation cannot be changed. 



