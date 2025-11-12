What are single compartment neuron models useful for, and how should I approach building and interpreting them in NEURON?
=========================================================================================================================

Single compartment models represent a neuron or part of a neuron as a single electrical compartment with uniform membrane potential. They are useful as simplified test systems for developing and evaluating ionic channel mechanisms, intracellular processes, and for modeling neurons or networks when extended morphology is not critical to the phenomena studied.

Key points:

- Single compartment models do not capture spatially extended properties and cannot simulate effects where location-dependent dynamics (e.g., dendrites vs. soma) are critical.
- Extracellular stimulation refers to current applied outside the membrane, not to voltage clamp or current clamp electrodes placed on the membrane.
- A single compartment model treats all ion channels as uniformly distributed over the modeled surface area; you do not explicitly specify channel location.
- To simulate complex geometries or spatially distinct stimulations, multiple compartments (sections) with spatial discretization (nseg > 1) are necessary.
- Surface area can be estimated from cell capacitance (assuming 1 µF/cm² specific capacitance) or morphological measurements; total transmembrane currents (in nA) can be obtained by multiplying current density (mA/cm²) by surface area and scaling appropriately.
- Conductance and gating dynamics specify channel behavior and implicitly set effective channel numbers; experiments and literature data inform parameter values.
- Dual voltage clamp simulations require multiple clamps (SEClamp) applied to different sections or cells, often involving explicit gap junction models to simulate coupled cells.

Example: A single compartment with Hodgkin-Huxley channels, stimulating with current clamp:

.. code-block:: python

    from neuron import h, gui
    soma = h.Section(name='soma')
    soma.L = soma.diam = 20  # microns
    soma.insert('hh')  # Hodgkin-Huxley channels
    
    stim = h.IClamp(soma(0.5))
    stim.delay = 100  # ms
    stim.dur = 500    # ms
    stim.amp = 0.1    # nA

    v_vec = h.Vector().record(soma(0.5)._ref_v)
    t_vec = h.Vector().record(h._ref_t)

    h.finitialize(-65)
    h.continuerun(700)

    import matplotlib.pyplot as plt
    plt.plot(t_vec, v_vec)
    plt.xlabel('Time (ms)')
    plt.ylabel('Membrane Potential (mV)')
    plt.show()

.. code-block:: hoc

    create soma
    access soma
    soma {
        L = 20
        diam = 20
        insert hh
    }
    objref stim, v_vec, t_vec
    stim = new IClamp(0.5)
    stim.delay = 100
    stim.dur = 500
    stim.amp = 0.1

    objref vplot, tplot
    v_vec = new Vector()
    t_vec = new Vector()
    v_vec.record(&v(0.5))
    t_vec.record(&t)
    
    init = -65
    finitialize(init)
    tstop = 700
    run()

    // Plotting requires additional tools; export vectors or use NEURON GUI tools

Summary:

- Start with simple single compartment models to formulate and test hypotheses.
- Ensure your model parameters (surface area, input resistance, capacitance) correspond to experimental measurements.
- Add complexity only when simpler models do not capture the phenomena of interest, such as spatial channel distributions or gap junction coupling.
- Use SEClamp to simulate voltage clamp conditions; IClamp for current clamp.
- For simulating multi-location recordings or stimulation, use multi-compartment models with appropriate spatial discretization.


Original Thread: https://neuron.yale.edu/phpBB/viewtopic.php?t=2714
