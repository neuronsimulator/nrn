.. _guimath:

GUIMath
-------



.. class:: GUIMath


    Syntax:
        ``obj = h.GUIMath()``


    Description:
        Contains functions which calculate common relationships between graphics 
        objects. 

         

----



.. method:: GUIMath.d2line


    Syntax:
        ``d = guimath.d2line(xpoint, ypoint, xline0, yline0, xline1, yline1)``


    Description:
        Return distance between the point (xpoint,ypoint) and the (infinitely long) 
        line defined by the 0 and 1 points. 

         

----



.. method:: GUIMath.d2line_seg


    Syntax:
        ``d = guimath.d2line_seg(xpoint, ypoint, xline0, yline0, xline1, yline1)``


    Description:
        Return distance between the point (xpoint, ypoint) and the line segment 
        line defined by the 0 and 1 points. 

         

----



.. method:: GUIMath.inside


    Syntax:
        ``boolean = guimath.inside(xpoint, ypoint, left, bottom, right, top)``


    Description:
        return True if the point is inside the box, False otherwise 

         

----



.. method:: GUIMath.feround


    Syntax:
        ``mode = guimath.feround()``

        ``lastmode = guimath.feround(mode)``


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

            from neuron import h

            gm = h.GUIMath()
            print('default rounding mode %d' % gm.feround())

            def test_round(mode):
                gm = h.GUIMath()
                old = gm.feround(mode)
                x = 0
                for i in range(1, 1000001):
                    x += 0.1
                print('round mode %d x=%25.17lf' % (mode, x))
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

