.. _seclist:

SectionList
-----------



.. class:: SectionList


    Syntax:
        ``sl = h.SectionList()``


    Description:
        Class for creating and managing a list of sections. Unlike a regular Python list, a ``SectionList`` allows including sections
        based on neuronal morphology (e.g. subtrees).

        If ``sl`` is a :class:`SectionList`, then to turn that into a Python list, use ``py_list = [sec for sec in sl]``; note
        that iterating over a SectionList is supported, so it may not be neccessary to create a Python list.

        To turn a Python list ``py_list`` of Sections into a :class:`SectionList`, use:

        .. code-block::
            python

            sl = h.SectionList()
            for sec in py_list:
                sl.append(sec=sec)

    .. seealso::
        :class:`SectionBrowser`, :class:`Shape`, :meth:`RangeVarPlot.list`

         

----



.. method:: SectionList.append


    Syntax:
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
        ``sl.children(sec=section)``


    Description:
        Appends the sections connected to ``section``. 
        Note that this includes children connected at position 0 of 
        parent. 

         

----



.. method:: SectionList.subtree


    Syntax:
        ``sl.subtree(sec=section)``


    Description:
        Appends the subtree of the ``section``. (including that one). 

         

----



.. method:: SectionList.wholetree


    Syntax:
        ``sl.wholetree(sec=section)``


    Description:
        Appends all sections which have a path to the ``section``. 
        (including the currently accessed section). The section list has the 
        important property that the sections are in root to leaf order. 

         

----



.. method:: SectionList.allroots


    Syntax:
        ``sl.allroots(sec=section)``


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
         

