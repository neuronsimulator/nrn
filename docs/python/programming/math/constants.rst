Constants
~~~~~~~~~

The following mathematical and physical constants are available through the ``h`` module: 

::

    h.PI        3.14159265358979323846
    
    h.E         2.71828182845904523536
    
    h.GAMMA     0.57721566490153286060 (Euler)
    
    h.DEG       57.29577951308232087680 (deg/radian)
    
    h.PHI       1.61803398874989484820 (golden ratio)
    
    h.FARADAY   96485.3321233100141 (modern value. derived from mole and electron charge)
    
    h.R         8.3144626181532395 (modern value. derived from boltzmann constant and mole)

    h.Avogadro_constant  6.02214076e23 (codata2018 value, introduced version 8.0)

As of Version 8.0 (circa October, 2020) modern units are the default.

.. warning::
    Constants are not treated specially by the interpreter and 
    may be changed with assignment statements. However a change of
    ``FARADAY``, ``R``, or ``Avogadro_constant`` generate a warning message
    (default once).
    Python warning messages can be managed with
    ``import warnings; warnings.filterwarnings(action)`` where useful actions
    are ``"error"``, ``"ignore"``, ``"always"``, or ``"once"``.
    If assignment takes
    place due to execution of a hoc interpreter statement, the warning occurs
    only once but cannot be avoided.

