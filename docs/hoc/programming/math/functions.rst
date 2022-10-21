
.. _hoc_math:

Common Math Functions (HOC)
---------------------------

These math functions return a double precision value and take a double 
precision argument. The exception is :hoc:func:`atan2` which has two double precision arguments.

Diagnostics:
    Arguments that are out of range give an argument domain diagnostic. 

    These functions call the library routines supplied by the compiler. 


----

.. hoc:function:: abs

        absolute value 

        see :hoc:meth:`Vector.abs` for the :hoc:class:`Vector` class.


----

.. hoc:function:: int

        returns the integer part of its argument (truncates toward 0). 


----

.. hoc:function:: sqrt

        square root 

        see :hoc:meth:`Vector.sqrt` for the :hoc:class:`Vector` class.


----

.. hoc:function:: exp

    Description:
        returns the exponential function to the base e 
         
        When exp is used in model descriptions, it is often the 
        case that the cvode variable step integrator extrapolates 
        voltages to values which return out of range values for the exp (often used 
        in rate functions). There were so many of these false warnings that it was 
        deemed better to turn off the warning message when Cvode is active. 
        In any case the return value is exp(700). This message is not turned off 
        at the interpreter level or when cvode is not active. 

        .. code-block::
            none

            for i=690, 710 print i, exp(i) 


----

.. hoc:function:: log

        logarithm to the base e 
        see :hoc:meth:`Vector.log` for the :hoc:class:`Vector` class.


----

.. hoc:function:: log10

        logarithm to the base 10 

        see :hoc:meth:`Vector.log10` for the :hoc:class:`Vector` class.


----

.. hoc:function:: cos

    trigonometric function of radian argument. 

    see :hoc:meth:`Vector.sin`


----

.. hoc:function:: sin

    trigonometric function of radian argument. 

    see :hoc:meth:`Vector.sin` for the :hoc:class:`Vector` class.


----

.. hoc:function:: tanh

        hyperbolic tangent. 
        see :hoc:meth:`Vector.tanh` for the :hoc:class:`Vector` class.


----

.. hoc:function:: atan

        returns the arc-tangent of y/x in the range -PI/2 to PI/2. (x > 0) 


----

.. hoc:function:: atan2

    Syntax:
        ``radians = atan2(y, x)``

    Description:
        returns the arc-tangent of y/x in the range -PI < radians <= PI. y and x 
        can be any double precision value, including 0. If both are 0 the value 
        returned is 0. 
        Imagine a right triangle with base x and height y. The result 
        is the angle in radians between the base and hypotenuse 

    Example:

        .. code-block::
            none

            atan2(0,0) 
            for i=-1,1 { print atan2(i*1e-6, 10) } 
            for i=-1,1 { print atan2(i*1e-6, -10) } 
            for i=-1,1 { print atan2(10, i*1e-6) } 
            for i=-1,1 { print atan2(-10, i*1e-6) } 
            atan2(10,10) 
            atan2(10,-10) 
            atan2(-10,10) 
            atan2(-10,-10) 


----

.. hoc:function:: erf

        normalized error function 

        .. math::

            {\rm erf}(z) = \frac{2}{\sqrt{\pi}} \int_{0}^{z} e^{-t^2} dt


----

.. hoc:function:: erfc

        returns ``1.0 - erf(z)`` but on sun machines computed by other methods 
        that avoid cancellation for large z. 


