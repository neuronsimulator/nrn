.. _plot_doc:

Obsolete Plotting
-----------------



.. function:: axis

        See :func:`plot` 

----



.. function:: plotx


----



.. function:: ploty


----



.. function:: regraph

        See :func:`graph` 

----



.. function:: plot

         

    Name:
        plot, axis - plot relative to scale given by axes 
         

    Syntax:
        :code:`axis(args)`

        :code:`plot(mode)`

        :code:`inrange = plot(x,y)`



    Description:
        :code:`Plot()` plots relative to the origin and scale defined by 
        calls to axis.  The default x and y axes have relative units of 0 to 1 with the plot 
        located in a 5x3.5 inch area. 
         

    Options:


        :code:`plot()` 
            print parameter usage help lines. 

        :code:`plot(0)` 
            subsequent calls will plot points. 

        :code:`plot(1)` 
            next call will be a move, subsequent call will draw lines. 

        :code:`plot(x, y)` 
            plots a point (or vector) relative to the axis scale. 
            Return value is 0 if the point is clipped (out of range). 

        :code:`plot(mode, x, y)` 
            Like :code:`plt()` but with scale and origin given by axis(). 

        :code:`axis()` 
            draw axes with label values. Closes plot device. 

        :code:`axis(clip)` 
            points are not plotted if they are a factor clip off the axis scale. 
            Default is no clipping. Set clip to 1 to not plot out of axis region. 
            A value of 1.1 allows plotting slightly outside the axis boundaries. 

        :code:`axis(xorg, xsize, yorg, ysize)` 
            Size and location of the plot region. 
            (Use the plt() absolute coordinates.) 

        :code:`axis(xstart, xstop, nticx, ystart, ystop, nticy)` 
            Determines relative scale and origin. 

         
        Specification of the precision of axis tic labels is available by 
        recompiling :file:`hoc/SRC/plot.c` with :code:`#define Jaslove 1+`. With this definition, 
        the number of tics specified in the 3rd and 6th arguments of :code:`axis()` should 
        be of the form m.n. m is the number of tic marks, and n is the number of 
        digits after the decimal point which are printed. This contribution was 
        made by Stewart Jaslove. 
         

    Example:

        .. code-block::
            none

            proc plotsin() { /* plot the sin function from 0 to 10 radians */ 
               axis(0, 10, 5, -1, 1, 2) /* setup scale */ 
               plot(1) 
               for (x=0; x<=10; x=x+.1) { 
                  plot(x, sin(x)) /* plot the function */ 
               } 
               axis() /* draw the axes */ 
            } 

         

    .. seealso::
        :func:`plt`, :func:`setcolor`, :func:`axis`
        

         

