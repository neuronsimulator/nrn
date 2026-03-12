Action Potential Amplitude Variation in Single Compartment Models
===================================================================

**Question:**  
Why does the amplitude of action potentials vary during a firing pattern in a single compartment Hodgkin-Huxley model, and how can similar spike amplitudes be achieved across different model parameter sets?

**Answer:**  
In single compartment models with Hodgkin-Huxley (`hh`) mechanisms, the peak membrane potential at the action potential (AP) is determined by the weighted sum of ionic equilibrium potentials, where weights correspond to the fractional membrane conductances of each ion channel at the spike peak:

.. math::

   V_{peak} = \frac{g_{Na} E_{Na} + g_{K} E_{K} + g_{L} E_{L}}{g_{Na} + g_{K} + g_{L}}

As the conductances for sodium (`g_{Na}`) and potassium (`g_{K}`) channels change during sustained firing because of channel activation/inactivation dynamics, the AP amplitude can vary accordingly.

A smaller compartment (e.g., reduced diameter) has different total ionic currents even if the channel densities remain the same, potentially causing reduced or altered AP amplitude. Adjusting channel densities (e.g., `gnabar_hh`, `gkbar_hh`) is typically required to preserve similar AP characteristics across cells of different sizes.

**Key points to consider:**

- At the AP peak, the total ionic currents sum to zero because the capacitive current is zero.
- Larger potassium conductance or sodium channel inactivation can decrease AP amplitude.
- To maintain AP amplitude across parameter changes, adapt channel densities rather than passive properties such as capacitance or leak conductance.
- Model adjustment may require manual tuning or automated fitting tools (e.g., Multiple Run Fitter).
- Complex morphologies or passive branches can electrically load the soma and reduce AP amplitude.

**Example NEURON code in Hoc:**

.. code-block:: hoc

    create A
    A {
        L = 10
        diam = 10
        insert hh
    }
    objref stim
    stim = new IClamp(0.5)
    stim.del = 200
    stim.dur = 100
    stim.amp = 0.02
    run() 

    // Changing diameter affects AP amplitude and firing pattern:
    A.diam = 1
    run()

**Example using Python with NEURON:**

.. code-block:: python

    from neuron import h, gui

    soma = h.Section(name='soma')
    soma.L = 10
    soma.diam = 10
    soma.insert('hh')

    stim = h.IClamp(soma(0.5))
    stim.delay = 200
    stim.dur = 100
    stim.amp = 0.02

    h.tstop = 400
    h.v_init = -65
    h.run()

    # Change diameter and re-run simulation to observe changes in AP amplitude
    soma.diam = 1
    h.run()

To achieve similar AP amplitudes in smaller compartments, increase the density of sodium channels (`gnabar_hh`) and/or adjust potassium channel densities accordingly. Leak conductance changes typically have less effect on AP amplitude.

Testing and parameter fitting can help find density values that preserve the desired AP amplitude and firing patterns.

Original Thread: https://neuron.yale.edu/phpBB/viewtopic.php?t=1459
