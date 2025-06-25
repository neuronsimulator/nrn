.. _pwman:

         
PWManager
---------



.. class:: PWManager

    .. tab:: Python
    
    
        Syntax:
            ``p = n.PWManager()``


        Description:
            A variety of hooks into the :ref:`pwm` to allow program control 
            of that tool. The implementation of the 
            Window item of the NeuronMainMenu makes 
            heavy use of this class. Note that the first window created is called 
            the leader. It cannot be closed. 

         

    .. tab:: HOC


        Syntax:
            ``p = new PWManager()``
        
        
        Description:
            A variety of hooks into the :ref:`pwm` to allow program control
            of that tool. The implementation of the 
            Window item of the NeuronMainMenu makes 
            heavy use of this class. Note that the first window created is called 
            the leader. It cannot be closed. 
        
----



.. method:: PWManager.count

    .. tab:: Python
    
    
        Syntax:
            ``cnt = p.count()``


        Description:
            Returns number of "Printable" windows on the screen. 

        Example:

        .. code-block::
            python

            from neuron import n, gui
            p = n.PWManager()
            print(p.count())


    .. tab:: HOC


        Syntax:
            ``cnt = p.count()``
        
        
        Description:
            Returns number of "Printable" windows on the screen. 
        
----



.. method:: PWManager.is_mapped

    .. tab:: Python
    
    
        Syntax:
            ``boolean = p.is_mapped(index)``


        Description:
            Return 1 if the index'th window is visible. 

        Example:

        .. code-block::
            python

            from neuron import n, gui
            p = n.PWManager()
            # not mapped
            print(p.is_mapped(1))
            p.map(1)
            # mapped
            print(p.is_mapped(1))



    .. tab:: HOC


        Syntax:
            ``boolean = p.is_mapped(index)``
        
        
        Description:
            Return 1 if the index'th window is visible. 
        
----



.. method:: PWManager.map

    .. tab:: Python
    
    
        Syntax:
            ``p.map(index)``


        Description:
            Makes the index'th window visible. 

        Example:

        .. code-block::
            python
        
            from neuron import n, gui
            p = n.PWManager()
            # mapped
            p.map(1)


    .. tab:: HOC


        Syntax:
            ``p.map(index)``
        
        
        Description:
            Makes the index'th window visible. 
        
----



.. method:: PWManager.hide

    .. tab:: Python
    
    
        Syntax:
            ``p.hide(index)``


        Description:
            Unmaps the index'th window. The window is NOT closed.

        Example:

        .. code-block::
            python
        
            from neuron import n, gui
            p = n.PWManager()
            # mapped
            p.map(1)
            print(p.is_mapped(1))
            # not mapped 
            p.hide(1)
            print(p.is_mapped(1))


    .. tab:: HOC


        Syntax:
            ``p.hide(index)``
        
        
        Description:
            Unmaps the index'th window. The window is NOT closed. 
        
----



.. method:: PWManager.close

    .. tab:: Python
    
    
        Syntax:
            ``p.close(index)``


        Description:
            Closes the index'th window. This will destroy the window and decrement the 
            reference count of the associated hoc object (if any). 

        Example:

        .. code-block::
            python
        
            from neuron import n, gui
            p = n.PWManager()
            p.map(1)
            p.close(1)


    .. tab:: HOC


        Syntax:
            ``p.close(index)``
        
        
        Description:
            Closes the index'th window. This will destroy the window and decrement the 
            reference count of the associated hoc object (if any). 
        
----



.. method:: PWManager.iconify

    .. tab:: Python
    
    
        Syntax:
            ``p.iconify()``


        Description:
            Hides all windows and iconifies the leader. 

        Example:

        .. code-block::
            python
        
            from neuron import n, gui
            p = n.PWManager()
            p.map(1)
            p.iconify()


    .. tab:: HOC


        Syntax:
            ``p.iconify()``
        
        
        Description:
            Hides all windows and iconifies the leader. 
        
----



.. method:: PWManager.deiconify

    .. tab:: Python
    
    
        Syntax:
            ``p.deiconify()``


        Description:
            Un-iconifies the leader window and maps any windows not hidden before it was 
            iconified. 


    .. tab:: HOC


        Syntax:
            ``p.deiconify()``
        
        
        Description:
            Un-iconifies the leader window and maps any windows not hidden before it was 
            iconified. 
        
----



.. method:: PWManager.leader

    .. tab:: Python
    
    
        Syntax:
            ``index = p.leader()``


        Description:
            Window index of the leader window. 


    .. tab:: HOC


        Syntax:
            ``index = p.leader()``
        
        
        Description:
            Window index of the leader window. 
        
----



.. method:: PWManager.manager

    .. tab:: Python
    
    
        Syntax:
            ``index = p.manager()``


        Description:
            Window index of the :ref:`PWM` window. 

    
        Example:

        .. code-block::
            python

            from neuron import n, gui
            p = n.PWManager()
            print(p.manager())



    .. tab:: HOC


        Syntax:
            ``index = p.manager()``
        
        
        Description:
            Window index of the :ref:`PWM` window.
        
----



.. method:: PWManager.save

    .. tab:: Python
    
    
        Syntax:
            ``m = p.save("filename", group_object, ["header"])``

            ``m = p.save("filename", selected, ["header"])``


        Description:
            Create a session file with the given filename 
            consisting oo all windows associated with a 
            particular group_object in a session file 
         
            If selected == 0 then all windows are saved. If selected==1 then only 
            the windows on the paper icon are saved in the session file. 
         
            If the header argument exists, it is copied to the beginning of the file. 

        .. seealso::
            :func:`save_session`

        Example:

        .. code-block::
            python

            from neuron import n, gui
            p = n.PWManager()
            p.map(1)
            selected = 1
            m = p.save("file", selected, "Header")


    .. tab:: HOC


        Syntax:
            ``n = p.save("filename", group_object, ["header"])``
        
        
            ``n = p.save("filename", selected, ["header"])``
        
        
        Description:
            Create a session file with the given filename 
            consisting oo all windows associated with a 
            particular group_object in a session file 
        
        
            If selected == 0 then all windows are saved. If selected==1 then only 
            the windows on the paper icon are saved in the session file. 
        
        
            If the header argument exists, it is copied to the beginning of the file. 
        
        
        .. seealso::
            :func:`save_session`
        
----



.. method:: PWManager.group

    .. tab:: Python
    
    
        Syntax:
            ``group_obj = p.group(index, group_obj)``

            ``group_obj = p.group(index)``


        Description:
            Associate the index'th window with the group object and returns the 
            group object associated with that window. 

        Example:

        .. code-block::
            python

            from neuron import n, gui
            p = n.PWManager()
            g1 = p.group(0)
            g2 = p.group(1, g1)

    .. tab:: HOC


        Syntax:
            ``group_obj = p.group(index, group_obj)``
        
        
            ``group_obj = p.group(index)``
        
        
        Description:
            Associate the index'th window with the group object and returns the 
            group object associated with that window. 
        
----



.. method:: PWManager.snap

    .. tab:: Python
    
    
        Syntax:
            ``p.snap()``

            ``p.snap("filename")``


        Description:
            Only works on the unix version. 
            Puts the GUI in snapshot mode until the 'p' keyboard character is pressed. 
            During this time the mouse can be used normally to pop up menus or drag 
            rubberbands on graphs. When the p character is pressed all windows including 
            drawings of the window decorations, menus, rubberband, and mouse arrow cursor is 
            printed to a postscript file with the "filename" or filebrowser selection. 

        Example:

        .. code-block::
            python

            from neuron import n, gui
            p = n.PWManager()
            p.snape("filename")

    .. tab:: HOC


        Syntax:
            ``p.snap()``
        
        
            ``p.snap("filename")``
        
        
        Description:
            Only works on the unix version. 
            Puts the GUI in snapshot mode until the 'p' keyboard character is pressed. 
            During this time the mouse can be used normally to pop up menus or drag 
            rubberbands on graphs. When the p character is pressed all windows including 
            drawings of the window decorations, menus, rubberband, and mouse arrow cursor is 
            printed to a postscript file with the "filename" or filebrowser selection. 
        
----



.. method:: PWManager.scale

    .. tab:: Python
    
    
        Syntax:
            ``p.scale(x)``


        Description:
            Works only under mswin. 
            Immediately rescales all the windows (including font size) and their position 
            relative to the top, left corner of the screen according to the absolute 
            scale factor x. 
            i.e, a scale value of 1 gives normal size windows. 

        Example:

        .. code-block::
            python

            from neuron import n, gui
            p = n.PWManager()
            p.scale(2)
    .. tab:: HOC


        Syntax:
            ``p.scale(x)``
        
        
        Description:
            Works only under mswin. 
            Immediately rescales all the windows (including font size) and their position 
            relative to the top, left corner of the screen according to the absolute 
            scale factor x. 
            i.e, a scale value of 1 gives normal size windows. 
        
----



.. method:: PWManager.name

    .. tab:: Python
    
    
        Syntax:
            ``strdef = p.name(index)``


        Description:
            Returns the window title bar string of the index'th window. 

        Example:

        .. code-block::
            python

            from neuron import n, gui
            p = n.PWManager()
            print(p.name(0))

         

    .. tab:: HOC


        Syntax:
            ``strdef = p.name(index)``
        
        
        Description:
            Returns the window title bar string of the index'th window. 
        
----



.. method:: PWManager.window_place

    .. tab:: Python
    
    
        Syntax:
            ``p.window_place(index, left, top)``


        Description:
            moves the index window to the left,top pixel 
            coordinates of the screen. 

        Example:

        .. code-block::
            python

            from neuron import n, gui
            p = n.PWManager()
            p.window_place(0, 1000, 1000)

         

    .. tab:: HOC


        Syntax:
            ``p.window_place(index, left, top)``
        
        
        Description:
            moves the index window to the left,top pixel 
            coordinates of the screen. 
        
----



.. method:: PWManager.paper_place

    .. tab:: Python
    
    
        Syntax:
            ``p.paper_place(index, show)``

            ``p.paper_place(index, left, bottom, scale)``


        Description:
            Shows or hides the ith window on the 
            paper icon. If showing, this constitutes adding this window to the list of 
            selected windows. 
         
            The 4 arg form shows, places, and scales 
            the index window on the paper icon. The scale and location only has an effect when 
            the paper is printed in postscript mode. 

         

    .. tab:: HOC


        Syntax:
            ``p.paper_place(index, show)``
        
        
            ``p.paper_place(index, left, bottom, scale)``
        
        
        Description:
            Shows or hides the ith window on the 
            paper icon. If showing, this constitutes adding this window to the list of 
            selected windows. 
        
        
            The 4 arg form shows, places, and scales 
            the index window on the paper icon. The scale and location only has an effect when 
            the paper is printed in postscript mode. 
        
----



.. method:: PWManager.landscape

    .. tab:: Python
    
    
        Syntax:
            ``p.landscape(boolean)``


        Description:
            Determines if postscript printing is in landscape 
            or portrait mode. 

         

    .. tab:: HOC


        Syntax:
            ``p.landscape(boolean)``
        
        
        Description:
            Determines if postscript printing is in landscape 
            or portrait mode. 
        
----



.. method:: PWManager.deco

    .. tab:: Python
    
    
        Syntax:
            ``p.deco(mode)``


        Description:
            When printing in postscript mode, 

            .. list-table:: Mode values for postscript printing
                :header-rows: 1

                * - Mode
                  - Description
                * - 0
                  - Print only the interior of the window.
                * - 1
                  - Print the interior and the title above each window.
                * - 2
                  - Print the interior and all window decorations including the window title.


    .. tab:: HOC


        Syntax:
            ``p.deco(mode)``
        
        
        Description:
            When printing in postscript mode, 

            .. list-table:: Mode values for postscript printing
                :header-rows: 1

                * - Mode
                  - Description
                * - 0
                  - Print only the interior of the window.
                * - 1
                  - Print the interior and the title above each window.
                * - 2
                  - Print the interior and all window decorations including the window title.
        
----



.. method:: PWManager.printfile

    .. tab:: Python
    
    
        Syntax:
            ``p.printfile("filename", mode, selected)``


        Description:
            Print to a file in postcript, idraw, or ascii mode (mode=0,1,2) the selected windows 
            or all the windows( selected=0,1) 

         
         

    .. tab:: HOC


        Syntax:
            ``p.printfile("filename", mode, selected)``
        
        
        Description:
            Print to a file in postcript, idraw, or ascii mode (mode=0,1,2) the selected windows 
            or all the windows( selected=0,1) 
        
