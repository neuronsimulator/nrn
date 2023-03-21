.. _mechtype:

MechanismType
-------------



.. class:: MechanismType


    Syntax:
        ``mt = h.MechanismType(0)``

        ``mt = h.MechanismType(1)``


    Description:
        Provides a way of iterating over all membrane mechanisms or point 
        processes and allows selection via a menu or under hoc control. 
         
        The 0 argument creates a list of all available distributed 
        membrane mechanisms (as opposed to PointProcesses). eg "hh", "pas", "extracellular". that can 
        be inserted into a section. 
         
        The 1 argument creates a 
        list of all available Point Processes. 
        eg. IClamp, AlphaSynapse, VClamp. 
         
        Mechanism order is the same as the argument order that created the "special" 
        during \ ``nrnivmodl`` or \ ``mknrndll``. Therefore when a saved session depends 
        on information in a \ ``MechanismType`` it is dependent on a particular special 
        or dll. 
         

    Example:

        .. code-block::
            python

            from neuron import h
            # Print the names of all density mechanisms 
            mt = h.MechanismType(0) 
            mname  = h.ref('')
            for i in range(mt.count()):
            	mt.select(i) 
            	mt.selected(mname) 
            	print(mname[0])


    .. seealso::
        :class:`MechanismStandard`

         

----



.. method:: MechanismType.select


    Syntax:
        ``mt.select("name")``

        ``mt.select(i)``


    Description:
        selects either the named mechanism or the i'th mechanism in the list. 

         

----



.. method:: MechanismType.selected


    Syntax:
        ``i = mt.selected([strdef])``


    Description:
        returns the index of the current selection.  If present, strdef is assigned 
        to the name of the current selection.

    .. note::

        ``strdef`` must be a NEURON string reference (e.g. one created via ``strdef = h.ref('')``);
        to access its contents use ``strdef[0]``; see the example for the constructor above. In
        particular ``strdef`` cannot be a Python string.

         

----



.. method:: MechanismType.remove


    Syntax:
        ``mt.remove(sec=section)``


    Description:
        For distributed mechanisms invoked with the "insert" statement. 
        Deletes selected mechanism from the specified section. A noop
        if the mechanism is not in the section. 

         

----



.. method:: MechanismType.make


    Syntax:
        ``mt.make(sec=section)``

        ``mt.make(objectref)``


    Description:


        \ ``mt.make(sec=section)`` 
            For distributed mechanisms. Inserts selected mechanism into ``section``. 

        \ ``mt.make(objectref)`` 
            For point processes.  The arg becomes a reference to a new point process 
            of type given by the selection. 
            Note that the newly created point process is not located in any section. 
            If *objectref* was the only reference to another object then 
            that object is destroyed. *objectref* is a NEURON pointer to an object, and
            may be created via ``objectref = h.ref(None)``; the object created by a call
            to ``make`` may be accessed via ``objectref[0]``.


         

----



.. method:: MechanismType.count


    Syntax:
        ``i = mt.count()``


    Description:
        The number of different mechanisms in the list. 

         

----



.. method:: MechanismType.menu


    Syntax:
        ``mt.menu()``


    Description:
        Inserts a special menu into the currently open \ ``xpanel``. The menu 
        label always reflects the current selection. Submenu items are indexed 
        according to position with the first item being item 0.  When the mouse 
        button is released on a submenu item that item becomes the selection 
        and the action (if any) is executed. 

         

----



.. method:: MechanismType.action


    Syntax:
        ``mt.action(py_callable)``


    Description:
        When a submenu item is selected, ``py_callable`` is invoked with two arguments:
        the MechanismType object, and the index.

    Example:

        .. code-block::
            python

            from neuron import h, gui

            def cb(mt, i):
                mt.select(i)
                nameref = h.ref("")
                mt.selected(nameref)
                print (f"selected {nameref[0]}")

            mtypes = [h.MechanismType(i) for i in range(2)]
            h.xpanel("MechanismTypes")
            for mt in mtypes:
                mt.action(cb)
                mt.menu()
            h.xpanel()


    .. note::

        Python support for this method was added in NEURON 7.5.
----



.. method:: MechanismType.is_netcon_target


    Syntax:
        ``boolean =  mt.is_netcon_target(i)``


    Description:
        The i'th point process has a NET_RECEIVE block and can therefore be 
        a target for a :class:`NetCon` object. 

         

----



.. method:: MechanismType.has_net_event


    Syntax:
        ``boolean = mt.has_net_event(i)``


    Description:
        The i'th point process has a net_event call in its NET_RECEIVE block 
        and can therefore be a source for a :class:`NetCon` object. 
        This means it is :class:`NetCon` stimulator or that 
        the point process can be used as an artificial neural network cell. 

         

----



.. method:: MechanismType.is_artificial


    Syntax:
        ``boolean = mt.is_artificial(i)``


    Description:
        The i'th point process is an ARTIFICIAL_CELL 
        and can therefore be a source for a :class:`NetCon` object. 
        This means it is :class:`NetCon` stimulator or that 
        the point process can be used as an artificial neural network cell. 
         
        This seems to have, but does not, equivalent functionality to 
        :func:`has_net_event` and was introduced because ARTIFICIAL_CELL objects are no 
        longer located in sections. Some ARTIFICIAL_CELLs such as the PatternStim 
        do not make use of net_event in their implementation, and some PointProcesses 
        do use net_event and must be located in sections for their proper function, 
        e.g. reciprocal synapses. 

         
         

----



.. method:: MechanismType.pp_begin


    Syntax:
        ``obj = mt.pp_begin(sec=section)``


    Description:
        Initializes an iterator used to iterate over point processes of 
        a particular type in ``section``. 
        Returns the first point process in 
        ``section`` having the type specified by the :meth:`MechanismType.select` 
        statement. This only works if the the MechanismType was instantiated 
        with the (1) argument. If there is no such point process in the 
        section the method returns None. Note that, prior to version 
        6.2, although 
        the x=1 node is normally 
        considered to be part of the section, the parent node 
        was not looked at (normally x = 0) unless the section was the 
        root of the tree. As of version 6.2, both the 0 and 1 locations 
        are looked at and if the point process used the section to locate 
        it, then it is returned. If the point process used the child or 
        parent section to locate it, it is not returned. 

    Example:

        .. code-block::
            python
            
            from neuron import h

            cable = h.Section(name='cable')
            cable.nseg = 5  
            stim = [h.IClamp(cable(i/2.)) for i in range(3)]

            mt = h.MechanismType(1) 
            mt.select("IClamp") 
            pp = mt.pp_begin()
            while h.object_id(pp) != 0:
                seg = pp.get_segment() 
                print(f"{pp} located at {seg}")
                pp = mt.pp_next()



----



.. method:: MechanismType.pp_next


    Syntax:
        ``obj = mt.pp_next()``


    Description:
        Returns the next point process of the type and in the section that 
        were specified in the earlier call to :meth:`MechanismType.pp_begin` . 
        When there are no more point processes, the return value is NULLobject. 

         

----



.. method:: MechanismType.internal_type


    Syntax:
        ``internal_type = mt.internal_type()``


    Description:
        Return the internal type index of the selected mechanism. 


----



.. method:: MechanismType.file


    Syntax:
        ``file_name = mt.file()``


    Description:
        Returns the mod file name for the currently selected mechanism.

        .. code-block::
            python
            
            from neuron import h
            s = h.Section(name='s')
            mt = h.MechanismType(0)
            mt.select('hh')
            print(mt.file())

----



.. method:: MechanismType.code


    Syntax:
        ``code_string = mt.code()``


    Description:
        Returns the nmodl code for the currently selected mechanism.
        .. code-block::
            python
            
            from neuron import h
            s = h.Section(name='s')
            mt = h.MechanismType(0)
            mt.select('hh')
            print('\n'.join(mt.code().split('\n')[:4]))
