.. _math:

Common Math Functions (HOC)
---------------------------

These math functions return a double precision value and take a double 
precision argument. The exception is :func:`atan2` which has two double precision arguments. 

Diagnostics:
    Arguments that are out of range give an argument domain diagnostic. 

    These functions call the library routines supplied by the compiler. 

.. note::

    Every function on this page has a pure-Python alternative.
    When working with :class:`Vector` objects, to create a new Vector with the
    function applied to each element of the original, use either list
    comprehensions or ``numpy``. When working with ``rxd`` objects (e.g.,
    a :class:`rxd.Rate`), use the :ref:`rxd.rxdmath <rxdmath_prog_ref>`
    module.
----

.. function:: abs

    .. tab:: Python
    
        Absolute value 

        .. code-block::
            none

            >>> n.abs(-42.2)
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
                >>> v = n.Vector([1, 6, -2, -65])
                >>> abs(v).printf()
                1       6       2       65
                4



    .. tab:: HOC


        absolute value 
        
        
        see :meth:`Vector.abs` for the :class:`Vector` class.
        
----

.. function:: int

    .. tab:: Python
    
        Returns the integer part of its argument (truncates toward 0). 

        .. code-block::
            python

            >>> n.int(3.14)
            3.0
            >>> n.int(-3.14)
            -3.0

        .. note::

            In Python code, use Python's ``int`` function instead. The behavior is slightly different in that the Python function returns an int type instead of a double:

            .. code-block::
                python

                >>> int(-3.14)
                -3
                >>> int(3.14)
                3



    .. tab:: HOC


        returns the integer part of its argument (truncates toward 0). 
        
----

.. function:: sqrt

    .. tab:: Python
    
        Square root 

        see :meth:`Vector.sqrt` for the :class:`Vector` class. 

        .. note::
    
            Consider using Python's built in ``math.sqrt`` instead.

    .. tab:: HOC


        square root 
        
        
        see :meth:`Vector.sqrt` for the :class:`Vector` class.
        
----

.. function:: exp

    .. tab:: Python
    
        Returns the exponential function to the base e 
        
        When exp is used in model descriptions, it is often the 
        case that the CVode variable step integrator extrapolates 
        voltages to values which return out of range values for the exp (often used 
        in rate functions). There were so many of these false warnings that it was 
        deemed better to turn off the warning message when CVode is active. 
        In any case the return value is exp(700). This message is not turned off 
        at the interpreter level or when CVode is not active. 

        .. code-block::
            python

            from neuron import n

            for i in range(6, 12):
                print(i, n.exp(i))
    
        .. note::
    
            Consider using Python's built in ``math.exp`` instead.

    .. tab:: HOC


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

.. function:: log

    .. tab:: Python
    
        Logarithm to the base e
        
        see :meth:`Vector.log` for the :class:`Vector` class. 

        .. note::
    
            Consider using Python's built in ``math.log`` instead.

    .. tab:: HOC


        logarithm to the base e 
        see :meth:`Vector.log` for the :class:`Vector` class.
        
----

.. function:: log10

    .. tab:: Python
    
        Logarithm to the base 10 

        see :meth:`Vector.log10` for the :class:`Vector` class. 
    
        .. note::

            Consider using Python's built in ``math.log10`` instead.



    .. tab:: HOC


        logarithm to the base 10 
        
        
        see :meth:`Vector.log10` for the :class:`Vector` class.
        
----

.. function:: cos

    .. tab:: Python
    
        Returns the trigonometric function of radian argument (a number).

        If you need to take the cosine of a Vector, use ``numpy``; e.g.,

        .. code-block::
            python

            import numpy as np
            from neuron import n

            v = n.Vector([0, n.PI/6, n.PI/4, n.PI/2])
            v2 = n.Vector(np.cos(v))
            print(list(v2))

            # [1.0, 0.8660254037844387, 0.7071067811865476, 6.123233995736766e-17]

        To create a vector filled with a cosine/sine wave, see :meth:`Vector.sin` or
        use ``numpy``.

        .. note::

            Consider using Python's built in ``math.cos`` instead.



    .. tab:: HOC


        trigonometric function of radian argument. 
        
        
        see :meth:`Vector.sin`
        
----

.. function:: sin

    .. tab:: Python
    
        Returns the trigonometric function of radian argument (a number).

        If you need to take the sine of a Vector, use ``numpy``; e.g.,

        .. code-block::
            python

            import numpy as np
            from neuron import n

            v = n.Vector([0, n.PI/6, n.PI/4, n.PI/2])
            v2 = n.Vector(np.sin(v))
            print(list(v2))

            # [0.0, 0.49999999999999994, 0.7071067811865475, 1.0]

        To create a vector filled with a sine wave, see :meth:`Vector.sin` or
        use ``numpy``.

        .. note::

            Consider using Python's built in ``math.sin`` instead.



    .. tab:: HOC


        trigonometric function of radian argument. 
        
        
        see :meth:`Vector.sin` for the :class:`Vector` class.
        
----

.. function:: tanh

    .. tab:: Python
    
        Hyperbolic tangent. 

        For :class:`Vector` objects, use :meth:`Vector.tanh` to store the
        values in-place, or use numpy to create a new Vector; e.g.,

        .. code-block::
            python

            import numpy as np
            from neuron import n

            v = n.Vector([0, 1, 2, 3])
            v2 = n.Vector(np.tanh(v))
            print(list(v2))

            # [0.0, 0.7615941559557649, 0.9640275800758169, 0.9950547536867305]
    
        .. note::

            Consider using Python's built in ``math.tanh`` instead.



    .. tab:: HOC


        hyperbolic tangent. 
        see :meth:`Vector.tanh` for the :class:`Vector` class.
        
----

.. function:: atan

    .. tab:: Python
    
        Returns the arc-tangent of y/x in the range :math:`-\pi/2` to :math:`\pi/2`. (x > 0) 
    
        .. note::

            Consider using Python's built in ``math.atan`` instead.



    .. tab:: HOC


        returns the arc-tangent of y/x in the range -PI/2 to PI/2. (x > 0) 
        
----

.. function:: atan2

    .. tab:: Python
    
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

                from neuron import n

                print(n.atan2(0, 0)) 
                for i in range(-1, 2):
                    print(n.atan2(i*1e-6, 10))
                for i in range(-1, 2):
                    print(n.atan2(i*1e-6, -10))
                for i in range(-1, 2):
                    print(n.atan2(10, i*1e-6))
                for i in range(-1, 2):
                    print(n.atan2(-10, i*1e-6))
                print(n.atan2(10, 10)) 
                print(n.atan2(10, -10)) 
                print(n.atan2(-10, 10)) 
                print(n.atan2(-10, -10)) 
            
            .. note::
    
                Consider using Python's built in ``math.atan2`` instead.



    .. tab:: HOC


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

.. function:: erf

    .. tab:: Python
    
        Normalized error function 

        .. math::

            {\rm erf}(z) = \frac{2}{\sqrt{\pi}} \int_{0}^{z} e^{-t^2} dt

        .. note::

            In Python 3.2+, use ``math.erf`` instead.


    .. tab:: HOC


        normalized error function 
        
        
        .. math::
        
        
            {\rm erf}(z) = \frac{2}{\sqrt{\pi}} \int_{0}^{z} e^{-t^2} dt
        
----

.. function:: erfc

    .. tab:: Python
    
        Returns ``1.0 - erf(z)`` but on sun machines computed by other methods 
        that avoid cancellation for large z. 

        .. note::

            In Python 3.2+, use ``math.erfc`` instead.
    .. tab:: HOC


        returns ``1.0 - erf(z)`` but on sun machines computed by other methods 
        that avoid cancellation for large z. 
        
