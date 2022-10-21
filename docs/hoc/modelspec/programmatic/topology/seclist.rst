
.. _hoc_seclist:

SectionList
-----------



.. hoc:class:: SectionList


    Syntax:
        ``sl = new SectionList()``


    Description:
        Class for creating and managing a list of sections 

    .. seealso::
        :hoc:class:`SectionBrowser`, :hoc:class:`Shape`, :ref:`forsec <hoc_keyword_forsec>`, :hoc:meth:`RangeVarPlot.list`

         

----



.. hoc:method:: SectionList.append


    Syntax:
        ``sl.append()``


    Description:
        append the currently accessed section to the list 

         

----



.. hoc:method:: SectionList.remove


    Syntax:
        ``n = sl.remove()``

        ``n = sl.remove(sectionlist)``


    Description:
        remove the currently accessed section from the list 
        If the argument is present then all the sections in sectionlist are 
        removed from sl. 
        Returns the number of sections removed. 

         

----



.. hoc:method:: SectionList.children


    Syntax:
        ``sl.children()``


    Description:
        Appends the sections connected to the currently accessed section. 
        Note that this includes children connected at position 0 of 
        parent. 

         

----



.. hoc:method:: SectionList.subtree


    Syntax:
        ``sl.subtree()``


    Description:
        Appends the subtree of the currently accessed section (including that one). 

         

----



.. hoc:method:: SectionList.wholetree


    Syntax:
        ``sl.wholetree()``


    Description:
        Appends all sections which have a path to the currently accessed section 
        (including the currently accessed section). The section list has the 
        important property that the sections are in root to leaf order. 

         

----



.. hoc:method:: SectionList.allroots


    Syntax:
        ``sl.allroots()``


    Description:
        Appends all the root sections. Root sections have no parent section. 
        The number of root sections is the number 
        of real cells in the simulation. 

         

----



.. hoc:method:: SectionList.unique


    Syntax:
        ``n = sl.unique()``


    Description:
        Removes all duplicates of sections in the SectionList. I.e. ensures that 
        no section appears more than once. Returns the number of sections references 
        that were removed. 

         

----



.. hoc:method:: SectionList.printnames


    Syntax:
        ``.printnames()``


    Description:
        print the names of the sections in the list. 
         
        The normal usage of a section list involves efficiently iterating 
        over all the sections in the list with 
        ``forsec sectionlist {statement}``


