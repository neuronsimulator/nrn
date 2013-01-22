.. _pwman:

         
PWManager
---------



.. class:: PWManager


    Syntax:
        :code:`p = new PWManager()`


    Description:
        A variety of hooks into the :ref:`pwm` to allow program control 
        of that tool. The implementation of the 
        Window item of the NeuronMainMenu makes 
        heavy use of this class. Note that the first window created is called 
        the leader. It cannot be closed. 

         

----



.. method:: PWManager.count


    Syntax:
        :code:`cnt = p.count()`


    Description:
        Returns number of "Printable" windows on the screen. 


----



.. method:: PWManager.is_mapped


    Syntax:
        :code:`boolean = p.is_mapped(index)`


    Description:
        Return 1 if the index'th window is visible. 


----



.. method:: PWManager.map


    Syntax:
        :code:`p.map(index)`


    Description:
        Makes the index'th window visible. 


----



.. method:: PWManager.hide


    Syntax:
        :code:`p.hide(index)`


    Description:
        Unmaps the index'th window. The window is NOT closed. 


----



.. method:: PWManager.close


    Syntax:
        :code:`p.close(index)`


    Description:
        Closes the index'th window. This will destroy the window and decrement the 
        reference count of the associated hoc object (if any). 


----



.. method:: PWManager.iconify


    Syntax:
        :code:`p.iconify()`


    Description:
        Hides all windows and iconifies the leader. 


----



.. method:: PWManager.deiconify


    Syntax:
        :code:`p.deiconify()`


    Description:
        Un-iconifies the leader window and maps any windows not hidden before it was 
        iconified. 


----



.. method:: PWManager.leader


    Syntax:
        :code:`index = p.leader()`


    Description:
        Window index of the leader window. 


----



.. method:: PWManager.manager


    Syntax:
        :code:`index = p.manager()`


    Description:
        Window index of the :ref:`PWM` window. 


----



.. method:: PWManager.save


    Syntax:
        :code:`n = p.save("filename", group_object, ["header"])`

        :code:`n = p.save("filename", selected, ["header"])`


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


    Syntax:
        :code:`group_obj = p.group(index, group_obj)`

        :code:`group_obj = p.group(index)`


    Description:
        Associate the index'th window with the group object and returns the 
        group object associated with that window. 


----



.. method:: PWManager.snap


    Syntax:
        :code:`p.snap()`

        :code:`p.snap("filename")`


    Description:
        Only works on the unix version. 
        Puts the GUI in snapshot mode until the 'p' keyboard character is pressed. 
        During this time the mouse can be used normally to pop up menus or drag 
        rubberbands on graphs. When the p character is pressed all windows including 
        drawings of the window decorations, menus, rubberband, and mouse arrow cursor is 
        printed to a postscript file with the "filename" or filebrowser selection. 


----



.. method:: PWManager.jwindow


    Syntax:
        :code:`index = p.jwindow(hoc_owner, mapORhide, x, y, w, h)`


    Description:
        Manipulate the position and size of a java window frame associated with the 
        java object referenced by the hoc object. The mapORhide value may be 0 
        or 1. The index of the window is returned. This is used by session file 
        statements created by the java object in order to specify window attributes. 


----



.. method:: PWManager.scale


    Syntax:
        :code:`p.scale(x)`


    Description:
        Works only under mswin. 
        Immediately rescales all the windows (including font size) and their position 
        relative to the top, left corner of the screen according to the absolute 
        scale factor x. 
        i.e, a scale value of 1 gives normal size windows. 


----



.. method:: PWManager.name


    Syntax:
        :code:`strdef = p.name(index)`


    Description:
        Returns the window title bar string of the index'th window. 

         

----



.. method:: PWManager.window_place


    Syntax:
        :code:`p.window_place(index, left, top)`


    Description:
        moves the index window to the left,top pixel 
        coordinates of the screen. 

         

----



.. method:: PWManager.paper_place


    Syntax:
        :code:`p.paper_place(index, show)`

        :code:`p.paper_place(index, left, bottom, scale)`


    Description:
        Shows or hides the ith window on the 
        paper icon. If showing, this constitutes adding this window to the list of 
        selected windows. 
         
        The 4 arg form shows, places, and scales 
        the index window on the paper icon. The scale and location only has an effect when 
        the paper is printed in postscript mode. 

         

----



.. method:: PWManager.landscape


    Syntax:
        :code:`p.landscape(boolean)`


    Description:
        Determines if postscript printing is in landscape 
        or portrait mode. 

         

----



.. method:: PWManager.deco


    Syntax:
        :code:`p.deco(mode)`


    Description:
        When printing in postscript mode, 
        0 print only the interior of the window. 
         
        1 print the interior and the title above each window 
         
        2 print the interior and all window decorations including the window title. 

         

----



.. method:: PWManager.printfile


    Syntax:
        :code:`p.printfile("filename", mode, selected)`


    Description:
        Print to a file in postcript, idraw, or ascii mode (mode=0,1,2) the selected windows 
        or all the windows( selected=0,1) 

         
         

