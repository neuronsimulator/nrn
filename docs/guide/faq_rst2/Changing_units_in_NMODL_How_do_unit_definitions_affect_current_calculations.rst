Changing units in NMODL: How do unit definitions affect current calculations?
=============================================================================

When modifying units in a NMODL file, it is critical to maintain dimensional consistency to ensure correct simulation results. For example, changing conductance units from microsiemens (uS) to nanosiemens (nS) without adjusting the corresponding calculations can cause errors by scaling the resulting current by a factor of 1000.

In this case, the current `i` is calculated as:

.. code-block:: hoc

    i = g * (v - e)

If `g` is in nanosiemens and `v`, `e` are in millivolts, the product has units of picoamperes, but `i` is declared in nanoamperes. This mismatch causes the current to be 1000 times too large.

To fix the issue, include an explicit conversion factor:

.. code-block:: hoc

    i = 0.001 * g * (v - e)

Below are example snippets demonstrating correct unit usage in NMODL:

.. code-block:: hoc

    NEURON {
        POINT_PROCESS synAMPA
        RANGE onset, taur, taud, gmax, e, i, Epsilon
        NONSPECIFIC_CURRENT i
    }

    UNITS {
        (nA) = (nanoamp)
        (mV) = (millivolt)
        (uS) = (microsiemens)
    }

    PARAMETER {
        onset = 0 (ms)
        taur = 1.0 (ms)
        taud = 5.0 (ms)
        gmax = 0.001 (uS)
        e = 0 (mV)
        Epsilon = 1e-6
    }

    ASSIGNED {
        v (mV)
        i (nA)
        g (uS)
        Frac
    }

    BREAKPOINT {
        at_time(onset)
        g = gmax * (1/Frac) * wexpzero( -(t-onset)/taud , -(t-onset)/taur )
        i = g * (v - e)  : units consistent
    }

Alternatively, if using nanosiemens:

.. code-block:: hoc

    UNITS {
        (nA) = (nanoamp)
        (mV) = (millivolt)
        (nS) = (nanosiemens)
    }

    PARAMETER {
        onset = 0 (ms)
        taur = 1.0 (ms)
        taud = 5.0 (ms)
        gmax = 1 (nS)
        e = 0 (mV)
        Epsilon = 1e-6
    }

    ASSIGNED {
        v (mV)
        i (nA)
        g (nS)
        Frac
    }

    BREAKPOINT {
        at_time(onset)
        g = gmax * (1/Frac) * wexpzero( -(t-onset)/taud , -(t-onset)/taur )
        i = 0.001 * g * (v - e)  : note conversion factor 0.001 to obtain nA
    }

Python Example for Mod File Compilation and Simulation with NEURON:

.. code-block:: python

    from neuron import h, gui

    h.load_file('stdrun.hoc')
    syn = h.synAMPA(0.5)
    syn.onset = 5
    syn.gmax = 0.001  # uS units version

    h.tstop = 40
    h.run()

Ensure to run `modlunit` on your mod file to check for dimensional consistency.

Original Thread: https://neuron.yale.edu/phpBB/viewtopic.php?t=1935
