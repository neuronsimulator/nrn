Programmatic Simulation Control
===============================

See also:

.. toctree:: :maxdepth: 1

    cvode.rst
    batch.rst
    savstate.rst
    bbsavestate.rst
    sessionsave.rst

Functions
---------

.. function:: initnrn


    Syntax:
        ``h.initnrn()``


    Description:
        Initialize ``t, dt, clamp_resist``, and ``celsius`` to the values 
        they had when the program was first run. 
         
        Note that in this 
        version ``Ra`` is no longer a global variable but a section variable 
        like *L* and *rallbranch*. Thus ``Ra`` can be different for different 
        sections.  In order to set ``Ra`` to a constant value, use: 
         
        ``for sec in h.allsec(): sec.Ra=...`` 

    .. warning::
        Not very useful. No way to completely restart neuron except to :func:`quit` and 
        re-load. 


----



.. function:: fadvance


    Syntax:
        ``h.fadvance()``


    Description:
        Integrate all section equations over the interval :data:`h.dt` . 
        The value of :data:`t` is incremented by dt. 
        The default method is first order implicit but may be changed to 
        Crank-Nicolson by changing :data:`h.secondorder` = 2.
         
        fadvance integrates the equation over the dt step by 
        calling all the BREAKPOINT blocks of models at t+dt/2 twice with 
        v+.001 and v in order to compute the current and conductance to form 
        the matrix conductance*voltage = current. 
        This matrix is then solved for v(t+dt). 
        (if secondorder == 2 the ionic currents are adjusted to be second order 
        correct. If secondorder == 1 the ionic currents are not adjusted but 
        the voltages are second order correct) 
        Lastly the SOLVE statement within the BREAKPOINT block of models is 
        executed with t+dt and the new values of v in order to integrate those 
        states (from new t-.5dt to new t+.5dt). 

         

----



.. function:: finitialize


    Syntax:
        ``h.finitialize()``

        ``h.finitialize(v)``


    Description:
        Call the INITIAL block for all mechanisms and point processes 
        inserted in the sections. 
        If the optional argument is present then all voltages of all sections 
        are initialized to *v*. 
        :data:`h.t` is set to 0. 
         
        The order of principal actions during an finitialize call is:
        
        -   Type 3 FInitializeHandler statements executed. 
        -   Make sure internal structures needed by integration methods are consistent 
             with the current biophysical spec. 
        -   t = 0 
        -   Clear the event queue. 
        -   Random.play values assigned to variables. 
        -   Vector.play at t=0 values assigned to variables. 
        -   All v = arg if the arg is present. 
        -   Type 0 FInitializeHandler statements executed. 
        -   All mechanism BEFORE INITIAL blocks are called. 
        -   All mechanism INITIAL blocks called. 
               Mechanisms that WRITE concentrations are after ion mechanisms and 
               before mechanisms that READ concentrations. 
        -   LinearMechanism states are initialized 
        -   INITIAL blocks inside NETRECEIVE blocks are called. 
        -   All mechanism AFTER INITIAL blocks are called. 
        -   Type 1 FInitializeHandler statements executed. 
        -   The INITIAL block net_send(0, flag) events are delivered. 
        -   Effectively a call to CVode.re_init or fcurrent(), whichever appropriate. 
        -   Various record functions at t=0. e.g. CVode.record, Vector.record  
        -   Type 2 FInitializeHandler statements executed. 
             


    .. seealso::
        :class:`FInitializeHandler`, :ref:`runcontrol_Init`, :meth:`CVode.re_init`, :func:`fcurrent`, :func:`frecord_init`

         

----



.. function:: frecord_init


    Syntax:
        ``h.frecord_init()``


    Description:
        Initializes the Vectors which are recording variables. i.e. resize to 0 and 
        append the current values of the variables.  This is done at the end 
        of an :func:`finitialize` call but needs to be done again to complete initialization 
        if the user changes states or assigned variables that are being recorded.. 

    .. seealso::
        :meth:`Vector.record`, :ref:`runcontrol_Init`

----





.. function:: fcurrent


    Syntax:
        ``h.fcurrent()``


    Description:
        Make all assigned variables (currents, conductances, etc) 
        consistent with the values of the states. Useful in combination 
        with :func:`finitialize`. 

    Example:

        .. code-block::
            python        

            from neuron import h

            soma = h.Section(name="soma")
            soma.insert(h.hh)
            print(f"default el_hh = {soma.el_hh}")

            # set el_hh so that the steady state is exactly -70 mV 
            h.finitialize(-70) # sets v to -70 and m,h,n to corresponding steady state values 
             
            h.fcurrent()       # set all assigned variables consistent with states 
             
            # use current balance: 0 = ina + ik + gl_hh*(v - el_hh)		 
            soma.el_hh = (soma.ina + soma.ik + soma.gl_hh * soma.v) / soma.gl_hh 
             
            print(f"-70 mV steady state el_hh = {soma.el_hh}")
            h.fcurrent()       # recalculate currents (il_hh) 


         

----



.. function:: fmatrix


    Syntax:
        ``h.fmatrix()``

        ``value = h.fmatrix(x, index, sec=section)``


    Description:
        No args: print the jacobian matrix for the tree structure in a particularly 
        confusing way. for debugging only. 
         
        With args, return the matrix element associated with the integer index 
        in the row corresponding to ``section(x)``.
        The index 1...4 is associated with: 
        The coefficient for the effect of this locations voltage on current balance at the parent location, 
        The coefficient for the effect of this locations voltage on current balance at this location, 
        The coefficient for the effect of the parent locations voltage on current balance at this location, 
        The right hand side of the matrix equation for this location. These are the 
        values of NODEA, NODED NODEB, and NODERHS respectively in 
        nrn/src/nrnoc/section.h . The matrix elements are properly setup on return 
        from a call to the :func:`fcurrent` function. For the fixed step method 
        :func:`fadvance` modifies NODED and NODERHS 
        but leaves NODEA and NODEB unchanged. 

----

.. data:: secondorder


    Syntax:
        ``h.secondorder``


    Description:
        This is a global variable which specifies the time integration method. 


        =0 
            default fully implicit backward euler. Very numerically stable. 
            gives steady state in one step when *dt=1e10*. Numerical errors 
            are proportional to :data:`dt`. 

        =1 
            crank-nicholson Can give large (but damped) numerical error 
            oscillations. For small :data:`dt` the numerical errors are proportional 
            to ``dt^2``. Cannot be used with voltage clamps. Ionic currents 
            are first order correct. Channel conductances are second order 
            correct when plotted at ``t+dt/2`` 

        =2 
            crank-nicholson like 1 but in addition Ion currents (*ina*, *ik*, 
            etc) are fixed up so that they are second order correct when 
            plotted at ``t-dt/2`` 


         

----



.. data:: t


    Syntax:
        ``h.t``


    Description:
        The global time variable. 

         

----



.. data:: dt


    Syntax:
        ``h.dt``


    Description:
        The integration interval for :func:`fadvance`. 
         
        When using the default implicit integration method (:data:`secondorder` = 0) 
        there is no upper limit on dt for numerical stability and in fact for 
        passive models it is often convenient to use dt=1.9 to obtain the 
        steady state in a single time step. 
         
        dt can be changed by the user at any time during a simulation. However, 
        some inserted mechanisms may use tables which depend on the value of dt 
        which will be automatically recomputed. In this situation, the tables 
        are not useful and should be bypassed by setting the appropriate 
        usetable_suffix global variables to 0. 

         

----



.. data:: clamp_resist


    Syntax:
        ``h.clamp_resist``


    Description:
        Obsolete, used by fclamp. 

         

----



.. data:: celsius


    Syntax:
        ``h.celsius = 6.3``


    Description:
        Temperature in degrees centigrade. 
         
        Generally, rate function tables (eg. used by the hh mechanism) 
        depend on temperature and will automatically be re-computed 
        whenever celsius changes. 

         

----



.. data:: stoprun


    Syntax:
        ``h.stoprun``


    Description:
        A flag which is watched by :func:`fit_praxis`, :class:`CVode`, and other procedures 
        during a run or family of runs. 
        When stoprun==1 they will immediately return without completing 
        normally. This allows safe stopping in the middle of a long run. Procedures 
        that do multiple runs should check stoprun after each run and exit 
        gracefully. The :meth:`RunControl.Stop` of the RunControl GUI sets this variable. 
        It is cleared at the beginning of a run or when continuing a run. 


----

.. function:: checkpoint

    Syntax:
        :samp:`h.checkpoint("{filename}")`

    Description:
        saves the current state of the system in a portable file to 
        allow one to take up where you left off -- possibly on another 
        machine. Returning to this state is accomplished by running the 
        program with the checkpoint file as the first argument. 
        If the checkpoint file is inconsistent with the executable the 
        program prints an error message and exits. 
         
        At this time many portions of the computer state are left out of the 
        checkpoint file, i.e. it is not as complete as a core dump. 
        Some things that ARE included are: 
        all interpreter symbols with definitions and values, 
        all hoc instructions, 
        all neuron state/parameters with mechanisms. 
        Many aspects of the GUI are not included. 
         
    .. warning::
        There is not enough implementation at this time to make this 
        facility useful. Use the :class:`SaveState` class instead.



         
----

.. _finithnd:

         
FInitializeHandler
------------------



.. class:: FInitializeHandler


    Syntax:
        ``fih = h.FInitializeHandler(py_callable)``

        ``fih = h.FInitializeHandler(type, py_callable)``


    Description:
        Install an initialization handler statement to be called during a call to 
        :func:`finitialize`. The default type is 1.
         
        Type 0 handlers are called before the mechanism INITIAL blocks. 
         
        Type 1 handlers are called after the mechanism INITIAL blocks. 
        This is the best place to change state values. 
         
        Type 2 handlers are called just before return from finitialize. 
        This is the best place to record values at t=0. 
         
        Type 3 handlers are called at the beginning of finitialize. 
        At this point it is allowed to change the structure of the model. 
         
        See :func:`finitialize` for more details about the order of initialization processes 
        within that function. 
         
        This class helps alleviate the administrative problems of maintaining variations 
        of the proc :ref:`RunControl_Init`. 

    Example:

        .. code-block::
            python

            # specify an example model 
            from neuron import h, gui

            a = h.Section(name="a")
            b = h.Section(name="b")

            for sec in h.allsec():
                sec.insert(h.hh)

            def fi0():
                print('fi0 called after v set but before INITIAL blocks')
                print(f'  a.v={a.v} a.m_hh={a.m_hh}')
                a.v = 10

            def fi1():
                print('fi1() called after INITIAL blocks but before BREAKPOINT blocks')
                print('     or variable step initialization.')
                print('     Good place to change any states.')
                print(f'  a.v={a.v} a.m_hh={a.m_hh}')
                print(f'  b.v={b.v} b.m_hh={b.m_hh}')
                b.v = 10 

            def fi2():
                print('fi2() called after everything initialized. Just before return')
                print('     from finitialize.')
                print('     Good place to record or plot initial values')
                print(f'  a.v={a.v} a.m_hh={a.m_hh}')
                print(f'  b.v={b.v} b.m_hh={b.m_hh}')

            fih = [h.FInitializeHandler(0, fi0),
                   h.FInitializeHandler(1, fi1),
                   h.FInitializeHandler(2, fi2)]

            class Test:
                def __init__(self):
                    self.fih = h.FInitializeHandler(self.p)
                def p(self):
                    print('inside %r.p()' % self)

            test = Test() 

            h.stdinit() 
            fih[0].allprint() 


         

----



.. method:: FInitializeHandler.allprint


    Syntax:
        ``fih.allprint()``


    Description:
        Prints all the FInitializeHandler statements along with their object context 
        in the order they will be executed during an :func:`finitialize` call. 



