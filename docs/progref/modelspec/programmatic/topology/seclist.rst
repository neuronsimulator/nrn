.. _seclist:

SectionList
-----------



.. class:: SectionList

    .. tab:: Python

        Syntax:

        .. code-block::
            python

            sl = n.SectionList()
            sl = n.SectionList(python_iterable_of_sections)

        Description:
            Class for creating and managing a list of sections. Unlike a regular list, a ``SectionList`` allows including sections
            based on neuronal morphology (e.g. subtrees).

        Example Usage:

            If ``sl`` is a :class:`SectionList`, then to turn that into a Python list, use ``py_list = list(sl)``; note
            that iterating over a SectionList is supported, so it may not be neccessary to create a Python list.

            The second syntax creates a SectionList from the Python iterable and is equivalent
            to:

            .. code-block::
                python

                sl = n.SectionList()
                for sec in python_iterable_of_sections:
                    sl.append(sec)

            ``len(sl)`` returns the number of sections in the SectionList.

            ``list(sl)`` and ``[s for s in sl]`` generate equivalent lists.


    .. tab:: HOC

        Syntax:

        .. code-block::
            C++

            sl = new SectionList()

        Description:
            Class for creating and managing a list of sections. Unlike a regular list, a ``SectionList`` allows including sections
            based on neuronal morphology (e.g. subtrees).

        Example Usage:

        .. code-block::
            C++

            create soma, apic, dend[5]
            secs_with_extra_IA = new SectionList()
            soma secs_with_extra_IA.append()
            apic secs_with_extra_IA.append()
            dend[3] secs_with_extra_IA.append()

    
    .. tab:: MATLAB

        Syntax:

        .. code-block::
            matlab

            sl = n.SectionList();

        Description:
            Class for creating and managing a list of sections. Unlike a regular list, a ``SectionList`` allows including sections
            based on neuronal morphology (e.g. subtrees).

        Example Usage:

        .. code-block::
            matlab

            sl = n.SectionList();
            soma = n.Section('soma');
            dend = n.Section('dend');
            sections = {soma, dend};
            
            for i = 1:numel(sections)
                sl.append(sections{i});
            end

            sl_secs = n.allsec(sl);
            for i = 1:numel(sl_secs)
                sec = sl_secs{i};
                disp(sec.name);
            end

    .. seealso::
        :class:`SectionBrowser`, :class:`Shape`, :meth:`RangeVarPlot.list`

         

----



.. method:: SectionList.append

    .. tab:: Python

        Syntax:

        .. code-block::
            python

            sl.append(section)
            sl.append(sec=section)

        Description:
            Append ``section`` to the list.

    .. tab:: HOC

        Syntax:
        
        .. code-block::
            C++

            sl.append()
            section {sl.append()}


        Description:
            Append the currently accessed section to the list.
            The syntax ``section {sl.append()}`` runs the append
            method with ``section`` being the currently accessd
            function.

    .. tab:: MATLAB

        Syntax:
        
        .. code-block::
            matlab

            sl.append(section);

        Description:
            Append ``section`` to the list.


.. method:: SectionList.remove

    .. tab:: Python

        Syntax:

        .. code-block::
            python

            n = sl.remove(sec=section)
            n = sl.remove(sectionlist)

        Description:
            Remove ``section`` from the list.

            If ``sectionlist`` is present then all the sections in sectionlist are 
            removed from sl. 

            Returns the number of sections removed. 

    .. tab:: HOC

        Syntax:

        .. code-block::
            C++

            sl.remove()
            sl.remove(sectionlist)

        Description:
            Remove the currently accessed section from the list.

            If ``sectionlist`` is present then all the sections in sectionlist are 
            removed from sl.

            Returns the number of sections removed.

    .. tab:: MATLAB

        Syntax:

        .. code-block::
            matlab

            n = sl.remove(section);
            n = sl.remove(sectionlist);

        Description:
            Remove ``section`` from the list.

            If ``sectionlist`` is present then all the sections in sectionlist are 
            removed from sl.

            Returns the number of sections removed.

----


.. method:: SectionList.children

    .. tab:: Python

        Syntax:

        .. code-block::
            python

            sl.children(section)
            sl.children(sec=section)

        Description:
            Appends the sections connected to ``section``. 
            Note that this includes children connected at position 0 of 
            parent. 

        .. note::

            To get a (Python) list of a section's children, use the section's
            ``children`` method. For example:

            .. code::
                python

                >>> from neuron import n
                >>> s = n.Section('s')
                >>> t = n.Section('t')
                >>> u = n.Section('u')
                >>> t.connect(s)
                t
                >>> u.connect(s)
                u
                >>> t.children()
                []
                >>> s.children()
                [u, t]

    .. tab:: HOC

        Syntax:

        .. code-block::
            C++

            section sl.children()

        Description:
            Appends the sections connected to ``section``. 
            If no ``section`` is specified, it defaults to the currently accessed section.
            Note that this includes children connected at position 0 of 
            parent.

    .. tab:: MATLAB

        Syntax:

        .. code-block::
            matlab

            sl.children(section);

        Description:
            Appends the sections connected to ``section``. 
            Note that this includes children connected at position 0 of 
            parent.

----


.. method:: SectionList.subtree

    .. tab:: Python

        Syntax:

        .. code-block::
            python

            sl.subtree(section)
            sl.subtree(sec=section)

        Description:
            Appends the subtree of the ``section`` (including that one).

        .. note::

            To get a (Python) list of a section's subtree, use the section's
            ``subtree`` method.         

    .. tab:: HOC

        Syntax:

        .. code-block::
            C++

            section sl.subtree()

        Description:
            Appends the subtree of the ``section`` (including that one).
            If no ``section`` is specified, it defaults to the currently accessed section.  

    .. tab:: MATLAB

        Syntax:

        .. code-block::
            matlab

            sl.subtree(section);

        Description:
            Appends the subtree of the ``section`` (including that one).

----


.. method:: SectionList.wholetree

    .. tab:: Python

        Syntax:

        .. code-block::
            python

            sl.wholetree(section)
            sl.wholetree(sec=section)

        Description:
            Appends all sections which have a path to the ``section``. 
            (including the specified section). The section list has the 
            important property that the sections are in root to leaf order. 

        .. note::

            To get a (Python) list of a section's wholetree, use the section's
            ``wholetree`` method. 

        .. seealso::
            :meth:`Section.wholetree`

    .. tab:: HOC

        Syntax:

        .. code-block::
            C++

            section sl.wholetree()

        Description:
            Appends all sections which have a path to the ``section``. 
            (including the specified section). 
            If no ``section`` is specified, it defaults to the currently accessed section.
            The section list has the 
            important property that the sections are in root to leaf order. 

    .. tab:: MATLAB

        Syntax:

        .. code-block::
            matlab

            sl.wholetree(section);

        Description:
            Appends all sections which have a path to the ``section``. 
            (including the specified section). The section list has the 
            important property that the sections are in root to leaf order. 
        
        Example:

            .. code-block::
                matlab

                sl = n.SectionList();
                soma = n.Section('soma');
                dend = n.Section('dend');
                soma.connect(dend);
                sl.wholetree(soma);

                % the following prints both soma and dend
                for sec = n.allsec(sl)
                    disp(sec{1}.name);
                end

----


.. method:: SectionList.allroots

    .. tab:: Python

        Syntax:

        .. code-block::
            python

            sl.allroots()

        Description:
            Appends all the root sections. Root sections have no parent section. 
            The number of root sections is the number 
            of real cells in the simulation. 

    .. tab:: HOC

        Syntax:

        .. code-block::
            C++

            sl.allroots()

        Description:
            Appends all the root sections. Root sections have no parent section. 
            The number of root sections is the number 
            of real cells in the simulation. 

    .. tab:: MATLAB

        Syntax:

        .. code-block::
            matlab

            sl.allroots();

        Description:
            Appends all the root sections. Root sections have no parent section. 
            The number of root sections is the number 
            of real cells in the simulation. 

----


.. method:: SectionList.unique

    .. tab:: Python

        Syntax:

        .. code-block::
            python

            n = sl.unique()

        Description:
            Removes all duplicates of sections in the SectionList. I.e. ensures that 
            no section appears more than once. Returns the number of sections references 
            that were removed. 

    .. tab:: HOC

        Syntax:

        .. code-block::
            C++

            n = sl.unique()

        Description:
            Removes all duplicates of sections in the SectionList. I.e. ensures that 
            no section appears more than once. Returns the number of sections references 
            that were removed. 

    .. tab:: MATLAB

        Syntax:

        .. code-block::
            matlab

            n = sl.unique();

        Description:
            Removes all duplicates of sections in the SectionList. i.e., ensures that 
            no section appears more than once. Returns the number of sections references 
            that were removed. 

----


.. method:: SectionList.printnames

    .. tab:: Python

        Syntax:

        .. code-block::
            python

            sl.printnames()

        Description:
            Print the names of the sections in the list. 

            ``sl.printnames()`` is approximately equivalent to:

            .. code::
                python

                for sec in sl:
                    print(sec)

    .. tab:: HOC

        Syntax:

        .. code-block::
            C++

            sl.printnames()

        Description:
            Print the names of the sections in the list.

    .. tab:: MATLAB

        Syntax:

        .. code-block::
            matlab

            sl.printnames();

        Description:
            Print the names of the sections in the list.

            ``sl.printnames()`` is approximately equivalent to:
            
            .. code-block::
                matlab

                secs = n.allsec(sl);
                for i = 1:numel(secs)
                    disp(secs{i}.name);
                end


