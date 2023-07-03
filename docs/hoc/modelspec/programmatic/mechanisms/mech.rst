
.. _hoc_mech:

         
Point Processes and Artificial Cells
------------------------------------


Description:
    Built-in POINT_PROCESS models and ARTIFICIAL_CELL models are listed on the left. 
    The user may add other classes of those types using mod files. Some properties 
    and functions that are available for all POINT_PROCESS models are described 
    under :ref:`hoc_pointprocesses_general`.

.. seealso::
    :ref:`hoc_pointprocessmanager`



.. _hoc_pointprocesses_general:

General
~~~~~~~

.. hoc:method:: pnt.get_loc


    Syntax:
        ``{ x = pnt.get_loc() stmt pop_section()}``


    Description:
        get_loc() pushes the section containing the POINT_PROCESS instance, pnt, 
        onto the section stack (makes it the currently accessed section), and 
        returns the position (ranging from 0 to 1) of the POINT_PROCESS instance. 
        The section stack should be popped when the section is no longer needed. 
        Note that the braces are necessary if the statement is typed at the top 
        level since the section stack is automatically popped when waiting for 
        user input. 

    .. seealso::
        :hoc:func:`pop_section`,
        :hoc:meth:`get_segment`

         

----



.. hoc:method:: pnt.get_segment


    Syntax:
        ``pyseg = pnt.get_segment()``


    Description:
        A more pythonic version of :hoc:func:`get_loc` in that it returns a python segment object
        without pushing the section stack. From a segment object one can get the 
        section with ``pyseg.sec`` and the position with ``pyseg.x``. If the 
        point process is not located anywhere, the return value is None. 

    .. warning::
        Segment objects become invalid if nseg changes. Discard them as soon as 
        possible and do not keep them around. 

         

----



.. hoc:method:: pnt.loc


    Syntax:
        ``pnt.loc(x)``


    Description:
        Moves the POINT_PROCESS instance, pnt, to the center of the segment containing 
        x of the currently accessed section. 

         

----



.. hoc:method:: pnt.has_loc


    Syntax:
        ``b = pnt.has_loc()``


    Description:
        Returns 1 if the POINT_PROCESS instance, pnt, is located in some section, 
        otherwise, 0. 

         

----



.. hoc:class:: IClamp


    Syntax:
        ``stimobj = new IClamp(x)``

        ``del -- ms``

        ``dur -- ms``

        ``amp -- nA``

        ``i -- nA``


    Description:
        See `<nrn src dir>/src/nrnoc/stim.mod <https://github.com/neuronsimulator/nrn/blob/master/src/nrnoc/stim.mod>`_
         
        Single pulse current clamp point process. This is an electrode current 
        so positive amp depolarizes the cell. i is set to amp when t is within 
        the closed interval del to del+dur. Time varying current stimuli can 
        be simulated by setting del=0, dur=1e9 and playing a vector into amp 
        with the :hoc:meth:`~Vector.play` :hoc:class:`Vector` method.


----



.. hoc:class:: AlphaSynapse


    Syntax:
        ``syn = new AlphaSynapse(x)``

        ``syn.onset --- ms``

        ``syn.tau --- ms``

        ``syn.gmax --- umho``

        ``syn.e	--- mV``

        ``syn.i	--- nA``


    Description:
        See `<nrn src dir>/src/nrnoc/syn.mod <https://github.com/neuronsimulator/nrn/blob/master/src/nrnoc/syn.mod>`_. The comment in this file reads: 

        .. code-block::
            none

            synaptic current with alpha function conductance defined by 
                    i = g * (v - e)      i(nanoamps), g(micromhos); 
                    where 
                     g = 0 for t < onset and 
                     g = gmax * (t - onset)/tau * exp(-(t - onset - tau)/tau) 
                      for t > onset 
            this has the property that the maximum value is gmax and occurs at 
             t = delay + tau. 



----



.. hoc:class:: VClamp


    Syntax:
        ``obj = new VClamp(x)``

        ``dur[3]``

        ``amp[3]``

        ``gain, rstim, tau1, tau2``

        ``i``


    Description:
        Two electrode voltage clamp. 
         
        See `<nrn src dir>/src/nrnoc/vclmp.mod <https://github.com/neuronsimulator/nrn/blob/master/src/nrnoc/vclmp.mod>`_. The comment in this file reads: 
         
        Voltage clamp with three levels. Clamp is on at time 0, and off at time 
        dur[0]+dur[1]+dur[2]. When clamp is off the injected current is 0. 
        Do not insert several instances of this model at the same location in 
        order to 
        make level changes. That is equivalent to independent clamps and they will 
        have incompatible internal state values. 
         
        The control amplifier has the indicated gain and time constant.  The 
        input amplifier is ideal. 

        .. code-block::
            none

             
                             tau2 
                             gain 
                            +-|\____rstim____>to cell 
            -amp --'\/`-------|/ 
                            | 
                            |----||--- 
                            |___    __|-----/|___from cell 
                                `'`'        \| 
                                tau1 
             

         
        The clamp has a three states which are the voltage input of the gain amplifier, 
        the voltage output of the gain amplfier, and the voltage output of the 
        measuring amplifier. 
        A good initial condition for these voltages are 0, 0, and v respectively. 
         
        This model is quite stiff.  For this reason the current is updated 
        within the solve block before updating the state of the clamp. This 
        gives the correct value of the current on exit from :hoc:func:`fadvance`. If we
        didn't do this and 
        instead used the values computed in the breakpoint block, it 
        would look like the clamp current is much larger than it actually is 
        since it 
        doesn't take into account the change in voltage within the timestep, ie 
        equivalent to an almost infinite capacitance. 
        Also, because of stiffness, do not use this model except with :hoc:data:`secondorder`\ =0.
         
        This model makes use of implementation details of how models are interfaced 
        to neuron. At some point I will make the translation such that these kinds 
        of models can be handled straightforwardly. 
         
        Note that since this is an electrode current model v refers to the 
        internal potential which is equivalent to the membrane potential v when 
        there is no extracellular membrane mechanism present but is v+vext when 
        one is present. 
        Also since i is an electrode current, 
        positive values of i depolarize the cell. (Normally, positive membrane currents 
        are outward and thus hyperpolarize the cell) 


----



.. hoc:class:: SEClamp


    Syntax:
        ``clampobj = new SEClamp(0.5)``

        ``dur1 dur2 dur3 -- ms``

        ``amp1 amp2 amp3 -- mV``

        ``rs -- MOhm``


        ``vc -- mV``

        ``i -- nA``


    Description:
        Single electrode voltage clamp with three levels. 
         
        See `<nrn src dir>/src/nrnoc/svclmp.mod <https://github.com/neuronsimulator/nrn/blob/master/src/nrnoc/svclmp.mod>`_. The comment in this file reads: 
         
        Single electrode Voltage clamp with three levels. 
        Clamp is on at time 0, and off at time 
        dur1+dur2+dur3. When clamp is off the injected current is 0. 
        The clamp levels are amp1, amp2, amp3. 
        i is the injected current, vc measures the control voltage) 
        Do not insert several instances of this model at the same location in 
        order to 
        make level changes. That is equivalent to independent clamps and they will 
        have incompatible internal state values. 
        The electrical circuit for the clamp is exceedingly simple: 

        .. image:: ../../../images/svclmp.png
            :align: center

        Note that since this is an electrode current model v refers to the 
        internal potential which is equivalent to the membrane potential v when 
        there is no extracellular membrane mechanism present but is v+vext when 
        one is present. 
        Also since i is an electrode current, 
        positive values of i depolarize the cell. (Normally, positive membrane currents 
        are outward and thus hyperpolarize the cell) 
         
        This model is careful to ensure the clamp current is properly computed 
        relative to the membrane voltage on exit from fadvance and can therefore 
        be used with time varying control potentials. Like :hoc:class:`VClamp` it is suitable
        for :hoc:meth:`~Vector.play`\ ing a Vector into the control potential.
         
        The following example compares the current that results from 
        clamping an action potential originally elicited by a current pulse.
 

        .. code-block::
            none

            // setup for three simulations 
            create s1, s2, s3 // will be stimulated by IClamp, SEClamp, and VClamp 
            forall {insert hh diam=3 L=3 } 
            objref c1, c2, c3, ap, apc 
            s1 c1 = new IClamp(0.5) 
            s2 c2 = new SEClamp(0.5) 
            s3 c3 = new VClamp(0.5) 
            {c1.dur=.1 c1.amp=0.3} 
            {c2.dur1 = 1 c2.rs=0.01 } 
            {c3.dur[0] = 1} 
             
            // record an action potential 
            ap = new Vector() 
            ap.record(&s1.v(0.5)) 
            finitialize(-65)    
            while(t<1) { fadvance() } 
             
            // do the three cases while playing the recorded ap 
            apc = ap.c	// unfortunately can't play into two variables so clone it. 
            ap.play_remove()   
            ap.play(&c2.amp1, dt) 
            apc.play(&c3.amp[0], dt) 
            finitialize(-65) 
            while(t<0.4) { 
                    fadvance() 
                    print s1.v, s2.v, s3.v, c1.i, c2.i, c3.i 
            } 



----



.. hoc:class:: APCount


    Syntax:
        ``apc = new APCount(x)``

        ``apc.thresh ---	mV``

        ``apc.n``

        ``apc.time --- ms``

        ``apc.record(vector)``


    Description:
        Counts the number of times the voltage at its location crosses a 
        threshold voltage in the positive direction. n contains the count 
        and time contains the time of last crossing. 
         
        If a Vector is attached to the apc, then it is resized to 0 when the 
        INITIAL block is called and the times of threshold crossing are 
        appended to the Vector. apc.record() will stop recording into the vector. 
        The apc is not notified if the vector is freed but this can be fixed if 
        it is convenient to add this feature. 
         
        See `<nrn src dir>/src/nrnoc/apcount.mod <https://github.com/neuronsimulator/nrn/blob/master/src/nrnoc/apcount.mod>`_


----



.. hoc:class:: ExpSyn


    Syntax:
        ``syn = new ExpSyn(x)``

        ``syn.tau --- ms decay time constant``

        ``syn.e -- mV reversal potential``

        ``syn.i -- nA synaptic current``


    Description:
        Synapse with discontinuous change in conductance at an event followed 
        by an exponential decay with time constant tau. 

        .. code-block::
            none

            i = G * (v - e)      i(nanoamps), g(micromhos); 
              G = weight * exp(-t/tau) 

         
        The weight is specified 
        by the :hoc:data:`~NetCon.weight` field of a :hoc:class:`NetCon` object.
         
        This synapse summates. 
         
        See `<nrn src dir>/src/nrnoc/expsyn.mod <https://github.com/neuronsimulator/nrn/blob/master/src/nrnoc/expsyn.mod>`_


----



.. hoc:class:: Exp2Syn


    Syntax:
        ``syn = new Exp2Syn(x)``

        ``syn.tau1 --- ms rise time``

        ``syn.tau2 --- ms decay time``

        ``syn.e -- mV reversal potential``

        ``syn.i -- nA synaptic current``


    Description:
        Two state kinetic scheme synapse described by rise time tau1, 
        and decay time constant tau2. The normalized peak condductance is 1. 
        Decay time MUST be greater than rise time. 
         
        The kinetic scheme 

        .. code-block::
            none

            A    ->   G   ->   bath 
               1/tau1   1/tau2 

        produces 
        a synaptic current with alpha function like conductance (if tau1/tau2 
        is appoximately 1) 
        defined by 

        .. code-block::
            none

            i = G * (v - e)      i(nanoamps), g(micromhos); 
              G = weight * factor * (exp(-t/tau2) - exp(-t/tau1)) 

        The weight is specified 
        by the :hoc:data:`~NetCon.weight` field of a :hoc:class:`NetCon` object.
        The factor is defined so that the normalized peak is 1. 
        If tau2 is close to tau1 
        this has the property that the maximum value is weight and occurs at 
        t = tau1. 
         
        Because the solution is a sum of exponentials, the 
        coupled equations for the kinetic scheme 
        can be solved as a pair of independent equations 
        by the more efficient cnexp method. 
         
        This synapse summates. 
         
        See `<nrn src dir>/src/nrnoc/exp2syn.mod <https://github.com/neuronsimulator/nrn/blob/master/src/nrnoc/exp2syn.mod>`_
         


----



.. hoc:class:: NetStim


    Syntax:
        ``s = new NetStim(x)``

        ``s.interval ms (mean) time between spikes``

        ``s.number (average) number of spikes``

        ``s.start ms (most likely) start time of first spike``

        ``s.noise ---- range 0 to 1. Fractional randomness.``

        ``0 deterministic, 1 intervals have negexp distribution.``


    Description:
        Generates a train of presynaptic stimuli. Can serve as the source for 
        a NetCon. This NetStim can also be 
        be triggered by an input event. i.e serve as the target of a NetCon. 
        If the stimulator is in the on=0 state and receives a positive weight 
        event, then the stimulator changes to the on=1 state and goes through 
        its burst sequence before changing to the on=0 state. During 
        that time it ignores any positive weight events. If, in the on=1 state, 
        the stimulator receives a negative weight event, the stimulator will 
        change to the off state. In the off state, it will ignore negative weight 
        events. A change to the on state immediately causes the first spike. 
         
        Fractional noise, 0 <= noise <= 1, means that an interval between spikes 
        consists of a fixed interval of duration (1 - noise)*interval plus a negexp 
        interval of mean duration noise*interval. Note that the most likely negexp 
        interval has duration 0. 
         
        Since NetStim sends events, the proper idiom for specifying it as a source 
        for a NetCon is 

        .. code-block::
            none

            objref ns, nc 
            nc = new NetStim(0.5) 
            ns = new NetCon(nc, target...) 

        That is, do not use ``&nc.y`` as the source for the netcon. 
         
        See `<nrn src dir>/src/nrnoc/netstim.mod <https://github.com/neuronsimulator/nrn/blob/master/src/nrnoc/netstim.mod>`_

    .. warning::
        Prior to version 5.2.1 an attempt was made to 
        make the mean start time (noise > 0) 
        correspond to the value of start. However since it is not possible to 
        simulate events occurring at t < 0, these spikes were generated at t=0. 
        Thus the mean start time was not start and the spikes at t=0 did not 
        obey negexp statistics. For this reason, beginning with version 5.2.1 
        the semantics of start are the time of the most likely first spike and the 
        mean start time is start + noise*interval. 

         

----



.. hoc:class:: IntFire1


    Syntax:
        ``c = new IntFire1(x)``

        ``c.tau --- ms time constant``

        ``c.refrac --- ms refractory period. Minimum time between events is refrac``

        ``c.m --- state variable``

        ``c.M --- analytic value of state at current time, t``


    Description:
        A point process that is equivalent to an entire integrate and fire cell. 
         
        An output 
        spike event is sent to all the NetCon instances which have this pointprocess 
        instance as their source when m >= 1 
        If m(t0) = m0 and an input event occurs at t1 
        then the value of m an infinitesimal time before the t1 event is 
        exp(-(t1 - t0)/tau). After the input event m(t1) = m(t1) + weight where weight 
        is the weight of the NetCon event. 
        Input events are ignored for refrac time after the spike output 
        event. 
         
        During the refractory period,  m = 2. 
        At the end of the refractory period, m = 0. 
        During the refractory period, the function M() returns a value of 2 
        for the first 0.5 ms and -1 for the rest of the period. Otherwise it 
        returns exp((t-t0)/tau) 
         
        See `<nrn src dir>/src/nrnoc/intfire1.mod <https://github.com/neuronsimulator/nrn/blob/master/src/nrnoc/intfire1.mod>`_


----



.. hoc:class:: IntFire2


    Syntax:
        ``c = new IntFire2(x)``

        ``c.taum --- ms membrane time constant``

        ``c.taus -- ms synaptic current time constant``

        ``c.ib -- constant current input``

        ``c.m --- membrane state variable``

        ``c.M --- analytic value of state at current time, t``

        ``c.i --- synaptic current state variable``

        ``c.I --- analytic value of synaptic current.``


    Description:
        A leaky integrator with time constant taum driven by a total 
        current that is the sum of 
        { a user-settable constant "bias" current } 
        plus 
        { a net synaptic current }. 
        Net synaptic current decays toward 0 with time constant taus, where 
        taus > taum (synaptic 
        current decays slowly compared to the rate at which "membrane potential" 
        m equilibrates). 
        When an input event with weight w arrives, the net synaptic current 
        changes abruptly by 
        the amount w. 
        
        See `<nrn src dir>/src/nrnoc/intfire2.mod <https://github.com/neuronsimulator/nrn/blob/master/src/nrnoc/intfire2.mod>`_         

         

----



.. hoc:class:: IntFire4


    Syntax:
        ``c = new IntFire4(x)``

        ``c.taue --- ms excitatory input time constant``

        ``c.taui1 --- ms inhibitory input rise time constant``

        ``c.taui2 --- ms inhibitory input fall time constant``

        ``c.taum --- membrane time constant``

        ``c.m --- membrane state variable``

        ``c.M --- analytic value of membrane state at current time, t``

        ``c.e --- excitatory current state variable``

        ``c.E --- analytic value of excitation current``

        ``c.i1 c.i2 -- inhibitory current state variables``

        ``c.I --- analytic value of inhibitory current.``



    Description:
        The IntFire4 artificial cell treats excitatory input (positive weight) 
        events as a sudden change in 
        current which decays exponentially with time constant taue. Inhibitory 
        input (negative weight) 
        events are treated as an alpha function like change to the current. More 
        precisely the current due 
        to a negative weight event is the difference between two exponentials 
        with time constants taui1 
        and taui2. In the limit as taui2 approaches taui1 then the current due 
        to the event approaches the 
        alpha function. The current due to the input events is integrated with a 
        membrane time constant 
        of taum. At present there is a constraint taue < taui1 < taui2 < taum 
        but this may become 
        relaxed to taue, taui1 < taui2, taum. When the membrane potential 
        reaches 1, the cell fires and 
        the membrane potential is re-initialized to 0 and starts integrating 
        according to the analytic 
        value of the current (which does NOT depend on firing). Excitatory 
        events are scaled such that 
        an isolated event of weight 1 will produce a maximum membrane potential 
        of 1 (threshold) and 
        an isolated inhibitory event of weight -1 will produce a minimum 
        membrane potential of -1. 
         
        See `<nrn src dir>/src/nrnoc/intfire4.mod <https://github.com/neuronsimulator/nrn/blob/master/src/nrnoc/intfire4.mod>`_         

----


.. _hoc_mech_mechanisms:

Mechanisms
----------

.. seealso::
    :ref:`insert <hoc_keyword_insert>`, :ref:`hoc_Inserter`, :ref:`hoc_nmodl`

         

----


.. index::  setdata (mechanism)


.. _hoc_mech_sethoc:data:

**setdata**

    Syntax:
        ``sec setdata_suffix(x)``


    Description:
        If a mechanism function is called that uses RANGE variables, then the 
        appropriate data needed by the function must first be indicated via a setdata call. 
        This is unnecessary if the function uses only GLOBAL variables. 
        The suffix refers to the name of the mechanism. E.g. setdata_hh(). 

    .. warning::
        The THREADSAFE mechanism case is a bit more complicated if the mechanism 
        anywhere assigns a value to a GLOBAL variable. When the user explicitly 
        specifies that a mechanism is THREADSAFE, those GLOBAL variables that 
        anywhere appear on the left hand side of an assignment statement (and there 
        is no such assignment with the PROTECT prefix) 
        are actually 
        thread specific variables. 
        Hoc access to thread specific global variables is with respect to a static 
        instance which is shared by 
        the first thread in which mechanism actually exists. 

         

----


.. index::  capacitance (mechanism)


.. _hoc_mech_capacitance:

**capacitance**


    Syntax:
        ``cm (uF/cm2)``

        ``i_cap (mA/cm2)``


    Description:
        capacitance is a mechanism that automatically is inserted into every section. 
        cm is a range variable with a default value of 1.0. 
        i_cap is a range variable which contains the varying membrane capacitive current 
        during a simulation. Note that i_cap is most accurate when a variable step 
        integration method is used. 

         

----


.. index::  hh (mechanism)


.. _hoc_mech_hh:

**hh**


    Syntax:
        ``insert hh``


    Description:
        See `<nrn src dir>/src/nrnoc/hh.mod <https://github.com/neuronsimulator/nrn/blob/master/src/nrnoc/hh.mod>`_
         
        Hodgkin-Huxley sodium, potassium, and leakage channels. Range variables 
        specific to this model are: 

        .. code-block::
            none

            gnabar_hh	0.120 mho/cm2	Maximum specific sodium channel conductance 
            gkbar_hh	0.036 mho/cm2	Maximum potassium channel conductance 
            gl_hh	0.0003 mho/cm2	Leakage conductance 
            el_hh	-54.3 mV	Leakage reversal potential 
            m_hh			sodium activation state variable 
            h_hh			sodium inactivation state variable 
            n_hh			potassium activation state variable 
            ina_hh	mA/cm2		sodium current through the hh channels 
            ik_hh	mA/cm2		potassium current through the hh channels 
             
            rates_hh(v) computes the global variables [mhn]inf_hh and [mhn]tau_hh 
            from the rate functions. usetable_hh defaults to 1. 

        This model used the na and k ions to read ena, ek and write ina, ik. 


----


.. index::  pas (mechanism)


.. _hoc_mech_pas:

**pas**

    Syntax:
        ``insert pas``

        ``g_pas -- mho/cm2	conductance``

        ``e_pas -- mV		reversal potential``

        ``i -- mA/cm2		non-specific current``


    Description:
        See `<nrn src dir>/src/nrnoc/passive.mod <https://github.com/neuronsimulator/nrn/blob/master/src/nrnoc/passive.mod>`_
         
        Passive membrane channel. 


----



.. index::  fastpas (mechanism)


.. _hoc_mech_fastpas:

**fastpas**

        See `<nrn src dir>/src/nrnoc/passive0.cpp <https://github.com/neuronsimulator/nrn/blob/master/src/nrnoc/passive0.cpp>`_
         
        Passive membrane channel. Same as the :ref:`pas <hoc_mech_pas>` mechanism but hand coded to
        be a bit faster (avoids the wasteful numerical derivative computation of 
        the conductance and does not save the current). Generally not worth 
        using since passive channel computations are not usually the rate limiting 
        step of a simulation. 
         

----



.. index::  extracellular (mechanism)


.. _hoc_mech_extracellular:

**extracellular**

    Syntax:
        ``insert extracellular``

        ``vext[2] -- mV``

        ``i_membrane -- mA/cm2``

        ``xraxial[2] -- MOhms/cm``

        ``xg[2]	-- mho/cm2``

        ``xc[2]	-- uF/cm2``

        ``e_extracellular -- mV``


    Description:
        Adds two layers of extracellular field to the section. Vext is 
        solved simultaneously with the v. When the extracellular mechanism 
        is present, v refers to the membrane potential and vext (i.e. vext[0]) 
        refers to 
        the extracellular potential just next to the membrane. Thus the 
        internal potential is v+vext (but see BUGS). 
         
        This mechanism is useful for simulating the stimulation with 
        extracellular electrodes, response in the presence of an extracellular 
        potential boundary condition computed by some external program, leaky 
        patch clamps, incomplete seals in the myelin sheath along with current 
        flow in the space between the myelin and the axon. It is required 
        when connecting :hoc:class:`LinearMechanism` (e.g. a circuit built with
        the :menuselection:`NEURON Main Menu --> Build --> Linear Circuit`) to extracellular nodes. 
         
        i_membrane correctly does not include contributions from ELECTRODE_CURRENT 
        point processes. 

        See i_membrane\_ at :hoc:meth:`CVode.use_fast_imem`.
         
        The figure illustrates the form the electrical equivalent circuit 
        when this mechanism is present. Note that previous documentation 
        was incorrect in showing that e_extracellular was in series with 
        the ``xg[nlayer-1],xc[nlayer-1]`` parallel combination. 
        In fact it has always been the case 
        that e_extracellular was in series with xg[nlayer-1] and xc[nlayer-1] 
        was in parallel 
        with that series combination. 
         
        .. note::
        
            The only reason the standard 
            distribution is built with nlayer=2 is so that when only a single 
            layer is needed (the usual case), then e_extracellular is consistent 
            with the previous documentation with the old default nlayer=1. 
         
        e_extracellular is connected in series with the conductance of 
        the last extracellular layer. 
        With two layers the equivalent circuit looks like: 
         

        .. code-block::
            none

             
                      Ra		 
            o/`--o--'\/\/`--o--'\/\/`--o--'\/\/`--o--'\o vext + v 
                 |          |          |          |      
                ---        ---        ---        --- 
               |   |      |   |      |   |      |   | 
                ---        ---        ---        --- 
                 |          |          |          |      
                 |          |          |          |     i_membrane      
                 |  xraxial |          |          | 
             /`--o--'\/\/`--o--'\/\/`--o--'\/\/`--o--'vext 
                 |          |          |          |      
                ---        ---        ---        ---     xc and xg 
               |   |      |   |      |   |      |   |    in  parallel 
                ---        ---        ---        --- 
                 |          |          |          |      
                 |          |          |          |      
                 |xraxial[1]|          |          |      
             /`--o--'\/\/`--o--'\/\/`--o--'\/\/`--o--'vext[1] 
                 |          |          |          |      
                ---        ---        ---        ---     the series xg[1], e_extracellular 
               |   |      |   |      |   |      |   |    combination is in parallel with 
               |  ---     |  ---     |  ---     |  ---   the xc[1] capacitance. This is 
               |   -      |   -      |   -      |   -    identical to a membrane with 
                ---        ---        ---        ---     cm, g_pas, e_pas 
                 |          |          |          |      
            -------------------------------------------- ground 
             

         
        Extracellular potentials do a great deal 
        of violence to one's intuition and it is important that the user 
        carefully consider the results of simulations that use them. 
        It is best to start out believing that there are bugs in the method 
        and attempt to prove their existence. 

        See `<nrn src dir>/src/nrnoc/extcell.cpp <https://github.com/neuronsimulator/nrn/blob/master/src/nrnoc/extcell.cpp>`_
        and `<nrn src dir>/share/examples/nrniv/nrnoc/extcab*.hoc <https://github.com/neuronsimulator/nrn/tree/master/share/examples/nrniv/nrnoc>`_.
         
        NEURON can be compiled with any number of extracellular layers. 
        See below. 

    .. warning::
        xcaxial is also defined but is not implemented. If you need those 
        then add them with the :hoc:class:`LinearMechanism` .
         
        Prior versions of this document indicated that 
        e_extracellular is in series with the parallel (xc,xg) 
        pair. In fact it was in series with xg of the layer. 
        The above equivalent circuit has been changed to reflect the truth 
        about the implementation. 
         
        In v4.3.1 2000/09/06 and before 
        vext(0) and vext(1) are the voltages at the centers of the first and 
        last segments instead of the zero area nodes. 
         
        Now the above bug is fixed and 
        vext(0) and vext(1) are the voltages at the zero area nodes. 
         
        From extcell.c the comment is: 

        .. code-block::
            none

                    i_membrane = sav_g * ndlist[i]->v + sav_rhs; 
            #if 1 
                    /* i_membrane is a current density (mA/cm2). However   
                       it contains contributions from Non-ELECTRODE_CURRENT 
                       point processes. i_membrane(0) and i_membrane(1) will 
                       return the membrane current density at the points 
                       0.5/nseg and 1-0.5/nseg respectively. This can cause 
                       confusion if non-ELECTRODE_CURRENT point processes 
                       are located at these 0-area nodes since 1) not only 
                       is the true current density infinite, but 2) the  
                       correct absolute current is being computed here  
                         at the x=1 point but is not available, and 3) the  
                       correct absolute current at x=0 is not computed 
                       if the parent is a rootnode or there is no 
                       extracellular mechanism for the parent of this 
                       section. Thus, if non-ELECTRODE_CURRENT point processes 
                       eg synapses, are being used it is not a good idea to 
                       insert them at the points x=0 or x=1 
                    */ 
            #else 
                       i_membrane *= ndlist[i]->area; 
                       /* i_membrane is nA for every segment. This is different 
                          from all other continuous mechanism currents and 
                          same as PointProcess currents since it contains 
                          non-ELECTRODE_CURRENT point processes and may 
                          be non-zero for the zero area nodes. 
                       */ 
            #endif 
             

         
         
        In v4.3.1 2000/09/06 and before 
        extracellular layers will not be connected across sections unless 
        the parent section of the connection contains the extracellular 
        mechanism. This is because the 0 area node of the connection is 
        "owned" by the parent section. In particular, root nodes never contain 
        extracellular mechanisms and thus multiple sections connected to the 
        root node always appear to be extracellularly disconnected. 
        This bug has been fixed. However it is still the case that 
        vext(0) can be non-zero only if the section owning the 0 node has had 
        the extracellular mechanism inserted. It is best to have every section 
        in a cell contain the extracellular mechanism if any one of them does 
        to avoid confusion with regard to (the in fact correct) boundary conditions. 
         
         
         

    Syntax:
        ``nrn/src/nrnoc/options.h``

        ``#define EXTRACELLULAR 2 /* number of extracellular layers */``

        ``insert extracellular``

        ``vext[i] -- mV``

        ``i_membrane -- mA/cm2``

        ``xraxial[i] -- MOhms/cm``

        ``xg[i]	-- mho/cm2``

        ``xc[i]	-- uF/cm2``

        ``e_extracellular -- mV``



    Description:
        If other than 2 extracellular layers is desired, you may recompile the 
        program by changing the :file:`nrn/src/nrnoc/options.h` line 
        ``#define EXTRACELLULAR 2``
        to the number of layers desired. Be sure to recompile both nrnoc and nrniv 
        as well as any user defined .mod files that use the ELECTRODE_CURRENT statement. 
         
        Note that vext is a synonym in hoc for vext[0]. Since the default value for 
        xg[i] = 1e9 all layers start out tightly connected to ground so 
        previous single layer extracellular simulations should produce the same 
        results if either xc or e_extracellular was 0. 
         
        e_extracellular is connected in series with the conductance of 
        the last extracellular layer. 


