.. _math:

Common Math Functions (HOC)
---------------------------

These math functions return a double precision value and take a double 
precision argument. The exception is :func:`atan2` which has two double precision arguments. 

Diagnostics:
    Arguments that are out of range give an argument domain diagnostic. 

    These functions call the library routines supplied by the compiler. 


----

.. function:: abs

        absolute value 

        .. code-block::
            none

            >>> h.abs(-42.2)
            42.2

        See :meth:`Vector.abs` for the :class:`Vector` class. 

        .. note::

            In Python code, use Python's ``abs`` function, which works on both numbers and numpy arrays, as well as Vectors (Vectors do not print their contents) :

            .. code-block::
                python

                >>> abs(-42.2)
                42.2
                >>> abs(-3 + 4j)
                5.0
                >>> v = h.Vector([1, 6, -2, -65])
                >>> abs(v).printf()
                1       6       2       65
                4



----

.. function:: int

        returns the integer part of its argument (truncates toward 0). 

        .. code-block::
            none

            >>> h.int(3.14)
            3.0
            >>> h.int(-3.14)
            -3.0

        .. note::

            In Python code, use Python's ``int`` function instead. The behavior is slightly different in that the Python function returns an int type instead of a double:

            .. code-block::
                python

                >>> int(-3.14)
                -3
                >>> int(3.14)
                3



----

.. function:: sqrt

        square root 

        see :meth:`Vector.sqrt` for the :class:`Vector` class. 

        .. note::
        
            Consider using Python's built in ``math.sqrt`` instead.

----

.. function:: exp

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
            python

            from neuron import h

            for i in range(6,12):
                print(i, h.exp(i))
        
        .. note::
        
            Consider using Python's built in ``math.exp`` instead.

----

.. function:: log

        logarithm to the base e 
        see :meth:`Vector.log` for the :class:`Vector` class. 

        .. note::
        
            Consider using Python's built in ``math.log`` instead.

----

.. function:: log10

        logarithm to the base 10 

        see :meth:`Vector.log10` for the :class:`Vector` class. 
        
        .. note::

            Consider using Python's built in ``math.log10`` instead.



----

.. function:: cos

    trigonometric function of radian argument. 

    see :meth:`Vector.sin` 

    .. note::

        Consider using Python's built in ``math.cos`` instead.



----

.. function:: sin

    trigonometric function of radian argument. 

    see :meth:`Vector.sin` for the :class:`Vector` class. 

    .. note::

        Consider using Python's built in ``math.sin`` instead.



----

.. function:: tanh

        hyperbolic tangent. 
        see :meth:`Vector.tanh` for the :class:`Vector` class. 
        
        .. note::

            Consider using Python's built in ``math.tanh`` instead.



----

.. function:: atan

        returns the arc-tangent of y/x in the range :math:`-\pi/2` to :math:`\pi/2`. (x > 0) 
        
        .. note::
    
            Consider using Python's built in ``math.atan`` instead.



----

.. function:: atan2

    Syntax:
        ``radians = atan2(y, x)``

    Description:
        returns the arc-tangent of y/x in the range :math:`-\pi` < radians <= :math:`\pi`. y and x 
        can be any double precision value, including 0. If both are 0 the value 
        returned is 0. 
        Imagine a right triangle with base x and height y. The result 
        is the angle in radians between the base and hypotenuse.

    Example:

        .. code-block::
            python

            from neuron import h

            h.atan2(0,0) 
            for i in range(-1,2):
                print(h.atan2(i*1e-6, 10))
            for i in range(-1,2):
                print(h.atan2(i*1e-6, -10))
            for i in range(-1,2):
                print(h.atan2(10, i*1e-6))
            for i in range(-1,2):
                print(h.atan2(-10, i*1e-6))
            h.atan2(10,10) 
            h.atan2(10,-10) 
            h.atan2(-10,10) 
            h.atan2(-10,-10) 
            
        .. note::
    
            Consider using Python's built in ``math.atan2`` instead.



----

.. function:: erf

        normalized error function 

        .. math::

            {\rm erf}(z) = \frac{2}{\sqrt{\pi}} \int_{0}^{z} e^{-t^2} dt

        .. note::

            In Python 3.2+, use ``math.erf`` instead.


----

.. function:: erfc

        returns ``1.0 - erf(z)`` but on sun machines computed by other methods 
        that avoid cancellation for large z. 

        .. note::

            In Python 3.2+, use ``math.erfc`` instead.
