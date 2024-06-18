
.. _hoc_pltshape_doc:

PlotShape
---------



.. hoc:class:: PlotShape

        Class for making a Shape window useful for coloring a shape 
        according to a variable value and creating time and space graphs 
        of a variable. The default variable is *v*. The first arg may be 
        a SectionList. 

----



.. hoc:method:: PlotShape.scale


    Syntax:
        ``.scale(low, high)``

        ``sets blue and red values for the color scale and default axes for``

        ``time and space plots``


----



.. hoc:method:: PlotShape.view


    .. seealso::
        :hoc:meth:`Shape.view`


----



.. hoc:method:: PlotShape.size


    .. seealso::
        :hoc:meth:`Shape.size`


----



.. hoc:method:: PlotShape.view_count


    .. seealso::
        :hoc:meth:`Shape.view_count`


----



.. hoc:method:: PlotShape.show


    .. seealso::
        :hoc:meth:`Shape.show`


----



.. hoc:method:: PlotShape.flush


    .. seealso::
        :hoc:meth:`Shape.flush`


----



.. hoc:method:: PlotShape.fastflush


    Syntax:
        ``shapeplot.fastflush()``


    Description:
        Speeds up drawing of :hoc:meth:`PlotShape.hinton` elements.


----



.. hoc:method:: PlotShape.variable


    Syntax:
        ``.variable("rangevar")``

        ``Range variable (v, m_hh, etc.) to be used for time, space, and``

        ``shape plots.``


----



.. hoc:method:: PlotShape.save_name


    .. seealso::
        :hoc:meth:`Shape.save_name`


----



.. hoc:method:: PlotShape.unmap


    .. seealso::
        :hoc:meth:`Shape.unmap`


----



.. hoc:method:: PlotShape.printfile


    .. seealso::
        :hoc:meth:`Shape.printfile`


----



.. hoc:method:: PlotShape.menu_action


    .. seealso::
        :hoc:meth:`Graph.menu_action`


----



.. hoc:method:: PlotShape.menu_tool


    .. seealso::
        :hoc:meth:`Shape.menu_tool`


----



.. hoc:method:: PlotShape.observe


    .. seealso::
        :hoc:meth:`Shape.observe`


----



.. hoc:method:: PlotShape.nearest


    .. seealso::
        :hoc:meth:`Shape.nearest`


----



.. hoc:method:: PlotShape.push_selected


    .. seealso::
        :hoc:meth:`Shape.push_selected`


----



.. hoc:method:: PlotShape.exec_menu


    .. seealso::
        :hoc:meth:`Graph.exec_menu`


----



.. hoc:method:: PlotShape.erase


    .. seealso::
        :hoc:meth:`Graph.erase`


----



.. hoc:method:: PlotShape.erase_all


    Description:
        Erases everything in the PlotShape, including all Sections and hinton plots 

    .. seealso::
        :hoc:meth:`Graph.erase_all`, :hoc:meth:`PlotShape.observe`, :hoc:meth:`PlotShape.hinton`


----



.. hoc:method:: PlotShape.beginline


    .. seealso::
        :hoc:meth:`Graph.beginline`


----



.. hoc:method:: PlotShape.line


    .. seealso::
        :hoc:meth:`Graph.line`


----



.. hoc:method:: PlotShape.mark


    .. seealso::
        :hoc:meth:`Graph.mark`


----



.. hoc:method:: PlotShape.label


    .. seealso::
        :hoc:meth:`Graph.label`


----



.. hoc:method:: PlotShape.color


    Syntax:
        ``section  shape.color(i)``


    Description:
        colors the currently accessed section according to color index 
        (index same as specified in Graph class). If there are several 
        sections to color it is more efficient to make a SectionList and 
        use \ ``.color_list`` 

         

----



.. hoc:method:: PlotShape.color_all


    Syntax:
        ``.color_all(i)``


    Description:
        colors all the sections 

         

----



.. hoc:method:: PlotShape.color_list


    Syntax:
        ``.color_list(SectionList, i)``


    Description:
        colors the sections in the list 

         

----



.. hoc:method:: PlotShape.colormap


    Syntax:
        ``s.colormap(size, [global = 0])``

        ``s.colormap(index, red, green, blue)``


    Description:
        If the optional global argument is 1 then these functions refer to 
        the global (default) Colormap and a change will affect all PlotShape instances 
        that use it. Otherwise these function create a colormap that is local to 
        this PlotShape. 
         
        With a single argument, destroys the old and creates a new colormap 
        for shape plots with space for size colors. All colors are initialized to 
        gray. 
         
        The four argument syntax, specifies the color of the index element of the 
        colormap. the red, green, and blue must be integers within the range 0-255 
        and specify the intensity of these colors. 
         
        If an existing colormap is displayed in the view, it will be redrawn with 
        the proper colors when :hoc:meth:`PlotShape.scale` is called.

         

----



.. hoc:method:: PlotShape.hinton


    Syntax:
        ``s.hinton(&varname, x, y, size)``

        ``s.hinton(&varname, x, y, xsize, ysize)``


    Description:
        A filled square or rectangle is drawn with center at (x, y) and edge length given by 
        size. The color depends on the :hoc:meth:`PlotShape.colormap` and :hoc:meth:`PlotShape.scale`
        and is redrawn on :hoc:meth:`PlotShape.flush`.
         
        If there are many of these elements then :hoc:meth:`PlotShape.fastflush` can
        speed plotting by up to a factor of 4 if not too many elements change 
        color between fastflush calls. 

    Example:

        .. code-block::
            none

            create soma 
            access soma 
            objref sl 
            sl = new SectionList() 
            objref s 
            s = new PlotShape(sl) 
            s.colormap(3) 
            s.colormap(0, 255, 0, 0) 
            s.colormap(1, 255, 255, 0) 
            s.colormap(2, 200, 200, 200) 
            s.scale(0, 2) 
            objref vec 
            nx = 30 
            ny = 30 
            vec = new Vector(nx*ny) 
            vec.fill(0) 
            for i=0,nx-1 for j=0,ny-1 { 
            	s.hinton(&vec.x[i*ny + j], i/nx, j/ny, 1/nx) 
            } 
            s.size(-.5, 1, 0, 1) 
            s.exec_menu("Shape Plot") 
             
            objref r 
            r = new Random() 
            r.poisson(.01) 
             
            doNotify() 
             
            proc p() {local i 
            	for i=1,1000 { 
            		vec.setrand(r) 
            		s.fastflush() // faster by up to a factor of 4 
            //		s.flush() 
            		doNotify() 
            	} 
            } 
            {startsw() p() print stopsw() } 


         

----



.. hoc:method:: PlotShape.len_scale


    Syntax:
        ``section shape.len_scale(scl)``


    Description:
        See :hoc:meth:`Shape.len_scale`

         

----



.. hoc:method:: PlotShape.rotate


    Syntax:
        ``shape.rotate()``

        ``shape.rotate(xorg, yorg, zorg, xrad, yrad, zrad)``


    Description:
        See :hoc:meth:`Shape.rotate`

         
         

