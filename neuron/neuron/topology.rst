.. _topology:

         
Topology
--------

         
         

----



.. function:: create


    Syntax:
        :code:`create`



    Description:
        This is an nrniv command which creates a list of section names.  Existing sections with 
        the same names are destroyed and recreated.  The create statement may 
        occur within procedures, but the names must have been previously declared with 
        a create statement at the command level. 
         

    Example:

        .. code-block::
            none

            create soma, axon, dend[3] 
            forall { 
            	print secname() 
            } 

        prints the names of all the sections which have been created. 

        .. code-block::
            none

            soma 
            axon 
            dend[0] 
            dend[1] 
            dend[2] 

         

    .. seealso::
        :func:`connect`, :func:`insert`, :func:`forall`
        

         

----



.. function:: connect


    Syntax:
        :code:`connect section(0or1), x`

        :code:`connect section(0or1), parent(x)`



    Description:
        The first form connects the section at end 0 or 1 to the currently 
        accessed section at position x.  An alternative syntax is the second 
        form in which the parent section is explicitly indicated.  If a section 
        is connected twice a Notice is printed on the standard error device 
        saying that the section has been reconnected (the last connection takes 
        precedence).  To avoid the notice, disconnect the section first with the 
        function "disconnect()".  If sections are inadvertently connected in a 
        loop, an error will be generated when the internal data structures are 
        created and the user will be required to disconnect one of the sections 
        forming the loop. 
         

    Example:

        .. code-block::
            none

             create soma, axon, dendrite[3] 
             connect axon(0), soma(0) 
             soma for i=0,2 { 
               connect dendrite[i](0), 1 
             } 
             topology() 
             objref s 
             s = new Shape() 


         

----



.. function:: topology


    Syntax:
        :code:`topology()`


    Description:
        Print the topology of how the sections are connected together. 

         
         

----



.. function:: delete_section


    Syntax:
        :code:`delete_section()`


    Description:
        Delete the currently accessed section from the main section 
        list which is used in computation. 
        \ :code:`forall delete_section` 
        will remove all sections. 
         
        Note: deleted sections still exist (even though 
        :meth:`SectionRef.SectionRef` . :func:`exists` 
        returns 0 and an error will result if one attempts to access 
        the section) so 
        that other objects (such as section lists and Shapes) which 
        hold pointers to these sections will still work. When the last 
        pointer to a section is destroyed, the section memory will be 
        freed. 

         

----



.. function:: section_exists


    Syntax:
        :code:`boolean = section_exists("name", [index], [object])`


    Description:
        Returns 1 if the section defined by the args exists and can be used 
        as a currently accessed section. Otherwise, returns 0. 
        The index is optional and if nonzero, can be incorporated into the name as 
        a literal value such as dend[25]. If the optional object arg is present, that 
        is the context, otherwise the context is the top level. "name" should 
        not contain the object prefix. Even if a section is multiply dimensioned, use 
        a single index value. 

         

----



.. function:: section_owner


    Syntax:
        :code:`section_owner()`


    Description:
        Return the object that created the currently accessed section. If the 
        section was created from the top level, The NULLobject is returned. 
        If the section was created as a Python section and the first constructor 
        arg is a Python object or the keyword argument, cell = ..., is used, a 
        PythonObject wrapper is returned. I.e. in the Python world, it is the Python 
        cell object. 

         

----



.. function:: disconnect


    Syntax:
        :code:`disconnect()`


    Description:
        Disconnect the currently accessed section from its parent. Such 
        a parent can be reconnected with the connect statement. 


----



.. data:: nseg


    Description:
        Number of segments (compartments) in the currently accessed section. 
        When a section is created, nseg is 1. 
        In versions prior to 3.2, changing nseg throws away all 
        "inserted" mechanisms including diam 
        (if 3-d points do not exist). PointProcesss, connectivity, L, and 3-d 
        point information remain unchanged. 
         
        Starting in version 3.2, a change to nseg re-uses information contained 
        in the old segments. 
         
        If nseg is increased, all old segments are 
        relocated to their nearest new locations (no instance variables are modified 
        and no pointers to data in those segments become invalid). 
        and new segments are allocated and given mechanisms and values that are 
        identical to the old segment in which the center of the new segment is 
        located.  This means that increasing nseg by an odd factor preserves 
        the locations of all previous data (including all Point Processes) 
        and, if PARAMETER range variables are 
        constant, that all the new segments have the proper PARAMETER values. 
        (It generally doesn't matter that ASSIGNED and STATE values do not get 
        interpolated since those values are computed with fadvance()). 
        If range variables are not constant then the hoc expressions used to 
        set them should be re-executed. 
         
        If nseg is decreased then all the new segments are in fact those old segments 
        that were nearest the centers of the new segments. Unused old segments 
        are freed (and thus any existing pointers to variables in those freed 
        segments are invalid). This means that decreasing nseg by an odd factor 
        preserves the locations of all previous data. However POINT PROCESSES 
        not located at the centers of the new segments will be discarded. 
         
        The intention is to guarantee that the following sequence 

        .. code-block::
            none

                    run() //sim1 
                    forall nseg *= oddfactor 
                    run() //sim2 
                    forall nseg /= oddfactor 
                    run() //sim3 

        will produce identical simulations for sim1 and sim3. And sim2 will be 
        oddfactor^2 more accurate with regard to spatial discretization error. 


