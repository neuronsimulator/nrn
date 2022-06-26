.. _rxdmath_prog_ref:

Mathematical functions for rate expressions
===========================================

NEURON's ``neuron.rxd.rxdmath`` module provides a number of mathematical functions that
can be used to define reaction rates. These generally mirror the functions available
through Python's ``math`` module but support :class:`rxd.Species` objects.

To use any of these, first do:

    .. code::
        python

        from neuron.rxd import rxdmath

Example:

    .. code::
        python

        degradation_switch = (1 + rxdmath.tanh((ip3 - threshold) * 2 * m)) / 2
        degradation = rxd.Rate(ip3, -k * ip3 * degradation_switch)

    For a full runnable example, see `this tutorial <../../../rxd-tutorials/thresholds.html>`_
    which as here uses the hyperbolic tangent as an approximate on/off switch for the reaction.

In each of the following, ``x`` and ``y`` (where present) are arithmetic expressions
comprised of :class:`rxd.Species`, :class:`rxd.State`, :class:`rxd.Parameter`, regular numbers,
and the functions defined below.

.. function:: rxdmath.acos

    Inverse cosine.

    Syntax:

        .. code::
            python

            result = rxdmath.acos(x)

.. function:: rxdmath.acosh

    Inverse hyperbolic cosine.

    Syntax:

        .. code::
            python

            result = rxdmath.acosh(x)

.. function:: rxdmath.asin

    Inverse sine.

    Syntax:

        .. code::
            python

            result = rxdmath.asin(x)

.. function:: rxdmath.asinh

    Inverse hyperbolic sine.

    Syntax:

        .. code::
            python

            result = rxdmath.asinh(x)

.. function:: rxdmath.atan

    Inverse tangent.

    Syntax:

        .. code::
            python

            result = rxdmath.atan(x)

.. function:: rxdmath.atan2

    Inverse tangent, returning the correct quadrant given both a ``y`` and an ``x``.
    (Note: ``y`` is passed in before ``x``.)
    See `Wikipedia's page on atan2 <https://en.wikipedia.org/wiki/Atan2>`_ for more
    information.   

    Syntax:

        .. code::
            python

            result = rxdmath.atan2(y, x)

.. function:: rxdmath.ceil

   Ceiling function.

    Syntax:

        .. code::
            python

            result = rxdmath.ceil(x)

.. function:: rxdmath.copysign

    Apply the sign (positive or negative) of ``expr_with_sign`` to the value of
    ``expr_to_get_sign``.

    Syntax:

        .. code::
            python

            result = rxdmath.copysign(expr_to_get_sign, expr_with_sign)

    The behavior mirrors that of the Python standard library's 
    `math.copysign <https://docs.python.org/3/library/math.html#math.copysign>`_
    which behaves as follows:

        .. code::
            python

            >>> math.copysign(-5, 1.3)
            5.0
            >>> math.copysign(-5, -1.3)
            -5.0
            >>> math.copysign(2, -1.3)
            -2.0
            >>> math.copysign(2, 1.3)
            2.0

.. function:: rxdmath.cos

    Cosine.

    Syntax:

        .. code::
            python

            result = rxdmath.cos(x)


.. function:: rxdmath.cosh

    Hyperbolic cosine.

    Syntax:

        .. code::
            python

            result = rxdmath.cosh(x)

.. function:: rxdmath.degrees

    Converts ``x`` from radians to degrees. Equivalent to multiplying by 180 / π.

    Syntax:

        .. code::
            python

            result = rxdmath.degrees(x)

.. function:: rxdmath.erf

    The Gauss error function; see `Wikipedia <https://en.wikipedia.org/wiki/Error_function>`_ for more.

    Syntax:

        .. code::
            python

            result = rxdmath.erf(x)

.. function:: rxdmath.erfc

    The complementary error function.
    In exact math, ``erfc(x) = 1 - erf(x)``, however using this function provides more
    accurate numerical results when ``erf(x)`` is near 1.
    See the `Wikipedia entry on the error function <https://en.wikipedia.org/wiki/Error_function>`_ for more.

    Syntax:

        .. code::
            python

            result = rxdmath.erfc(x)

.. function:: rxdmath.exp

    e raised to the power x.

    Syntax:

        .. code::
            python

            result = rxdmath.exp(x)

.. function:: rxdmath.expm1

    (e raised to the power x) - 1. More numerically accurate than ``rxdmath.exp(x) - 1`` when ``x`` is near 0.

    Syntax:

        .. code::
            python

            result = rxdmath.expm1(x)

.. function:: rxdmath.fabs

    Absolute value.

    Syntax:

        .. code::
            python

            result = rxdmath.fabs(x)

.. function:: rxdmath.factorial

    Factorial. Probably not likely to be used in practice as it requires integer values.
    Consider using :func:`rxdmath.gamma` instead, as 
    for integers ``x``, ``rxdmath.factorial(x) = rxdmath.gamma(x + 1)``.

    Syntax:

        .. code::
            python

            result = rxdmath.factorial(x)

.. function:: rxdmath.floor

    Floor function.

    Syntax:

        .. code::
            python

            result = rxdmath.floor(x)

.. function:: rxdmath.fmod

    Modulus operator (remainder after division ``x/y``).

    Syntax:

        .. code::
            python

            result = rxdmath.fmod(x, y)

.. function:: rxdmath.gamma

    Gamma function, an extension of the factorial.
    See `Wikipedia <https://en.wikipedia.org/wiki/Gamma_function>`_ for more.

    Syntax:

        .. code::
            python

            result = rxdmath.gamma(x)

.. function:: rxdmath.lgamma

    Equivalent to ``rxdmath.log(rxdmath.fabs(rxdmath.gamma(x)))``
    but more numerically accurate.

    Syntax:

        .. code::
            python

            result = rxdmath.lgamma(x)

.. function:: rxdmath.log

    Natural logarithm.

    Syntax:

        .. code::
            python

            result = rxdmath.log(x)

.. function:: rxdmath.log10

    Logarithm to the base 10.

    Syntax:

        .. code::
            python

            result = rxdmath.log10(x)

.. function:: rxdmath.log1p

    Natural logarithm of 1 + x; equivalent to
    ``rxdmath.log(1 + x)`` but more numerically accurate
    when ``x`` is near 0.

    Syntax:

        .. code::
            python

            result = rxdmath.log1p(x)

.. function:: rxdmath.pow

    Returns ``x`` raised to the ``y``.

    Syntax:

        .. code::
            python

            result = rxdmath.pow(x, y)

.. function:: rxdmath.radians

    Converts degrees to radians. Equivalent to multiplying by π / 180.

    Syntax:

        .. code::
            python

            result = rxdmath.radians(x)

.. function:: rxdmath.sin

    Sine.

    Syntax:

        .. code::
            python

            result = rxdmath.sin(x)

.. function:: rxdmath.sinh

    Hyperbolic sine.

    Syntax:

        .. code::
            python

            result = rxdmath.sinh(x)

.. function:: rxdmath.sqrt

    Square root.

    Syntax:

        .. code::
            python

            result = rxdmath.sqrt(x)

.. function:: rxdmath.tan

    Tangent.

    Syntax:

        .. code::
            python

            result = rxdmath.tan(x)

.. function:: rxdmath.tanh

    Hyperbolic tangent.

    Syntax:

        .. code::
            python

            result = rxdmath.tanh(x)

.. function:: rxdmath.trunc

    Rounds to the nearest integer no further from 0.
    i.e. 1.5 rounds down to 1 and -1.5 rounds up to -1.

    Syntax:

        .. code::
            python

            result = rxdmath.trunc(x)

.. function:: rxdmath.vtrap

    Returns a continuous approximation of 
    ``x / (exp(x/y) - 1)`` with the discontinuity
    at ``x/y`` near 0 replaced by the limiting behavior
    via L'Hôpital's rule. This is useful in avoiding issues
    with certain ion channel models, including Hodgkin-Huxley.
    For an example of this in use, see the
    `Hodgkin-Huxley using rxd <https://neuron.yale.edu/neuron/docs/hodgkin-huxley-using-rxd>`_
    tutorial (as opposed to using ``h.hh``) .

    Syntax:

        .. code::
            python

            result = rxdmath.vtrap(x, y)
