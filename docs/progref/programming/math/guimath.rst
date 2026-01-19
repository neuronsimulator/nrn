.. _guimath:

GUIMath
-------



.. class:: GUIMath

    .. tab:: Python
    
    
        Syntax:
            ``obj = n.GUIMath()``


        Description:
            Contains functions which calculate common relationships between graphics 
            objects. 

         

    .. tab:: HOC


        Syntax:
            ``obj = new GUIMath()``
        
        
        Description:
            Contains functions which calculate common relationships between graphics 
            objects. 
        
----



.. method:: GUIMath.d2line

    .. tab:: Python
    
    
        Syntax:
            ``d = gm.d2line(xpoint, ypoint, xline0, yline0, xline1, yline1)``


        Description:
            Return distance between the point (xpoint,ypoint) and the (infinitely long) 
            line defined by the 0 and 1 points. 

         

    .. tab:: HOC


        Syntax:
            ``d = gm.d2line(xpoint, ypoint, xline0, yline0, xline1, yline1)``
        
        
        Description:
            Return distance between the point (xpoint,ypoint) and the (infinitely long) 
            line defined by the 0 and 1 points. 
        
----



.. method:: GUIMath.d2line_seg

    .. tab:: Python
    
    
        Syntax:
            ``d = gm.d2line_seg(xpoint, ypoint, xline0, yline0, xline1, yline1)``


        Description:
            Return distance between the point (xpoint, ypoint) and the line segment 
            line defined by the 0 and 1 points. 

         

    .. tab:: HOC


        Syntax:
            ``d = gm.d2line_seg(xpoint, ypoint, xline0, yline0, xline1, yline1)``
        
        
        Description:
            Return distance between the point (xpoint,ypoint) and the line segment 
            line defined by the 0 and 1 points. 
        
----



.. method:: GUIMath.inside

    .. tab:: Python
    
    
        Syntax:
            ``boolean = gm.inside(xpoint, ypoint, left, bottom, right, top)``


        Description:
            return True if the point is inside the box, False otherwise 

         

    .. tab:: HOC


        Syntax:
            ``boolean = gm.inside(xpoint, ypoint, left, bottom, right, top)``
        
        
        Description:
            return 1 if the point is inside the box, 0 otherwise 
        
----



.. method:: GUIMath.feround

    .. tab:: Python
    
    
        Syntax:
            ``mode = gm.feround()``

            ``lastmode = gm.feround(mode)``


        Description:
            Set the floating point rounding mode. Mode 1, 2, 3, 4 refers to 
            FE_DOWNWARD, FE_TONEAREST, FE_TOWARDZERO, FE_UPWARD respectively. 
            The default, and most accurate, mode is FE_TONEAREST. 
            The mode is changed only if the argument is 1-4. If there is no 
            support for this function the return value is 0. 
         
            This function is useful to determine if a simulation depends unduly 
            on double precision round-off error.

            This affects calculations performed in both Python and HOC. 

        Example:

            .. code::
                python

                from neuron import n

                gm = n.GUIMath()
                print(f'default rounding mode {gm.feround()}')

                def test_round(mode):
                    gm = n.GUIMath()
                    old = gm.feround(mode)
                    x = 0
                    for i in range(1, 1_000_001):
                        x += 0.1
                    print(f'round mode {mode} x={x:25.17f}')
                    gm.feround(old)

                for i in range(1, 5):
                    test_round(i)

            Output:

                .. code-block::
                    none

                    default rounding mode 2
                    round mode 1 x=  99999.99999613071850035
                    round mode 2 x= 100000.00000133288267534
                    round mode 3 x=  99999.99999613071850035
                    round mode 4 x= 100000.00000432481465396

    .. tab:: HOC


        Syntax:
            ``mode = gm.feround()``
        
        
            ``lastmode = gm.feround(mode)``
        
        
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
        
