PtrVector
---------

.. class:: PtrVector

  Syntax:
    :samp:`pv = h.PtrVector({size})`
    
   
  Description:

    Fast scatter and gather from a Vector to a list of pointers (must have
    same length). The size of the pointer vector is fixed at creation
    and must be an integer greater than 0.
    Pointers to variables are specified with the pset method. At creation,
    all pointers point to an internal dummy variable. So it is possible
    to scatter from a larger Vector into a smaller Vector.

    A callback should be registered with the
    :meth:`PtrVector.ptr_update_callback` method in order to prevent
    memory segfaults when internal memory is reallocated.

  Example:
  
    .. code-block::
      python
      
      from neuron import h
      a = h.Vector(range(5))
      b = h.Vector([0] * 5)
      pv = h.PtrVector(5)
      for i in range(len(a)):
        pv.pset(i, b._ref_x[i])
        
      pv.scatter(a)
      b.printf()
      b.mul(10)
      pv.gather(a)
      a.printf()

----

.. method:: PtrVector.size

  Syntax:
    ``length = pv.size()``
    
   
  Description:
    Return the number of elements in the PtrVector.
    
----

.. method:: PtrVector.resize

  Syntax:
    ``newsize = pv.resize(newsize)``


  Description:
    Old pointer array is freed and new pointer array with specified size
    is created. All the pointers point to a dummy variable. If the specified
    new size is the same as the old size, the old existing array is kept.
    Newsize must be an integer greater than 0.

----

.. method:: PtrVector.pset

  Syntax:
     ``var_val = pv.pset(i, _ref_var)``
     
    
  Description:
    The ith pointer in the PtrVector points to var. 0 <= i < pv.size()

----
 
.. method:: PtrVector.scatter

  Syntax:
    ``0. = pv.scatter(srcvec)``
    
  Description:
    The elements of the Vector argument are copied to the variables pointed
    to. The size of the Vector must be the same as the size of the PtrVector
  
----
 
.. method:: PtrVector.gather

  Syntax:
    ``0. = pv.gather(destvec)``

  Description:
    The variable values pointed to by the PtrVector are copied into the
    destination Vector.

----

.. method:: PtrVector.getval

  Syntax:
    :samp:`{val} = pv.getval({i})`

  Description:
    Return the value pointed to by the ith pointer in the PtrVector.

----

.. method:: PtrVector.setval

  Syntax:
    :samp:`{val} = pv.getval({i}, {x})`

  Description:
    Set the variable pointed to by the ith pointer to the value of x.

----

.. method:: PtrVector.ptr_update_callback

  Syntax:
    :samp:`pv.ptr_update_callback(pythoncallback)`

    :samp:`pv.ptr_update_callback("hoc_statement", [object])`


  Description:
    The statement or pythoncallback is executed whenever range variables
    are re-allocated.
    Within the callback, the :meth:`PtrVector.resize` method may be called but
    the PtrVector should not be destroyed.
    The return value is 0.

----

.. method:: PtrVector.plot

    Syntax:
        ``0 = pv.plot(graphobj)``

        ``0 = pv.plot(graphobj, color, brush)``

        ``0 = pv.plot(graphobj, x_vec)``

        ``0 = pv.plot(graphobj, x_vec, color, brush)``

        ``0 = pv.plot(graphobj, x_increment)``

        ``0 = pv.plot(graphobj, x_increment, color, brush)``


    Description:
        Analogous to :meth:`Vector.plot` but always returns 0 instead of self.
        Plots the pointer vector elements in a :class:`Graph` object.  The default is to plot the dereferenced
        elements of the 
        pointer vector as y values with their indices as x values.  An optional 
        argument can be used to 
        specify the x-axis.  Such an argument can be either a 
        vector, *x_vec*, in which case its values are used for x values, or 
        a scalar,  *x_increment*, in 
        which case x is incremented according to this number. 
         
        This function plots the 
        ``pv.getval(i)`` values that are pointed to by the pointer vector at the time of graph flushing or window 
        resizing. There is currently no corresponding alternative to :meth:`Vector.line` which plots the vector values 
        that exist at the time of the call to ``plot``.  So the best way to produce multiple line plots is to first
        :meth:`PtrVector.gather` into a Vector and use
        ``vec.line()``.
         
        Once a pointer vector is plotted, it is only necessary to call ``graphobj.flush()`` 
        in order to display further changes to the valuses pointed to.  In this way it 
        is possible to produce rather rapid line animation. 
         
        If the vector :meth:`PtrVector.label` is not empty it will be used as the label for 
        the line on the Graph. 
         
        Resizing a pointer vector that has been plotted will remove it from the Graph. 
         
        The number of points plotted is the minimum of vec.size and x_vec.size 
        at the time pv.plot is called. x_vec is assumed to be an unchanging 
        Vector. 
         

    Example:

        .. code-block::
            python

            from neuron import h, gui
            import time
            
            g = h.Graph() 
            g.size(0,10,-1,1) 
            vec = h.Vector().indgen(0, 10, 0.1) 
            vec.apply("sin")

            pv = h.PtrVector(len(vec))
            pv.label("PtrVector")
            for i in range(len(vec)):
              pv.pset(i, vec._ref_x[i])
          
            pv.plot(g, .1) 
            def do_run():
                for i in range(len(vec)):
                    vec.rotate(1)
                    g.flush()
                    h.doNotify()
                    time.sleep(0.01)

            h.xpanel("") 
            h.xbutton("run", do_run) 
            h.xpanel() 
----

.. method:: PtrVector.label

  Syntax:
    :samp:`{curstr} = pv.label("str")`

    :samp:`{curstr} = pv.label()`

  Description:
    Set the label to the string arg. Return the current label. When plotting, the label will be displayed.
    Very similar to functionality of :meth:`Vector.label`.
