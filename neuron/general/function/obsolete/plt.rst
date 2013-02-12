.. _plt_doc:

Obsolete Plotting
-----------------



.. function:: setcolor

        obsolete. See :func:`plt`.

----



.. function:: settext

        obsolete. See :func:`plt`.

----


.. function:: plt

         

    Name:
        plt, setcolor- low level plot functions 
         

    Syntax:
        ``plt(mode)``

        ``plt(mode, x, y)``

        ``setcolor(colorval)``



    Description:
        \ ``Plt()`` plots points, lines, and text using 
        the Tektronix 4010 standard. Absolute 
        coordinates of the lower left corner and upper right corner of the plot 
        region are (0,0) and (1000, 780) respectively. 
        \ ``Setcolor()`` sets the color (or pen number for HP plotter) 
         
        TURBO-C graphics drivers for VGA, EGA, CGA, and Hercules are automatically 
        selected when the first plotting command is executed. An HP7475 compatible 
        plotter may be connected to COM1:. 
         

    Options:


        \ ``plt(-1)`` 
            Place cursor in home position. 

        \ ``plt(-2)`` 
            Subsequent text printed starting at current coordinate position. 

        \ ``plt(-3)`` 
            Erase screen, cursor in home position. 

        \ ``plt(-5)`` 
            Open HP plotter on PC. 

        \ ``setcolor()`` 
            The plotter will stay open till another \ ``plt(-5)`` is executed. 

        \ ``plt(0, x, y)`` 
            Plot point. 

        \ ``plt(1, x, y)`` 
            Move to point without plotting. 

        \ ``plt(2, x, y)`` 
            Draw vector from former position to new position given by (x,y). 
            (*mode* can be any number >= 2) 

        Several extra options are available for X11 graphics. 


        \ ``plt(-4, x, y)`` 
            Erases area defined by previous plot position and 
            the point, (x, y). 

        \ ``plt(-5)`` 
            Fast mode. By default, the X11 server is flushed on every 
            plot command so one can see the plot as it develops. Fast mode caches plot 
            commands and only flushes on \ ``plt(-1)`` and \ ``setcolor()``.  Fast mode is 
            three times faster than the default mode.  It is most useful when 
            plotting is the rate limiting step of the simulation. 

        \ ``plt(-6)`` 
            X11 server flushed on every plot call. 

        When the graphic window is resized, hoc is notified after 
        the next erase command. 
         
        Argument to \ ``setcolor()`` produces the following screen 
        colors with an EGA adapter, X11 graphics: 

        .. code-block::
            none

            0      black  (pen 1 on HP plotter)         black 
            1      blue                                 white 
            2      green                                yellow 
            3      cyan                                 red 
            4      red                                  green 
            5      magenta                              blue 
            6      brown                                violet 
            7      light gray  (pen 1 on HP plotter)    cyan 
            ... 
            15     white                                green	 

         

    Example:

        .. code-block::
            none

            proc plotsin() { /* This procedure plots the sin function in red.*/ 
               setcolor(4) 
               plt(1, 100, 500)  plt(2, 100, 100) /* y-axis*/ 
               plt(1, 100, 300)  plt(2, 600, 300) /* x-axis*/ 
               plt(1, 200, 550) 
               plt(-2)  print "SIN(x) from 0 to 2*PI" /* label*/ 
               for(i=0; i<=100;i=i+1){ 
                  plt(i+1, i*500/100, 300 + 200*sin(2*PI*i/100)) 
               } 
               plt(-1) /* close plot */ 
            } 

         

    .. seealso::
        :func:`plot`, :func:`axis`, :func:`lw`
        

    .. warning::
        EGA adaptor used extensively but CGA and Hercules used hardly at all. 
         
        When the X11 graphic window is killed, hoc exits without asking about 
        unsaved edit buffers. 
         


