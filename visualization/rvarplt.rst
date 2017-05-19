.. _rvarplt:

         
RangeVarPlot
------------



.. class:: RangeVarPlot


    Syntax:
        ``h.RangeVarPlot("rangevar")``

        ``h.RangeVarPlot(py_callable)``


    Description:
        Class for making a space plot. eg. voltage as function of path between 
        two points on a cell.  For plotting, an object of this type needs 
        to be inserted in a Graph with 
        \ ``g.addobject(rvp)`` 
        By default, the location of the path nearest the root is location 0 
        (the origin) of the space plot. 
         
        If the *rangevar* does not exist at certain places in the path it 
        is assumed to have a value of 0. 
         
        The first form where *rangevar* is "*v*" or "*m_hh*", etc. is very 
        efficient since the object can store pointers to the variables 
        for fast plotting. 
         
        The second form is much slower since the expression 
        must be executed by the interpreter for each point along the path 
        for each plot.  Execution of the expression is equivalent to 
        \ ``for sec in h.allsec(): for seg in sec: f(seg.x)``
        where the expression is the body of f. All section-dependent NEURON
        functions will default to the correct section for the call; i.e. there is no need
        to say ``sec=`` unless you want to refer to a section that is not the one
        whose data is being plotted. The current section may be read via ``h.cas()``.

    .. seealso::
        :func:`distance`, :meth:`Graph.addobject`


    Example (plotting by name):

        .. code-block::
            python

            from neuron import h, gui

            dend1 = h.Section(name='dend1')
            dend2 = h.Section(name='dend2')

            for sec in h.allsec():
                sec.nseg = sec.L = 501
                sec.diam = 1

            dend2.connect(dend1)

            ic = h.IClamp(dend1(0.5))
            ic.amp = 0.5
            ic.delay = 0
            ic.dur = 1

            h.finitialize(-65)
            h.continuerun(1)

            rvp = h.RangeVarPlot('v')
            rvp.begin(0, sec=dend1)
            rvp.end(1, sec=dend2)
            g = h.Graph()
            g.addobject(rvp)
            g.size(0, 1002, -70, 50)

        .. image:: ../images/rangevarplot1.png
            :align: center

    Example (plotting a function):

        .. code-block::
            python

            from neuron import h, gui

            dend1 = h.Section(name='dend1')
            dend2 = h.Section(name='dend2')

            for sec in h.allsec():
                sec.nseg = sec.L = 501
                sec.diam = 1

            dend2.connect(dend1)

            def my_func(x):
                sec = h.cas()  # find out which section
                if sec == dend1:
                    y = x ** 2
                else:
                    y = 1 + x ** 2
                return y

            rvp = h.RangeVarPlot(my_func)
            rvp.begin(0, sec=dend1)
            rvp.end(1, sec=dend2)
            g = h.Graph()
            g.addobject(rvp)
            g.size(0, 1002, 0, 2)
            g.flush()

        .. image:: ../images/rangevarplot2.png
            :align: center

    Example (transfer impedance):
        .. code-block::
            python

            imp = h.Impedance()

            rvp = h.RangeVarPlot(imp.transfer)
            rvp... #specify range begin and end 
            imp... #specify impedance computation 
            g = h.Graph() 
            g.addobject(rvp) 
----



.. method:: RangeVarPlot.begin


    Syntax:
        ``rvp.begin(x, sec=section)``


    Description:
        Starts the path for the space plot at the segment ``section(x)``.

         

----



.. method:: RangeVarPlot.end


    Syntax:
        ``rvp.end(x, sec=section)``


    Description:
        Ends the path for the space plot at the segment ``section(x)``.

         

----



.. method:: RangeVarPlot.origin


    Syntax:
        ``rvp.origin(x, sec=section)``


    Description:
        Defines the origin (location 0) of the space plot as ``section(x)``.
        The default is usually 
        suitable unless you want to have several rangvarplots in one graph 
        in which case this function is used to arrange all the plots relative 
        to each other. 

         

----



.. method:: RangeVarPlot.left


    Syntax:
        ``rvp.left()``


    Description:
        returns the coordinate of the beginning of the path. 

         

----



.. method:: RangeVarPlot.right


    Syntax:
        ``rvp.right()``


    Description:
        returns the coordinate of the end of the path. The total length 
        of the path is ``rvp.right() - rvp.left()``. 

         

----



.. method:: RangeVarPlot.list


    Syntax:
        ``rvp.list(sectionlist)``


    Description:
        append the path of sections to the :class:`SectionList` object argument. 
         


----



.. method:: RangeVarPlot.color


    Syntax:
        ``rvp.color(index)``


    Description:
        Change the color property. To see the change on an already plotted 
        RangeVarPlot in a Graph, the Graph should be :meth:`~Graph.flush`\ ed. 

         

----



.. method:: RangeVarPlot.to_vector


    Syntax:
        ``rvp.to_vector(yvec)``

        ``rvp.to_vector(yvec, xvec)``


    Description:
        Copy the range variable values to the :func:`Vector` yvec. yvec is resized 
        to the number of range points. If the second arg is present then 
        the locations are copied to xvec. A plot of \ ``yvec.line(g, xvec)`` would 
        be identical to a plot using \ ``g.addobject(rvp)``. 

    .. seealso::
        :meth:`Graph.addobject`

         

----



.. method:: RangeVarPlot.from_vector


    Syntax:
        ``rvp.from_vector(yvec)``


    Description:
        Copy the values in yvec to the range variables along the rvp path. 
        The size of the vector must be consistent with rvp. 

         

