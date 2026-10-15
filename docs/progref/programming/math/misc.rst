Miscellaneous
-------------

.. data:: float_epsilon

    .. tab:: Python

        Syntax:

        .. code-block:: python
            
            n.float_epsilon = 1e-11


        Description:
            The default value is 1e-11 
            
            Allows somewhat safer NEURON logical comparisons and integer truncation for 
            floating point numbers. Most NEURON comparisons are treated as true if they are 
            within float_epsilon of being true. e.g., 

            .. code-block:: python

                from neuron import n

                n("""
                proc count_to_1() {for (i = 0; i < 1; i += 0.1) $o1.__call__(i)}
                """)

                def print_i(i):
                    print(f'{i:3g} {int(10*i)}')

                rv = n.count_to_1(print_i)

                n.float_epsilon = 0

                # two bugs due to roundoff
                rv = n.count_to_1(print_i)

        .. warning::

            This has no effect on Python comparisons, which is why the example uses a
            HOC procedure.

        .. warning::

            We certainly haven't gotten every floating comparison in the program to use 
            ``float_epsilon`` but NEURON has most of them including all HOC interpreter logical 
            operations, int, array indices, and Vector logic methods. 

    .. tab:: HOC

        Syntax:

            .. code-block:: c++

                float_epsilon = 1e-11


            Description:
                The default value is 1e-11 
                
                Allows somewhat safer logical comparisons and integer truncation for 
                floating point numbers. Most comparisons are treated as true if they are 
                within float_epsilon of being true. e.g., 
                

                .. code-block::
                    C++

                    for (i = 0; i < 1; i += .1) { 
                        print i, int(10*i) 
                    } 
                    float_epsilon = 0 
                    // two bugs due to roundoff 
                    for (i = 0; i < 1; i += .1) { 
                        print i, int(10*i) 
                    } 


        .. warning::

            We certainly haven't gotten every floating comparison in the program to use 
            ``float_epsilon`` but NEURON has most of them including all HOC interpreter logical 
            operations, int, array indices, and Vector logic methods. 



    .. tab:: MATLAB

        Syntax:

        .. code-block:: matlab
            
            n.float_epsilon = 1e-11


        Description:
            The default value is 1e-11 
            
            Allows somewhat safer NEURON logical comparisons and integer truncation for 
            floating point numbers. Most NEURON comparisons are treated as true if they are 
            within `float_epsilon` of being true. e.g., 

            .. code-block:: matlab

                n = neuron.launch();

                n('print 1.01 == 1.0');  % 0 i.e., false
                n.float_epsilon = 0.1;
                n('print 1.01 == 1.0');  % 1 i.e., true

        .. warning::

            This has no effect on MATLAB comparisons, which is why the example does the
            comparison using a HOC statement.

        .. warning::
            We certainly haven't gotten every floating comparison in the program to use 
            ``float_epsilon`` but NEURON has most of them including all HOC interpreter logical 
            operations, int, array indices, and :class:`Vector` logic methods. 
