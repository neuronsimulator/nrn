.. _nmodlfunc:

NMODL's built-in functions
--------------------------

These functions, which are provided by the C language math library, can be called from within NMODL code. With the exception of abs(), all expect double-precision floating point arguments and return double-precision floating point values.

``acos(x)`` - compute the arc cosine
    Returns the arc cosine of its argument (arccosx). The argument must be in the range -1.0 to 1.0.

``asin(x)`` - compute the arc sine
    Returns the arc sine of its argument (arcsinx). The argument must be in the range -1.0 to 1.0.

``atan(x)`` - compute the arc tangent
    Returns the arc tangent of its argument (arctanx).

``atan2(y,x)`` - compute the arc tangent of y/x
    Returns the arc tangent of the ratio y/x (arctany/x).

``ceil(x)`` - round upwards
    Returns the smallest integral value greater than or equal to its argument (x).

``cos(x)`` - compute the cosine
    Returns the cosine of its argument (cosx). The angle is expressed in radians.

``cosh(x)`` - compute the hyperbolic cosine
    Returns the hyperbolic cosine for a real argument (coshx).

``exp(x)`` - compute the exponential function
    Returns the value of e raised to the argument power (ex).

``fabs(x)`` - compute the absolute value of a floating point number
    Returns the absolute value of its argument (|x|).

``floor(x)`` - round downwards
    Returns the largest integral value less than or equal to its argument (x).

``fmod(x,y)`` - compute x modulo y
    Returns the value of x modulo y (x y), the remainder resulting from x/y.

``log(x)`` - compute the natural logarithm
    Returns the natural logarithm of its argument (lnx).

``log10(x)`` - compute the base 10 logarithm
    Returns the base 10 logarithm of its argument (log10 x).

``pow(x,y)`` - compute x raised to the y power
    Returns the value of x raised to the power of y (x :superscript:`y`).

``sin(x)`` - compute the sine
    Returns the sine of its argument (sinx). The angle is expressed in radians.

``sinh(x)`` - compute the hyperbolic sine
    Returns the hyperbolic sine for a real argument (sinhx).

``sqrt(x)`` - compute the square root
    Returns the positive square root of its argument (x).

``tan(x)`` - compute the tangent
    Returns the tangent of its argument (tanx). The angle is expressed in radians.

``tanh(x)`` - compute the hyperbolic tangent
    Returns the hyperbolic tangent for a real argument (tanhx).
