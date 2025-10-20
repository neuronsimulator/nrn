Plotting Separate Ionic Currents from Different Mechanisms
===========================================================

**Question:**  
How can I plot the ionic current contributed by a specific self-created channel mechanism (e.g., a custom persistent sodium current `NaP`), separately from other mechanisms (e.g., Hodgkin-Huxley sodium current) when both use the same ion (e.g., `na`) and current variable (`ina`)?

**Answer:**  
NEURON combines currents for the same ion species into a single ionic current variable (e.g., `ina`), which makes it impossible to distinguish individual mechanism currents directly by that name. To plot a specific mechanism's current separately, you should declare a separate `RANGE` variable in the NMODL file to represent the current through that mechanism, assign it inside the `BREAKPOINT` block, and plot that variable instead.

**Example NMODL (.mod) snippet:**

.. code-block:: mod

    NEURON {
        SUFFIX nap
        USEION na READ ena WRITE ina
        RANGE g, gbar, i     : 'i' is the specific mechanism current to plot
        GLOBAL winf, tauw
    }

    ASSIGNED {
        v (mV)
        ena (mV)
        ina (mA/cm2)
        i (mA/cm2)          : specific nap current
        g (mho/cm2)
        gbar (mho/cm2)
        winf
        tauw
    }

    BREAKPOINT {
        SOLVE states METHOD cnexp
        g = gbar * w
        i = g * (v - ena)   : assign mechanism current to 'i'
        ina = i             : contribute to total sodium current
    }

**Plotting in Python:**

.. code-block:: python

    from neuron import h, gui
    import matplotlib.pyplot as plt
    
    soma = h.Section(name='soma')
    soma.insert('nap')          # insert custom persistent Na channel
    soma.insert('hh')           # insert Hodgkin-Huxley channels
    
    v_vec = h.Vector().record(soma(0.5)._ref_v)
    t_vec = h.Vector().record(h._ref_t)
    nap_i_vec = h.Vector().record(soma(0.5)._ref_i_nap)  # specific nap current
    
    stim = h.IClamp(soma(0.5))
    stim.delay = 5
    stim.dur = 1
    stim.amp = 0.1
    
    h.finitialize(-65)
    h.continuerun(40)
    
    plt.figure()
    plt.plot(t_vec, nap_i_vec, label='NaP current (nap.i)')
    plt.plot(t_vec, v_vec, label='Membrane potential (mV)')
    plt.xlabel('Time (ms)')
    plt.ylabel('Current (mA/cm²) / Voltage (mV)')
    plt.legend()
    plt.show()

**Plotting in Hoc:**

.. code-block:: hoc

    create soma
    access soma
    insert nap
    insert hh

    objref v_vec, t_vec, nap_i_vec
    v_vec = new Vector()
    t_vec = new Vector()
    nap_i_vec = new Vector()

    v_vec.record(&v(0.5))
    t_vec.record(&t)
    nap_i_vec.record(&i_nap(0.5))

    objref stim
    stim = new IClamp(0.5)
    stim.delay = 5
    stim.dur = 1
    stim.amp = 0.1

    init()
    run()
    
    nap_i_vec.line()
    // Use Vector's line() method or print and plot externally
    
**Summary:**  
By defining and recording a mechanism-specific RANGE variable for the current (`i` in the example), you can separate and plot the individual ionic currents from distinct channel mechanisms, even if they share the same underlying ionic species and combined current variable like `ina`.

Original Thread: https://neuron.yale.edu/phpBB/viewtopic.php?t=2656
