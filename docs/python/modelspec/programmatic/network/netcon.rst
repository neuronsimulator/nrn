.. _netcon:

NetCon
------



.. class:: NetCon


    Syntax:
        ``netcon = h.NetCon(source_ref_v, target, [threshold, delay, weight], sec=section)``

        ``netcon = h.NetCon(source, target, [threshold, delay, weight])``


    Description:
        Network Connection object that defines a synaptic connection between 
        a source and target. When the source variable passes threshold in the 
        positive direction at time t-delay, the target will receive an event 
        at time t along with weight information. There is no limit on delay 
        except that it be >= 0 and there is no limit on the number of events 
        pending delivery. 
         
        If the optional threshold, delay, and weight arguments are not 
        specified, their default values are 10, 1, and 0 respectively. In 
        any case, their values can be specified after the NetCon has been 
        constructed, see :data:`NetCon.threshold`, :data:`NetCon.weight`, and :data:`NetCon.delay` . 
         
        Note that prior to 12-Jul-2006, when the first form of the constructor 
        was used without a threshold, the threshold was 
        reset to the default 10 (mV) even if the threshold for that source location 
        had been explicitly set earlier. That behavior caused confusion and has been 
        changed so that if there is no threshold argument and the threshold location 
        already exists, the previous threshold is retained. 
         
        The target must be a POINT_PROCESS or ARTIFICIAL_CELL that defines a NET_RECEIVE procedure. 
        The number of NET_RECEIVE procedure arguments define a weight vector 
        whose elements can be accessed with through the :data:`NetCon.weight` variable 
        but the weight argument in the above constructors specify the value of 
        the first argument, with the normal interpretation of weight or maximum 
        conductance. On initialization, all weight elements with index > 0 are 
        set to 0 unless the NET_RECEIVE block contains an INITIAL block. In the 
        latter case, that block is executed on a call to :func:`finitialize`  and 
        allows non-zero initialization of NetCon "states" --- args not initialized 
        in the INITIAL block would be analogous to a :ref:`Parameter <nmodl_parameter>` except that it 
        can have a different value for different NetCon instances and can be set 
        to a desired value with :data:`NetCon.weight`. 
         
        The target is allowed to be ``None`` in which case the NetCon 
        is always inactive. However this can be useful for recording (see 
        :meth:`NetCon.record`) the spike train from an output cell. 
         
        The source (``source_ref_v``) is normally a reference to a membrane potential (e.g. ``cell.soma(0.5)._ref_v``) which is 
        watched during simulation for passage past threshold. The 
        section is required by the local variable 
        time step method in order to determine the source "cell". 
        Any range variable may be a source variable but I suspect that membrane 
        potential is the only practical one. 
         
        N.B. When calling with a pointer (the first usage form, above), the
        specified ``section`` must contain the pointer. If it does not, a
        RuntimeError exception will be raised.
         
        The source may also be a PointProcess with a NET_RECEIVE block which 
        contains a call to net_event. PointProcesses like this serve as entire 
        artificial cells. 
         
        The source may also 
        be a PointProcess which contains a "x" variable which is watched for 
        threshold crossing, but this is obsolete since NET_RECEIVE blocks which 
        explicitly call net_event are much more efficient since they avoid 
        the overhead of threshold detection at every time step. 
         
        The source may be ``None``. In this case events can only occur by 
        calling :func:`event` from Python. (It is also used by NEOSIM to implement 
        its own delivery system.) 
         
        A source used by multiple NetCon instances is shared by those instances 
        to allow faster threshold detection (ie on a per source basis instead 
        of a per NetCon basis). Therefore, there is really only one threshold 
        for all NetCon objects that share a source. However, delay and weight 
        are distinct for each NetCon object. 
         
        The only way one can have multiple threshold values at the same location is 
        to create a threshold detector point process with a NET_RECEIVE block implemented 
        with a WATCH statement and calling net_event. 
         
        And I'll say it again: 
        Note that prior to 12-Jul-2006, when the first form of the constructor 
        was used, (i.e. a NetCon having a pointer to a source 
        variable was created, but having no threshold argument) the threshold was 
        reset to the default 10 (mV) even if the threshold for that source location 
        had been explicitly set earlier. That behavior caused confusion and has been 
        changed so that if there is no threshold argument and the threshold location 
        already exists, the previous threshold is retained. 
         
        From a NetCon instance, various lists of NetCon's can be created 
        with the same target, precell, or postcell. See :meth:`CVode.netconlist` 
        for creation of NetCon lists from a target, precell, or  postcell 
        pattern or object. 
         

    .. warning::
        When calling with a pointer (the first usage form, above), the
        specified ``section`` (currently accessed section) must contain the pointer.
        If it does not, a RuntimeError exception will be raised.
         

         

----



.. method:: NetCon.valid


    Syntax:
        ``boolean = netcon.valid()``


    Description:
        Returns 0 if the netcon does not have both a source and a target.

         

----



.. method:: NetCon.active


    Syntax:
        ``1or0 = netcon.active(boolean)``

        ``1or0 = netcon.active()``


    Description:
        Turns the synapse on or off in the sense that when off, no events 
        are delivered using this NetCon instance. Returns the previous 
        state (or current state if no argument) as 1 if True; 0 if False. 
        The argument must be 0, 1, False, or True; other input values raise
        a RuntimeError Exception.

         

----



.. method:: NetCon.event


    Syntax:
        ``netcon.event(tdeliver)``

        ``netcon.event(tdeliver, flag)``


    Description:
        Delivers an event to the postsynaptic point process at time, tdeliver. 
        tdeliver must be >= t . Note that the netcon.delay is not used by this 
        function. Because it is a delivery event as opposed to an initiating 
        event, it will not be recorded in a NetCon.record(Vector). 
         
        A flag value can only be sent to an ARTIFICIAL_CELL. 

         

----



.. method:: NetCon.syn


    Syntax:
        ``target_object = netcon.syn()``


    Description:
        Returns a reference to the synaptic target PointProcess. 

         

----



.. method:: NetCon.pre


    Syntax:
        ``source_object = netcon.pre()``


    Description:
        Returns a reference to the source PointProcess. If the source is a membrane 
        potential then the return value is ``None``. 

         

----



.. method:: NetCon.preloc


    Syntax:
        .. code-block::
            python

            x = netcon.preloc()
            sec = h.cas()
            h.pop_section()


    Description:
        The source section is pushed onto the section stack so that it is 
        the currently accessed section (``h.cas()``). ``h.pop_section()`` must be called after you are 
        finished with the section or have saved it as in the syntax block above.

    .. warning::
        If the source was an object, the section is not pushed and the return 
        value is -1. 
        If the source is not a membrane potential (or an object) the
	return value is -2. But the section was pushed and `h.pop_section()
        needs to be called.
        

    .. warning::

        This function modifies the section stack. It is generally safer to use
        :meth:`NetCon.preseg` instead.
         

----

.. method:: NetCon.preseg


    Syntax:
        .. code-block::
            python

            seg = netcon.preseg()


    Description:

        Returns a segment associated with the source variable.
        If the source is not a membrane potential the return value is None.
         

----


.. method:: NetCon.postloc


    Syntax:
        .. code-block::
            python

            x = netcon.postloc()
            sec = h.cas()
            h.pop_section()

    Description:
        The section of the target point process is pushed onto the section stack 
        so that it is the currently accessed section (``h.cas()``). ``h.pop_section()`` must be called 
        after you are finished with the section or have saved it as in the syntax block above.
        The x return value is the 
        relative location of the point process in that section. If there
        is no target, the return value is -1 and no section is pushed
        onto the section stack.

        In new code, it is recommended to use :meth:`NetCon.postseg` to avoid modifying
        the section stack.

----


.. method:: NetCon.postseg


    Syntax:
        .. code-block::
            python

            seg = netcon.postseg()

    Description:

        Returns the segment containing the target point process (or None
        if there is no target). The section is
        accessible via ``seg.sec`` and the normalized position ``x`` is accessible
        via ``seg.x``.
         

----



.. method:: NetCon.precell


    Syntax:
        ``cellobj = netcon.precell()``


    Description:
        If the source is a membrane potential and the section was created with a ``cell=`` keyword
        argument, then it returns the value of that argument. For sections created inside a HOC
        object (defined in a cell template), a reference to the presynaptic cell 
        (object) is returned. 

         

----



.. method:: NetCon.postcell


    Syntax:
        ``cellobj = netcon.postcell()``


    Description:
        If the synaptic point process is located in a section that was created with a ``cell=`` keyword
        argument, then it returns the value of that argument. For sections created inside a HOC
        object (defined in a cell template), a reference to the postsynaptic cell 
        (object) is returned. 

         

----



.. method:: NetCon.setpost


    Syntax:
        ``netcon.setpost(newtarget)``


    Description:
        Will change the old postsynaptic POINT_PROCESS target to the one specified 
        by the newtarget. If there is no argument 
        or the argument is None then NetCon will have no target and the 
        active flag will be set to 0. Note that a target change will preserve the 
        current weight vector only if the new and old targets have the same 
        weight vector size (number of arguments in the NET_RECEIVE block). 

         

----



.. method:: NetCon.prelist


    Syntax:
        ``List = netcon.prelist()``

        ``List = netcon.prelist(List)``


    Description:
        :class:`List` (i.e. not a Python list) of all the NetCon objects with source the same as ``netcon``. 
        With no argument, a new List is created. 
        If the List arg is present, the objects are appended. 

         

----



.. method:: NetCon.synlist


    Syntax:
        ``List = netcon.synlist()``

        ``List = netcon.synlist(List)``


    Description:
        :class:`List` (i.e. not a Python list) of all the NetCon objects with target the same as ``netcon``. 
        With no argument, a new List is created. 
        If the List arg is present, the objects are appended. 

    .. seealso::
        :meth:`CVode.netconlist`

         

----



.. method:: NetCon.postcelllist


    Syntax:
        ``List = netcon.postcelllist()``

        ``List = netcon.postcelllist(List)``


    Description:
        :class:`List` (i.e. not a Python list) of all the NetCon objects with postsynaptic cell object the same as netcon. 
        With no argument, a new List is created. 
        If the List arg is present, the objects are appended.

        Returns empty list if the target is an ARTIFICIAL_CELL. For that
        case use :meth:`NetCon.synlist`

    .. seealso::
        :meth:`CVode.netconlist`

         

----



.. method:: NetCon.precelllist


    Syntax:
        ``List = netcon.precelllist()``

        ``List = netcon.precelllist(List)``


    Description:
        :class:`List` (i.e. not a Python list) of all the NetCon objects with presynaptic cell object the same as netcon. 
        With no argument, a new List is created. 
        If the List arg is present, the objects are appended. 

        Returns empty list if the source is an ARTIFICIAL_CELL. For that
        case use :meth:NetCon.prelist . Note that it rare for a Cell to
        have more than one distinct NetCon source but olfactory bulb reciprocal
        synapses are an example.

    .. seealso::
        :meth:`CVode.netconlist`

         

----



.. data:: NetCon.delay


    Syntax:
        ``delay = netcon.delay``

        ``netcon.delay = delay``


    Description:
        Time (ms) between source crossing threshold and delivery of event 
        to target. Any number of threshold events may occur before delivery of 
        previous events. delay may be any value >= 0. 

         

----



.. method:: NetCon.wcnt


    Syntax:
        ``n = netcon.wcnt()``


    Description:
        Returns the size of the weight array. 

         

----



.. data:: NetCon.weight


    Syntax:
        ``x = netcon.weight[i]``

        ``netcon.weight[i] = x``


    Description:
        Weight variable which is delivered to the target point processes 
        NET_RECEIVE procedure. The number of arguments in the model descriptions 
        NET_RECEIVE procedure determines the size of the weight vector. 
        Generally the 0th element refers to synaptic weight 
        and remaining elements are used as storage by a synaptic model for purposes 
        of distinguishing NetCon streams of events. However if the NET_RECEIVE 
        block of the post synaptic point process contains an INITIAL block, 
        that block is executed instead of setting all weight[i>0] = 0. 

     .. note::

        In Python, the index is `always` required; this is different from HOC, where
        it can be omitted if it is 0.

----



.. data:: NetCon.threshold


    Syntax:
        ``th = netcon.threshold``

        ``netcon.threshold = th``


    Description:
        Source threshold. Note that many NetCon objects may share the same 
        source. 
         
        Note that prior to 12-Jul-2006, when a NecCon was constructed with no threshold 
        argument, the threshold was 
        reset to the default 10 (mV) even if the threshold for that source location 
        had been explicitly set earlier. That behavior caused confusion and has been 
        changed so that if the constructor has no threshold argument and the 
        threshold location already exists, the previous threshold is retained. 
         

         

----



.. data:: NetCon.x


    Syntax:
        ``x = netcon.x``

        ``netcon.x = x``


    Description:
        Value of the source variable which is watched for threshold crossing. 
        If the source is a membrane potential (or other RANGE variable)
        then ``netcon.x`` is a reference to 
        that potential or variable.
        If the source is an object, the source has no
        NET_RECEIVE block, and the source declares an x RANGE variable,
        then ``netcon.x`` is a reference 
        to the objects field called "x", ie source.x (otherwise it
        evaluates to 0.0 . 

         

----



.. method:: NetCon.record


    Syntax:
        ``netcon.record(Vector)``

        ``netcon.record()``

        ``netcon.record(py_callable)``

        ``netcon.record("")``

        ``netcon.record(tvec, idvec)``

        ``netcon.record(tvec, idvec, id)``


    Description:
        Records the event times at the source the netcon connects to. 
         
        With no argument, no vector recording at the source takes place. 
         
        The vector is resized to 0 when :func:`finitialize` is called. 
         
        NB: Recording takes place on a per source, not a per netcon basis, 
        and the source only records into one vector at a time. 
         
        When the argument is a py_callable, then py_callable is called on a 
        source event. Like the Vector case, the source only manages 
        one py_callable at a time, which is removed when the arg is "". 
         
        If a source is recording a vector, that source is not destroyed when 
        the last netcon connecting to it is destroyed and it continues to record. 
        The source is notified when the vector it is recording 
        ceases to exist---at that time it will be destroyed if no netcons currently 
        connect to it. To do a recording of a source, the following idiom 
        works: 

        .. code-block::
            python

            vec = h.Vector() 
            netcon = h.NetCon(section(x)._ref_v, None, sec=section) 
            netcon.record(vec) 


        The source will continue to record events until record is called 
        with another netcon connecting to the source or until the vec is 
        destroyed. Notice that this idiom allows recording from output cells 
        (which normally have no connecting netcons) as well as simplifying the 
        management of recording from cells. 
         
        Note that NetCon.event(t) events are NOT recorded. 
         
        The netcon.record(tvec, idvec) form is similar to netcon.record(tvec) but 
        in addition the id value of NetCon[id] is also recorded in idvec (or the 
        specified id integer if the third arg is present). This allows 
        many source recordings with a single pair of vectors and obviates the use 
        of separate tvec objects for each recording. 

    Example:
        To stop the simulation when a particular compartment reaches a threshold. 
        
        .. code-block::
            python  
            
            from neuron import h, gui

            soma = h.Section(name='soma')
            soma.insert(h.hh)
            soma.L = 3.183098861837907
            soma.diam = 10
            ic = h.IClamp(soma(0.5))
            ic.dur = 0.1
            ic.amp = 3

            g = h.Graph()
            g.size(0, 5, -80, 40)
            g.addexpr('v(0.5)', 1, 1, 0.8, 0.9, 2, sec=soma)

            def handle():
                print("called handle() at time %g  when soma(0.5).v = %g" % (h.t, soma(0.5).v))
                h.stoprun = 1 # Will stop but may go one extra step. Also with 
                # local step the cells will be at different times. 
                # So may wish to do a further... 
                h.cvode.event(h.t + 1e-6)  

            nc = h.NetCon(soma(0.5)._ref_v, None, sec=soma) 
            nc.threshold = 0 # watch out! only one threshold per presyn location 
            nc.record(handle) 
             
            h.cvode_active(True) # optional. but fixed step will probably do one extra time step 
            h.cvode.condition_order(2) # optional. but much more accurate event time evaluation. 
             
            h.run() 
            print("after h.run(), t = %g  when soma(0.5).v = %g" % (h.t, soma(0.5).v))



         

----



.. method:: NetCon.get_recordvec


    Syntax:
        ``tvec = netcon.get_recordvec()``


    Description:
        Returns the Vector being recorded by the netcon. If the NetCon is not 
        recording or is recording via a hoc statement, the return value is 
        ``None``. Note that record vector is also returned if the NetCon is one of 
        many recording into the same Vector via the NetCon.record(tvec, idvec) 
        style. 

         

----



.. method:: NetCon.srcgid


    Syntax:
        ``gid = netcon.srcgid()``


    Description:
        Returns the global source id integer that sends events through the NetCon. 
        May return -1 or -2 if the NetCon has no source or if the source does not 
        send interprocessor events. If the gid >= 0 then the netcon must have been 
        created by a :meth:`ParallelContext.gid_connect` call with gid as the first 
        arg or else it is connected to spike detector that was associated with a 
        gid via :meth:`ParallelContext.cell`. 
         
        There is no way to determine the corresponding target cell gid (assuming there 
        is one and only one gid source integer for each cell. But see 
        :meth:`NetCon.syn` and :meth:`NetCon.postcell`. 

         
         

