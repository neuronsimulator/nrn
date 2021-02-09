.. _pltshape_doc:

PlotShape
---------



.. class:: PlotShape

        Class for making a Shape window useful for coloring a shape 
        according to a variable value and creating time and space graphs 
        of a variable. The default variable is *v*. The first arg may be 
        a SectionList. 

----

.. method:: PlotShape.plot

    
    Syntax:

        ``ps.plot(matplotlib_object)``

    Description:
        In NEURON 7.7+, PlotShape.plot works both with and without Interviews support.
	Variables, sectionlists, and scale are supported.
        Clicking on a segment displays the value and the segment id.
	
	Extra arguments and keyword arguments are passed to the underlying graphics library
	(currently only matplotlib ``Figure`` objects and ``pyplot`` are supported).

    .. note::
    
        If Interviews is enabled, the flag ``False`` must be passed to the ``h.PlotShape``
	constructor to avoid additionally displaying a PlotShape using Interviews graphics.
	See the example:

    Example:
        If no seclist argument is provided, PlotShape.plot will plot all sections


        .. code-block::
            python

            from neuron import h
            from matplotlib import pyplot 
            h.load_file('c91662.ses')  # a morphology file
            for sec in h.allsec():
                if 'apic' in str(sec):
                    sec.v = 0
            ps = h.PlotShape(False)  # False tells h.PlotShape not to use NEURON's gui
            ps.plot(pyplot)
            pyplot.show()
        
        .. note::
            In Jupyter, you can use %matplotlib notebook to get interactive PlotShape
    
    Example:
        You can also pass in a SectionList argument to only plot specific sections


        .. code-block::   
            python

            from neuron import h
            from matplotlib import pyplot, cm
            h.load_file('c91662.ses')
            sl = h.SectionList([sec for sec in h.allsec() if 'apic' in str(sec)])
            for sec in sl:
                sec.v = 0
            ps = h.PlotShape(sl, False)
            ps.scale(-80, 40)
            ps.variable('v')
            ax = ps.plot(pyplot, cmap=cm.jet)
            pyplot.show()    
            
----

.. method:: PlotShape.scale


    Syntax:
        ``.scale(low, high)``

    Description:
        Sets blue and red values for the color scale and default axes for
        time and space plots.


----



.. method:: PlotShape.view


    .. seealso::
        :meth:`Shape.view`


----



.. method:: PlotShape.size


    .. seealso::
        :meth:`Shape.size`


----



.. method:: PlotShape.view_count


    .. seealso::
        :meth:`Shape.view_count`


----



.. method:: PlotShape.show


    .. seealso::
        :meth:`Shape.show`


----



.. method:: PlotShape.flush


    .. seealso::
        :meth:`Shape.flush`


----



.. method:: PlotShape.fastflush


    Syntax:
        ``shapeplot.fastflush()``


    Description:
        Speeds up drawing of :meth:`PlotShape.hinton` elements. 


----



.. method:: PlotShape.variable


    Syntax:
        ``.variable("rangevar")``

    Description:
    Range variable (v, m_hh, etc.) to be used for time, space, and
    shape plots.


----



.. method:: PlotShape.save_name


    .. seealso::
        :meth:`Shape.save_name`


----



.. method:: PlotShape.unmap


    .. seealso::
        :meth:`Shape.unmap`


----



.. method:: PlotShape.printfile


    .. seealso::
        :meth:`Shape.printfile`


----



.. method:: PlotShape.menu_action


    .. seealso::
        :meth:`Graph.menu_action`


----



.. method:: PlotShape.menu_tool


    .. seealso::
        :meth:`Shape.menu_tool`


----



.. method:: PlotShape.observe


    .. seealso::
        :meth:`Shape.observe`


----



.. method:: PlotShape.nearest


    .. seealso::
        :meth:`Shape.nearest`


----



.. method:: PlotShape.push_selected


    .. seealso::
        :meth:`Shape.push_selected`


----



.. method:: PlotShape.exec_menu


    .. seealso::
        :meth:`Graph.exec_menu`


----



.. method:: PlotShape.erase


    .. seealso::
        :meth:`Graph.erase`


----



.. method:: PlotShape.erase_all


    Description:
        Erases everything in the PlotShape, including all Sections and hinton plots 

    .. seealso::
        :meth:`Graph.erase_all`, :meth:`PlotShape.observe`, :meth:`PlotShape.hinton`


----



.. method:: PlotShape.beginline


    .. seealso::
        :meth:`Graph.beginline`


----



.. method:: PlotShape.line


    .. seealso::
        :meth:`Graph.line`


----



.. method:: PlotShape.mark

    Syntax:
        ``ps = h.PlotShape(False)``

        ``ps.plot(pyplot).mark(h.soma[0](0.5)).mark(h.apical_dendrite[68](1))``

        ``plt.show``

    Description:
        Above syntax is allowed in NEURON 7.7+, for older versions:

    .. seealso::
        :meth:`Graph.mark`


----



.. method:: PlotShape.label


    .. seealso::
        :meth:`Graph.label`


----



.. method:: PlotShape.color


    Syntax:
        ``shape.color(i, sec=sec)``


    Description:
        colors the specified section according to color index 
        (index same as specified in Graph class). If there are several 
        sections to color it is more efficient to make a SectionList and 
        use \ ``.color_list`` 

         

----



.. method:: PlotShape.color_all


    Syntax:
        ``.color_all(i)``


    Description:
        colors all the sections 

         

----



.. method:: PlotShape.color_list


    Syntax:
        ``.color_list(SectionList, i)``


    Description:
        colors the sections in the list 

         

----



.. method:: PlotShape.colormap


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
        the proper colors when :meth:`PlotShape.scale` is called. 

         

----



.. method:: PlotShape.hinton


    Syntax:
        ``s.hinton(_ref_varname, x, y, size)``

        ``s.hinton(_ref_varname, x, y, xsize, ysize)``


    Description:
        A filled square or rectangle is drawn with center at (x, y) and edge length given by 
        size. The color depends on the :meth:`PlotShape.colormap` and :meth:`PlotShape.scale` 
        and is redrawn on :meth:`PlotShape.flush`. 
         
        If there are many of these elements then :meth:`PlotShape.fastflush` can 
        speed plotting by up to a factor of 4 if not too many elements change 
        color between fastflush calls. 

    Example:

        .. code-block::
            python

			from neuron import h, gui

			soma = h.Section()  

			sl = h.SectionList() 
 
			s = h.PlotShape(sl) 
			s.colormap(3) 
			s.colormap(0, 255, 0, 0) 
			s.colormap(1, 255, 255, 0) 
			s.colormap(2, 200, 200, 200) 
			s.scale(0, 2) 

			nx = 30 
			ny = 30 
			vec = h.Vector(nx*ny) 
			vec.fill(0) 

			for i in range(nx):
				for j in range(ny): 
					s.hinton(vec._ref_x[i*ny + j], float(i)/nx, float(j)/ny, 1./nx) 

			s.size(-.5, 1, 0, 1) 
			s.exec_menu("Shape Plot") 
 
			r = h.Random() 
			r.poisson(.01) 
 
			h.doNotify() 
 
			def p():
				for i in range(1,1001): 
					vec.setrand(r) 
					s.fastflush() # faster by up to a factor of 4 
					h.doNotify() 

			h.startsw()
			p()
			print(h.stopsw())


         
----



.. method:: PlotShape.len_scale


    Syntax:
        ``shape.len_scale(scl, sec=sec)``


    Description:
        See :meth:`Shape.len_scale` 

         

----



.. method:: PlotShape.rotate


    Syntax:
        ``shape.rotate()``

        ``shape.rotate(xorg, yorg, zorg, xrad, yrad, zrad)``


    Description:
        See :meth:`Shape.rotate` 

         
         

