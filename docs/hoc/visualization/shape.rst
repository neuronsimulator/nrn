
.. _hoc_shape:

Shape
-----



.. hoc:class:: Shape

        Class for making a Shape window for executing a user defined action 
        when a section is clicked on. (When the section mode is selected 
        from the mouse menu.) An argument of 0 will prevent default mapping 
        of the	window. 
        If the first arg is a SectionList (then a second arg of 0 will 
        prevent default mapping) then only the sections in the list are 
        drawn. Shape is redrawn automatically whenever length or diameter 
        of a section changes. 

----



.. hoc:method:: Shape.view


    Syntax:
        ``.view(mleft, mbottom, mwidth, mheight, sleft, stop, swidth, sheight)``


    Description:
        maps a view of the Shape scene. m stands for model coordinates, 
        s stands for screen pixel coordinates where 0,0 is the top left 
        corner of the screen. 

         

----



.. hoc:method:: Shape.size


    Syntax:
        ``.size(mleft, mright, mbottom, mtop)``

        ``...``


    Description:
        Model coordinates for the scene. 
        This is the "whole scene" size. 
        Since, the aspect ratio for shape views is unity, the bounding box expressed 
        by the arguments may not fit exactly on the screen window. The scale factor 
        is decreased so that the first view window displays the entire bounding box 
        with the center of the bounding box in the center of the view. 
         
        See :hoc:meth:`Graph.size` for other, more rarely use argument sequences.

         

----



.. hoc:method:: Shape.show


    Syntax:
        ``shape.show(mode)``


    Description:


        mode = 0 
            displays diameters 

        mode = 1 
            displays centroid. ie line through all the 3d points. 

        mode = 2 
            displays schematic. ie line through 1st and last 2d points of each 
            section. 



----



.. hoc:method:: Shape.flush


    Syntax:
        ``.flush()``


    Description:
        Redraws all views into this scene. 

         

----



.. hoc:method:: Shape.observe


    Syntax:
        ``shape.observe()``

        ``shape.observe(sectionlist)``


    Description:
        Replace the list of observed sections in the Shape with the specified 
        list. With no arguments, all sections are observed. 

    Example:
        In the context of the pyramidal cell demo of neurondemo the following 
        will change the Shape shown in the point process manager 
        to show only the soma and the main part of the primary dendrite. 

        .. code-block::
            none

            objref sl 
            sl = new SectionList() 
            soma sl.append() 
            dendrite_1[8] sl.append() 
            Shape[0].observe(sl) 


         

----



.. hoc:method:: Shape.view_count


    Syntax:
        ``.view_count()``


    Description:
        Returns number of views into this scene. (stdrun.hoc removes 
        scenes from the \ ``flush_list`` and \ ``graphList[]`` when this goes to 
        0. If no other \ ``objectvar`` points to the scene, it will be 
        freed.) 

         

----



.. hoc:method:: Shape.select


    Syntax:
        ``.select()``


    Description:
        Colors red the currently accessed section. 

         

----



.. hoc:method:: Shape.action


    Syntax:
        ``.action("command")``


    Description:
        command is executed whenever the user clicks on a section. 
        The clicked section is pushed before execution and popped after. 
        \ :hoc:data:`hoc_ac_` contains the arc position 0 - 1 of the nearest node.

         

----



.. hoc:method:: Shape.color


    Syntax:
        ``section  shape.color(i)``


    Description:
        colors the currently accessed section according to color index 
        (index same as specified in :hoc:class:`Graph` class). If there are several
        sections to color it is more efficient to make a SectionList and 
        use \ ``.color_list`` 

         

----



.. hoc:method:: Shape.color_all


    Syntax:
        ``.color_all(i)``


    Description:
        colors all the sections 

         

----



.. hoc:method:: Shape.color_list


    Syntax:
        ``.color_list(SectionList, i)``


    Description:
        colors the sections in the list 

         

----



.. hoc:method:: Shape.point_mark


    Syntax:
        ``.point_mark(objvar, colorindex)``

        ``.point_mark(objvar, colorindex, style)``

        ``.point_mark(objvar, colorindex, style, size)``



    Description:
        draw a little filled circle with indicated color where the point process 
        referenced by \ ``objvar`` is located. Note, if you subsequently relocate 
        the point process or destroy it the proper thing will happen to the 
        mark. (at least after a flush) 
         
        The optional arguments specify the style and size as in the 
        :hoc:meth:`Graph.mark` method of :hoc:class:`Graph`. This extension was contributed
        by Yichun Wei ``yichunwe@usc.edu``.

         

----



.. hoc:method:: Shape.point_mark_remove


    Syntax:
        ``.point_mark_remove([objvar])``


    Description:
        With no arg, removes all the point process marks. 

         

----



.. hoc:method:: Shape.save_name


    Syntax:
        ``.save_name("name")``


    Description:
        The \ ``objectvar`` used to save the scene when the print window 
        manager is used to save a session. 

         

----



.. hoc:method:: Shape.unmap


    Syntax:
        ``.unmap()``


    Description:
        dismisses all windows that are a direct view into this scene. 
        (does not unmap boxes containing scenes.) \ ``unmap`` is called 
        automatically when no hoc object variable references the Shape. 

         

----



.. hoc:method:: Shape.printfile


    Syntax:
        ``.printfile("filename")``


    Description:
        prints the first view of the graph as an encapsulated post script 
        file 


----



.. hoc:method:: Shape.menu_action


    .. seealso::
        :hoc:meth:`Graph.menu_action`

         

----



.. hoc:method:: Shape.exec_menu


    .. seealso::
        :hoc:meth:`Graph.exec_menu`


----



.. hoc:method:: Shape.erase


    .. seealso::
        :hoc:meth:`Graph.erase`


----



.. hoc:method:: Shape.erase_all


    Description:
        Erases everything in the Shape, including all PointMarks and Sections. 

    .. seealso::
        :hoc:meth:`Graph.erase_all`, :hoc:meth:`Shape.observe`, :hoc:meth:`Shape.point_mark`


----



.. hoc:method:: Shape.beginline


    .. seealso::
        :hoc:meth:`Graph.beginline`


----



.. hoc:method:: Shape.line


    .. seealso::
        :hoc:meth:`Graph.line`


----



.. hoc:method:: Shape.mark


    .. seealso::
        :hoc:meth:`Graph.mark`


----



.. hoc:method:: Shape.label


    .. seealso::
        :hoc:meth:`Graph.label`


----



.. hoc:method:: Shape.menu_tool


    Syntax:
        ``s.menu_tool("label", "procname")``


    Description:
        Same as :hoc:meth:`Graph.menu_tool` for the :hoc:func:`Graph` class. When procname is
        called it is given four arguments: type, x, y, keystate. Type = 1,2,3 means 
        move, press, release respectively and x and are in model coordinates. 
        Keystate reflects the 
        state of control (bit 1), shift (bit 2), and meta (bit 3) keys, ie 
        control and shift down has a value of 3. 
         

    .. seealso::
        :hoc:meth:`Graph.menu_tool`, :hoc:meth:`Shape.nearest`, :hoc:meth:`Shape.push_selected`

    Example:
        The following example will work if executed in the context of the 
        pyramidal cell demo of the neurondemo. It colors red the section 
        you click nearest and prints the name and position of the selected section 
        as well as the mouse distance the selection. 

        .. code-block::
            none

            objref ss 
            ss = Shape[0] 
            proc p() {local d, a 
                    if ($1 == 2) { 
                            ss.color_all(1) 
                            d = ss.nearest($2,$3)  
                            a = ss.push_selected() 
                            if (a >= 0) { 
                                    ss.select() 
                                    printf("%g from %s(%g)\n", d, secname(), a) 
                                    pop_section() 
                            } 
                    } 
            } 
            ss.menu_tool("test", "p") 
            ss.exec_menu("test") 



----



.. hoc:method:: Shape.nearest


    Syntax:
        ``d = shape.nearest(x, y)``


    Description:
        returns the distance (in model coordinates) to the nearest section. 
        The section becomes the selected section of the Shape. It is NOT 
        pushed onto the section stack and it is NOT colored. The nearest 
        arc position of the selected section as well 
        as the section is available from :hoc:func:`push_section`.

         

----



.. hoc:method:: Shape.push_selected


    Syntax:
        ``arc = shape.push_selected()``

        ``if (arc >= 0) {``

        ``pop_section()``

        ``}``


    Description:
        If there is a selection for the Shape class, then it is pushed onto 
        the section stack (becomes the currently accessed section) and the 
        arc position (0 to 1) returned. If no section is selected the function 
        returns -1 and no section is pushed. 
         
        Note that it is important that a pop_section be executed if a section 
        is pushed onto the stack. 

    .. warning::
        The arc position is relevant only if the section was selected using 
        :hoc:meth:`Shape.nearest`. Note, e.g., that :hoc:meth:`Shape.select` does not
        set the arc position. 

         

----



.. hoc:method:: Shape.len_scale


    Syntax:
        ``section shape.len_scale(scl)``


    Description:
        The drawing of the section length (currently accessed section) in the Shape 
        scene is scaled by the factor. Diameter is drawn normally. 
        Note that this does not change the physical length of the section but 
        only its appearance in this Shape instance. 

         

----



.. hoc:method:: Shape.rotate


    Syntax:
        ``shape.rotate()``

        ``shape.rotate(xorg, yorg, zorg, xrad, yrad, zrad)``


    Description:
        With no args the view is in the xy plane. 
        With args, incrementally rotate about the indicated origin by the 
        amount given in radians around the current view coordinates (order is 
        sequentially about x,y,z axes) 

         
         

