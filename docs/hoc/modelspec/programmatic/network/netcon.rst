
.. _hoc_netcon:

NetCon
------



.. hoc:class:: NetCon


    Syntax:
        ``section netcon = new NetCon(&v(x), target)``

        ``netcon = new NetCon(source, target)``

        ``section netcon = new NetCon(&v(x), target, threshold, delay, weight)``

        ``netcon = new NetCon(source, target, threshold, delay, weight)``


    Description:
        Network Connection object that defines a synaptic connection between 
        a source and target. When the source variable passes threshold in the 
        positive direction at time t-delay, the target will receive an event 
        at time t along with weight information. There is no limit on delay 
        except that it be >= 0 and there is no limit on the number of events 
        pending delivery. 
         
        If the optional threshold, delay, and weight arguments are not 
        specified, their default values are 10, 1, and 0 respectively. In 
        any case, their values can be specified after the netcon has been 
        constructed, see :hoc:data:`NetCon.threshold`, :hoc:data:`NetCon.weight`, and :hoc:data:`NetCon.delay` .
         
        Note that prior to 12-Jul-2006, when the first form of the constructor 
        was used, (i.e. a NetCon having a pointer to a source 
        variable was created, but having no threshold argument) the threshold was 
        reset to the default 10 (mV) even if the threshold for that source location 
        had been explicitly set earlier. That behavior caused confusion and has been 
        changed so that if there is no threshold argument and the threshold location 
        already exists, the previous threshold is retained. 
         
        The target must be a POINT_PROCESS or ARTIFICIAL_CELL that defines a NET_RECEIVE procedure. 
        The number of NET_RECEIVE procedure arguments define a weight vector 
        whose elements can be accessed with through the NetCon.weight ( :hoc:data:`NetCon.weight` )variable
        but the weight argument in the above constructors specify the value of 
        the first argument, with the normal interpretation of weight or maximum 
        conductance. On initialization, all weight elements with index > 0 are 
        set to 0 unless the NET_RECEIVE block contains an INITIAL block. In the 
        latter case, that block is executed on a call to :hoc:func:`finitialize`  and
        allows non-zero initialization of netcon "states" --- args not initialized 
        in the INITIAL block would be analogous to a :ref:`hoc_Parameter <hoc_nmodl_parameter>` except that it
        can have a different value for different NetCon instances and can be set 
        to a desired value with :hoc:data:`NetCon.weight`.
         
        The target is allowed to be nil (NULLObject) in which case the NetCon 
        is always inactive. However this can be useful for recording (see 
        :hoc:meth:`NetCon.record`) the spike train from an output cell.
         
        The source is normally a reference to a membrane potential which is 
        watched during simulation for passage past threshold. The 
        currently accessed section is required by the local variable 
        time step method in order to determine the source "cell". 
        Any range variable may be a source variable but I suspect that membrane 
        potential is the only practical one. 
         
        N.B. For the local variable time step method :hoc:meth:`CVode.use_local_dt` , the
        proper currently accessed section for the source must be correct during 
        the creation of the NetCon so that the proper cell may be associated 
        with the source. i.e, 
        \ ``netcon = new NetCon(&obj.sec.v(.5), ...)`` 
        will not work with the local step method because, although the pointer 
        is correct, the proper section was popped from the section stack prior 
        to the constructor call. Instead, the proper syntax is 
        \ ``obj.sec netcon = new NetCon(&v(.5),...)`` 
         
        The source may also be a PointProcess with a NET_RECEIVE block which 
        contains a call to net_event. PointProcesses like this serve as entire 
        artificial cells. 
         
        The source may also 
        be a PointProcess which contains a "x" variable which is watched for 
        threshold crossing, but this is obsolete since NET_RECEIVE blocks which 
        explicitly call net_event are much more efficient since they avoid 
        the overhead of threshold detection at every time step. 
         
        The source may be a NULLObject. In this case events can only occur by 
        calling :hoc:func:`event` from hoc. (It is also used by NEOSIM to implement
        its own delivery system.) 
         
        A source used by multiple NetCon instances is shared by those instances 
        to allow faster threshold detection (ie on a per source basis instead 
        of a per NetCon basis) Therefore, there is really only one threshold 
        for all NetCon objects that share a source. However, delay and weight 
        are distinct for each NetCon object. 
         
        The only way one can have multiple threshold values at the same location is 
        to create a threshold detector point process with a NET_RECEIVE block implemented 
        with a WATCH statement and calling net_event . 
         
        And I'll say it again: 
        Note that prior to 12-Jul-2006, when the first form of the constructor 
        was used, (i.e. a NetCon having a pointer to a source 
        variable was created, but having no threshold argument) the threshold was 
        reset to the default 10 (mV) even if the threshold for that source location 
        had been explicitly set earlier. That behavior caused confusion and has been 
        changed so that if there is no threshold argument and the threshold location 
        already exists, the previous threshold is retained. 
         
        From a NetCon instance, various lists of NetCon's can be created 
        with the same target, precell, or postcell. See :hoc:meth:`CVode.netconlist`
        for creation of NetCon lists from a target, precell, or  postcell 
        pattern or object. 
         

    .. warning::
        NetCon can currently only be used if a CVode object exists. 
         
        The local variable step method does not work when the source is specified 
        with the syntax \ ``netcon = new NetCon(&soma.v(.5),...)``. The 
        currently accessed section must be correct during the construction of 
        the object and the above example is correct only during calculation of 
        the pointer argument. 
         

         

----



.. hoc:method:: NetCon.valid


    Syntax:
        ``boolean = netcon.valid()``


    Description:
        Returns 0 if the source or target have been freed. If the NetCon object 
        is used when it is not valid a runtime error message will be printed on 
        the console terminal. 

         

----



.. hoc:method:: NetCon.active


    Syntax:
        ``boolean = netcon.active(boolean)``

        ``boolean = netcon.active()``


    Description:
        Turns the synapse on or off in the sense that when off, no events 
        are delivered using this NetCon instance. Returns the previous 
        state (or current state if no argument). 

         

----



.. hoc:method:: NetCon.event


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



.. hoc:method:: NetCon.syn


    Syntax:
        ``target_object = netcon.syn()``


    Description:
        Returns a reference to the synaptic target PointProcess. 

         

----



.. hoc:method:: NetCon.pre


    Syntax:
        ``source_object = netcon.pre()``


    Description:
        Returns a reference to the source PointProcess. If the source is a membrane 
        potential then the return value is NULLobject 

         

----



.. hoc:method:: NetCon.preloc


    Syntax:
        ``{x = netcon.preloc() ... pop_section()}``


    Description:
        The source section is pushed onto the section stack so that it is 
        the currently accessed section. Pop_section must be called after you are 
        finished with the section. 

    .. warning::
        The return value of x is .5 unless the source is a membrane potential and 
        located at 0, or 1, in which case value returned is 0 or 1, respectively. 
        Therefore it does not necessarily correspond to the actual x value location. 
        If the source was an object, the section is not pushed and the return 
        value is -1. 

         

----



.. hoc:method:: NetCon.postloc


    Syntax:
        ``{x = netcon.postloc() ... pop_section()}``


    Description:
        The section of the target point process is pushed onto the section stack 
        so that it is the currently accessed section. Pop_section must be called 
        after you are finished with the section. The x return value is the 
        relative location of the point process in that section. 

         

----



.. hoc:method:: NetCon.precell


    Syntax:
        ``cellobj = netcon.precell()``


    Description:
        If the source is a membrane potential and the section was declared in 
        an object (defined in a cell template), a reference to the presynaptic cell 
        (object) is returned. 

         

----



.. hoc:method:: NetCon.postcell


    Syntax:
        ``cellobj = netcon.postcell()``


    Description:
        If the synaptic point process is located in a section which was declared in 
        an object (defined in a cell template), a reference to the postsynaptic cell 
        (object) is returned. 

         

----



.. hoc:method:: NetCon.setpost


    Syntax:
        ``netcon.setpost(newtarget)``


    Description:
        Will change the old postsynaptic POINT_PROCESS target to the one specified 
        by the newtarget. If there is no argument 
        or the argument is NullObject then NetCon will have no target and the 
        active flag will be set to 0. Note that a target change will preserve the 
        current weight vector only if the new and old targets have the same 
        weight vector size (number of arguments in the NET_RECEIVE block). 

         

----



.. hoc:method:: NetCon.prelist


    Syntax:
        ``List = netcon.prelist()``

        ``List = netcon.prelist(List)``


    Description:
        List of all the NetCon objects with source the same as netcon. 
        With no argument, a new List is created. 
        If the List arg is present, the objects are appended. 

         

----



.. hoc:method:: NetCon.synlist


    Syntax:
        ``List = netcon.synlist()``

        ``List = netcon.synlist(List)``


    Description:
        List of all the NetCon objects with target the same as netcon. 
        With no argument, a new List is created. 
        If the List arg is present, the objects are appended. 

    .. seealso::
        :hoc:meth:`CVode.netconlist`

         

----



.. hoc:method:: NetCon.postcelllist


    Syntax:
        ``List = netcon.postcelllist()``

        ``List = netcon.postcelllist(List)``


    Description:
        List of all the NetCon objects with postsynaptic cell object the same as netcon. 
        With no argument, a new List is created. 
        If the List arg is present, the objects are appended. 

    .. seealso::
        :hoc:meth:`CVode.netconlist`

         

----



.. hoc:method:: NetCon.precelllist


    Syntax:
        ``List = netcon.precelllist()``

        ``List = netcon.precelllist(List)``


    Description:
        List of all the NetCon objects with presynaptic cell object the same as netcon. 
        With no argument, a new List is created. 
        If the List arg is present, the objects are appended. 

    .. seealso::
        :hoc:meth:`CVode.netconlist`

         

----



.. hoc:data:: NetCon.delay


    Syntax:
        ``del = netcon.delay``

        ``netcon.delay = del``


    Description:
        Time (ms) between source crossing threshold and delivery of event 
        to target. Any number of threshold events may occur before delivery of 
        previous events. delay may be any value >= 0. 

         

----



.. hoc:method:: NetCon.wcnt


    Syntax:
        ``n = netcon.wcnt()``


    Description:
        Returns the size of the weight array. 

         

----



.. hoc:data:: NetCon.weight


    Syntax:
        ``w = netcon.weight``

        ``netcon.weight = w``

        ``x = netcon.weight[i]``

        ``netcon.weight[i] = x``


    Description:
        Weight variable which is delivered to the target point processes 
        NET_RECEIVE procedure. The number of arguments in the model descriptions 
        NET_RECEIVE procedure determines the size of the weight vector. 
        Generally the 0th element (no index required) refers to synaptic weight 
        and remaining elements are used as storage by a synaptic model for purposes 
        of distinguishing NetCon streams of events. However if the NET_RECEIVE 
        block of the post synaptic point process contains an INITIAL block, 
        that block is executed instead of setting all weight[i>0] = 0. 

         

----



.. hoc:data:: NetCon.threshold


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



.. hoc:data:: NetCon.x


    Syntax:
        ``x = netcon.x``

        ``netcon.x = x``


    Description:
        Value of the source variable which is watched for threshold crossing. 
        If the source is a membrane potential then netcon.x is a reference to 
        that potential. If the source is an object, then netcon.x is a reference 
        to the objects field called "x", ie source.x . 

         

----



.. hoc:method:: NetCon.record


    Syntax:
        ``netcon.record(Vector)``

        ``netcon.record()``

        ``netcon.record("stmt")``

        ``netcon.record(tvec, idvec)``

        ``netcon.record(tvec, idvec, id)``


    Description:
        Records the event times at the source the netcon connects to. 
         
        With no argument, no vector recording at the source takes place. 
         
        The vector is resized to 0 when :hoc:func:`finitialize` is called.
         
        NB: Recording takes place on a per source, not a per netcon basis, 
        and the source only records into one vector at a time. 
         
        When the argument is a "stmt", then the statement is called on a 
        source event. Like the Vector case, the source only manages 
        one statement at a time. The stmt is removed when the arg is "". 
         
        If a source is recording a vector, that source is not destroyed when 
        the last netcon connecting to it is destroyed and it continues to record. 
        The source is notified when the vector it is recording 
        ceases to exist---at that time it will be destroyed if no netcons currently 
        connect to it. To do a recording of a source, the following idiom 
        works: 

        .. code-block::
            none

            objref vec, netcon, nil 
            vec = new Vector() 
            netcon = new NetCon(source, nil) 
            netcon.record(vec) 
            objref netcon 

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
            none  
        
            load_file("nrngui.hoc") 
            objectvar save_window_, rvp_ 
            objectvar scene_vector_[4] 
            objectvar ocbox_, ocbox_list_, scene_, scene_list_ 
            {ocbox_list_ = new List()  scene_list_ = new List()} 
            {pwman_place(0,0,0)} 
             
            //Begin SingleCompartment 
            { 
            load_file("single.hoc") 
            } 
            ocbox_ = new SingleCompartment(0) 
            ocbox_.inserter = new Inserter(0) 
            {object_push(ocbox_.inserter)} 
            { 
            mt.select("hh") i = mt.selected() 
            ms[i] = new MechanismStandard("hh") 
            ms[i].set("gnabar_hh", 0.12, 0) 
            ms[i].set("gkbar_hh", 0.036, 0) 
            ms[i].set("gl_hh", 0.0003, 0) 
            ms[i].set("el_hh", -54.3, 0) 
            mstate[i]= 1 
            maction(i) 
            } 
            {object_pop() doNotify()} 
            {object_push(ocbox_)} 
            {inserter.v1.map()} 
            {endbox()} 
            {object_pop() doNotify()} 
            { 
            ocbox_ = ocbox_.vbox 
            ocbox_.map("SingleCompartment", 382, 22, 91.2, 96) 
            } 
            objref ocbox_ 
            //End SingleCompartment 
             
             
            //Begin PointProcessManager 
            { 
            load_file("pointman.hoc") 
            } 
            { 
            soma ocbox_ = new PointProcessManager(0) 
            } 
            {object_push(ocbox_)} 
            { 
            mt.select("IClamp") i = mt.selected() 
            ms[i] = new MechanismStandard("IClamp") 
            ms[i].set("del", 0, 0) 
            ms[i].set("dur", 0.1, 0) 
            ms[i].set("amp", 0.3, 0) 
            mt.select("IClamp") i = mt.selected() maction(i) 
            hoc_ac_ = 0.5 
            sec.sec move() d1.flip_to(0) 
            } 
            {object_pop() doNotify()} 
            { 
            ocbox_ = ocbox_.v1 
            ocbox_.map("PointProcessManager", 152, 109, 208.32, 326.4) 
            } 
            objref ocbox_ 
            //End PointProcessManager 
             
            { 
            save_window_ = new Graph(0) 
            save_window_.size(0,5,-80,40) 
            scene_vector_[2] = save_window_ 
            {save_window_.view(0, -80, 5, 120, 493, 23, 300.48, 200.32)} 
            graphList[0].append(save_window_) 
            save_window_.save_name("graphList[0].") 
            save_window_.addexpr("v(.5)", 1, 1, 0.8, 0.9, 2) 
            } 
            objectvar scene_vector_[1] 
            {doNotify()} 
             


            none

            // ... soma with hh, IClamp, and voltage plot ... 
             
            objref nc, nil 
            soma nc = new NetCon(&v(.5), nil) 
            nc.threshold = 0 // watch out! only one threshold per presyn location 
            nc.record("handle()") 
             
            proc handle() { 
            	print "called handle() at time ", t, " when soma.v(.5) = ", soma.v(.5) 
            	stoprun = 1 // Will stop but may go one extra step. Also with 
            		// local step the cells will be at different times. 
            		// So may wish to do a further... 
            	cvode.event(t+1e-6)  
            } 
             
            cvode_active(1) // optional. but fixed step will probably do one extra time step 
            cvode.condition_order(2) // optional. but much more accurate event time evaluation. 
             
            run() 
            print "after run(), t = ", t, " and soma.v(.5) = ", soma.v(.5) 


         

----



.. hoc:method:: NetCon.get_recordvec


    Syntax:
        ``tvec = netcon.get_recordvec()``


    Description:
        Returns the Vector being recorded by the netcon. If the NetCon is not 
        recording or is recording via a hoc statement, the return value is 
        NULLobject. Note that record vector is also returned if the NetCon is one of 
        many recording into the same Vector via the NetCon.record(tvec, idvec) 
        style. 

         

----



.. hoc:method:: NetCon.srcgid


    Syntax:
        ``gid = netcon.srcgid()``


    Description:
        Returns the global source id integer that sends events through the NetCon. 
        May return -1 or -2 if the NetCon has no source or if the source does not 
        send interprocessor events. If the gid >= 0 then the netcon must have been 
        created by a :hoc:meth:`ParallelContext.gid_connect` call with gid as the first
        arg or else it is connected to spike detector that was associated with a 
        gid via :hoc:meth:`ParallelContext.cell`.
         
        There is no way to determine the corresponding target cell gid (assuming there 
        is one and only one gid source integer for each cell. But see 
        :hoc:meth:`NetCon.syn` and :hoc:meth:`NetCon.postcell`.

         
         

