Miscellaneous
-------------

.. data:: float_epsilon


    Syntax:
        ``h.float_epsilon = 1e-11``


    Description:
        The default value is 1e-11 
         
        Allows somewhat safer NEURON logical comparisons and integer truncation for 
        floating point numbers. Most NEURON comparisons are treated as true if they are 
        within float_epsilon of being true. eg. 
         

        .. code::

            from neuron import h

            h("""
            proc count_to_1() {for (i = 0; i < 1; i += 0.1) $o1.__call__(i)}
            """)

            def print_i(i):
                print('%3g %g' % (i, int(10*i)))

            rv = h.count_to_1(print_i)

            h.float_epsilon = 0

            # two bugs due to roundoff
            rv = h.count_to_1(print_i)

    .. warning::

        This has no effect on Python comparisons, which is why the example uses a
        HOC procedure.

    .. warning::
        I certainly haven't gotten every floating comparison in the program to use 
        ``float_epsilon`` but I have most of them including all HOC interpreter logical 
        operations, int, array indices, and Vector logic methods. 


