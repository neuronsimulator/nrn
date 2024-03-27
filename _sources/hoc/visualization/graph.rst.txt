
.. _graph:

         
Graph
-----



.. hoc:class:: Graph


    Syntax:
        ``g = new Graph()``

        ``g = new Graph(0)``


    Description:
         
        An instance of the Graph class  manages a window on which  x-y plots can 
        be drawn by calling various member functions. 
        The first form immediately maps the window to the screen. With a 0 argument 
        the window is not mapped but can be sized and placed with the :hoc:func:`view` function.
         

    Example:
        The most basic interpreter prototype for producing a plot follows: 
         

        .. code-block::
            none

            objref g	//Creates an object reference "g" which can be made to 
            		//point to any object. 
            g = new Graph()		//Assigns "g" the role of pointing to a Graph instance 
            			//created from the Graph class, and produces 
            			//a graph window with x and y axes on the  
            			//screen. 
            g.size(0, 10, -1, 1)	// specify coordinate system for the canvas drawing area 
            			// numbers refer to xmin, xmax, ymin, ymax respectively 
            g.beginline()		//The next g.line command will move the drawing pen 
            			// to the indicated point without drawing anything 
            for(x=0; x<=10; x=x+0.1){	//States that x values to be plotted 
            				//will go from 0 to 10 in increments 
            				//of 0.1. 
            	g.line(x, sin(x))	//States that the y values on the plot 
            				//will be the sin of the x values. 
            } 
            g.flush()	//Actually draws the plot on the graph in the window. 

         
        The function ``.line()``, however, only allows the user to plot one function 
        per ``for`` loop, whereas the function ``.plot()`` can produce several 
        plots per ``for`` loop and is therefore more effective in comparing plots. 
        You must use ``.begin()`` and ``.addvar()`` or ``.addexpr()`` in 
        conjunction with the ``.plot`` function. 
         

        .. code-block::
            none

            objref g 
            g = new Graph() 
            g.size(0, 10, -1, 1) 
            g.addexpr("sin(x)")	//stores sin(x) as a function to be plotted in g 
            g.addexpr("cos(x)")	//stores cos(x) for use with g 
            g.addexpr("exp(-x)")	//stores exp(x) for use with g 
            x=0 
            g.begin()		//The next g.plot command will move the drawing pens 
            			// for the three curves to indicated x position 
            for(x=0; x<=10; x=x+0.1){ 
            	g.plot(x)	// The x value used for each expression in the 
            			// addexpr list 
            } 
            g.flush() 

         
        The size in the above example is appropriate to show the sine waves nicely 
        but the view of the exponential only shows the first few points before it 
        goes out of view. Hold the right mouse button while the mouse in in the 
        graph window and select the "View = plot" menu item to see the entire exponential. 
        Selecting the "Whole Scene" menu item will return to the size specified 
        in the size command. 

         

----



.. hoc:method:: Graph.xaxis


    Syntax:
        ``g.xaxis()``

        ``g.xaxis(mode)``

        ``g.xaxis(xstart, xstop)``

        ``g.xaxis(xstart, xstop, ypos, ntic, nminor, invert, shownumbers)``


    Description:
        The single mode argument draws both x and y axes (no arg == mode 0). 
        See :hoc:func:`yaxis` for a complete description of the arguments.

         

----



.. hoc:method:: Graph.yaxis


    Syntax:
        ``g.yaxis()``

        ``g.yaxis(mode)``

        ``g.yaxis(ystart, ystop)``

        ``g.yaxis(ystart, ystop, ypos, ntic, nminor, invert, shownumbers)``


    Description:
        The single mode argument draws both x and y axes (no arg == mode 0). 


        mode = 0 
            view axes (axes in each view drawn dynamically) 
            when graph is created these axes are the default 

        mode = 1 
            fixed axes as in long form but start and stop chosen 
            according to first view size. 

        mode = 2 
            view box (box axes drawn dynamically) 

        mode = 3 
            erase axes 



        Arguments which specify the numbers on the axis are rounded, 
            and the number of tic marks is chosen so that axis labels are short numbers 
            (eg. not 3.3333333... or the like). 

        The *xpos* argument gives the location of the yaxis on the xaxis (default 0). 

        Without the *ntic* argument (or *ntic*\ =-1), 
            the number of tics will be chosen for you. 

        *nminor* is the number 
            of minor tic marks. 

        *shownumbers*\ =0 will not draw the axis labels. 

        *invert*\ =1 will invert the axes. 

         
        Note: 
         
        It is easiest to control the size of the axes and the scale of 
        the graph through the graphical user interface.  Normally, when a 
        new graph is declared (eg. ``g = new Graph()``), the y axis 
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


        You can also use the size command to determine the size of what you view in the 
        graph window.  Eg. ``g.size(-1,1,-1,1)`` makes both axes go from -1 to 1. 

         

----



.. hoc:method:: Graph.addvar


    Syntax:
        ``g.addvar("variable")``

        ``g.addvar("variable", color_index, brush_index)``

        ``g.addvar("label", "variable")``

        ``g.addvar("label", "variable", color_index, brush_index)``

        ``g.addvar("label", &variable, ...)``


    Description:
        Add the variable to the list of items graphed when ``g.plot(x)`` is called. 
        The address of the variable is computed so this is fast. The current 
        color and brush is used if the optional arguments are not present. The name 
        of the variable is 
        also added to the graph as a label associated with the line. If the 
        first two args are strings, then the first "label" arg is associated 
        with the line on the 
        graph whereas the second arg defines the variable. 
         
        The second arg may be an explicit pointer arg which allows g.addvar to be 
        used in Python using section(x)._ref_rangevar . 
    
    .. note::
    
        To automatically plot a variable added to a graph ``g`` with addvar against
        ``t`` during a ``run()``, ``stdrun.hoc`` must be loaded and the graph must be
        added to a graphList, such as by executing ``graphList[0].append(g)``.

         

----



.. hoc:method:: Graph.addexpr


    Syntax:
        ``g.addexpr("expression")``

        ``g.addexpr("expression", color_index, brush_index)``

        ``g.addexpr("label", "expr", object, ....)``


    Description:
        Add an expression (eg. sin(x), cos(x), exp(x)) to the list of items graphed when 
        ``g.plot(x)`` is called. 
         
        The current 
        color and brush is used if the optional arguments are not present. A label 
        is also added to the graph that indicates the name of the variable. 
        The expression is interpreted every time ``g.plot(x)`` is 
        called so it is more general than :hoc:func:`addvar`, but slower.
         
        If the optional label is present that string will appear as the label instead 
        of the expr string. If the optional object is present the expr will be 
        evaluated in the context of that object. 

    Example:

        .. code-block::
            none

            objref g	//Creates an object reference "g" which will 
            		//point to the graph object. 
            g = new Graph()		//Assigns "g" the role of pointing to a Graph 
            g.size(0,10,-1,1)	//created from the Graph class, and produces 
            			//a graph window with x and y axes on the  
            			//screen. 
            g.addexpr("sin(x)")	//stores sin(x) as a function to be plotted in g graphs 
            g.addexpr("cos(x)")	//stores cos(x) for use with g 
            g.addexpr("exp(-x)")	//stores exp(x) for use with g 
            x=0			// has to be defined prior to execution of expressions 
            g.begin()		//Tells the interpreter that commands to plot  
            			//specific functions will follow. 
            for(x=0; x<=10; x=x+0.1){	//States that x values to be plotted 
            				//will go from 0 to 10 in increments 
            				//of 0.1. 
            	g.plot(x)	//States that the y values on the plot 
            			//will be the sin of the x values. 
            } 
            g.flush()	//Actually draws the plot on the graph in the window. 


         

----



.. hoc:method:: Graph.addobject


    Syntax:
        ``g.addobject(rangevarplot)``

        ``g.addobject(rangevarplot, color, brush)``


    Description:
        Adds the :hoc:class:`RangeVarPlot` to the list of items to be plotted on
        :hoc:meth:`Graph.flush`

         

----



.. hoc:method:: Graph.begin


    Syntax:
        ``g.begin()``


    Description:
        Initialize the list of graph variables so the next ``g.plot(x)`` 
        is the first point of each graph line. 

    Example:

        .. code-block::
            none

            objref g	//Creates an object reference "g" which will 
            		//point to the graph object. 
            g = new Graph()		//Assigns "g" the role of pointing to a Graph 
            			//created from the Graph class, and produces 
            			//a graph window with x and y axes on the  
            			//screen. 
            g.addexpr("sin(x)")	//stores sin(x) as a function to be plotted in g graphs 
            g.addexpr("cos(x)")	//stores cos(x) for use with g 
            g.addexpr("-exp(x)")	//stores exp(x) for use with g 
            x=0 
            g.begin()		//Tells the interpreter that commands to plot  
            			//specific functions will follow. 
            for(x=0; x<=10; x=x+0.1){	//States that x values to be plotted 
            				//will go from 0 to 10 in increments 
            				//of 0.1. 
            	g.plot(x)	//States that the y values on the plot 
            			//will be the sin of the x values. 
            } 
            g.flush()	//Actually draws the plot on the graph in the window. 


         

----



.. hoc:method:: Graph.plot


    Syntax:
        ``g.plot(x)``


    Description:
        The abscissa value for each item in the list of graph lines. Usually 
        used in a ``for`` loop. 

    Example:

        .. code-block::
            none

            objref g	//Creates an object reference "g" which will 
            		//point to the graph object. 
            g = new Graph()		//Assigns "g" the role of pointing to a Graph 
            			//created from the Graph class, and produces 
            			//a graph window with x and y axes on the  
            			//screen. 
            g.addexpr("sin(x)")	//stores sin(x) as a function to be plotted in g graphs 
            g.addexpr("cos(x)")	//stores cos(x) for use with g 
            g.addexpr("cos(2*x)")	//stores cos(2*x) for use with g 
            x=0 
            g.begin()		//Tells the interpreter that commands to plot  
            			//specific functions will follow. 
            for(x=0; x<=10; x=x+0.1){	//States that x values to be plotted 
            				//will go from 0 to 10 in increments 
            				//of 0.1. 
            	g.plot(x)	//States that the y values on the plot 
            			//will be the sin of the x values. 
            } 
            g.flush()	//Actually draws the plot on the graph in the window. 


         

----



.. hoc:method:: Graph.xexpr


    Syntax:
        ``g.xexpr("expression")``

        ``g.xexpr("expression", usepointer)``


    Description:
        Use this expression for plotting two-dimensional functions such as (x(*t*), y(*t*)), 
        where the x and y coordinates are separately dependent on a single variable *t*. 
        This expression calculates the x value each time ``.plot`` is called, while functions 
        declared by ``.addexpr`` will calculate the y value when ``.plot`` is called. 
        This can be used for phase plane plots, etc. Note that the normal argument to 
        ``.plot`` is ignored when such an expression is invoked. When ``usepointer`` 
        is 1 the expression must be a variable name and its address is used. 

    Example:

        .. code-block::
            none

            objref g	//Creates an object reference "g" which will 
            		//point to the graph object. 
            g = new Graph()		//Assigns "g" the role of pointing to a Graph 
            			//created from the Graph class, and produces 
            			//a graph window with x and y axes on the  
            			//screen. 
            g.size(-4,4,-4,4)	//sizes the window to fit the graph 
            t = 0		//Declares t as a possible variable 
            g.addexpr("3*sin(t)")	//stores 3*sin(t) as a function to be plotted in g graphs 
            g.color(3)		//the next graph will be drawn in blue 
            g.addexpr("3*sin(2*t)") //stores 3*sin(2*t) as a function to be plotted 
            g.xexpr("3*cos(t)")	//stores 3*cos(t) as the x function to be plotted in g graphs 
            			//sin(x) becomes the y function 
            g.begin()		//Tells the interpreter that commands to plot  
            			//specific functions will follow. 
            for(t=0; t<=2*PI+0.1; t=t+0.1){	//States that x values to be plotted 
            				//will go from 0 to 10 in increments 
            				//of 0.1. 
            	g.plot(t)	//States that the y values on the plot 
            			//will be the sin of the x values. 
            } 
            g.flush()	//Actually draws the plot on the graph in the window. 

        plots a black circle of radius=3 and a blue infinity-like figure, spanning from x=-3 
        to x=3. 

         

----



.. hoc:method:: Graph.flush


    Syntax:
        ``.flush()``


    Description:
        Actually draw what has been placed in the graph scene. (If 
        you are continuing to compute you will also need to call :hoc:func:`doEvents`
        before you see the results on the screen.) This redraws all objects 
        in the scene and therefore should not be executed very much during 
        plotting of lines with thousands of points. 

    .. warning::
        Because Microsoft Windows is a second-class operating system, too many points, too close 
        together will not appear at all on a graph window.  You can, in such a case, zoom in to view 
        selected parts of the function. 

         

----



.. hoc:method:: Graph.fastflush


    Syntax:
        ``.fastflush()``


    Description:
        Flushes only the :hoc:func:`plot` (x) points since the last :hoc:func:`flush`
        (or ``fastflush``). 
        This is useful for seeing the progress of :hoc:func:`addvar` plots during long
        computations in which the graphlines contain many thousands of points. 
        Make sure you do a normal ``.flush`` when the lines are complete since 
        fastflush does not notify the system of the true size of the lines. 
        In such cases, zooming, translation, and crosshairs do not always 
        work properly till after the ``flush()`` command has been given. 
        (Note, this is most useful for time plots). 
         

        .. code-block::
            none

            objectvar g 
            g = new Graph() 
            g.size(0,4000, -1,1) 
             
            g.addexpr("cos(x/100)") 
            g.addexpr("cos(x/150)") 
            g.addexpr("cos(x/200)") 
            g.addexpr("cos(x/250)") 
            g.addexpr("cos(x/300)") 
            g.addexpr("cos(x/450)") 
             
            proc pl() { 
            	g.erase() 
            	g.begin() 
            	for (x=0; x < 4000; x=x+1) { 
            		g.plot(x) 
            		if (x%10 == 0) { 
            			g.fastflush() 
            			doNotify() 
            		} 
            	} 
            	g.flush() 
            	doNotify() 
            } 
             
            pl() 
             


         

----



.. hoc:method:: Graph.family


    Syntax:
        ``g.family(boolean)``

        ``g.family("varname")``


    Description:
        The first form is similar to the Keep Lines item in the graph menu of the 
        graphical user interface. 


        1 
            equivalent to the sequence ---Erase lines; Keep Lines toggled on; 
            use current graph color and brush when plotting the lines. 

        0 
            Turn off family mode. Original color restored to plot expressions; 
            Keep Lines toggled off. 

         
        With a string argument which is a variable name, 
        the string is printed as a label and when keep lines 
        is selected each line is labeled with the value of the variable. 
         
        When graphs are printed to a file in :ref:`hoc_printtofile_ascii` mode,
        the lines are labeled 
        with these labels. If every line has a label and each line has the same size, 
        then the file is printed in matrix form. 

         

----



.. hoc:method:: Graph.vector


    Syntax:
        ``.vector(n, &x[0], &y[0])``

        ``.vector("namey")``


    Description:


        ``.vector(n, &x[0], &y[0])`` 
            Rudimentary graphing of a y-vector vs. a fixed x-vector. The y-vector 
            is reread on each ``.flush()`` (x-vector is not reread). Cannot save 
            and cannot keep lines. 
             
            Notes: 
             
            These vectors are assumed to be doubles and not vectors from 
            the Vector class.  The Vector class has its own functions 
            :hoc:meth:`Vector.plot`, :hoc:meth:`Vector.line`, :hoc:meth:`Vector.mark`
            for graphing vectors constructed in that class. 
             
            A segmentation violation will result if 
            n is greater than the vector size. 
             

        ``.vector("namey")`` 
            equivalent to ``.vector(n, ..., &namey[0])`` above with the advantage 
            that it is saved in a session (because the symbol name is known). 
            It is simpler in that the size n is obtained from the symbol but 
            the plot is vs. the index of the vector. Not implemented. 


         

----



.. hoc:method:: Graph.getline


    Syntax:
        ``thisindex = g.getline(previndex, xvec, yvec)``


    Description:
        Copy a graph line into the :hoc:class:`Vector`\ 's xvec and yvec. Those vectors are
        resized to the number of points in the line. Also, if the line has a 
        label, it is copied to the vector as well (see :hoc:meth:`Vector.label`).
        The index of the line is returned. To re-get the line at a later time 
        (assuming no line has been inserted into the graphlist earlier than 
        its index value --- new lines are generally appended to the list but 
        if an earlier line has been removed, the indices of all later lines will 
        be reduced) then use index-1 as the argument. Note that an argument of 
        -1 will always return the first line in the Graph. If the argument is 
        the index of the last line then -1 is returned and xvec and yvec are 
        unchanged. Note that thisindex is not necessarily equal to previndex+1. 

    Example:
        To iterate over all the lines in a Graph use: 

        .. code-block::
            none

            objref xvec, yvec 
            xvec = new Vector() 
            yvec = new Vector() 
            for (j=0 i=-1; (i = Graph[0].getline(i, xvec, yvec) != -1 ; j+=1 ) { 
            	// xvec and yvec contain the line with Graph internal index i. 
            	// and can be associated with the sequential index j. 
            	print j, i, yvec.label 
            	xline[j] = xvec.c 
            	yline[j] = yvec.cl // clone label as well 
            } 


         

----



.. hoc:method:: Graph.line_info


    Syntax:
        ``thisindex = g.line_info(previndex, Vector(5))``


    Description:
        For the next line after the internal index, previndex, copy the label into the 
        vector as well as colorindex, brushindex, label x location, label y location, 
        and label style and return the index of the line. If the argument is the 
        index of the last line then -1 is returned and Vector is unchanged. 
        Note that an argument of -1 will always return the line info for the first 
        polyline in the graph. 

         

----



.. hoc:method:: Graph.erase


    Syntax:
        ``.erase()``


    Description:
        Erase only the drawings of graph lines. 

         

----



.. hoc:method:: Graph.erase_all


    Syntax:
        ``e.erase_all()``


    Description:
        Erase everything on the graph. 

         

----



.. hoc:method:: Graph.size


    Syntax:
        ``g.size(xstart, xstop, ystart, ystop)``

        ``g.size(1-4)``

        ``g.size(&dbl[0])``


    Description:


        .size(*xstart*, *xstop*, *ystart*, *ystop*) 
            The natural size of the scene in model coordinates. The "Whole Scene" 
            menu item in the graphical user interface will change the view to this size. 
            Default axes are this size. 

        .size(1-4) 
            Returns left, right, bottom or top of first view of the scene. Useful for programming. 

        .size(&dbl[0]) 
            Returns the xmin, xmax, ymin, ymax values of all marks and lines of more than two 
            points in the graph in dbl[0],..., dbl[3] respectively. This allows 
            convenient computation of a view size which will display everything on the 
            graph. See :ref:`hoc_gui_view_equal_plot`. In the absence of any graphics, it gives
            the size as in the .size(1-4) prototype. 


         

----



.. hoc:method:: Graph.label


    Syntax:
        ``.label(x, y, "label")``

        ``.label(x, y)``

        ``.label("label")``

        ``.label(x, y, "string", fixtype, scale, x_align, y_align, color)``


    Description:


        ``.label(x, y, "label")`` 
            Draw a label at indicated position with current color. 

        ``.label("label")`` 
            Add a label one line below the previous label 

        ``.label(x, y)`` 
            Next ``label("string")`` will be printed at this location 

         
        The many arg form is used by sessions to completely specify an individual 
        label. 

         

----



.. hoc:method:: Graph.fixed


    Syntax:
        ``.fixed(scale)``


    Description:
        Sizes labels. Future labels are by default 
        attached with respect to scene coordinates. The labels maintain 
        their size as the view changes. 


----



.. hoc:method:: Graph.vfixed


    Syntax:
        ``.vfixed(scale)``


    Description:
        Sizes labels. Future labels are by default 
        attached with respect to relative view coordinates in which 
        (0,0) is the left,bottom and (1,1) is the right,top of the view. 
        Thus zooming and translation does not affect the placement of 
        the label. 

         

----



.. hoc:method:: Graph.relative


    Syntax:
        ``.relative(scale)``


    Description:
        I never used it so I don't know if it works. The most 
        useful labels are fixed in that they maintain their size as the 
        view is zoomed. 

         

----



.. hoc:method:: Graph.align


    Syntax:
        ``.align([x_align], [y_align])``


    Description:
        Alignment is a number between 0 and 1 which signifies which location 
        of the label is at the x,y position. .5 means centering. 0 means 
        left(bottom) alignment, 1 means right(top) alignment 

    Example:

        .. code-block::
            none

            objref g 
            g = new Graph() 
            g.align(0, 0) 
            g.label(.5,.5, "left bottom at (.5,.5)") 
            g.align(0, 1) 
            g.label(.5,.5, "left top at (.5,.5)") 
            g.align(1, 0) 
            g.label(.5,.5, "right bottom at (.5,.5)") 
            g.align(.5,2) 
            g.label(.5,.5, "middle but twice height at (.5, .5)") 


         

----



.. hoc:method:: Graph.color


    Syntax:
        ``.color(index)``

        ``.color(index, "colorname")``


    Description:
        Set the default color (starts at 1 == black). The default color palette 
        is: 

        .. code-block::
            none

            0 white 
            1 black 
            2 red 
            3 blue 
            4 green 
            5 orange 
            6 brown 
            7 violet 
            8 yellow 
            9 gray 



        ``.color(index, "colorname")`` 
            Install a color in the Color Palette to be accessed with that index. 
            The possible indices are 0-100. 

        The user may also use the colors/brushes button in the graphical user interface, which 
        is called by placing the mouse arrow in the graph window and pressing the right button. 

         

----



.. hoc:method:: Graph.brush


    Syntax:
        ``.brush(index)``

        ``.brush(index, pattern, width)``


    Description:


        ``.brush(index)`` 
            Set the default brush. 0 is the thinnest line possible, 1-4 are 
            thickness in pixel. Higher indices cycle through these line 
            thicknesses with different brush patterns. 

        ``.brush(index, pattern, width)`` 
            Install a brush in the Brush Palette to be accessed with the index. 
            The width is in pixel coords (< 1000). The pattern is a 31 bit pattern 
            of 1's and 0's which is used to make dash patterns. Fractional widths 
            work with postscript but not idraw. Axes are drawn with the 
            nrn.defaults property ``*default_brush: 0.0`` 

        The user may also use the :ref:`hoc_gui_changecolor_brush` button in the graphical user interface, which
        is called by placing the mouse arrow in the graph window and pressing the right button. 

         

----



.. hoc:method:: Graph.view


    Syntax:
        ``.view(mleft, mbottom, mwidth, mheight, wleft,``

        ``wtop, wwidth, wheight)``

        ``.view(2)``


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



.. hoc:method:: Graph.save_name


    Syntax:
        ``.save_name("objectvar")``

        ``.save_name("objectvar", 1)``


    Description:
        The objectvar used to save the scene when the print window 
        manager is used to save a session. 
        If the second arg is present then info about the graph 
        is immediately saved to the open session file. This is used by objects 
        that create their own graphs but need to save graph information. 

         

----



.. hoc:method:: Graph.beginline


    Syntax:
        ``.beginline()``

        ``.beginline(color_index, brush_index)``

        ``.beginline("label")``

        ``.beginline("label", color, brush)``


    Description:
        State that the next ``g.line(x)`` 
        is the first point of the next line to be graphed. 
        This is a less general command than ``.begin()`` which prepares a graph for 
        the ``.plot()`` command. 
        The optional label argument labels the line. 

    Example:
        Notice that the argument to ``g.line()`` is the expression sin(x) 
        itself, whereas if you were using the ``.plot()`` command, the arguments 
        would have to be specified before the ``for`` loop using ``.addexpr()`` 
        commands. The addexpr/begin/plot method of plotting is preferred since it 
        is capable of simultaneously plotting multiple lines. 

        .. code-block::
            none

            objref g	//Creates an object reference "g" which will 
            		//point to the graph object. 
            g = new Graph()		//Assigns "g" the role of pointing to a Graph 
            			//created from the Graph class, and produces 
            			//a graph window with x and y axes on the  
            			//screen. 
            g.beginline()		//Tells the interpreter that commands to create a line for 
            			//specific functions will follow. 
            for(x=0; x<=10; x=x+0.1){	//States that x values to be plotted 
            				//will go from 0 to 10 in increments 
            				//of 0.1. 
            	g.line(x, sin(x))	//States that the y values on the line 
            				//will be the sin of the x values. 
            } 
            g.flush()	//Actually draws the plot on the graph in the window. 

         

         

----



.. hoc:method:: Graph.line


    Syntax:
        ``.line(x, y)``


    Description:
        Draw a line from the previous point to this point. This command is normally 
        used inside of a ``for`` loop.  It is analogous to ``.plot()`` and the commands which 
        go along with it.  In the case of ``.line()`` however, all arguments are given in 
        the line command itself.  Therefore, the line command only plots one line at a time, whereas 
        the ``.plot*()`` command can plot several lines using the same for loop on the same graph. 
         
        This command takes arguments for both x and y values, so it can serve the same purpose of 
        the ``.plot`` command in conjunction with an ``.addexpr()`` command and an ``.xexpr()`` 
        command. 

    Example:

        .. code-block::
            none

             
            objref g	 
            g = new Graph()		 
            g.beginline()		 
            for(t=0; t<=2*PI+0.1; t=t+0.1){	 
            	g.line(sin(t), cos(t))	 
            } 
            g.flush() 
            	 

         
        graphs a circle of radius=1, just as would the following code using ``g.plot()``: 
         

        .. code-block::
            none

             
            objref g	 
            g = new Graph()		 
            t = 0		 
            g.addexpr("sin(t)")	 
            g.xexpr("cos(t)")	 
            g.begin()		 
            for(t=0; t<=2*PI+0.1; t=t+0.1){	 
            	g.plot(t)	 
            } 
            g.flush()	 
             

         
        Note that the arguments to ``g.line`` are doubles, and not chars as they are in ``g.plot()``. 
         
         

         

----



.. hoc:method:: Graph.mark


    Syntax:
        ``.mark(x, y)``

        ``.mark(x, y, "style")``

        ``.mark(x, y, "style", size)``

        ``.mark(x, y, "style", size, color, brush)``


    Description:
        Make a mark centered at the indicated position which does not 
        change size when window is zoomed or resized. The style is a single 
        character ``+, o, s, t, O, S, T, |, -`` where ``o,t,s`` stand for circle, triangle, 
        square and capitalized means filled. Default size is 12 points. 
        For the style, an integer index, 0-8, relative to the above list may 
        also be used. 

         

----



.. hoc:method:: Graph.crosshair_action


    Syntax:
        ``.crosshair_action("procedure_name")``

        ``.crosshair_action("procedure_name", vectorflag=0)``

        ``.crosshair_action("")``


    Description:
        While the crosshair is visible (left mouse button pressed) one 
        can type any key and the procedure will be executed with 
        three arguments added: 
        ``procedure_name(x, y, c)`` 
        where x and y are the coordinates of the crosshair (in model 
        coordinates) and c is the ascii code for the key pressed. 
         
        The procedure will be executed in the context of the object 
        where ``crosshair_action`` was executed. 
        When the optional vectorflag argument is 1, then, just prior 
        to each call of the *procedure_name* due to a keypress, 
        two temporary *objectref*'s are created and assigned to a 
        new ``Vector()`` and the line coordinate data is copied to those Vectors. 
        With this form the call to the procedure has two args added: 
        ``procedure_name(i, c, $o3, $o4)`` 
        where ``i`` is the index of the crosshair into the Vector. 
         
        If you wish the Vector data to persist then you can assign to 
        another objectvar before returning from the ``procedure_name``. 
        Note that one can copy any line to a Vector with this method whereas 
        the interpreter controlled ``Graph.dump("expr", y_objectref)`` is 
        limited to the current graphline of an ``addvar`` or ``addexpr``. 
         
        With an empty string arg, the existing action is removed. 

    .. seealso::
        :ref:`hoc_gui_PickVector`, :hoc:func:`menu_tool`

         

----



.. hoc:method:: Graph.view_count


    Syntax:
        ``.view_count()``


    Description:
        Returns number of views into this scene. (stdrun.hoc removes 
        scenes from the ``flush_list`` and ``graphList[]`` when this goes to 
        0. If no other ``objectvar`` points to the scene, it will be 
        freed.) 

         

----



.. hoc:method:: Graph.unmap


    Syntax:
        ``.unmap()``


    Description:
        Dismiss all windows that are a direct view into this scene. 
        (does not unmap boxes containing scenes.) ``.unmap`` is called 
        automatically when no hoc object variable references the Graph. 

         

----



.. hoc:method:: Graph.printfile


    Syntax:
        ``.printfile("filename")``


    Description:
        Print the first view of the graph as an encapsulated post script 
        file. 

         

----



.. hoc:method:: Graph.menu_remove


    Syntax:
        ``g.menu_remove("item name")``


    Description:
        Removes the named menu item from the Graph instance. 

         

----



.. hoc:method:: Graph.exec_menu


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
        with :hoc:meth:`Graph.menu_action` or :hoc:meth:`Graph.menu_tool` are selectable with this
        function. 

    Example:

        .. code-block::
            none

            objref g 
            g = new Graph() 
            g.exec_menu("Keep Lines") 


         

----



.. hoc:method:: Graph.menu_action


    Syntax:
        ``.menu_action("label", "action")``


    Description:
        Add a menu item to the Graph popup menu. When pressed, the action will be 
        executed 

    Example:

        .. code-block::
            none

            objref g 
            g = new Graph() 
            g.menu_action("Print File", "g.printfile(\"temp.eps\")  system(\"lp temp.eps\")") 


         

----



.. hoc:method:: Graph.menu_tool


    Syntax:
        ``.menu_tool("label", "procedure_name")``

        ``.menu_tool("label", "procedure_name", "select_action")``


    Description:
        Add a selectable tool menu item to the Graph popup menu or else, if an 
        :hoc:func:`xpanel` is open, an :hoc:func:`xradiobutton` will be added to the panel having the
        same action. (note: all menu_tool radiobuttons whether in the graph menu 
        or in a panel, are in the same telltalegroup, so selecting one deselects the 
        previous selection.) 
         
        If the third arg exists, the select_action will be executed when 
        the radioitem is pressed (if it is not already selected). 
         
        When selected, the item will be marked and the label will appear on 
        the window title bar (but not if the Graph is enclosed in a :hoc:func:`VBox` ).
        When this tool is selected, pressing the left mouse 
        button, dragging the mouse, and releasing the left button, will cause 
        procedure_name to be called with four arguments: type, x, y, keystate. 
        x and y are the scene (model) coordinates of the mouse pointer, and type is 
        2 for press, 1 for dragging, and 3 for release. Keystate reflects the 
        state of control (bit 1), shift (bit 2), and meta (bit 3) keys, ie control 
        and shift down has a value of 3. 
         
        The rate of calls for dragging is of course dependent on the time it takes 
        to execute the procedure name. 

    Example:

        .. code-block::
            none

            objref g 
            g = new Graph() 
            g.menu_tool("mouse events", "p") 
            proc p() { 
            	print $1, $2, $3, $4 
            } 


         

----



.. hoc:method:: Graph.gif


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



.. hoc:method:: Graph.view_info


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
         

        .. code-block::
            none

            	case 1: // width 
            	case 2: // height 
            	case 3: // point width 
            	case 4: // point height 
            	case 5: // left 
            	case 6: // right 
            	case 7: // bottom 
            	case 8: // top 
            	case 9: // model x distance for one point 
            	case 10: // model y distance for one point 
            The following cases (11 - 14) require a third argument 
            relative location means (0,0) is lower left and (1,1) is upper right. 
            	case 11: // relative x location (from x model coord) 
            	case 12: // relative y location (from y model coord) 
            	case 13: // points from left (from x model coord) 
            	case 14: // points from top (from y model coord) 
            		Note: this last is from the top, not from the bottom. 
            	case 15: // height of font in points 

         

         

----



.. hoc:method:: Graph.view_size


    Syntax:
        ``g.view_size(i, left, right, bottom, top)``


    Description:
        Specifies the model coordinates of the ith view of a Graph. 
        It is possible to use this in a :hoc:meth:`Graph.menu_tool` callback procedure.

         

----



.. hoc:method:: Graph.glyph


    Syntax:
        ``g.glyph(glyphobject, x, y, scalex, scaley, angle, fixtype)``


    Description:
        Add the :hoc:func:`Glyph` object to the graph at indicated coordinates (the origin
        of the Glyph will appear at x,y) first scaling the Glyph and then 
        rotating by the indicated angle in degrees. The last four arguments 
        are optional and have defaults of 1,1,0,0 respectively. Fixtype 
        refers to whether the glyph moves and scales with zooming and translation, 
        moves only with translation but does not scale, or neither moves nor 
        scales. 

         

----



.. hoc:method:: Graph.simgraph


    Syntax:
        ``g.simgraph()``


    Description:
        Adds all the :hoc:meth:`Graph.addvar` lines to a list managed by :hoc:class:`CVode` which
        allows the local variable time step method to properly graph the lines. 
        See the implementation in share/lib/hoc/stdrun.hoc for usage. 

         

