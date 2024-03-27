
.. _hoc_rvarplt:

         
RangeVarPlot
------------



.. hoc:class:: RangeVarPlot


    Syntax:
        ``RangeVarPlot("rangevar")``

        ``RangeVarPlot("expression involving $1")``


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
        \ ``forsec seclist`` for (*x*) f(*x*) 
        where the expression is the body of f. (Hence the use of $1 to 
        denote the arc length position of the (temporary 
        currently accessed section.) 

    .. seealso::
        :hoc:func:`distance`, :hoc:meth:`Graph.addobject`

    Example:
        An example is plotting the transfer impedance with 

        .. code-block::
            none

            objectvar imp, rvp, g 
            imp = new Impedance() 
            rvp = new RangeVarPlot("imp.transfer($1)") 
            rvp... //specify range begin and end 
            imp... //specify impedance computation 
            g = new Graph() 
            g.addobject(rvp) 


         

----



.. hoc:method:: RangeVarPlot.begin


    Syntax:
        ``.begin(x)``


    Description:
        x position of the currently accessed section that starts the 
        path used for the space plot. 

         

----



.. hoc:method:: RangeVarPlot.end


    Syntax:
        ``.end(x)``


    Description:
        x position of the currently accessed section that ends the 
        path used for the space plot. 

         

----



.. hoc:method:: RangeVarPlot.origin


    Syntax:
        ``.origin(x)``


    Description:
        x position of the currently accessed section that is treated 
        as the origin (location 0) of the space plot. The default is usually 
        suitable unless you want to have several rangvarplots in one graph 
        in which case this function is used to arrange all the plots relative 
        to each other. 

         

----



.. hoc:method:: RangeVarPlot.left


    Syntax:
        ``.left()``


    Description:
        returns the coordinate of the beginning of the path. 

         

----



.. hoc:method:: RangeVarPlot.right


    Syntax:
        ``.right()``


    Description:
        returns the coordinate of the end of the path. The total length 
        of the path is ``right() - left()``. 

         

----



.. hoc:method:: RangeVarPlot.list


    Syntax:
        ``.list(sectionlist)``


    Description:
        append the path of sections to the :hoc:class:`SectionList` object argument.
         


----



.. hoc:method:: RangeVarPlot.color


    Syntax:
        ``.color(index)``


    Description:
        Change the color property. To see the change on an already plotted 
        RangeVarPlot in a Graph, the Graph should be :hoc:meth:`~Graph.flush`\ ed.

         

----



.. hoc:method:: RangeVarPlot.to_vector


    Syntax:
        ``rvp.to_vector(yvec)``

        ``rvp.to_vector(yvec, xvec)``


    Description:
        Copy the range variable values to the :hoc:func:`Vector` yvec. yvec is resized
        to the number of range points. If the second arg is present then 
        the locations are copied to xvec. A plot of \ ``yvec.line(g, xvec)`` would 
        be identical to a plot using \ ``g.addobject(rvp)``. 

    .. seealso::
        :hoc:meth:`Graph.addobject`

         

----



.. hoc:method:: RangeVarPlot.from_vector


    Syntax:
        ``rvp.from_vector(yvec)``


    Description:
        Copy the values in yvec to the range variables along the rvp path. 
        The size of the vector must be consistent with rvp. 

         

