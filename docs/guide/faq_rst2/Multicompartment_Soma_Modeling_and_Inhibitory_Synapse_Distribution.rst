Multicompartment Soma Modeling and Inhibitory Synapse Distribution
====================================================================

Is it necessary or useful to model the soma as multiple compartments, and how can multiple inhibitory synapses be connected to a soma compartment in NEURON?

Modeling the soma as a single compartment is generally adequate for electrical simulations because voltage equilibrates rapidly across the soma. Multicompartment modeling of the soma might be considered only if spatially detailed chemical signaling (e.g., calcium dynamics) within the soma or nucleus is relevant. In such cases, spatial discretization can be implemented using NMODL or the RxD module.

Regarding electrical properties, cytoplasmic resistivity and organelle effects are difficult to quantify precisely but can be approximated using ball-and-stick models with empirical parameters.

For modeling multiple inhibitory synapses on the soma, several synapses can be attached to the same compartment. If synapses activate simultaneously, their conductances can be summed by adjusting peak conductance or synaptic weights. For asynchronous activations, instantiate individual synapse objects with their own NetStim inputs.

Example in Python
-----------------

.. code-block:: python

    from neuron import h, gui

    soma = h.Section(name='soma')
    soma.L = soma.diam = 20  # 20 microns cube for simplicity

    # Insert passive channels
    soma.insert('pas')

    # Create multiple inhibitory synapses on the soma
    syn1 = h.AlphaSynapse(soma(0.5))
    syn1.e = -75  # reversal potential for inhibition (mV)
    syn1.onset = 5  # ms
    syn1.gmax = 0.001  # uS

    syn2 = h.AlphaSynapse(soma(0.5))
    syn2.e = -75
    syn2.onset = 15
    syn2.gmax = 0.0015

    stim = h.NetStim()
    stim.number = 1
    stim.start = 5

    nc1 = h.NetCon(stim, syn1)
    nc2 = h.NetCon(stim, syn2)
    nc2.delay = 10  # activate syn2 later

    h.tstop = 40
    h.run()


Example in Hoc
--------------

.. code-block:: hoc

    create soma
    soma {
        L = 20
        diam = 20
        insert pas
    }

    objref syn1, syn2, stim, nc1, nc2
    syn1 = new AlphaSynapse(0.5)
    syn1.e = -75
    syn1.onset = 5
    syn1.gmax = 0.001

    syn2 = new AlphaSynapse(0.5)
    syn2.e = -75
    syn2.onset = 15
    syn2.gmax = 0.0015

    stim = new NetStim()
    stim.number = 1
    stim.start = 5

    nc1 = new NetCon(stim, syn1)
    nc2 = new NetCon(stim, syn2)
    nc2.delay = 10

    tstop = 40
    run()

Original Thread: https://neuron.yale.edu/phpBB/viewtopic.php?t=4718
