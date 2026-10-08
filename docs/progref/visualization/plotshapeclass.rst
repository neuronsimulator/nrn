.. _pltshape_doc:

PlotShape
---------



.. class:: PlotShape

    .. tab:: Python
    
            Class for making a Shape window useful for coloring a shape 
            according to a variable value and creating time and space graphs 
            of a variable. The default variable is *v*. The first arg may be 
            a SectionList.  This automatically calls :func:`define_shape`.

    .. tab:: HOC


        Class for making a Shape window useful for coloring a shape 
        according to a variable value and creating time and space graphs 
        of a variable. The default variable is *v*. The first arg may be 
        a SectionList. 
        
----

.. method:: PlotShape.plot

    .. tab:: Python
    
        
        Syntax:

            ``ps.plot(graphics_object)``

        Description:

            In NEURON 7.7+, PlotShape.plot works both with and without Interviews support.
            Variables, sectionlists, and scale are supported.
            Clicking on a segment displays the value and the segment id.
        
            Extra arguments and keyword arguments are passed to the underlying graphics library
            (currently only matplotlib ``Figure`` objects, ``pyplot``, and ``plotly`` are
            supported, with plotly support added in 7.8).

        .. note::
    
            If Interviews is enabled, the flag ``False`` must be passed to the ``n.PlotShape``
            constructor to avoid additionally displaying a PlotShape using Interviews graphics.
            See the example:

        Example:
            If no seclist argument is provided, PlotShape.plot will plot all sections


            .. code-block::
                python

                from neuron import n
                from matplotlib import pyplot 
                n.load_file('c91662.ses')  # a morphology file
                for sec in n.allsec():
                    if 'apic' in str(sec):
                        sec.v = 0
                ps = n.PlotShape(False)  # False tells n.PlotShape not to use NEURON's gui
                ps.plot(pyplot)
                pyplot.show()
        
            .. note::

                In Jupyter, you can use ``%matplotlib notebook`` to get interactive PlotShape
                    or use plotly instead.
    
        Example:

            You can also pass in a SectionList argument to only plot specific sections


            .. code-block::   
                python

                from neuron import n
                from matplotlib import pyplot, cm
                n.load_file('c91662.ses')
                sl = n.SectionList([sec for sec in n.allsec() if 'apic' in str(sec)])
                for sec in sl:
                    sec.v = 0
                ps = n.PlotShape(sl, False)
                ps.scale(-80, 40)
                ps.variable('v')
                ax = ps.plot(pyplot, cmap=cm.jet)
                pyplot.show()    

        Example:

            Line width across the neuron morphology is able to be altered depending on different modes. ``ps.show(0)`` allows for visualizing diameters for each segment across the cell. Additionally, when ``mode = 1`` or ``mode = 2`` , line_width argument can be passed in to specify fixed width across cell.

            For plotting on matplotlib:

            .. code-block::
                python

                from neuron import n, gui
                from neuron.units import mV, ms
                from matplotlib.pyplot import cm
                from matplotlib import pyplot

                n.load_file("c91662.ses")

                for sec in n.allsec():
                    sec.nseg = int(1 + 2 * (sec.L // 40))
                    sec.insert(n.hh)

                ic = n.IClamp(n.soma(0.5))
                ic.delay = 1 * ms
                ic.dur = 1 * ms
                ic.amp = 10

                n.finitialize(-65 * mV)
                n.continuerun(2 * ms)

                ps = n.PlotShape(False)
                ps.variable("v")
                ps.show(1)
                ps.plot(pyplot, cmap=cm.magma, line_width=10, color="red")
                pyplot.show()

            For plotting on plotly:

                .. code-block::
                    python

                    import plotly
                    import matplotlib
                    from neuron import n
                    from neuron.units import mV, ms

                    n.load_file("c91662.ses")
                    for sec in n.allsec():
                        sec.nseg = int(1 + 2 * (sec.L // 40))
                        sec.insert(n.hh)

                    ic = n.IClamp(n.soma(0.5))
                    ic.delay = 1 * ms
                    ic.dur = 1 * ms
                    ic.amp = 10

                    n.finitialize(-65 * mV)
                    n.continuerun(2 * ms)

                    ps = n.PlotShape(False)
                    ps.variable("v")
                    ps.show(1)
                    ps.plot(plotly, width=7, cmap=matplotlib.colormaps["viridis"]).show()


        Example:
            Color argument can also be passed in when consistent color across cell is preferred. When not specified, the morphology will be plotted in color gradient passed as ``cmap`` in accordance with voltage values of each segment after simulation is initiated. To specifiy cmap, 

            .. code-block::   
                python

                from neuron import n
                from matplotlib import pyplot, cm

                n.load_file("c91662.ses")
                sl = n.SectionList([sec for sec in n.allsec() if "apic" in str(sec)])
                for sec in sl:
                    sec.v = 0
                ps = n.PlotShape(False)
                ps.scale(-80, 40)
                ps.variable("v")
                ax = ps.plot(pyplot, line_width=3, color="red")
                pyplot.show()

----

.. method:: PlotShape.scale

    .. tab:: Python
    
    
        Syntax:
            ``.scale(low, high)``

        Description:
            Sets blue and red values for the color scale and default axes for
            time and space plots.


    .. tab:: HOC


        Syntax:
            ``.scale(low, high)``
        
        
            ``sets blue and red values for the color scale and default axes for``
        
        
            ``time and space plots``
        
----



.. method:: PlotShape.view

    .. tab:: Python
    
    
        .. seealso::
            :meth:`Shape.view`


    .. tab:: HOC


        .. seealso::
            :meth:`Shape.view`
        
----



.. method:: PlotShape.size

    .. tab:: Python
    
    
        .. seealso::
            :meth:`Shape.size`


    .. tab:: HOC


        .. seealso::
            :meth:`Shape.size`
        
----



.. method:: PlotShape.view_count

    .. tab:: Python
    
    
        .. seealso::
            :meth:`Shape.view_count`


    .. tab:: HOC


        .. seealso::
            :meth:`Shape.view_count`
        
----



.. method:: PlotShape.show

    .. tab:: Python
    
    
        .. seealso::
            :meth:`Shape.show`


    .. tab:: HOC


        .. seealso::
            :meth:`Shape.show`
        
----



.. method:: PlotShape.flush

    .. tab:: Python
    
    
        .. seealso::
            :meth:`Shape.flush`


    .. tab:: HOC


        .. seealso::
            :meth:`Shape.flush`
        
----



.. method:: PlotShape.fastflush

    .. tab:: Python
    
    
        Syntax:
            ``shapeplot.fastflush()``


        Description:
            Speeds up drawing of :meth:`PlotShape.hinton` elements. 


    .. tab:: HOC


        Syntax:
            ``shapeplot.fastflush()``
        
        
        Description:
            Speeds up drawing of :meth:`PlotShape.hinton` elements.
        
----



.. method:: PlotShape.variable

    .. tab:: Python
    
    
        Syntax:
            ``.variable("rangevar")``

        Description:
        Range variable (v, m_hh, etc.) to be used for time, space, and
        shape plots.
    
        Additionally, the variable can also be identified by species or specific region to show the corresponding voltage across.

        Example:

            .. code-block::
                python
            
                from neuron import n, rxd
                from neuron.units import mM, µm, ms, mV
                import plotly
                n.load_file("stdrun.hoc")

                dend1 = n.Section('dend1')
                dend2 = n.Section('dend2')
                dend2.connect(dend1(1))

                dend1.nseg = dend1.L = dend2.nseg = dend2.L = 11
                dend1.diam = dend2.diam = 2 * µm

                cyt = rxd.Region(dend1.wholetree(), nrn_region="i")
                cyt2 = rxd.Region(dend2.wholetree(), nrn_region="i")

                ca = rxd.Species([cyt,cyt2], name="ca", charge=2, initial=0 * mM, d=1 * µm ** 2 / ms)

                ca.nodes(dend1(0.5))[0].include_flux(1e-13, units="mmol/ms")

                n.finitialize(-65 * mV)
                n.continuerun(50 * ms)

                ps = n.PlotShape(False)

                ps.variable(ca[cyt])

                ps.plot(plotly).show()





    .. tab:: HOC


        Syntax:
            ``.variable("rangevar")``
        
        
            ``Range variable (v, m_hh, etc.) to be used for time, space, and``
        
        
            ``shape plots.``
        
----



.. method:: PlotShape.save_name

    .. tab:: Python
    
    
        .. seealso::
            :meth:`Shape.save_name`


    .. tab:: HOC


        .. seealso::
            :meth:`Shape.save_name`
        
----



.. method:: PlotShape.unmap

    .. tab:: Python
    
    
        .. seealso::
            :meth:`Shape.unmap`


    .. tab:: HOC


        .. seealso::
            :meth:`Shape.unmap`
        
----



.. method:: PlotShape.printfile

    .. tab:: Python
    
    
        .. seealso::
            :meth:`Shape.printfile`


    .. tab:: HOC


        .. seealso::
            :meth:`Shape.printfile`
        
----



.. method:: PlotShape.menu_action

    .. tab:: Python
    
    
        .. seealso::
            :meth:`Graph.menu_action`


    .. tab:: HOC


        .. seealso::
            :meth:`Graph.menu_action`
        
----



.. method:: PlotShape.menu_tool

    .. tab:: Python
    
    
        .. seealso::
            :meth:`Shape.menu_tool`


    .. tab:: HOC


        .. seealso::
            :meth:`Shape.menu_tool`
        
----



.. method:: PlotShape.observe

    .. tab:: Python
    
    
        .. seealso::
            :meth:`Shape.observe`


    .. tab:: HOC


        .. seealso::
            :meth:`Shape.observe`
        
----



.. method:: PlotShape.nearest

    .. tab:: Python
    
    
        .. seealso::
            :meth:`Shape.nearest`


    .. tab:: HOC


        .. seealso::
            :meth:`Shape.nearest`
        
----



.. method:: PlotShape.push_selected

    .. tab:: Python
    
    
        .. seealso::
            :meth:`Shape.push_selected`


    .. tab:: HOC


        .. seealso::
            :meth:`Shape.push_selected`
        
----



.. method:: PlotShape.exec_menu

    .. tab:: Python
    
    
        .. seealso::
            :meth:`Graph.exec_menu`


    .. tab:: HOC


        .. seealso::
            :meth:`Graph.exec_menu`
        
----



.. method:: PlotShape.erase

    .. tab:: Python
    
    
        .. seealso::
            :meth:`Graph.erase`


    .. tab:: HOC


        .. seealso::
            :meth:`Graph.erase`
        
----



.. method:: PlotShape.erase_all

    .. tab:: Python
    
    
        Description:
            Erases everything in the PlotShape, including all Sections and hinton plots 

        .. seealso::
            :meth:`Graph.erase_all`, :meth:`PlotShape.observe`, :meth:`PlotShape.hinton`


    .. tab:: HOC


        Description:
            Erases everything in the PlotShape, including all Sections and hinton plots 
        
        
        .. seealso::
            :meth:`Graph.erase_all`, :meth:`PlotShape.observe`, :meth:`PlotShape.hinton`
        
----



.. method:: PlotShape.beginline

    .. tab:: Python
    
    
        .. seealso::
            :meth:`Graph.beginline`


    .. tab:: HOC


        .. seealso::
            :meth:`Graph.beginline`
        
----



.. method:: PlotShape.line

    .. tab:: Python
    
    
        .. seealso::
            :meth:`Graph.line`


    .. tab:: HOC


        .. seealso::
            :meth:`Graph.line`
        
----



.. method:: PlotShape.mark

    .. tab:: Python
    
        Syntax:
            ``ps = n.PlotShape(False)``

            ``ps.plot(pyplot).mark(n.soma[0](0.5)).mark(n.apical_dendrite[68](1))``

            ``plt.show()``

        Description:
            Above syntax is allowed in NEURON 7.7+, for older versions:

        .. seealso::
            :meth:`Graph.mark`


    .. tab:: HOC


        .. seealso::
            :meth:`Graph.mark`
        
----



.. method:: PlotShape.label

    .. tab:: Python
    
    
        .. seealso::
            :meth:`Graph.label`


    .. tab:: HOC


        .. seealso::
            :meth:`Graph.label`
        
----



.. method:: PlotShape.color

    .. tab:: Python
    
    
        Syntax:
            ``shape.color(i, sec=sec)``


        Description:
            colors the specified section according to color index 
            (index same as specified in Graph class). If there are several 
            sections to color it is more efficient to make a SectionList and 
            use \ ``.color_list`` 

         

    .. tab:: HOC


        Syntax:
            ``section  shape.color(i)``
        
        
        Description:
            colors the currently accessed section according to color index 
            (index same as specified in Graph class). If there are several 
            sections to color it is more efficient to make a SectionList and 
            use \ ``.color_list`` 
        
----



.. method:: PlotShape.color_all

    .. tab:: Python
    
    
        Syntax:
            ``.color_all(i)``


        Description:
            colors all the sections 

         

    .. tab:: HOC


        Syntax:
            ``.color_all(i)``
        
        
        Description:
            colors all the sections 
        
----



.. method:: PlotShape.color_list

    .. tab:: Python
    
    
        Syntax:
            ``.color_list(SectionList, i)``


        Description:
            colors the sections in the list 

         

    .. tab:: HOC


        Syntax:
            ``.color_list(SectionList, i)``
        
        
        Description:
            colors the sections in the list 
        
----



.. method:: PlotShape.colormap

    .. tab:: Python
    
    
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

         

    .. tab:: HOC


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

    .. tab:: Python
    
    
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

                            from neuron import n, gui
                            import time

                            soma = n.Section("soma")  

                            sl = n.SectionList() 
 
                            s = n.PlotShape(sl) 
                            s.colormap(3) 
                            s.colormap(0, 255, 0, 0) 
                            s.colormap(1, 255, 255, 0) 
                            s.colormap(2, 200, 200, 200) 
                            s.scale(0, 2) 

                            nx = 30 
                            ny = 30 
                            vec = n.Vector(nx*ny) 
                            vec.fill(0) 

                            for i in range(nx):
                                    for j in range(ny): 
                                            s.hinton(vec._ref_x[i*ny + j], float(i)/nx, float(j)/ny, 1./nx) 

                            s.size(-.5, 1, 0, 1) 
                            s.exec_menu("Shape Plot") 
 
                            r = n.Random() 
                            r.poisson(.01) 
 
                            n.doNotify() 
 
                            def p():
                                    for i in range(1,1001): 
                                            vec.setrand(r) 
                                            s.fastflush() # faster by up to a factor of 4 
                                            n.doNotify() 

                            start = time.perf_counter()
                            p()
                            print(time.perf_counter() - start)


         
    .. tab:: HOC


        Syntax:
            ``s.hinton(&varname, x, y, size)``
        
        
            ``s.hinton(&varname, x, y, xsize, ysize)``
        
        
        Description:
            A filled square or rectangle is drawn with center at (x, y) and edge length given by 
            size. The color depends on the :meth:`PlotShape.colormap` and :meth:`PlotShape.scale`
            and is redrawn on :meth:`PlotShape.flush`.
        
        
            If there are many of these elements then :meth:`PlotShape.fastflush` can
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
                //          s.flush() 
                            doNotify() 
                    } 
                } 
                {startsw() p() print stopsw() } 
        
----



.. method:: PlotShape.len_scale

    .. tab:: Python
    
    
        Syntax:
            ``shape.len_scale(scl, sec=sec)``


        Description:
            See :meth:`Shape.len_scale` 

         

    .. tab:: HOC


        Syntax:
            ``section shape.len_scale(scl)``
        
        
        Description:
            See :meth:`Shape.len_scale`
        
----



.. method:: PlotShape.rotate

    .. tab:: Python
    
    
        Syntax:
            ``shape.rotate()``

            ``shape.rotate(xorg, yorg, zorg, xrad, yrad, zrad)``


        Description:
            See :meth:`Shape.rotate` 

         
         

    .. tab:: HOC


        Syntax:
            ``shape.rotate()``
        
        
            ``shape.rotate(xorg, yorg, zorg, xrad, yrad, zrad)``
        
        
        Description:
            See :meth:`Shape.rotate`
        
