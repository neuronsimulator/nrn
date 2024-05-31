.. _shape:

Shape
-----



.. class:: Shape

        Class for making a Shape window for executing a user defined action 
        when a section is clicked on. (When the section mode is selected 
        from the mouse menu.) An argument of 0 will prevent default mapping 
        of the	window. 
        If the first arg is a :class:`SectionList` (then a second arg of 0 will 
        prevent default mapping) then only the sections in the list are 
        drawn. Shape is redrawn automatically whenever length or diameter 
        of a section changes. 
        
        .. warning::
        
            The form of the constructor that takes a :class:`SectionList` does not
            currently work in Python.
            

----



.. method:: Shape.view


    Syntax:
        ``.view(mleft, mbottom, mwidth, mheight, sleft, stop, swidth, sheight)``


    Description:
        maps a view of the Shape scene. m stands for model coordinates, 
        s stands for screen pixel coordinates where 0,0 is the top left 
        corner of the screen. 

         

----



.. method:: Shape.size


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
         
        See :meth:`Graph.size` for other, more rarely use argument sequences. 

         

----



.. method:: Shape.show


    Syntax:
        ``shape.show(mode)``


    Description:

        Mode for ``shape.show()`` can be adjusted for different way to display the cell, and can be adjusted as the following example (available from NEURON 9.0:
        
        mode = 0 
            displays diameters 

        mode = 1 
            displays centroid. ie line through all the 3d points. 

        mode = 2 
            displays schematic. ie line through 1st and last 2d points of each 
            section. 
        .. code-block::
            python

            import plotly
            from neuron import h, gui
            from neuron.units import mV, ms
            import matplotlib

            h.load_file("c91662.ses")

            for sec in h.allsec():
                sec.nseg = int(1 + 2 * (sec.L // 40))
                sec.insert(h.hh)

            ic = h.IClamp(h.soma(0.5))
            ic.delay = 1 * ms
            ic.dur = 1 * ms
            ic.amp = 10

            h.finitialize(-65 * mV)
            h.continuerun(2 * ms)

            ps = h.PlotShape(False)
            ps.variable("v")
            print(ps.show())  # prints the current mode
            ps.show(0)  # alters the mode to 0 that displays diameters for each segment
            print(ps.show())  # should print 0 as the mode set
            ps.plot(plotly, width=7, cmap=matplotlib.colormaps["viridis"]).show()

        


----



.. method:: Shape.flush


    Syntax:
        ``.flush()``


    Description:
        Redraws all views into this scene. 

         

----



.. method:: Shape.observe


    Syntax:
        ``shape.observe()``

        ``shape.observe(sectionlist)``


    Description:
        Replace the list of observed sections in the Shape with the specified 
        list. With no arguments, all sections are observed. 

    Example:
        In the context of the pyramidal cell demo of neurondemo (launch via
        ``neurondemo --python``) the following 
        will change the Shape shown in the point process manager 
        to show only the soma and the main part of the primary dendrite. 

        .. code-block::
            python
            
            from neuron import h
            sl = h.SectionList()
            sl.append(h.soma)
            sl.append(h.dendrite_1[8])
            h.Shape[0].observe(sl)



         

----



.. method:: Shape.view_count


    Syntax:
        ``.view_count()``


    Description:
        Returns number of views into this scene. (stdrun.hoc removes 
        scenes from the \ ``flush_list`` and \ ``graphList[]`` when this goes to 
        0. If no other \ ``objectvar`` points to the scene, it will be 
        freed.) 

         

----



.. method:: Shape.select


    Syntax:
        ``.select(sec=section)``


    Description:
        Colors red the specified section. 

         

----



.. method:: Shape.action


    Syntax:
        ``.action("command")``


    Description:
        command is executed whenever the user clicks on a section. 
        The clicked section is pushed before execution and popped after. 
        \ :data:`hoc_ac_` contains the arc position 0 - 1 of the nearest node. 

         

----



.. method:: Shape.color


    Syntax:
        ``shape.color(i, sec=section)``


    Description:
        colors the specified section according to color index 
        (index same as specified in :class:`Graph` class). If there are several 
        sections to color it is more efficient to make a :class:`SectionList` and 
        use \ ``.color_list`` 

         

----



.. method:: Shape.color_all


    Syntax:
        ``.color_all(i)``


    Description:
        colors all the sections 

         

----



.. method:: Shape.color_list


    Syntax:
        ``.color_list(SectionList, i)``


    Description:
        colors the sections in the list 

         

----



.. method:: Shape.point_mark


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
        :meth:`Graph.mark` method of :class:`Graph`. This extension was contributed 
        by Yichun Wei ``yichunwe@usc.edu``.

         

----



.. method:: Shape.point_mark_remove


    Syntax:
        ``.point_mark_remove([objvar])``


    Description:
        With no arg, removes all the point process marks. 

         

----



.. method:: Shape.save_name


    Syntax:
        ``.save_name("name")``


    Description:
        The \ ``objectvar`` used to save the scene when the print window 
        manager is used to save a session. 

         

----



.. method:: Shape.unmap


    Syntax:
        ``.unmap()``


    Description:
        dismisses all windows that are a direct view into this scene. 
        (does not unmap boxes containing scenes.) \ ``unmap`` is called 
        automatically when no hoc object variable references the Shape. 

         

----



.. method:: Shape.printfile


    Syntax:
        ``.printfile("filename")``


    Description:
        prints the first view of the graph as an encapsulated post script 
        file 


----



.. method:: Shape.menu_action


    .. seealso::
        :meth:`Graph.menu_action`

         

----



.. method:: Shape.exec_menu


    .. seealso::
        :meth:`Graph.exec_menu`


----



.. method:: Shape.erase


    .. seealso::
        :meth:`Graph.erase`


----



.. method:: Shape.erase_all


    Description:
        Erases everything in the Shape, including all PointMarks and Sections. 

    .. seealso::
        :meth:`Graph.erase_all`, :meth:`Shape.observe`, :meth:`Shape.point_mark`


----



.. method:: Shape.beginline


    .. seealso::
        :meth:`Graph.beginline`


----



.. method:: Shape.line


    .. seealso::
        :meth:`Graph.line`


----



.. method:: Shape.mark


    .. seealso::
        :meth:`Graph.mark`


----



.. method:: Shape.label


    .. seealso::
        :meth:`Graph.label`


----



.. method:: Shape.menu_tool


    Syntax:
        ``s.menu_tool("label", "procname")``


    Description:
        Same as :meth:`Graph.menu_tool` for the :func:`Graph` class. When procname is 
        called it is given four arguments: type, x, y, keystate. Type = 1,2,3 means 
        move, press, release respectively and x and are in model coordinates. 
        Keystate reflects the 
        state of control (bit 1), shift (bit 2), and meta (bit 3) keys, ie 
        control and shift down has a value of 3. 
         

    .. seealso::
        :meth:`Graph.menu_tool`, :meth:`Shape.nearest`, :meth:`Shape.push_selected`

    Example:
        The following example will work if executed in the context of the 
        pyramidal cell demo of the neurondemo. It colors red the section 
        you click nearest and prints the name and position of the selected section 
        as well as the mouse distance the selection. 

        .. code-block::
            python

            from neuron import h

            ss = h.Shape[0]
            def p(type, x, y, keystate):
                if type == 2:
                    ss.color_all(1)
                    d = ss.nearest(x, y)
                    arc = ss.push_selected()
                    if arc >= 0:
                        ss.select()
                        print('%g from %s(%g)' % (d, h.secname(), a))
                    h.pop_section()

            ss.menu_tool('test', p)
            ss.exec_menu('test')



----



.. method:: Shape.nearest


    Syntax:
        ``d = shape.nearest(x, y)``


    Description:
        returns the distance (in model coordinates) to the nearest section. 
        The section becomes the selected section of the Shape. It is NOT 
        pushed onto the section stack and it is NOT colored. The nearest 
        arc position of the selected section as well 
        as the section is available from :func:`push_section`. 

         

----



.. method:: Shape.push_selected


    Syntax:
    
        .. code-block::
            python
            
            arc = shape.push_selected()
            if arc >= 0:
                # do something, then end with:
            h.pop_section()


    Description:
        If there is a selection for the Shape class, then it is pushed onto 
        the section stack (becomes the currently accessed section) and the 
        arc position (0 to 1) returned. If no section is selected the function 
        returns -1 and no section is pushed. 

    .. note::
        
        The pushed section can be read via ``h.cas()``.

    .. note::
             
        It is important that a :func:`pop_section` be executed if a section 
        is pushed onto the stack.

    .. warning::
        The arc position is relevant only if the section was selected using 
        :meth:`Shape.nearest`. Note, e.g., that :meth:`Shape.select` does not 
        set the arc position. 

         

----



.. method:: Shape.len_scale


    Syntax:
        ``shape.len_scale(scl, sec=section)``


    Description:
        The drawing of the section length (for the specified section) in the Shape 
        scene is scaled by the factor. Diameter is drawn normally. 
        Note that this does not change the physical length of the section but 
        only its appearance in this Shape instance. 

         

----



.. method:: Shape.rotate


    Syntax:
        ``shape.rotate()``

        ``shape.rotate(xorg, yorg, zorg, xrad, yrad, zrad)``


    Description:
        With no args the view is in the xy plane. 
        With args, incrementally rotate about the indicated origin by the 
        amount given in radians around the current view coordinates (order is 
        sequentially about x,y,z axes) 

         
         


