.. _pwman:

         
PWManager
---------



.. class:: h.PWManager()


    A variety of hooks into the :ref:`pwm` to allow program control 
    of that tool. The implementation of the 
    Window item of the NeuronMainMenu makes 
    heavy use of this class. Note that the first window created is called 
    the leader. It cannot be closed. 

         

----



.. method:: PWManager.count()


    Returns number of "Printable" windows on the screen. 

    Example:

    .. code-block::
        python

        from neuron import h, gui
        p = h.PWManager()
        print(p.count())


----



.. method:: PWManager.is_mapped(index)


    Return 1 if the index'th window is visible. 

    Example:

    .. code-block::
        python

        from neuron import h, gui
        p = h.PWManager()
        # not mapped
        print(p.is_mapped(1))
        p.map(1)
        # mapped
        print(p.is_mapped(1))



----



.. method:: PWManager.map(index)


    Makes the index'th window visible. 

    Example:

    .. code-block::
        python
        
        from neuron import h, gui
        p = h.PWManager()
        # mapped
        p.map(1)


----



.. method:: PWManager.hide(index)


   
    Unmaps the index'th window. The window is NOT closed.

    Example:

    .. code-block::
        python
        
        from neuron import h, gui
        p = h.PWManager()
        # mapped
        p.map(1)
        print(p.is_mapped(1))
        # not mapped 
        p.hide(1)
        print(p.is_mapped(1))


----



.. method:: PWManager.close(index)


    Closes the index'th window. This will destroy the window and decrement the 
    reference count of the associated hoc object (if any). 

    Example:

    .. code-block::
        python
        
        from neuron import h, gui
        p = h.PWManager()
        p.map(1)
        p.close(1)


----



.. method:: PWManager.iconify()


    Hides all windows and iconifies the leader. 

    Example:

    .. code-block::
        python
        
        from neuron import h, gui
        p = h.PWManager()
        p.map(1)
        p.iconify()


----



.. method:: PWManager.deiconify()


    Un-iconifies the leader window and maps any windows not hidden before it was 
    iconified. 


----



.. method:: PWManager.leader()


    Window index of the leader window. 


----



.. method:: PWManager.manager()


   
    Window index of the :ref:`PWM` window. 

    
    Example:

    .. code-block::
        python

        from neuron import h, gui
        p = h.PWManager()
        print(p.manager())



----



.. method:: PWManager.save("filename", group_object, ["header"])
            PWManager.save("filename", selected, ["header"])

   
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

        from neuron import h, gui
        p = h.PWManager()
        p.map(1)
        selected = 1
        n = p.save("file", selected, "Header")


----



.. method:: PWManager.group(index, group_obj)
            PWManager.group(index)

    
    Associate the index'th window with the group object and returns the 
    group object associated with that window. 

    Example:

    .. code-block::
        python

        from neuron import h, gui
        p = h.PWManager()
        g1 = p.group(0)
        g2 = p.group(1, g1)

----



.. method:: PWManager.snap()
            PWManager.snap("filename")


    Only works on the unix version. 
    Puts the GUI in snapshot mode until the 'p' keyboard character is pressed. 
    During this time the mouse can be used normally to pop up menus or drag 
    rubberbands on graphs. When the p character is pressed all windows including 
    drawings of the window decorations, menus, rubberband, and mouse arrow cursor is 
    printed to a postscript file with the "filename" or filebrowser selection. 

    Example:

    .. code-block::
        python

        from neuron import h, gui
        p = h.PWManager()
        p.snape("filename")

----



.. method:: PWManager.jwindow(hoc_owner, mapORhide, x, y, w, h)


   
    Manipulate the position and size of a java window frame associated with the 
    java object referenced by the hoc object. The mapORhide value may be 0 
    or 1. The index of the window is returned. This is used by session file 
    statements created by the java object in order to specify window attributes. 


----



.. method:: PWManager.scale(x)


    
    Works only under mswin. 
    Immediately rescales all the windows (including font size) and their position 
    relative to the top, left corner of the screen according to the absolute 
    scale factor x. 
    i.e, a scale value of 1 gives normal size windows. 

    Example:

    .. code-block::
        python

        from neuron import h, gui
        p = h.PWManager()
        p.scale(2)
----



.. method:: PWManager.name(index)


    Returns the window title bar string of the index'th window. 

    Example:

    .. code-block::
        python

        from neuron import h, gui
        p = h.PWManager()
        print(p.name(0))

         

----



.. method:: PWManager.window_place(index, left, top)


   
    moves the index window to the left,top pixel 
    coordinates of the screen. 

    Example:

    .. code-block::
        python

        from neuron import h, gui
        p = h.PWManager()
        p.window_place(0, 1000, 1000)

         

----



.. method:: PWManager.paper_place(index, show)
            PWManager.paper_place(index, left, bottom, scale)

    
    Shows or hides the ith window on the 
    paper icon. If showing, this constitutes adding this window to the list of 
    selected windows. 
        
    The 4 arg form shows, places, and scales 
    the index window on the paper icon. The scale and location only has an effect when 
    the paper is printed in postscript mode. 

         

----



.. method:: PWManager.landscape(boolean)


    Determines if postscript printing is in landscape 
    or portrait mode. 

         

----



.. method:: PWManager.deco(mode)


    When printing in postscript mode, 
    0 print only the interior of the window. 
        
    1 print the interior and the title above each window 
        
    2 print the interior and all window decorations including the window title. 

         

----



.. method:: PWManager.printfile("filename", mode, selected)


    Print to a file in postcript, idraw, or ascii mode (mode=0,1,2) the selected windows 
    or all the windows( selected=0,1) 

         
         

