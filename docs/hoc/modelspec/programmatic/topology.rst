
.. _hoc_topology:

         
Topology
--------

This document describes the construction and manipulation of a stylized topology, which may later be given a 3d shape. For more details and higher level functions, see:

.. toctree::
    :maxdepth: 3

    topology/geometry.rst
    topology/secspec.rst
    topology/seclist.rst
    topology/secref.rst
       
         

----



.. index::  create (keyword)


.. _hoc_keyword_create:

**create**

    Syntax:
        ``create``



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
        :ref:`connect <hoc_keyword_connect>`, :ref:`insert <hoc_keyword_insert>`, :ref:`forall <hoc_keyword_forall>`
        

         

----



.. index::  connect (keyword)


.. _hoc_keyword_connect:

**connect**

    Syntax:
        ``connect section(0or1), x``

        ``connect section(0or1), parent(x)``



    Description:
        The first form connects the section at end 0 or 1 to the currently 
        accessed section at position x.  An alternative syntax is the second 
        form in which the parent section is explicitly indicated.  If a section 
        is connected twice a Notice is printed on the standard error device 
        saying that the section has been reconnected (the last connection takes 
        precedence).  To avoid the notice, disconnect the section first with the 
        function :hoc:func:`disconnect`.  If sections are inadvertently connected in a
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



.. hoc:function:: topology


    Syntax:
        ``topology()``


    Description:
        Print the topology of how the sections are connected together. 

         
         

----



.. hoc:function:: delete_section


    Syntax:
        ``delete_section()``


    Description:
        Delete the currently accessed section from the main section 
        list which is used in computation. 
        \ ``forall delete_section`` 
        will remove all sections. 
         
        Note: deleted sections still exist (even though 
        :hoc:meth:`SectionRef.exists`
        returns 0 and an error will result if one attempts to access 
        the section) so 
        that other objects (such as :hoc:class:`SectionList`\ s and :hoc:class:`Shape`\ s) which
        hold pointers to these sections will still work. When the last 
        pointer to a section is destroyed, the section memory will be 
        freed. 

         

----



.. hoc:function:: section_exists


    Syntax:
        ``boolean = section_exists("name", [index], [object])``


    Description:
        Returns 1 if the section defined by the args exists and can be used 
        as a currently accessed section. Otherwise, returns 0. 
        The index is optional and if nonzero, can be incorporated into the name as 
        a literal value such as dend[25]. If the optional object arg is present, that 
        is the context, otherwise the context is the top level. "name" should 
        not contain the object prefix. Even if a section is multiply dimensioned, use 
        a single index value. 

         

----



.. hoc:function:: section_owner


    Syntax:
        ``section_owner()``


    Description:
        Return the object that created the currently accessed section. If the 
        section was created from the top level, The NULLobject is returned. 
        If the section was created as a Python section and the first constructor 
        arg is a Python object or the keyword argument, cell = ..., is used, a 
        PythonObject wrapper is returned. I.e. in the Python world, it is the Python 
        cell object. 

         

----



.. hoc:function:: disconnect


    Syntax:
        ``disconnect()``


    Description:
        Disconnect the currently accessed section from its parent. Such 
        a parent can be reconnected with the connect statement. 


----



.. hoc:data:: nseg


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
        interpolated since those values are computed with :hoc:func:`fadvance`).
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

----



.. hoc:function:: issection


    Syntax:
        ``issection("regular expression")``


    Description:
        Return 1 if the currently accessed section matches the regular expression. 
        Return 0 if otherwise. 
         
        Regular expressions are like those of grep except {n1-n2} denotes
	an integer range and [] is literal instead of denoting a character
	range. For character ranges use <>. For example <a-z> or <abz45> denotes
	any character from a to z or to any of the characters abz45.
        Thus a[{8-15}] matches sections a[8] through a[15]. 
        A match always begins from the beginning of a section name. If you 
        don't want to require a match at the beginning use the dot. 
         
        (Note, 
        that ``.`` matches any character and ``*`` matches 0 or more occurrences 
        of the previous character). The interpreter always closes each string with 
        an implicit ``$`` to require a match at the end of the string. If you 
        don't require a match at the end use "``.*``". 

    Example:

        .. code-block::
            none

            create soma, axon, dendrite[3] 
            forall if (issection("s.*")) { 
            	print secname() 
            } 

        will print ``soma`` 

        .. code-block::
            none

            forall if (issection("d.*2]")) { 
            	print secname() 
            } 

        will print ``dendrite[2]`` 

        .. code-block::
            none

            forall if (issection(".*a.*")) { 
            	print secname() 
            } 

        will print all names which contain the letter "a" 

        .. code-block::
            none

            soma 
            axon 


    .. seealso::
        :ref:`ifsec <hoc_keyword_ifsec>`, :ref:`forsec <hoc_keyword_forsec>`

         

----



.. hoc:function:: ismembrane


    Syntax:
        ``ismembrane("mechanism")``


    Description:
        This function returns a 1 if the current membrane contains this 
        (density) mechanism.  This is not for point 
        processes. 
         

    Example:

        .. code-block::
            none

            forall if (ismembrane("hh") && ismembrane("ca_ion")) { 
            	print secname() 
            } 

        will print the names of all the sections which contain both Hodgkin-Huxley and Calcium ions. 

         

----



.. hoc:function:: sectionname


    Syntax:
        ``sectionname(strvar)``


    Description:
        The name of the currently accessed section is placed in *strvar*. 
         
        This function is superseded by the easier to use, :hoc:func:`secname`.

         

----



.. hoc:function:: secname


    Syntax:
        ``secname()``


    Description:
        Returns the currently accessed section name. Usage is 

        .. code-block::
            none

            		strdef s 
            		s = secname() 

        or 

        .. code-block::
            none

            		print secname() 

        or 

        .. code-block::
            none

            		forall for(x) printf("%s(%g)\n", secname(), x) 


         

----



.. hoc:function:: psection


    Syntax:
        ``psection()``


    Description:
        Print info about currently accessed section in a format which is executable. 
        (length, parent, diameter, membrane information) 
         

         
         


         


----


.. hoc:function:: parent_section


    Syntax:
        ``parent_section(x)``


    Description:
        Return the pointer to the section parent of the segment containing *x*. 
        Because a 64 bit pointer cannot safely be represented as a 
        double this function is deprecated in favor of :hoc:meth:`SectionRef.parent`.

         

----



.. hoc:function:: parent_node


    Syntax:
        ``parent_node(x)``


    Description:
        Return the pointer of the parent of the segment containing *x*. 

    .. warning::
        This function is useless and currently returns an error. 

         

----



.. hoc:function:: parent_connection


    Syntax:
        ``y = parent_connection()``


    Description:
        Return location on parent that currently accessed section is 
        connected to. (0 <= x <= 1). This is the value, y, used in 

        .. code-block::
            none

                    connect child(x), parent(y) 


         

----



.. hoc:function:: section_orientation


    Syntax:
        ``y = section_orientation()``


    Description:
        Return the end (0 or 1) which connects to the parent. This is the 
        value, x, used in 
         

        .. code-block::
            none

                    connect child(x), parent(y) 


         
      
