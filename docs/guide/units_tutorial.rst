.. _units_tutorial:

UNITS Tutorial 
==============

Units in models are checked for consistency with

.. code::
    bash

    modlunit file

where *file* is the prefix to a model description file (having a ``.mod`` suffix).
 
 Units are understood at the level of the unix units command. The syntax for units is any string understood by the unix units command and enclosed in parentheses. In particular, scientific notation is expressed as 1.111-5 or 1.111+5 instead of the more usual 1.111e-4, etc. Also an integer following a name is the power of that dimension, e.g. cm4 is centimeter to the fourth power. Lastly, all units in the numerator must come first followed by a slash character followed by all the units in the denominator. e.g a volt can be expressed either as (volt), (m2-kg/sec2-coul), or as (m m kg/sec sec coul). ie. one cannot have two slashes in a units expression.

Consider the model description language fragment:

.. code::

    ASSIGNED {
        i   (milliamp)
        v   (volt)
        r   (ohm)
    }
    EQUATION {
    v = i
    }

will produce the error message

.. code::
    python

     i
    units:  0.001 coul/sec
     v 
    units:  1 m2-kg/sec2-coul
    The units of the previous two expressions are not conformable
    at line 8 in file utest1.mod
       v = i<<ERROR>>

It is an error to equate milliamps and volts. The last two lines of the error message show where the error was found and the first two lines show what is inconsistent about the expressions in terms of fundamental units. In this case, the statement has a serious error due to a missing term.

Replacing the offending statement with "``v = i*r``" and retranslating the model gives

.. code::
    python

    The previous primary expression with units: 0.001 m2-kg/sec2-coul
    is missing a conversion factor and should read:
        (0.001)*( i*r)
    at line 8 in file utest1.mod
        v = i*r<<ERROR>>

Now, the units are conformable but the statement is still in error since we are trying to equate volts and millivolts. One way to correct the error is to choose at the outset a completely consistent set of units, e.g. replace the ASSIGNED statement for v with "``v (millivolt)``". A second way is indicated in the error message.

A third obvious method, replacing the offending statement by

.. code::
    python

    v = .001*i*r

WILL NOT WORK. Although an error message for such a natural and obviously correct statement is very annoying (after all, the *numbers* come out right), we do not allow you to do unit conversion in this way for one overpowering reason. It is error prone for you, and impossible for the computer, to determine whether a number refers to a conversion factor or whether a number refers to "how many" units we are talking about.

The syntactic feature that we use to distinguish conversion factors from quantity is to enclose conversion factors within parentheses. The benefits of this convention are: 1) it is unlikely and certainly unnecessary for single numbers with the semantic meaning of quantity to be enclosed within parentheses. 2) Single numbers enclosed in parentheses still produce well formed expressions that are arithmetically meaningful and correct in the absence of unit consistency checks. 3) If parentheses are ommitted, an error message will result since the unit factors will be inconsistent. 4) If parentheses surround a number which the user intended to be a quantity, an error message will result since the unit factors will be inconsistent. Note that the requirement that a conversion factor be a single number is important and that "(1 + 1)" is NOT a conversion factor but the quantity 2.

To see the necessity of disambiguating "quantity" and "conversion" Suppose we have variables x and y where x has the units of feet and a value of 2 (i.e. x is 2 feet) and y has the units of inches. Now consider the statement: ``y = 5*x``. The most likely intended meaning is that y is 5 times longer than x, i.e. y has the value 120 (inches). But to silently assign a value of 120 to y would take a lot of nerve since that kind of computation is very different from the way computers normally do things. Instead, the computer would print an error message suggesting the form ``(12)*5*x``. Now, imagine that the user wrote ``y = (5)*x``. Then the computer would print an error message suggesting another conversion factor of the form (2.4)*(5)*x. At this, the user should see his mistake -- although the computer misinterpreted the exact nature of the mistake which was writing a quantity in the conversion format.

There is one case in which a strict formal conformability is not required. This happens in primary expressions that contain no variables within any of its sub-expressions. numbers\footnote{ A primary expression is a product of terms in which each term is a general expression enclosed in parentheses, a variable, or a number.} such as

.. code::
    python

    v = 10
    y = (12)*x + 10
    y = (12)*(x + 10)

    v = (.001)*i*r + (1 + 2)/3
    v = (.001)*(i*r + (1 + 2)/3)

in which the primary expression certainly means 10 volts, 10 feet, 10 inches, 1 volt, and 1 millivolt respectively. Thus primary expressions containing only numbers take units consistent with their position in the general expression. Numbers within an expression can also be given explicit units as in

.. code::
    python

    exp(v/18(mV))
	exp(v/18(.001 volt))


The units follow the number. Note that the two expressions above evaluate to the same number --- the (``.001 volt``) is not a conversion factor in the second expression but the units for the number 18.

Local variables inherit units through the assignment statement. Thus, consider,

.. code::

    BREAKPOINT {
       LOCAL temp
      temp = i*r
      v = temp
      temp = (12)*5*x
      y = temp
      temp = 10
      v = temp
    }

"Millivolts' is assigned to temp in its first assignment (v = temp will produce an error message), "feet" is assigned to temp in its second assignment (the units are correct in y = temp), and temp is dimensionless in its third assignment (v = temp will produce an error message).

Definition of new units
===================

New units can be defined in terms of default units and previously defined units by placing definitions in the UNITS block. eg.

.. code::

    UNITS {
        (uF)    =  (microfarad)
        (Mohms) =  (megohms)
        (V)     =  (volt)
        (molar) =  (/liter)
        (mM)    =  (millimolar)
    }

A UNITS block can appear anywhere within a file but units used in definitions must be previously defined. Notice that molar is not moles/liter since mole has the default definition of 6.022169e+23. Default definitions can not be redefined within a model description file since that may invalidate other unit definitions appearing in the units database. There is no reason why the user cannot change the database although it is not recommended.

The units database knows about a lot of physical constants in addition to mole, e.g.

.. code::
    python

    faraday \= 9.652+4 coul\\
    e \> 1.602192-19 coul\\
    rydberg \> 2.179846-18 m2-kg/sec2\\
    pi \> 3.141593\\
    lambert \> 3.183099+3 candela/m2

and it is more convenient to define *constant* constants in the UNITS block rather than in the CONSTANTS block --- there is less chance of a typo, and they do not appear in SCoP where they can be inadvertently changed. For example:

.. code::

    UNITS {
        F      = (faraday) (coulomb)
        PI     = (pi) (1)
        e      = (e) (coulomb)
        R      = (k-mole) (joule/degC)
        C      = (c) (cm/sec)
    }

Here, ``C`` is the speed of light in cm/sec and ``R`` is the Gas constant.

Constant factors are defined in the UNITS block in the following manner.

.. code::

    UNITS {
        F   = 96520    (coul)
        PI  = 3.14159  ()
        foot2inch = 12 (inch/foot)
    }

Note that one could also write the last example as

.. code::
    python

    foot2inch = (1) (inch/foot)

This shows that it can sometimes take too much clear thinking to specify dimensionless conversion factors. To avoid misunderstanding, conversion factors ( dimensionless factors used to convert between conformable units) can most clearly be written

.. code::

    UNITS {
        foot2inch = (foot) -> (inch)
    }

With the above, a statement such as 

.. code::
    python

    i = 5*foot2inch*f

makes sense. ie. if f=2 then i should end up as 120 and there is no complaint by the units checker.

Function and Argument units
===================

Standard mathematical functions are dimensionless and take dimensionless arguments. In those special circumstances (such as printf()) where dimensions don't matter or where there is some overreaching reason why units should not be checked, one can turn off all unit checking with the statement, ``UNITSOFF``. Unit checking is turned back on with UNITSON. An example of this last case are Hodgkin-Huxley rates which involve terms like ``exp(-v/18)``. In the model description the argument has the dimensions of mV and it may seem like too much trouble to give all the numbers explicit units as in ``exp(-v/18(mV))``.

User declared functions and arguments can be given units with the syntax

.. code::
    python

    FUNCTION f(a1 (u1), a2 (u2), ...) (uf) { statements }
    PROCEDURE p(a1 (u1), a2 (u2), ...) { statements }

In every instance, the absence of units implies that the argument or function is dimensionless.

Units for KINETIC blocks
===================

First, it is ascertained that the quantity units are the same for each reactant. Quantity units are the reactant units times that reactant's COMPARTMENT size. Then the flux units are determined (quantity units / independent units) and it is ascertained that the rate units are consistent.

Units in NEURON models
===================

If a NEURON block exists in the model description the following variables are checked for compatible units

.. code::
    python

    name		        units

    v		            millivolt
    t		            ms
    dt		            ms
    celsius		        degC
    diam		        micron

    concentration	    milli/liter
    potential               millivolt 
    current density	    milliamp/cm2
    point current	    nanoamp

Here's another resource for :ref:`units used in NEURON <units_used_in_neuron>`. 