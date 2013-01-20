.. _seclist:

SectionList
-----------



.. class:: SectionList


    Syntax:
        :code:`sl = new SectionList()`


    Description:
        Class for creating and managing a list of sections 

    .. seealso::
        :meth:`RangeVarPlot.SectionBrowser`, :func:`Shape`, :func:`forsec`, :func:`list`

         

----



.. method:: SectionList.append


    Syntax:
        :code:`sl.append()`


    Description:
        append the currently accessed section to the list 

         

----



.. method:: SectionList.remove


    Syntax:
        :code:`n = sl.remove()`

        :code:`n = sl.remove(sectionlist)`


    Description:
        remove the currently accessed section from the list 
        If the argument is present then all the sections in sectionlist are 
        removed from sl. 
        Returns the number of sections removed. 

         

----



.. method:: SectionList.children


    Syntax:
        :code:`sl.children()`


    Description:
        Appends the sections connected to the currently accessed section. 
        Note that this includes children connected at position 0 of 
        parent. 

         

----



.. method:: SectionList.subtree


    Syntax:
        :code:`sl.subtree()`


    Description:
        Appends the subtree of the currently accessed section (including that one). 

         

----



.. method:: SectionList.wholetree


    Syntax:
        :code:`sl.wholetree()`


    Description:
        Appends all sections which have a path to the currently accessed section 
        (including the currently accessed section). The section list has the 
        important property that the sections are in root to leaf order. 

         

----



.. method:: SectionList.allroots


    Syntax:
        :code:`sl.allroots()`


    Description:
        Appends all the root sections. Root sections have no parent section. 
        The number of root sections is the number 
        of real cells in the simulation. 

         

----



.. method:: SectionList.unique


    Syntax:
        :code:`n = sl.unique()`


    Description:
        Removes all duplicates of sections in the SectionList. I.e. ensures that 
        no section appears more than once. Returns the number of sections references 
        that were removed. 

         

----



.. method:: SectionList.printnames


    Syntax:
        :code:`.printnames()`


    Description:
        print the names of the sections in the list. 
         
        The normal usage of a section list involves efficiently iterating 
        over all the sections in the list with 
        forsec sectionlist {statement} 


