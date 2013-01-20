.. _predec:

predeclared-variables
---------------------

         

----



.. function:: float_epsilon


    Syntax:
        :code:`float_epsilon = 1e-11`


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
        float_epsilon but I have most of them including all interpreter logical 
        operations, int, array indices, and Vector logic methods. 

----



.. function:: hoc_ac_

        A variable used by the graphical interface to communicate with the 
        interpreter. It is very volatile. It sometimes holds a value on a 
        function call. If this value is needed by the user it should be 
        copied to another variable prior to any other function call. 

----



.. function:: hoc_obj_


    Syntax:
        :code:`hoc_obj_[0]`

        :code:`hoc_obj_[1]`


    Description:
        When a line on a :class:`Graph` is picked with the :func:`PickVector` tool 
        two new :func:`Vector` 's are created containing the y and x coordinates of the 
        line. The y vector is referenced by hoc_obj_[0] and the x vector is 
        referenced by hoc_obj_[1]. 


----



.. function:: hoc_cross_x_


    Syntax:
        :code:`hoc_cross_x_`


    Description:
        X coordinate value of the last :func:`Crosshair` manipulation. 

    .. seealso::
        :meth:`Graph.crosshair`


----



.. function:: hoc_cross_y_


    Description:
        Y coordinate value of the last :func:`Crosshair` manipulation. 

    .. seealso::
        :meth:`Graph.crosshair`


----



.. function:: Constants

        The following mathematical and physical constants are built-in: 

        .. code-block::
            none

                    "PI",   3.14159265358979323846, 
                    "E",    2.71828182845904523536, 
                    "GAMMA",0.57721566490153286060, /* Euler */ 
                    "DEG", 57.29577951308232087680, /* deg/radian */ 
                    "PHI",  1.61803398874989484820, /* golden ratio */ 
                    "FARADAY", 96484.56,    /*coulombs/mole*/ 
                    "R", 8.31441,           /*molar gas constant, joules/mole/deg-K*/ 


    .. warning::
        Constants are not treated specially by the interpreter and 
        may be changed with assignment statements. 
         
        The FARADAY is a bit different than the faraday of the units database. 
        The faraday in a .mod mechanism is 96520. 

         
         

