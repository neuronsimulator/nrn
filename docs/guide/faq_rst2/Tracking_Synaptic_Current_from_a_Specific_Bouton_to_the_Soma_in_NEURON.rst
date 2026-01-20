Tracking Synaptic Current from a Specific Bouton to the Soma in NEURON
========================================================================

**Question:**  
How can I measure or track the synaptic current originating from a particular synapse (bouton) distributed on dendrites and determine its contribution to the somatic current in NEURON?

**Answer:**  
In NEURON, synaptic currents from conductance-based synapses do not propagate as identifiable individual currents through compartments to the soma. Instead, synaptic activation locally changes the membrane conductance and contributes to the total transmembrane current and voltage changes in that compartment. This perturbation spreads electrotonically through the neuron to the soma according to the cable equation.

Because of nonlinearities like changes in driving force and shunting effects, the contribution of a synapse to somatic current depends on the full spatiotemporal pattern of synaptic activation. Therefore, tracking the exact portion of synaptic current arriving at the soma is not straightforward.

A common approximate method is to compare simulations with and without activation of the synapse(s) of interest:

1. Run a control simulation with all synapses active, recording the soma transmembrane current (`i_soma_control`).
2. Run a test simulation identical to the first but without activating the synapse(s) of interest, recording the soma current (`i_soma_test`).
3. Subtract `i_soma_test` from `i_soma_control` to estimate the synapse's contribution to somatic current.

This approach is approximate and assumes linearity, which may not hold for all conditions.

**Conceptual Note:**  
The synaptic current "loses its identity" once it flows into the compartment; it becomes part of the total charge and voltage perturbation that spreads electrotonically. Ions entering through synapses do not physically travel through the neuron to the soma; changes in membrane potential propagate instead.

**Example Code to Record Somatic Current Contribution**

Python:

.. code-block:: python

    from neuron import h, gui

    # Create a soma and dendrite
    soma = h.Section(name='soma')
    dend = h.Section(name='dend')
    dend.connect(soma(1))
    soma.L = soma.diam = 20
    dend.L = 200
    dend.diam = 1

    # Insert passive properties
    for sec in [soma, dend]:
        sec.insert('pas')

    # Create a synapse on the dendrite
    syn = h.AlphaSynapse(dend(0.5))
    syn.onset = 5    # ms
    syn.gmax = 0.001 # uS
    syn.tau = 0.5    # ms

    # Record somatic membrane current (passive current)
    vec_i_soma = h.Vector()
    vec_t = h.Vector()
    vec_i_soma.record(soma(0.5)._ref_i_membrane_)
    vec_t.record(h._ref_t)

    # Run simulation with synapse active
    h.finitialize(-65)
    h.continuerun(20)
    i_soma_control = vec_i_soma.as_numpy().copy()

    # Run simulation with synapse inactive (gmax=0)
    syn.gmax = 0
    vec_i_soma.resize(0)
    h.finitialize(-65)
    h.continuerun(20)
    i_soma_test = vec_i_soma.as_numpy().copy()

    # Estimate synaptic contribution by subtraction
    synaptic_contribution = i_soma_control - i_soma_test

HOC:

.. code-block:: hoc

    create soma, dend
    access soma
    soma {
        L = 20
        diam = 20
        insert pas
    }
    dend {
        L = 200
        diam = 1
        insert pas
        connect soma(1)
    }

    objref syn, vec_i_soma, vec_t
    syn = new AlphaSynapse(0.5)
    syn.tau = 0.5
    syn.gmax = 0.001
    syn.onset = 5

    vec_i_soma = new Vector()
    vec_t = new Vector()
    vec_i_soma.record(&soma.i_membrane_)
    vec_t.record(&t)

    // Run simulation with synapse active
    finitialize(-65)
    run(20)
    objref i_soma_control
    i_soma_control = vec_i_soma.clone()

    // Run simulation with synapse inactive
    syn.gmax = 0
    vec_i_soma.resize(0)
    finitialize(-65)
    run(20)
    objref i_soma_test
    i_soma_test = vec_i_soma.clone()

    // Subtract vectors externally to estimate synaptic contribution

**Summary:** 
 
- Synaptic currents cannot be tracked as discrete entities along compartments due to electrotonic and nonlinear properties of neurons.  
- The difference method (subtracting soma current traces from control and synapse-off simulations) provides an approximation.  
- Understanding membrane potential spread and cable theory is essential for interpreting synaptic current propagation in compartmental models.

Original Thread: https://neuron.yale.edu/phpBB/viewtopic.php?t=3835
