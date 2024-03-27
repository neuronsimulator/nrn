.. _seclist:

SectionList
-----------



.. class:: SectionList


    Syntax:
        ``sl = h.SectionList()``

        ``sl = h.SectionList(python_iterable_of_sections)``


    Description:
        Class for creating and managing a list of sections. Unlike a regular Python list, a ``SectionList`` allows including sections
        based on neuronal morphology (e.g. subtrees).

        If ``sl`` is a :class:`SectionList`, then to turn that into a Python list, use ``py_list = list(sl)``; note
        that iterating over a SectionList is supported, so it may not be neccessary to create a Python list.

        The second syntax creates a SectionList from the Python iterable and is equivalent
        to:

        .. code-block::
            python

            sl = h.SectionList()
            for sec in python_iterable_of_sections:
                sl.append(sec)

    .. seealso::
        :class:`SectionBrowser`, :class:`Shape`, :meth:`RangeVarPlot.list`

         

----



.. method:: SectionList.append


    Syntax:
        ``sl.append(section)``
        
        ``sl.append(sec=section)``


    Description:
        append ``section`` to the list 

         

----



.. method:: SectionList.remove


    Syntax:
        ``n = sl.remove(sec=section)``

        ``n = sl.remove(sectionlist)``


    Description:
        Remove ``section`` from the list.

        If ``sectionlist`` is present then all the sections in sectionlist are 
        removed from sl. 

        Returns the number of sections removed. 

         

----



.. method:: SectionList.children


    Syntax:
        ``sl.children(section)``

        ``sl.children(sec=section)``


    Description:
        Appends the sections connected to ``section``. 
        Note that this includes children connected at position 0 of 
        parent. 
    
    .. note::

        To get a (Python) list of a section's children, use the section's
        ``children`` method. For example:

        .. code::
            python

            >>> from neuron import h
            >>> s = h.Section(name='s')
            >>> t = h.Section(name='t')
            >>> u = h.Section(name='u')
            >>> t.connect(s)
            t
            >>> u.connect(s)
            u
            >>> t.children()
            []
            >>> s.children()
            [u, t]

         

----



.. method:: SectionList.subtree


    Syntax:

        ``sl.subtree(section)``
    
        ``sl.subtree(sec=section)``


    Description:
        Appends the subtree of the ``section``. (including that one). 

    .. note::

        To get a (Python) list of a section's subtree, use the section's
        ``subtree`` method.         

    .. seealso::
        :meth:`Section.subtree`

----



.. method:: SectionList.wholetree


    Syntax:

        ``sl.wholetree(section)``

        ``sl.wholetree(sec=section)``


    Description:
        Appends all sections which have a path to the ``section``. 
        (including the specified section). The section list has the 
        important property that the sections are in root to leaf order. 

    .. note::

        To get a (Python) list of a section's wholetree, use the section's
        ``wholetree`` method. 

    .. seealso::
        :meth:`Section.wholetree`
         

----



.. method:: SectionList.allroots


    Syntax:
        ``sl.allroots()``


    Description:
        Appends all the root sections. Root sections have no parent section. 
        The number of root sections is the number 
        of real cells in the simulation. 

         

----



.. method:: SectionList.unique


    Syntax:
        ``n = sl.unique()``


    Description:
        Removes all duplicates of sections in the SectionList. I.e. ensures that 
        no section appears more than once. Returns the number of sections references 
        that were removed. 

         

----



.. method:: SectionList.printnames


    Syntax:
        ``.printnames()``


    Description:
        print the names of the sections in the list. 

        ``sl.printnames()`` is approximately equivalent to:

        .. code::
            python

            for sec in sl:
                print(sec)
         

