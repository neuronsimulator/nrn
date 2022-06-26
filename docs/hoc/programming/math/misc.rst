Miscellaneous
-------------

.. hoc:data:: float_epsilon


    Syntax:
        ``float_epsilon = 1e-11``


    Description:
        The default value is 1e-11 
         
        Allows somewhat safer logical comparisons and integer truncation for 
        floating point numbers. Most comparisons are treated as true if they are 
        within float_epsilon of being true. eg. 
         

        .. code-block::
            none

            for (i = 0; i < 1; i += .1) { 
            	print i, int(10*i) 
            } 
            float_epsilon = 0 
            // two bugs due to roundoff 
            for (i = 0; i < 1; i += .1) { 
            	print i, int(10*i) 
            } 


    .. warning::
        I certainly haven't gotten every floating comparison in the program to use 
        ``float_epsilon`` but I have most of them including all interpreter logical 
        operations, int, array indices, and Vector logic methods. 


