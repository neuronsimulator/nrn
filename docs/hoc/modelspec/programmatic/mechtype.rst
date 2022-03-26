
.. _hoc_mechtype:

MechanismType
-------------



.. hoc:class:: MechanismType


    Syntax:
        ``mt = new MechanismType(0)``

        ``mt = new MechanismType(1)``


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
            none

            objref mt 
            //Print the names of all density mechanisms 
            mt = new MechanismType(0) 
            strdef mname 
            for i=0, mt.count()-1 { 
            	mt.select(i) 
            	mt.selected(mname) 
            	print mname 
            } 


    .. seealso::
        :hoc:class:`MechanismStandard`

         

----



.. hoc:method:: MechanismType.select


    Syntax:
        ``mt.select("name")``

        ``mt.select(i)``


    Description:
        selects either the named mechanism or the i'th mechanism in the list. 

         

----



.. hoc:method:: MechanismType.selected


    Syntax:
        ``i = mt.selected([strdef])``


    Description:
        returns the index of the current selection.  If present, strarg is assigned 
        to the name of the current selection. 

         

----



.. hoc:method:: MechanismType.remove


    Syntax:
        ``mt.remove()``


    Description:
        For distributed mechanisms invoked with the "insert" statement. 
        Deletes selected mechanism from currently 
        accessed section. A nop if the mechanism is not in the section. 

         

----



.. hoc:method:: MechanismType.make


    Syntax:
        ``mt.make()``

        ``mt.make(objectvar)``


    Description:


        \ ``mt.make()`` 
            For distributed mechanisms. Inserts selected mechanism into currently 
            accessed section. 

        \ ``mt.make(objectvar)`` 
            For point processes.  The arg becomes a reference to a new point process 
            of type given by the selection. 
            Note that the newly created point process is not located in any section. 
            Note that if *objectvar* was the only reference to another object then 
            that object is destroyed. 


         

----



.. hoc:method:: MechanismType.count


    Syntax:
        ``i = mt.count()``


    Description:
        The number of  different mechanisms in the list. 

         

----



.. hoc:method:: MechanismType.menu


    Syntax:
        ``mt.menu()``


    Description:
        Inserts a special menu into the currently open \ ``xpanel``. The menu 
        label always reflects the current selection. Submenu items are indexed 
        according to position with the first item being item 0.  When the mouse 
        button is released on a submenu item that item becomes the selection 
        and the action (if any) is executed. 

         

----



.. hoc:method:: MechanismType.action


    Syntax:
        ``mt.action("command")``


    Description:
        The action to be executed when a submenu item is selected. 

         

----



.. hoc:method:: MechanismType.is_netcon_target


    Syntax:
        ``boolean =  mt.is_netcon_target(i)``


    Description:
        The i'th point process has a NET_RECEIVE block and can therefore be 
        a target for a :hoc:class:`NetCon` object.

         

----



.. hoc:method:: MechanismType.has_net_event


    Syntax:
        ``boolean = mt.has_net_event(i)``


    Description:
        The i'th point process has a net_event call in its NET_RECEIVE block 
        and can therefore be a source for a :hoc:class:`NetCon` object.
        This means it is :hoc:class:`NetCon` stimulator or that
        the point process can be used as an artificial neural network cell. 

         

----



.. hoc:method:: MechanismType.is_artificial


    Syntax:
        ``boolean = mt.is_artificial(i)``


    Description:
        The i'th point process is an ARTIFICIAL_CELL 
        and can therefore be a source for a :hoc:class:`NetCon` object.
        This means it is :hoc:class:`NetCon` stimulator or that
        the point process can be used as an artificial neural network cell. 
         
        This seems to have, but does not, equivalent functionality to 
        :hoc:func:`has_net_event` and was introduced because ARTIFICIAL_CELL objects are no
        longer located in sections. Some ARTIFICIAL_CELLs such as the PatternStim 
        do not make use of net_event in their implementation, and some PointProcesses 
        do use net_event and must be located in sections for their proper function, 
        e.g. reciprocal synapses. 

         
         

----



.. hoc:method:: MechanismType.pp_begin


    Syntax:
        ``obj = mt.pp_begin()``


    Description:
        Initializes an iterator used to iterate over point processes of 
        a particular type in the currently accessed section. 
        Returns the first point process in the currently accessed 
        section having the type specified by the :hoc:meth:`MechanismType.select`
        statement. This only works if the the MechanismType was instantiated 
        with the (1) argument. If there is no such point process in the 
        section the method returns NULLobject. Note that, prior to version 
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
            none

            create cable 
            access cable 
            nseg = 5 
            objref stim[3] 
            for i=0,2 stim[i] = new IClamp(i/2) 
             
            objref mt, pp 
            mt = new MechanismType(1) 
            mt.select("IClamp") 
            for (pp = mt.pp_begin(); object_id(pp) != 0; pp = mt.pp_next()) { 
            	x = pp.get_loc() 
            	printf("%s located at %s(%g)\n", pp, secname(), x) 
            	pop_section() 
            } 


         

----



.. hoc:method:: MechanismType.pp_next


    Syntax:
        ``obj = mt.pp_next()``


    Description:
        Returns the next point process of the type and in the section that 
        were specified in the earlier call to :hoc:meth:`MechanismType.pp_begin` .
        When there are no more point processes, the return value is NULLobject. 

         

----



.. hoc:method:: MechanismType.internal_type


    Syntax:
        ``internal_type = mt.internal_type()``


    Description:
        Return the internal type index of the selected mechanism. 


