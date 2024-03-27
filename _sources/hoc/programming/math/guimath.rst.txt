
.. _hoc_guimath:

GUIMath
-------



.. hoc:class:: GUIMath


    Syntax:
        ``obj = new GUIMath()``


    Description:
        Contains functions which calculate common relationships between graphics 
        objects. 

         

----



.. hoc:method:: GUIMath.d2line


    Syntax:
        ``d = math.d2line(xpoint, ypoint, xline0, yline0, xline1, yline1)``


    Description:
        Return distance between the point (xpoint,ypoint) and the (infinitely long) 
        line defined by the 0 and 1 points. 

         

----



.. hoc:method:: GUIMath.d2line_seg


    Syntax:
        ``d = math.d2line_seg(xpoint, ypoint, xline0, yline0, xline1, yline1)``


    Description:
        Return distance between the point (xpoint,ypoint) and the line segment 
        line defined by the 0 and 1 points. 

         

----



.. hoc:method:: GUIMath.inside


    Syntax:
        ``boolean = math.inside(xpoint, ypoint, left, bottom, right, top)``


    Description:
        return 1 if the point is inside the box, 0 otherwise 

         

----



.. hoc:method:: GUIMath.feround


    Syntax:
        ``mode = math.feround()``

        ``lastmode = math.feround(mode)``


    Description:
        Set the floating point rounding mode. Mode 1, 2, 3, 4 refers to 
        FE_DOWNWARD, FE_TONEAREST, FE_TOWARDZERO, FE_UPWARD respectively. 
        The default, and most accurate, mode is FE_TONEAREST. 
        The mode is changed only if the argument is 1-4. If there is no 
        support for this function the return value is 0. 
         
        This function is useful to determine if a simulation depends unduly 
        on double precision round-off error. 

    Example:

        .. code-block::
            none

            objref gm 
            gm = new GUIMath() 
            {printf("default rounding mode %d\n", gm.feround())} 
             
            proc test_round() {local i, old, x  localobj gm 
                    gm = new GUIMath() 
                    old = gm.feround($1) 
                    x = 0 
                    for i=1, 1000000 x += 0.1 
                    printf("rounding mode %d x=%25.17lf\n", $1, x) 
                    gm.feround(old) 
            } 
             
            for i=1, 4 test_round(i) 



