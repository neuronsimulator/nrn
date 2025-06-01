.. _graph:

         
Graph
-----



.. class:: Graph


    Syntax:
        ``g = h.Graph()``

        ``g = h.Graph(False)``


    Description:
         
        An instance of the ``Graph`` class manages a window on which x-y plots can 
        be drawn by calling various member functions. 
        The first form immediately maps the window to the screen. With a ``False`` (or 0) argument 
        the window is not mapped but can be sized and placed with the :func:`view` function. 
         

    Example:
        The most basic interpreter prototype for producing a plot follows: 
         

        .. code-block::
            python

            from neuron import n, gui

            # Create the graph
            g = h.Graph()

            # specify coordinate system for the canvas drawing area
            # numbers are: xmin, xmax, ymin, ymax respectively
            g.size(0, 10, -1, 1)

            # the next g.line command will move the drawing pen to the
            # indicated point without drawing anything
            g.beginline()

            # define a sine wave, 0 <= x <= 10
            for i in range(101):
                x = i * 0.1
                g.line(x, h.sin(x))

            # actually draw the plot on the graph in the window
            g.flush()

        .. image:: ../images/graph-constructor.png
            :align: center


----



.. method:: Graph.xaxis


    Syntax:
        ``g.xaxis()``

        ``g.xaxis(mode)``

        ``g.xaxis(xstart, xstop)``

        ``g.xaxis(xstart, xstop, ypos, ntic, nminor, invert, shownumbers)``


    Description:
        The single mode argument draws both x and y axes (no arg == mode 0). 
        See :meth:`Graph.yaxis` for a complete description of the arguments. 

         

----



.. method:: Graph.yaxis


    Syntax:
        ``g.yaxis()``

        ``g.yaxis(mode)``

        ``g.yaxis(ystart, ystop)``

        ``g.yaxis(ystart, ystop, ypos, ntic, nminor, invert, shownumbers)``


    Description:
        The single mode argument draws both x and y axes (no arg == mode 0). 


    .. list-table:: Mode Action
        :widths: 20 80
        :header-rows: 1

        * - Mode
            - Action
        * - 0
            - View axes (axes in each view drawn dynamically). When the graph is created, these axes are the default.
        * - 1
            - Fixed axes as in long form, but start and stop chosen according to the first view size.
        * - 2
            - View box (box axes drawn dynamically).
        * - 3
            - Erase axes.


        Arguments which specify the numbers on the axis are rounded, 
            and the number of tic marks is chosen so that axis labels are short numbers 
            (eg. not 3.3333333... or the like). 

        The *xpos* argument gives the location of the yaxis on the xaxis (default 0). 

        Without the *ntic* argument (or *ntic* =-1), 
            the number of tics will be chosen for you. 

        *nminor* is the number 
            of minor tic marks. 

        *shownumbers* = 0 will not draw the axis labels. 

        *invert* = 1 will invert the axes. 

         
        .. note:: 
         
            It is easiest to control the size of the axes and the scale of 
            the graph through the graphical user interface.  Normally, when a 
            new graph is declared (eg. ``g = h.Graph()``), the y axis 
            ranges from 20-180 and the x axis ranges from 50-250. 
            With the mouse arrow on the graph window, click on the right button 
            and set the arrow on :guilabel:`View` at the top of the button window 
            column.  A second button 
            window will appear to the right of the first, and from this button window 
            you can select several options.  Two of the most common are: 


            1)  view=plot
                    Size the window to best-fit the plot which it contains. 

            2)  Zoom in/out 
                    Allows you to click on the left mouse button and perform the following 
                    tasks: 
                    
                    move arrow to the right 
                        scale down the x axis (eg. 50 - 250 becomes 100 - 110) 

                    "shift" + move arrow to the right 
                        view parts of the axis which are to the left of the original window 

                    move arrow to the left 
                        scale up the x axis (eg. 50 - 250 becomes -100 - 500) 

                    "shift" + move arrow to the left 
                        view parts of the axis which are to the right of the original window 

                    move arrow up 
                        scale down the y axis (eg. 20 - 180 becomes 57.5 - 62) 

                    "shift" + move arrow up 
                        view parts of the axis which are below the original window 

                    move arrow down 
                        scale up the y axis (eg. 20 - 180 becomes -10,000 - 5,000) 

                    "shift" + move arrow down 
                        view parts of the axis which are above the original window 


        You can also use the :meth:`Graph.size` method to determine the size of what you view in the 
        graph window.  Eg. ``g.size(-1,1,-1,1)`` makes both axes go from -1 to 1. 

         

----



.. method:: Graph.addvar


    Syntax:
        ``g.addvar("label", _ref_variable)``

        ``g.addvar("label", _ref_variable, color_index, brush_index)``


    Description:
        Add the variable to the list of items graphed when ``g.plot(x)`` is called. 
        The address of the variable is used so this is fast. The current 
        color and brush is used if the optional arguments are not present.

        Additional syntaxes are available for plotting HOC variables.
    
    .. note::
    
        To automatically plot a variable added to a graph ``g`` with ``addvar`` against
        ``t`` during a ``run()``, ``stdrun.hoc`` must be loaded (this is done automatically
        with a ``from neuron import gui``) and the graph must be
        added to a graphList, such as by executing ``h.graphList[0].append(g)``.

    Example:

        .. code::

            g.addvar('Calcium', soma(0.5)._ref_cai)

         

----



.. method:: Graph.addexpr

    .. note::

        Not that useful in Python; only works with HOC expressions.



    Syntax:
        ``g.addexpr("HOC expression")``

        ``g.addexpr("HOC expression", color_index, brush_index)``

        ``g.addexpr("label", "HOC expr", object, ....)``


    Description:
        Add a HOC expression (eg. sin(x), cos(x), exp(x)) to the list of items graphed when 
        ``g.plot(x)`` is called. 
         
        The current 
        color and brush is used if the optional arguments are not present. A label 
        is also added to the graph that indicates the name of the variable. 
        The expression is interpreted every time ``g.plot(x)`` is 
        called so it is more general than :func:`addvar`, but slower. 
         
        If the optional label is present that string will appear as the label instead 
        of the expr string. If the optional object is present the expr will be 
        evaluated in the context of that object. 

    Example:

        .. code::

            from neuron import n, gui
            import numpy as np

            g = h.Graph()
            g.size(0, 10, -1, 1)
            g.addexpr("sin(x)")
            g.addexpr("cos(x)")
            g.addexpr("exp(-x)")

            # have to initialize the variable in HOC
            h("x = 0")

            g.begin()

            for h.x in np.arange(0, 10.1, 0.1):
                g.plot(h.x)

            g.flush()


        .. image:: ../images/graph-addexpr.png
            :align: center        

----



.. method:: Graph.addobject


    Syntax:
        ``g.addobject(rangevarplot)``

        ``g.addobject(rangevarplot, color, brush)``


    Description:
        Adds the :class:`RangeVarPlot` to the list of items to be plotted on 
        :meth:`Graph.flush` 

         

----



.. method:: Graph.begin

    .. note::

        Not that useful in Python since only works with :meth:`Graph.addexpr` which uses
        HOC expressions.

    Syntax:
        ``g.begin()``


    Description:
        Initialize the list of graph variables so the next ``g.plot(x)`` 
        is the first point of each graph line. 

        See :meth:`Graph.plot` for an example.


         

----



.. method:: Graph.plot

    .. note::

        Not that useful in Python since only works with :meth:`Graph.addexpr` and
        :meth:`Graph.xexpr` which use HOC expressions.

    Syntax:
        ``g.plot(x)``


    Description:
        The abscissa value for each item in the list of graph lines. Usually 
        used in a ``for`` loop. 

        See :meth:`Graph.addexpr` for an example.


         

----



.. method:: Graph.xexpr

    .. note::

        Not that useful in Python since only works with HOC expressions.

    Syntax:
        ``g.xexpr("HOC expression")``

        ``g.xexpr("HOC expression", usepointer)``


    Description:
        Use this expression for plotting two-dimensional functions such as (x(*t*), y(*t*)), 
        where the x and y coordinates are separately dependent on a single variable *t*. 
        This expression calculates the x value each time ``.plot`` is called, while functions 
        declared by ``.addexpr`` will calculate the y value when ``.plot`` is called. 
        This can be used for phase plane plots, etc. Note that the normal argument to 
        ``.plot`` is ignored when such an expression is invoked. When ``usepointer`` 
        is True (or 1) the expression must be a variable name and its address is used. 

    Example:

        .. code::

            from neuron import n, gui
            import numpy as np

            # Assign "g" the role of pointing to a Graph 
            # created from the Graph class, and produces 
            # a graph window with x and y axes on the  
            # screen. 
            g = h.Graph()

            # size the window to fit the graph 
            g.size(-4, 4, -4, 4)

            # store 3*sin(t) as a function to be plotted in g graphs 
            g.addexpr('3*sin(t)')

            # the next graph will be blue
            g.color(3)

            # store 3 * sin(2 * t) as a function to be plotted
            g.addexpr("3*sin(2*t)")

            # store 3*cos(t) as the x function to be plotted in g graphs 
            # The two previous expressions become the y values
            g.xexpr('3*cos(t)') 

            g.begin()
            for h.t in np.arange(0, 6.4, 0.1):
                # h.t ranges from 0 to 6.3 \approx 2 * pi
                g.plot(h.t)

            # actually draws the graph
            g.flush()
 

        plots a black circle of radius=3 and a blue infinity-like figure, spanning from x=-3 
        to x=3. 

        .. image:: ../images/graph-xexpr.png
            :align: center        


----



.. method:: Graph.flush


    Syntax:
        ``g.flush()``


    Description:
        Actually draw what has been placed in the graph scene. (If 
        you are continuing to compute you will also need to call :func:`doEvents` 
        before you see the results on the screen.) This redraws all objects 
        in the scene and therefore should not be executed very much during 
        plotting of lines with thousands of points. 

    .. warning::
        On Microsoft Windows, too many points, too close 
        together will not appear at all on a graph window.  You can, in such a case, zoom in to view 
        selected parts of the function. 

         

----



.. method:: Graph.fastflush


    Syntax:
        ``.fastflush()``


    Description:
        Flushes only the :meth:`g.plot(x) <Graph.plot>` points since the last :meth:`Graph.flush` 
        (or ``fastflush``). 
        This is useful for seeing the progress of :func:`addvar` plots during long 
        computations in which the graphlines contain many thousands of points. 
        Make sure you do a normal ``.flush`` when the lines are complete since 
        fastflush does not notify the system of the true size of the lines. 
        In such cases, zooming, translation, and crosshairs do not always 
        work properly till after the ``flush()`` command has been given. 
        (Note, this is most useful for time plots). 
         

        .. code::

            from neuron import n, gui

            g = h.Graph() 
            g.size(0, 4000, -1, 1) 
            
            # plot a bunch of cosine waves of different periods
            for val in [100, 150, 200, 250, 300, 450]:
                g.addexpr(f"cos(t/{val})")
             
            def pl():
                g.erase()
                g.begin()
                for h.t in range(4_000):
                    g.plot(h.t) 
                    if h.t % 10 == 0:
                        g.fastflush() 
                        h.doNotify() 
                g.flush() 
                h.doNotify() 

            pl() 
             

        .. image:: ../images/graph-fastflush.png
            :align: center        

         

----



.. method:: Graph.family


    Syntax:
        ``g.family(boolean)``

        ``g.family("varname")``


    Description:
        The first form is similar to the Keep Lines item in the graph menu of the 
        graphical user interface. 


        ``True`` (or 1)
            equivalent to the sequence ---Erase lines; Keep Lines toggled on; 
            use current graph color and brush when plotting the lines. 

        ``False`` (or 0)
            Turn off family mode. Original color restored to plot expressions; 
            Keep Lines toggled off. 

         
        With a string argument which is a HOC variable name, 
        the string is printed as a label and when keep lines 
        is selected each line is labeled with the value of the variable. 
         
        When graphs are printed to a file in :ref:`printtofile_ascii` mode, 
        the lines are labeled 
        with these labels. If every line has a label and each line has the same size, 
        then the file is printed in matrix form. 

         

----



.. method:: Graph.vector


    Syntax:
        ``g.vector(n, _ref_x, _ref_y)``


    Description:

        Rudimentary graphing of a y-vector vs. a fixed x-vector. The y-vector 
        is reread on each ``.flush()`` (x-vector is not reread). Cannot save 
        and cannot keep lines. 


    .. note::

        For plotting :class:`Vector` objects, it is typically easier to use         
        :meth:`Vector.plot`, :meth:`Vector.line`, and :meth:`Vector.mark`.

    .. note::

        A segmentation violation will result if 
        n is greater than the Vector size. 


    Example:

        .. code::

            from neuron import n, gui
            import numpy as np

            # x = 0, 0.01, 0.02, ..., 6.28
            x = h.Vector(np.arange(0., 6.29, 0.01))
            y = h.Vector(np.sin(x))

            # create the graph
            g = h.Graph()
            g.size(0, 6.28, -1, 1)
            g.vector(num_elements, x._ref_x[0], y._ref_x[0])
            g.flush()

        .. image:: ../images/graph-vector.png
            :align: center        



----



.. method:: Graph.getline


    Syntax:
        ``thisindex = g.getline(previndex, xvec, yvec)``


    Description:
        Copy a graph line into the :class:`Vector`\ 's xvec and yvec. Those vectors are 
        resized to the number of points in the line. Also, if the line has a 
        label, it is copied to the vector as well (see :meth:`Vector.label`). 
        The index of the line is returned. To re-get the line at a later time 
        (assuming no line has been inserted into the graphlist earlier than 
        its index value --- new lines are generally appended to the list but 
        if an earlier line has been removed, the indices of all later lines will 
        be reduced) then use index-1 as the argument. Note that an argument of 
        -1 will always return the first line in the Graph. If the argument is 
        the index of the last line then -1 is returned and xvec and yvec are 
        unchanged. Note that thisindex is not necessarily equal to previndex+1. 

    Example:
        To iterate over all the lines in ``h.Graph[0]`` use: 

        .. code-block::
            python

            xline = []
            yline = []
            xvec = h.Vector() 
            yvec = h.Vector() 
            j = 0
            i = h.Graph[0].getline(-i, xvec, yvec)
            while i != -1:
            	# xvec and yvec contain the line with Graph internal index i. 
            	# and can be associated with the sequential index j. 
            	print(j, i, yvec.label)
            	xline.append(xvec.c())
            	yline.append(yvec.cl()) # clone label as well 
                i = h.Graph[0].getline(i, xvec, yvec)

         

----



.. method:: Graph.line_info


    Syntax:
        ``thisindex = g.line_info(previndex, vector)``


    Description:
        For the next line after the internal index, previndex, copy the label into the :class:`Vector`
        ``vector`` as well as colorindex, brushindex, label x location, label y location, 
        and label style and return the index of the line. If the argument is the 
        index of the last line then -1 is returned and ``vector`` is unchanged. 
        Note that an argument of -1 will always return the line info for the first 
        polyline in the graph. 

         

----



.. method:: Graph.erase


    Syntax:
        ``g.erase()``


    Description:
        Erase only the drawings of graph lines. 

         

----



.. method:: Graph.erase_all


    Syntax:
        ``g.erase_all()``


    Description:
        Erase everything on the graph. 

         

----



.. method:: Graph.size


    Syntax:
        ``g.size(xstart, xstop, ystart, ystop)``

        ``g.size(1-4)``

        ``g.size(_ref_dbl)``


    Description:


        ``g.size(xstart, xstop, ystart, ystop)`` 
            The natural size of the scene in model coordinates. The "Whole Scene" 
            menu item in the graphical user interface will change the view to this size. 
            Default axes are this size. 

        ``g.size(1-4)`` 
            Returns left, right, bottom or top of first view of the scene. Useful for programming. 

        ``g.size(_ref_dbl)`` 
            Returns the xmin, xmax, ymin, ymax values of all marks and lines of more than two 
            points in the graph in dbl[0],..., dbl[3] respectively. This allows 
            convenient computation of a view size which will display everything on the 
            graph. See :ref:`gui_view_equal_plot`. In the absence of any graphics, it gives 
            the size as in the .size(1-4) prototype. (e.g. if ``dbl = h.Vector(4)``, then use
            ``g.size(dbl._ref_x[0])`` to store starting at the beginning.)


         

----



.. method:: Graph.label


    Syntax:
        ``g.label(x, y, "label")``

        ``g.label(x, y)``

        ``g.label("label")``

        ``g.label(x, y, "string", fixtype, scale, x_align, y_align, color)``


    Description:


        ``g.label(x, y, "label")`` 
            Draw a label at indicated position with current color. 

        ``g.label("label")`` 
            Add a label one line below the previous label 

        ``g.label(x, y)`` 
            Next ``label("string")`` will be printed at this location 

         
        The many arg form is used by sessions to completely specify an individual 
        label. 

         

----



.. method:: Graph.fixed


    Syntax:
        ``g.fixed(scale)``


    Description:
        Sizes labels. Future labels are by default 
        attached with respect to scene coordinates. The labels maintain 
        their size as the view changes. 


----



.. method:: Graph.vfixed


    Syntax:
        ``g.vfixed(scale)``


    Description:
        Sizes labels. Future labels are by default 
        attached with respect to relative view coordinates in which 
        (0,0) is the left,bottom and (1,1) is the right,top of the view. 
        Thus zooming and translation does not affect the placement of 
        the label. 

         

----



.. method:: Graph.relative


    Syntax:
        ``g.relative(scale)``


    Description:
        I never used it so I don't know if it works. The most 
        useful labels are fixed in that they maintain their size as the 
        view is zoomed. 

         

----



.. method:: Graph.align


    Syntax:
        ``g.align([x_align], [y_align])``


    Description:
        Alignment is a number between 0 and 1 which signifies which location 
        of the label is at the x,y position. .5 means centering. 0 means 
        left(bottom) alignment, 1 means right(top) alignment 

    Example:

        .. code-block::
            python

            from neuron import n, gui
            g = h.Graph() 
            g.align(0, 0) 
            g.label(.5,.5, "left bottom at (.5,.5)") 
            g.align(0, 1) 
            g.label(.5,.5, "left top at (.5,.5)") 
            g.align(1, 0) 
            g.label(.5,.5, "right bottom at (.5,.5)") 
            g.align(.5,2) 
            g.label(.5,.5, "middle but twice height (i.e. a line below) at (.5, .5)") 

        .. image:: ../images/graph-align.png
            :align: center


         

----



.. method:: Graph.color


    Syntax:
        ``g.color(index)``

        ``g.color(index, "colorname")``


    Description:
        Set the default color (starts at 1 == black). The default color palette 
        is: 

        .. list-table:: Color Codes
            :header-rows: 1

            * - Code
                - Color
            * - 0
                - White
            * - 1
                - Black
            * - 2
                - Red
            * - 3
                - Blue
            * - 4
                - Green
            * - 5
                - Orange
            * - 6
                - Brown
            * - 7
                - Violet
            * - 8
                - Yellow
            * - 9
                - Gray

        ``g.color(index, "colorname")`` 
            Install a color in the Color Palette to be accessed with that index. 
            The possible indices are 0-100. 

        The user may also use the colors/brushes button in the graphical user interface, which 
        is called by placing the mouse arrow in the graph window and pressing the right button. 

         

----



.. method:: Graph.brush


    Syntax:
        ``g.brush(index)``

        ``g.brush(index, pattern, width)``


    Description:


        ``g.brush(index)`` 
            Set the default brush. 0 is the thinnest line possible, 1-4 are 
            thickness in pixel. Higher indices cycle through these line 
            thicknesses with different brush patterns. 

        ``g.brush(index, pattern, width)`` 
            Install a brush in the Brush Palette to be accessed with the index. 
            The width is in pixel coords (< 1000). The pattern is a 31 bit pattern 
            of 1's and 0's which is used to make dash patterns. Fractional widths 
            work with postscript but not idraw. Axes are drawn with the 
            ``nrn.defaults`` property ``*default_brush: 0.0`` 

        The user may also use the :ref:`gui_changecolor_brush` button in the graphical user interface, which 
        is called by placing the mouse arrow in the graph window and pressing the right button. 

         

----



.. method:: Graph.view


    Syntax:
        ``g.view(mleft, mbottom, mwidth, mheight, wleft, wtop, wwidth, wheight)``

        ``g.view(2)``


    Description:
        Map a view of the Shape scene. *m* stands for model coordinates 
        within the window, 
        *w* stands for screen coordinates for placement and size of the 
        window. The placement of the window with respect to the screen 
        is intended to be precise and is with respect to pixel coordinates 
        where 0,0 is the top left corner of the screen. 
         
        The single argument form maps a view in which the aspect ratio 
        between x and y axes is always 1. eg like a shape window. 

         

----



.. method:: Graph.save_name


    Syntax:
        ``g.save_name("objectvar")``

        ``g.save_name("objectvar", 1)``


    Description:
        The objectvar used to save the scene when the print window 
        manager is used to save a session. 
        If the second arg is present then info about the graph 
        is immediately saved to the open session file. This is used by objects 
        that create their own graphs but need to save graph information. 

         

----



.. method:: Graph.beginline


    Syntax:
        ``g.beginline()``

        ``g.beginline(color_index, brush_index)``

        ``g.beginline("label")``

        ``g.beginline("label", color, brush)``


    Description:
        State that the next ``g.line(x)`` 
        is the first point of the next line to be graphed. 
        This is a less general command than ``g.begin()`` which prepares a graph for 
        the ``g.plot()`` command. 
        The optional label argument labels the line. 

         

         

----



.. method:: Graph.line


    Syntax:
        ``g.line(x, y)``


    Description:
        Draw a line from the previous point to this point. This command is normally 
        used inside of a ``for`` loop.  It is analogous to :meth:`Graph.plot` and the commands which 
        go along with it but avoids the need to use HOC expressions, since it plots one line at
        a time.
         
        This method takes arguments for both x and y values, so it can serve the same purpose of 
        the :meth:`Graph.plot` method in conjunction with an :meth:`Graph.addexpr` command and an ``.xexpr()`` 
        command. 

    Example:

        .. code-block::
            python

              
            from neuron import n, gui
            import numpy as np

            g = h.Graph()
            g.size(-1, 1, -1, 1)

            g.beginline()   
            dt = 0.1
            for t in np.arange(0, 2 * h.PI + dt, dt):
                g.line(h.sin(t), h.cos(t))

            g.flush()
             
            	 

         
        graphs a circle of radius = 1.

----



.. method:: Graph.mark


    Syntax:
        ``g.mark(x, y)``

        ``g.mark(x, y, "style")``

        ``g.mark(x, y, "style", size)``

        ``g.mark(x, y, "style", size, color, brush)``


    Description:
        Make a mark centered at the indicated position which does not 
        change size when window is zoomed or resized. The style is a single 
        character ``+, o, s, t, O, S, T, |, -`` where ``o,t,s`` stand for circle, triangle, 
        square and capitalized means filled. Default size is 12 points. 
        For the style, an integer index, 0-8, relative to the above list may 
        also be used. 

         

----



.. method:: Graph.crosshair_action


    Syntax:
        ``g.crosshair_action(py_callable)``

        ``g.crosshair_action(py_callable, vectorflag=0)``

        ``g.crosshair_action("")``


    Description:
        While the crosshair is visible (left mouse button pressed) one 
        can type any key and the procedure will be executed with 
        three arguments added: ``py_callable(x, y, c)``
        where x and y are the coordinates of the crosshair (in model 
        coordinates) and c is the ascii code for the key pressed. 
         
        When the optional vectorflag argument is 1, then, just prior 
        to each call of the *procedure_name* due to a keypress, 
        two temporary :class:`Vector` objects are created 
        and the line coordinate data is copied to those Vectors. 
        With this form the call to the procedure has two args added: 
        ``procedure_name(i, c, xvec, yvec)`` 
        where ``i`` is the index of the crosshair into the Vector. 
         
        If you wish the :class:`Vector` data to persist then you can assign to 
        another objectvar before returning from the ``py_callable``. 
        Note that one can copy any line to a :class:`Vector` with this method whereas 
        the interpreter controlled ``Graph.dump("expr", y_objectref)`` is 
        limited to the current graphline of an ``addvar`` or ``addexpr``. 
         
        With an empty string arg, the existing action is removed. 

    Example:

        .. code-block::
            python

            from neuron import n, gui

            g = h.Graph()

            def crossact(x, y, c):
              '''For g.crosshair_action(crossact)'''
              print (f"x={x} y={y} c={int(c)}")

            def crossact_vflag1(i, c, x, y):
              '''For g.crosshair_action(crossact_vflag1, 1)'''
              i = int(i)
              print (f"i={i} x[i]={x[i]} y[i]={y[i]} c={int(c)}")

            g.crosshair_action(crossact_vflag1, 1)

            # plot something
            x = h.Vector(range(50, 101))
            y = x + 50  # needs NEURON 7.7+
            y.line(g, x)

            # now click/drag on the plotted line and occasionally press a key

    Example:

        .. code-block::
            python

            from neuron import n, gui
            import numpy as np

            # x = 0, 0.01, 0.02, ..., 6.28
            x = h.Vector(np.arange(0., 6.29, 0.01))
            y = h.Vector(np.sin(x))

            # create the graph
            g = h.Graph()
            g.size(0, 6.28, -1, 1)
            g.vector(num_elements, x._ref_x[0], y._ref_x[0])

            def crosshair(x, y, key):
                print(f'x = {x}, y = {y}, key = {key}')

            g.crosshair_action(crosshair)

            g.flush()

        .. image:: ../images/graph-constructor.png
            :align: center


        To test the crosshair_action functionality, run the above code, move the mouse over the graph with the left mouse button held down, and simultaneously press a key;
        the coordinates and the key pressed will be displayed in the terminal.

    .. note::

        Python support for ``Graph.crosshair_action`` was added in NEURON 7.5.




    .. seealso::
        :ref:`gui_PickVector`, :func:`menu_tool`

         

----



.. method:: Graph.view_count


    Syntax:
        ``g.view_count()``


    Description:
        Returns number of views into this scene. (``stdrun.hoc`` removes 
        scenes from the ``flush_list`` and ``graphList[]`` when this goes to 
        0. If no other ``objectvar`` points to the scene, it will be 
        freed.) 

         

----



.. method:: Graph.unmap


    Syntax:
        ``g.unmap()``


    Description:
        Dismiss all windows that are a direct view into this scene. 
        (does not unmap boxes containing scenes.) ``.unmap`` is called 
        automatically when no object variable references the ``Graph``. 

         

----



.. method:: Graph.printfile


    Syntax:
        ``g.printfile("filename")``


    Description:
        Print the first view of the graph as an encapsulated post script 
        file. 

         

----



.. method:: Graph.menu_remove


    Syntax:
        ``g.menu_remove("item name")``


    Description:
        Removes the named menu item from the :class:`Graph` instance. 

         

----



.. method:: Graph.exec_menu


    Syntax:
        ``g.exec_menu("item name")``


    Description:
        Equivalent to by pressing and releasing one of the items in the 
        Graph menu with the right mouse button. This executes an action for 
        regular items, toggles for items like "Keep Lines", and specifies the 
        left mouse tool for radio buttons. The "item name" must be identical to 
        the string in the menu item, including spaces and case. Some items may 
        not work unless the graph is mapped to the screen. Selection is with respect 
        to the primary (first) view, eg selecting "View = plot" of a Grapher will 
        always refer to the view in the Grapher tool as opposed to other views of 
        the same graph created via the "NewView" menu item. Any items created 
        with :meth:`Graph.menu_action` or :meth:`Graph.menu_tool` are selectable with this 
        function. 

    Example:

        .. code-block::
            python

            g = h.Graph() 
            g.exec_menu("Keep Lines") 


         

----



.. method:: Graph.menu_action


    Syntax:
        ``g.menu_action("label", py_callable)``


    Description:
        Add a menu item to the Graph popup menu. When pressed, the py_callable will be 
        called.

    Example:

        .. code-block::
            python
            
            from neuron import n, gui

            def say_hi():
                print('Hello world!')

            g = h.Graph()
            g.menu_action("Say hello", say_hi)

        .. image:: ../images/graph-menuaction.png
            :align: center
         

----



.. method:: Graph.menu_tool


    Syntax:
        ``g.menu_tool("label", "procedure_name")``

        ``g.menu_tool("label", "procedure_name", "select_action")``


    Description:
        Add a selectable tool menu item to the Graph popup menu or else, if an 
        :func:`xpanel` is open, an :func:`xradiobutton` will be added to the panel having the 
        same action. (note: all menu_tool radiobuttons whether in the graph menu 
        or in a panel, are in the same telltalegroup, so selecting one deselects the 
        previous selection.) 
         
        If the third arg exists, the ``select_action`` will be executed when 
        the radioitem is pressed (if it is not already selected). 
         
        When selected, the item will be marked and the label will appear on 
        the window title bar (but not if the Graph is enclosed in a :func:`VBox` ). 
        When this tool is selected, pressing the left mouse 
        button, dragging the mouse, and releasing the left button, will cause 
        ``procedure_name`` to be called with four arguments: type, x, y, keystate. 
        x and y are the scene (model) coordinates of the mouse pointer, and type is 
        2 for press, 1 for dragging, and 3 for release. Keystate reflects the 
        state of control (bit 1), shift (bit 2), and meta (bit 3) keys, ie control 
        and shift down has a value of 3. 
         
        The rate of calls for dragging is of course dependent on the time it takes 
        to execute the procedure name. 

    Example:

        .. code-block::
            python

            from neuron import n, gui

            def on_event(event_type, x, y, keystate):
                print(event_type, x, y, keystate)

            g = h.Graph()
            g.menu_tool("mouse events", on_event)

        In this example, you must first select "mouse events" from the Graph's menu,
        then left-click or drag over the graph, optionally while holding a modifier key;
        output will appear on the terminal.         


         

----



.. method:: Graph.gif


    Syntax:
        ``g.gif("file.gif")``

        ``g.gif("file.gif", left, bottom, width, height)``


    Description:
        Display the gif image in model coordinates with lower left corner at 0,0 
        or indicated left, bottom coords. The width and height of the gif file are the 
        desired width and height of the image in model coordinates, by default they 
        are the pixel Width and Height of the gif file. 

    Example:
        Suppose we have a gif with pixel width and height, wg and hg respectively. 
        Also suppose we want the gif pixel point (xg0, yg0) mapped to graph 
        model coordinate (x0, y0) and the gif pixel point (xg1, yg1) mapped to 
        graph model coordinate (x1, y1). Then the last four arguments to 
        g.gif should be: 

        .. code-block::
            none

            left = x0 - xg0*(x1-x0)/(xg1-xg0) 
            bottom = y0 - yg0*(y1-y0)/(yg1-yg0) 
            width = wg*(x1-x0)/(xg1-xg0) 
            height= hg*(y1-y0)/(yg1-yg0) 
             

        If, for example with xv, you have constructed a desired rectangle on the 
        gif and the info (xv controls/Windows/Image Info)presented is 
        Resolution: 377x420 
        Selection: 225x279 rectangle starting at 135,44 
        then use 

        .. code-block::
            none

            {wg=377 hg=420} 
            {xg0=135 yg0=420-(279+44) xg1=135+225 yg1=420-44} 


    .. warning::
        In the single arg form, if the gif size is larger than the graph model 
        coodinates, the graph is resized to the size of the gif. This prevents 
        excessive use of memory and computation time when the graph size is on 
        the order of a gif pixel. 

         

----



.. method:: Graph.view_info


    Syntax:
        ``i = g.view_info()``

        ``val = g.view_info(i, case)``

        ``val = g.view_info(i, case, model_coord)``


    Description:
         
        Return information about the ith view. 
         
        With no args the return value is the view number where the mouse is. 
        If the mouse was not last in a view of g, the return value is -1. Therefore 
        this no arg function call should only be made on a mouse down event and 
        saved for handling the other mouse events. Note that the two arg cases 
        are generally constant between a mouse down and up event. 
         
        .. list-table:: Case Codes
            :header-rows: 1

            * - Case
                - Description
            * - 1
                - Width
            * - 2
                - Height
            * - 3
                - Point width
            * - 4
                - Point height
            * - 5
                - Left
            * - 6
                - Right
            * - 7
                - Bottom
            * - 8
                - Top
            * - 9
                - Model x distance for one point
            * - 10
                - Model y distance for one point


            The following cases (11 - 14) require a third argument 
            relative location means (0,0) is lower left and (1,1) is upper right. 
        .. list-table:: Case Codes
            :header-rows: 1

            * - Case
                - Description
            * - 11
                - Relative x location (from x model coord)
            * - 12
                - Relative y location (from y model coord)
            * - 13
                - Points from left (from x model coord)
            * - 14
                - Points from top (from y model coord) 
                - Note: this last is from the top, not from the bottom.
            * - 15
                - Height of font in points         

         

----



.. method:: Graph.view_size


    Syntax:
        ``g.view_size(i, left, right, bottom, top)``


    Description:
        Specifies the model coordinates of the ith view of a Graph. 
        It is possible to use this in a :meth:`Graph.menu_tool` callback procedure. 

         

----



.. method:: Graph.glyph


    Syntax:
        ``g.glyph(glyphobject, x, y, scalex, scaley, angle, fixtype)``


    Description:
        Add the :class:`Glyph` object to the graph at indicated coordinates (the origin 
        of the Glyph will appear at x,y) first scaling the Glyph and then 
        rotating by the indicated angle in degrees. The last four arguments 
        are optional and have defaults of 1,1,0,0 respectively. Fixtype 
        refers to whether the glyph moves and scales with zooming and translation, 
        moves only with translation but does not scale, or neither moves nor 
        scales. 

         

----



.. method:: Graph.simgraph


    Syntax:
        ``g.simgraph()``


    Description:
        Adds all the :meth:`Graph.addvar` lines to a list managed by :class:`CVode` which 
        allows the local variable time step method to properly graph the lines. 
        See the implementation in 
        `share/lib/hoc/stdrun.hoc <https://github.com/neuronsimulator/nrn/blob/master/share/lib/hoc/stdrun.hoc>`_
         for usage. 

         

