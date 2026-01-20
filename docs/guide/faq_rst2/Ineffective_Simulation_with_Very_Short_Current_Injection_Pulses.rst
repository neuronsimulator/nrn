Ineffective Simulation with Very Short Current Injection Pulses
================================================================

**Question:**  
Why does a very brief current injection (e.g., 0.01 ms) with high amplitude sometimes produce no observable voltage response in NEURON simulations?

**Answer:**  
When using fixed time step integration in NEURON, the simulation updates voltages at discrete time points separated by the time step `dt`. If the duration of the current injection pulse is shorter than `dt`, the pulse may not coincide with any integration time point, resulting in no effective stimulus and no voltage change. By contrast, when using variable time step integration, NEURON automatically adjusts `dt` to capture rapid changes, ensuring the stimulus affects the neuron.

**Recommendations:**
- Use a stimulus duration longer than or equal to the fixed time step `dt`.
- Alternatively, use variable time step integration (`cvode`), which adapts the time steps to the stimulus.

**Example in Python (fixed time step):**

.. code-block:: python

    from neuron import h, gui

    soma = h.Section(name='soma')
    soma.L = soma.diam = 20
    soma.nseg = 99
    soma.insert('pas')

    stim = h.IClamp(soma(0.5))
    stim.delay = 0    # ms
    stim.dur = 0.01   # ms, shorter than default dt=0.025 ms
    stim.amp = 1000   # nA

    h.dt = 0.025      # Fixed time step
    h.tstop = 1.0

    v_vec = h.Vector().record(soma(0.5)._ref_v)
    t_vec = h.Vector().record(h._ref_t)

    h.finitialize(-65)
    h.continuerun(h.tstop)

    import matplotlib.pyplot as plt
    plt.plot(t_vec, v_vec)
    plt.xlabel('Time (ms)')
    plt.ylabel('Voltage (mV)')
    plt.show()

**Example in Hoc (variable time step):**

.. code-block:: hoc

    create soma
    access soma
    soma {
        L = 20
        diam = 20
        nseg = 99
        insert pas
    }

    objref stim, vvec, tvec
    stim = new IClamp(0.5)
    stim.delay = 0
    stim.dur = 0.01
    stim.amp = 1000

    vvec = new Vector()
    tvec = new Vector()
    vvec.record(&soma.v(0.5))
    tvec.record(&t)

    dt = 0.025
    tstop = 1
    cvode_active(1)  // Use variable time step integration

    finitialize(-65)
    run()

    // Plotting can be done outside NEURON with recorded vectors

**Note:** Using `cvode_active(1)` enables variable time step integration, which will capture brief stimuli accurately.

Original Thread: https://neuron.yale.edu/phpBB/viewtopic.php?t=3930
