Direction of Current in a Point Process in NEURON
==================================================

**Question:**  
In a point process defining a fluctuating synaptic conductance (e.g., Ornstein-Uhlenbeck Process), what is the sign convention for the current `i`? Specifically, does a positive `i` depolarize or hyperpolarize the cell, and how is this incorporated in the membrane equation?

**Answer:**  
NEURON follows the classical neurophysiological sign convention for currents: a *depolarizing* current flowing *into* the cell is represented as a **negative** value of the transmembrane current (`i < 0`), and a *hyperpolarizing* current is represented as a **positive** current (`i > 0`) flowing *out* of the cell. This sign convention applies to all ionic currents and NONSPECIFIC_CURRENT mechanisms.

Hence, for a one-compartment model with membrane capacitance `c_m` and membrane potential `V`, the membrane equation can be written as:

.. math::

   c_m \frac{dV}{dt} = -I_{ion} + I_{syn}

where `I_syn` (synaptic or nonspecific current) is consistent with the sign convention, i.e., if `i` (the current computed by the point process) is positive, it hyperpolarizes the cell.

On the other hand, current injected directly into the cell by a microelectrode is modeled by an `ELECTRODE_CURRENT` and follows the opposite sign convention: depolarizing current is **positive**.

---

**Example NEURON (NMODL) snippet for a fluctuating conductance point process:**

.. code-block:: hoc

    NEURON {
        POINT_PROCESS Gfluct
        RANGE g_e, g_i, E_e, E_i
        NONSPECIFIC_CURRENT i
    }

    ASSIGNED {
        v (mV)
        i (nA)
        g_e (umho)
        g_i (umho)
        E_e (mV)
        E_i (mV)
    }

    BREAKPOINT {
        SOLVE oup
        i = g_e * (v - E_e) + g_i * (v - E_i)
    }

---

**Example Python usage:**

.. code-block:: python

    from neuron import h

    # Load the mechanism (compiled .mod file) before usage
    gfluct = h.Gfluct(sec=some_section)
    gfluct.E_e = 0     # Excitatory reversal potential (mV)
    gfluct.E_i = -75   # Inhibitory reversal potential (mV)

    # Example: reading the fluctuating current at a given time step
    current = gfluct.i
    # current > 0 hyperpolarizes the cell, current < 0 depolarizes the cell

---

This sign convention ensures consistent interpretation of synaptic and injected currents in simulations using the NEURON simulator.

Original Thread: https://neuron.yale.edu/phpBB/viewtopic.php?t=3820
