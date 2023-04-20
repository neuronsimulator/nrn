.. _hoc_python_accessing_hoc:

HOC accessing Python
~~~~~~~~~~~~~~~~~~~~

This section describes how one can interact with Python from HOC code.
For more information about the Python interface to NEURON, see
:ref:`the Python documentation <python_prog_ref>`.

.. _hoc_function_nrnpython:
.. hoc:function:: nrnpython


    Syntax:
        ``nrnpython("any python statement")``


    Description:
        Executes any python statement. Returns 1 on success; 0 if an exception
        was raised or if python support is not available.
        
        In particular, ``python_available = nrnpython("")`` is 1 (true) if
        python support is available and 0 (false) if python support is not
        available.
    
    Example:

        .. code-block::
            python

            nrnpython("import sys") 
            nrnpython("print sys.path") 
            nrnpython("a = [1,2,3]") 
            nrnpython("print a") 
            nrnpython("import hoc") 
            nrnpython("hoc.execute('print PI')") 
            

         

----



.. hoc:class:: PythonObject


    Syntax:
        ``p = new PythonObject()``


    Description:
        Accesses any python object. Almost equivalent to :hoc:class:`~neuron.hoc.HocObject` in the
        python world but because of some hoc syntax limitations, ie. hoc does not 
        allow an object to be a callable function, and top level indices have 
        different semantics, we sometimes need to use a special idiom, ie. the '_' 
        method. Strings and double numbers move back and forth between Python and 
        Hoc (but Python integers, etc. become double values in Hoc, and when they 
        get back to the Python world, they are doubles). 
         

        .. code-block::

            objref p 
            p = new PythonObject() 
            nrnpython("ev = lambda arg : eval(arg)") // interprets the string arg as an 
                                      //expression and returns the value 
            objref tup 
            print p.ev("3 + 4")       // prints 7 
            print p.ev("'hello' + 'world'") // prints helloworld 
            tup = p.ev("('xyz',2,3)") // tup is a PythonObject wrapping a Python tuple 
            print tup                 // prints PythonObject[1] 
            print tup._[2]            // the 2th tuple element is 3 
            print tup._[0]            // the 0th tuple element is xyz 
             
            nrnpython("import hoc")   // back in the Python world 
            nrnpython("h = hoc.HocObject()") // tup is a Python Tuple object 
            nrnpython("print h.tup")   // prints ('xyz', 2, 3) 

        Note that one needs the '_' method, equivalent to 'this', because trying to 
        get at an element through the built-in python method name via 

        .. code-block:: python

            tup.__getitem__(0) 

        gives the error "TypeError: tuple indices must be integers" since 
        the Hoc 0 argument is a double 0.0 when it gets into Python. 
        It is difficult to pass an integer to a Python function from the hoc world. 
        The only time Hoc doubles appear as integers in Python, is when they are 
        the value of an index. If the index is not an integer, e.g. a string, use 
        the __getitem__ idiom. 

        .. code-block::

            objref p 
            p = new PythonObject() 
            nrnpython("ev = lambda arg : eval(arg)") 
            objref d 
            d = p.ev("{'one':1, 'two':2, 'three':3}") 
            print d.__getitem__("two")        // prints 2 
             
            objref dg 
            dg = d.__getitem__ 
            print dg._("two")                // prints 2 

         
        To assign a value to a python variable that exists in a module use 

        .. code-block::

            nrnpython("a = 10") 
            p = new PythonObject() 
            p.a = 25 
            p.a = "hello" 
            p.a = new Vector(4) 
            nrnpython("b = []") 
            p.a = p.b 
