.. _topology:

         
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

.. class:: Section

    Syntax:
        .. code::

            dend = n.Section()
            dend = n.Section('dend')
            dend = n.Section(cell=mycell)
            dend = n.Section('dend', cell=mycell)

    Description:
        Creates a new section. If no cell argument is specified, the name argument (optional) will be returned via ``str(s)`` or ``s.hname()``; if no name is provided, one will be automatically generated.
        If a cell argument is passed, its repr will be combined with the name to form ``str(s)``.

    Example 1:

        .. code::

            soma = n.Section('soma')
            axon = n.Section('axon')
            dend = [n.Section(f'dend[{i}]') for i in range(3)]
            for sec in n.allsec():
                print(sec)


        prints the names of all the sections which have been created:

        .. code-block::
            none

            soma 
            axon 
            dend[0] 
            dend[1] 
            dend[2] 

    Example 2:

        .. code::

            import itertools

            class MyCell:
                _ids = itertools.count(0)
                def __repr__(self):
                    return f'MyCell[{self.id}]'
                def __init__(self):
                    self.id = self._ids.next()
                    # create the morphology and connect it
                    self.soma = n.Section('soma', cell=self)
                    self.dend = n.Section('dend', cell=self)
                    self.dend.connect(self.soma(0.5))

            # create two cells
            my_cells = [MyCell(), MyCell()]

            # print the topology
            n.topology()
            
        Displays:

            .. code-block::
                none

                |-|       MyCell[0].soma(0-1)
                  `|       MyCell[0].dend(0-1)
                |-|       MyCell[1].soma(0-1)
                  `|       MyCell[1].dend(0-1)

    .. seealso::
    
        :meth:`Section.connect`, :meth:`Section.insert`, :func:`allsec`
        

         

----



.. method:: Section.connect


    Syntax:
        ``child.connect(parent, [0 or 1])``

        ``child.connect(parent(x), [0 or 1])``



    Description:
        The first form connects the child at end 0 or 1 to the parent
        section at position x. By default the child end 0 connects to the parent end 1.
        An alternative syntax is the second 
        form in which the location on the parent section is indicated.  If a section 
        is connected twice a Notice is printed on the standard error device 
        saying that the section has been reconnected (the last connection takes 
        precedence).  To avoid the notice, disconnect the section first with the 
        function :func:`disconnect`.  If sections are inadvertently connected in a 
        loop, an error will be generated when the internal data structures are 
        created and the user will be required to disconnect one of the sections 
        forming the loop. 
         

    Example:

        .. code::

            from neuron import n, gui
            soma = n.Section('soma')
            axon = n.Section('axon')
            dend = [n.Section(f'dend[{i}]') for i in range(3)]
            for sec in dend:
                sec.connect(soma(1), 0)

            n.topology()
            s = n.Shape()

        .. image:: ../../images/section-connection.png
            :align: center

----



.. method:: Section.disconnect


    Syntax:
        ``section.disconnect()``

    Description:
        Disconnect the section. The section becomes the root of its subtree.

    Example:

        .. code::

            from neuron import n
            sl = [n.Section(f"s_{i}") for i in range(4)]
            for i, sec in enumerate(sl[1:]):
                sec.connect(sl[i](1))

            n.topology()
            sl[2].disconnect()
            n.topology()
            sl[2].connect(sl[0](.5), 1)
            n.topology()
            sl[2].disconnect()
            n.topology()
            sl[2].connect(sl[0](.5))
            n.topology()

----


.. data:: Section.nseg

    Syntax:
        ``section.nseg``

    Description:
        Number of segments (compartments) in ``section``. 
        When a section is created, nseg is 1. 
        In versions prior to 3.2, changing nseg throws away all 
        "inserted" mechanisms including diam 
        (if 3-d points do not exist). PointProcess, connectivity, L, and 3-d 
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
        interpolated since those values are computed with :func:`fadvance`). 
        If range variables are not constant then the hoc expressions used to 
        set them should be re-executed. 
         
        If nseg is decreased then all the new segments are in fact those old 
        segments that were nearest the centers of the new segments. Unused old 
        segments are freed (and thus any existing pointers to variables in those 
        freed segments are invalid). This means that decreasing nseg by an odd 
        factor preserves the locations of all previous data.

        POINT PROCESSes are preserved regardless of how nseg is changed.
        However, any POINT PROCESS that was attached to a location other
        than 0 or 1 will be moved to the center of the "new segment" that
        is nearest to the "old segment" to which it was attached.  The same
        rule applies to child sections that had been attached to locations
        other than 0 or 1.
         
        The intention is to guarantee that the following sequence 

        .. code::

            run() # sim1 
            for sec in n.allsec():
                sec.nseg *= oddfactor
            run() # sim2 
            for sec in n.allsec():
                sec.nseg /= oddfactor
            run() # sim3 

        will produce identical simulations for sim1 and sim3. And sim2 will be 
        oddfactor^2 more accurate with regard to spatial discretization error. 

----

.. method:: Section.orientation

    Syntax:
        ``y = section.orientation()``

    Description:
        Return the end (0 or 1) which connects to the parent. This is the 
        value, y, used in 
         
        .. code::

            child.connect(parent(x), y)

----

.. method:: Section.parentseg

    Syntax:
        ``seg = child.parentseg()``

    Description:
        Return the parent segment of the ``child`` section. This is ``parent(x)`` in:

        .. code::

            child.connect(parent(x), y)

        To get the x value, use ``seg.x``.

----

.. method:: Section.cell

    Syntax:
        ``section.cell()``

    Description:
        Returns the value of the cell keyword argument provided when the Section was created.

----

.. method:: Section.hname

    Syntax:
        ``section.hname()``

    Description:
        Returns the value of the name keyword argument provided when the Section was created.
        If no name was provided, the internally provided name is returned instead.

----

.. method:: Section.name

    Syntax:
        ``section.name()``

    Description:
        Same as :meth:`Section.hname`

----

.. method:: Section.subtree()

    Syntax:
        ``section.subtree()``

    Description:
        Returns a Python list of the sub-tree of the Section

    Example:
        .. code-block::
            python

            >>> from neuron import n
            >>> soma = n.Section('soma')
            >>> dend1 = n.Section('dend1')
            >>> dend2 = n.Section('dend2')
            >>> dend3 = n.Section('dend3')
            >>> dend4 = n.Section('dend4')
            >>> dend5 = n.Section('dend5')
            >>> dend2.connect(soma)
            dend2
            >>> dend1.connect(soma)
            dend1
            >>> dend3.connect(dend2)
            dend3
            >>> dend4.connect(dend2)
            dend4
            >>> dend5.connect(dend4)
            dend5
            >>> n.topology()

            |-|       soma(0-1)
               `|       dend2(0-1)
                 `|       dend3(0-1)
                 `|       dend4(0-1)
                   `|       dend5(0-1)
                `|       dend1(0-1)

            1.0
            >>> dend2.subtree()
            [dend2, dend4, dend5, dend3]
            >>> dend7 = n.Section('dend7')
            >>> dend7.subtree()
            [dend7]
            >>> dend1.subtree()
            [dend1]
            >>> dend4.subtree()
            [dend4, dend5]
            >>> soma.subtree()
            [soma, dend1, dend2, dend4, dend5, dend3]

----

.. method:: Section.wholetree()

    Syntax:
        ``section.wholetree()``

    Description:
        Returns a Python list of the whole tree of the Section

    Example:
        .. code-block::
            python

            >>> from neuron import n
            >>> soma = n.Section('soma')
            >>> dend1 = n.Section('dend1')
            >>> dend2 = n.Section('dend2')
            >>> dend3 = n.Section('dend3')
            >>> dend4 = n.Section('dend4')
            >>> dend5 = n.Section('dend5')
            >>> dend2.connect(soma)
            dend2
            >>> dend1.connect(soma)
            dend1
            >>> dend3.connect(dend2)
            dend3
            >>> dend4.connect(dend2)
            dend4
            >>> dend5.connect(dend4)
            dend5
            >>> n.topology()

            |-|       soma(0-1)
               `|       dend2(0-1)
                 `|       dend3(0-1)
                 `|       dend4(0-1)
                   `|       dend5(0-1)
               `|       dend1(0-1)

            1.0
            >>> dend2.wholetree()
            [soma, dend1, dend2, dend4, dend5, dend3]
            >>> dend7 = n.Section('dend7')
            >>> dend7.wholetree()
            [dend7]
            >>> soma.wholetree()
            [soma, dend1, dend2, dend4, dend5, dend3]
            >>> dend3.wholetree()
            [soma, dend1, dend2, dend4, dend5, dend3]
----

.. function:: topology


    Syntax:
        ``n.topology()``


    Description:
        Print the topology of how the sections are connected together. 

         
         

----



.. function:: delete_section


    Syntax:
        ``n.delete_section(sec=sec)``


    Description:
        Delete the specified section ``sec`` from the main section
        list which is used in computation.

        .. code::

            for sec in n.allsec():
                n.delete_section(sec=sec)
 
        will remove all sections. 
         
        Note: deleted sections still exist (even though 
        :meth:`SectionRef.exists`
        returns 0 and an error will result if one attempts to access 
        the section) so 
        that other objects (such as :class:`SectionList`\ s and :class:`Shape`\ s) which 
        hold pointers to these sections will still work. When the last 
        pointer to a section is destroyed, the section memory will be 
        freed. 

    .. warning::

        If the ``sec`` argument is omitted, the currently accessed section is deleted instead.

         

----



.. function:: section_exists


    Syntax:
        ``boolean = n.section_exists("name", [index], [object])``


    Description:
        Returns 1.0 if the section defined by the args exists and can be used 
        as a currently accessed section. Otherwise, returns 0.0.
        The index is optional and if nonzero, can be incorporated into the name as 
        a literal value such as dend[25]. If the optional object arg is present, that 
        is the context, otherwise the context is the top level. "name" should 
        not contain the object prefix. Even if a section is multiply dimensioned, use 
        a single index value. 

    .. warning::

        This function does not work with Sections created in Python.
         

----



.. function:: section_owner


    Syntax:
        ``n.section_owner(sec=section)``


    Description:
        
        If ``section`` was created in Python, returns the ``cell`` keyword argument or
        None. This is accessible directly from the Section object via :meth:`Section.cell`.
        If the section was created in HOC, returns the object that created the section, or
        None if created at the top level.
         

----



.. function:: disconnect


    Syntax:
        ``n.disconnect(sec=section)``


    Description:
        Disconnect ``section`` from its parent. Such 
        a section can be reconnected with the connect method. The alternative
        :meth:`Section.disconnect` is recommended.

    .. warning::

        If no section is specified, will disconnect the currently accessed section.

----



.. function:: issection


    Syntax:
        ``n.issection("regular expression", sec=section)``


    Description:
        Return 1.0 if the name of ``section`` matches the regular expression. 
        Return 0.0 otherwise. 
         
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

        .. code::

            from neuron import n, gui
            soma = n.Section('soma')
            axon = n.Section('axon')
            dend = [n.Section(f'dend[{i}]') for i in range(3)]
            for section in n.allsec():
                if n.issection('s.*', sec=section):
                    print(section)

        will print ``soma`` 

        .. code::

            for section in n.allsec():
                if n.issection('d.*2]', sec=section):
                    print(section)

        will print ``dend[2]`` 

        .. code-block::
            none

            for section in n.allsec():
                if n.issection(".*a.*", sec=section):
                    print(section)

        will print all names which contain the letter "a" 

        .. code-block::
            none

            soma 
            axon 


    .. note::

        This can also be done using Python's ``re`` module and testing ``str(sec)`` 

    .. warning::

        If the ``sec`` keyword argument is omitted, this will operate on the currently accessed section.
         

----



.. function:: ismembrane


    Syntax:
        ``n.ismembrane("mechanism", sec=section)``


    Description:
        This function returns a 1.0 if the membrane of ``section`` contains this 
        (density) mechanism.  This is not for point 
        processes. 
         

    Example:

        .. code::


            for sec in n.allsec():
                if n.ismembrane('hh', sec=sec) and n.ismembrane('ca_ion', sec=sec):
                    print(sec)

        will print the names of all the sections which contain both Hodgkin-Huxley and Calcium ions. 

         
     .. warning::

        If the ``sec`` keyword argument is omitted, returns a result based on the currently accessed
        section.

----



.. function:: sectionname


    Syntax:
        ``n.sectionname(strvar, sec=section)``


    Description:
        The name of ``section`` is placed in *strvar*, a HOC string reference.
        Such a string reference may be created by: ``strvar = n.ref('')``; it's value is ``strvar[0]``.
         
        This function is superseded by the easier to use, ``str(section)``.
        

----



.. function:: secname


    Syntax:
        ``n.secname(sec=section)``


    Description:
        This function is superseded by the easier to use, ``str(section)``. The below examples
        can be more cleanly written as: ``s = str(soma)``, ``print(soma)``, and ``for sec in n.allsec(): for seg in sec: print(seg)``.

        Returns the name of ``section``. Usage is 

        .. code::

            s = n.secname(sec=soma)

        or 

        .. code::

            print(n.secname(sec=soma))

        or 

        .. code::


            for sec in n.allsec():
                for seg in sec:
                    print(f'{n.secname(sec=sec)}({seg.x})')  # same as print(seg)

         

----



.. function:: psection


    Syntax:
        ``n.psection(sec=section)``


    Description:
        Print info about ``section`` in a format which is executable in HOC. 
        (length, parent, diameter, membrane information) 
    
    .. note::

        Beginning in NEURON 7.6, ``section.psection()`` returns a Python dictionary
        with all the information displayed by n.psection and more (e.g.
        sec.psection() returns information about reaction-diffusion kinetics).
         



----


.. function:: parent_section


    Syntax:
        ``n.parent_section(x, sec=section)``


    Description:
        Return the pointer to the section parent of the segment ``section(x)``. 
        Because a 64 bit pointer cannot safely be represented as a 
        double this function is deprecated in favor of :meth:`SectionRef.parent`. 

    .. seealso::

        :meth:`Section.parentseg`
         

----



.. function:: parent_node


    Syntax:
        ``n.parent_node(x, sec=section)``


    Description:
        Return the pointer of the parent of the segment ``section(x)``. 

    .. warning::
        This function is useless and currently returns an error. 

         

----



.. function:: parent_connection


    Syntax:
        ``y = n.parent_connection(sec=child)``


    Description:
        Return location on parent that ``child`` is 
        connected to. (0 <= x <= 1). This is the value, y, used in 

        .. code::

            child.connect(parent(x), y)

        This information is also available via: ``child.parentseg().x``

    .. seealso::

        :meth:`Section.parentseg`


         

----



.. function:: section_orientation


    Syntax:
        ``y = n.section_orientation(sec=child)``

    Description:
        Return the end (0 or 1) which connects to the parent. This is the 
        value, y, used in 
         
        .. code::

            child.connect(parent(x), y)

    .. note::

        It is cleaner to use the equivalent section method: :meth:`Section.orientation`.
