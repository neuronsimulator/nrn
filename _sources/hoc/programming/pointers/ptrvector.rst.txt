PtrVector
---------

.. hoc:class:: PtrVector

  Syntax:
    :samp:`pv = new PtrVector({size})`
    
   
  Description:

    Fast scatter and gather from a Vector to a list of pointers (must have
    same length). The size of the pointer vector is fixed at creation
    and must be an integer greater than 0.
    Pointers to variables are specified with the pset method. At creation,
    all pointers point to an internal dummy variable. So it is possible
    to scatter from a larger Vector into a smaller Vector.

    A callback should be registered with the
    :hoc:meth:`PtrVector.ptr_update_callback` method in order to prevent
    memory segfaults when internal memory is reallocated.

  Python Example:
  
    .. code-block::
      python
      
      from neuron import h
      a = h.Vector(5).indgen()
      b = h.Vector(5).fill(0)
      pv = h.PtrVector(5)
      for i in range(len(a)):
        pv.pset(i, b._ref_x[i])
        
      pv.scatter(a)
      b.printf()
      b.mul(10)
      pv.gather(a)
      a.printf()

----

.. hoc:method:: PtrVector.size

  Syntax:
    ``length = pv.size()``
    
   
  Description:
    Return the number of elements in the PtrVector.
    
----

.. hoc:method:: PtrVector.resize

  Syntax:
    ``newsize = pv.resize(newsize)``


  Description:
    Old pointer array is freed and new pointer array with specified size
    is created. All the pointers point to a dummy variable. If the specified
    new size is the same as the old size, the old existing array is kept.
    Newsize must be an integer greater than 0.

----

.. hoc:method:: PtrVector.pset

  Syntax:
     ``var_val = pv.pset(i, &var)``
     
    
  Description:
    The ith pointer in the PtrVector points to var. 0 <= i < pv.size()
 
.. hoc:method:: PtrVector.scatter

  Syntax:
    ``0. = pv.scatter(srcvec)``
    
  Description:
    The elements of the Vector argument are copied to the variables pointed
    to. The size of the Vector must be the same as the size of the PtrVector
  
----
 
.. hoc:method:: PtrVector.gather

  Syntax:
    ``0. = pv.gather(destvec)``

  Description:
    The variable values pointed to by the PtrVector are copied into the
    destination Vector.

----

.. hoc:method:: PtrVector.getval

  Syntax:
    :samp:`{val} = pv.getval({i})`

  Description:
    Return the value pointed to by the ith pointer in the PtrVector.

----

.. hoc:method:: PtrVector.setval

  Syntax:
    :samp:`{val} = pv.getval({i}, {x})`

  Description:
    Set the variable pointed to by the ith pointer to the value of x.

----

.. hoc:method:: PtrVector.ptr_update_callback

  Syntax:
    :samp:`0. = pv.ptr_update_callback("hoc_statement", [object])`

    :samp:`0. = pv.ptr_update_callback(pythoncallback)`

  Description:
    The statement or pythoncallback is executed whenever range variables
    are re-allocated. Within the callback, the
    :hoc:meth:`PtrVector.resize` method may be called but the PtrVector should
    not be destroyed.
