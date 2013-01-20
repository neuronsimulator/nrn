.. _oldgrph:

oldgrph
-------



.. function:: graphmode

        obsolete. see :func:`graph` . Use :class:`Graph` . 

----



.. function:: graph

         

    Name:
        graph --- multiple line plots 
         

    Syntax:
        :code:`graph()`

        :code:`graph(expression, setup)`

        :code:`graph(t)`

        :code:`graphmode(mode)`



    Description:
        \ :code:`Graph()` solves the problem of obtaining multiple line plots during 
        a single run. During calls to \ :code:`graph(t)`, specified variables are stored 
        and plotted using scales determined by calls to \ :code:`axis()`. 
         

    Options:


        \ :code:`graph()` @dd erases the old list and starts a new (empty) 
            list of plot expressions and setup statements. 

        \ :code:`graph(s1, s2)` @dd Adds a new plot specification 
            to the graph list. s1 must be 
            string which contains an expression, usually a variable. e.g "y". s2 is a 
            string which contains any number of statements used to initialize axes. etc. 
            E.G "\ :code:`axis(0,5,1,-1,1,2) axis()`". 

        \ :code:`graph(t)` @dd The current value of each 
            expression in the graph list is saved along with the abscissa value, t. 
            The line plots are flushed every 50 points. 

        \ :code:`graphmode(1)` @dd Executes the list of setup statements. 
            This is also done on the 
            first call to \ :code:`graph(t)` after 
            a new setup statement is added to the list. 

        \ :code:`graphmode(-1)` @dd Flushes the stored plots. Subsequent calls to 
            \ :code:`graph(t)` will start new lines. 
            Should be executed just before a \ :code:`plt(-1)` to ensure the entire lines 
            are plotted. 

        \ :code:`graphmode(2)` @dd Flushes the stored plots. Subsequent calls to 
            \ :code:`graph(t)` will continue the lines. 
            Graphs are normally flushed every  50 points. 

         

    Example:

        .. code-block::
            none

            proc p() { /* plot ramp */ 
               axis(100,300,450,200) 
               axis(0,15,3,-1,1,2) 
               axis() 
               plot(1) 
               for (x=0; x<15; x=x+.1) { 
                  plot(x, x/15)	/* ramp */ 
                  graph(x) /* plots graph list if any */ 
               } 
               graph(-1) /* flush remaining part of graphs, if any */ 
               plt(-1) 
            }	 
             
            p()    /*plots the ramp alone*/ 
             
            graph() 
            graph("sin(x)","axis(100,300,100,300) axis()") 
            graph("cos(x)","")  /* same axes as previous call to graph */ 
             
            p()    /*plots the sin and cos along with the ramp*/ 

         

    Diagnostics:
        The strings are parsed when \ :code:`graph(s1, s2)` is executed.  The strings are 
        executed on calls to \ :code:`graph(t)`. 
         
        The best method for complicated plots is to make the setup string a 
        simple call to a user defined procedure.  This procedure can setup the 
        axes, write the labels, etc.  Newlines and strings within strings are 
        possible by quoting with the `\verb+\+' character but generally are 
        too confusing to be practical. 
         
        Local variables in graph strings make no sense. 
         

    .. seealso::
        :func:`plot`
        


