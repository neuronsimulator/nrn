Inhibition Synaptic Weight: How to Choose and Interpret It in NEURON
====================================================================

**Q:** How should I choose the synaptic weight for inhibitory synapses in NEURON, and what does the weight value represent?

**A:** The choice of synaptic weight depends on the type of synapse model and the specific hypothesis or experimental condition you want to simulate. There is no universal "correct" weight value; instead, weights should be chosen based on the dynamics you wish to achieve and the biophysical context.

- For conductance-based synapses such as `ExpSyn` or `Exp2Syn`, the synaptic weight represents the peak conductance change caused by a synaptic event, measured in microsiemens (µS). These weights are always non-negative, since the nature of excitation or inhibition is determined by the synaptic reversal (equilibrium) potential.
- For artificial spiking cell models (e.g., `IntFire1`, `IntFire2`), the weight affects the "activation variable" and can be positive (excitatory) or negative (inhibitory). The exact interpretation depends on the model.
- Weights are not limited between 0 and 1; for example, a very large conductance (like 10000 µS) might have little effect if placed far from the spike initiation zone, or if the reversal potential is near resting potential.
- Synaptic effects depend on additional factors such as synapse location, postsynaptic cell properties, time constants, and reversal potentials.

As a starting point, experiment with physiologically plausible conductance values, observe their effects on postsynaptic firing, and adjust accordingly based on your model goals.

Example of creating an inhibitory synapse with weight in Python:

.. code-block:: python

    from neuron import h

    soma = h.Section(name='soma')
    syn = h.ExpSyn(soma(0.5))
    syn.e = -80    # inhibitory reversal potential in mV
    stim = h.NetStim()
    nc = h.NetCon(stim, syn)
    nc.weight[0] = 0.001  # synaptic weight in microsiemens (µS)
    stim.interval = 10
    stim.number = 10
    stim.start = 5
    stim.noise = 0

Example of similar setup in HOC:

.. code-block:: hoc

    create soma
    access soma

    objref syn, stim, nc
    soma {
        syn = new ExpSyn(0.5)
        syn.e = -80    // inhibitory reversal potential in mV
    }

    stim = new NetStim()
    stim.interval = 10
    stim.number = 10
    stim.start = 5
    stim.noise = 0
    nc = new NetCon(stim, syn)
    nc.weight[0] = 0.001  // synaptic weight in microsiemens (µS)

In summary, the synaptic weight scale and choice depend on your model type and objectives. It is recommended to refer to relevant modeling literature to determine suitable starting values and to understand how synaptic parameters influence network dynamics.

Original Thread: https://neuron.yale.edu/phpBB/viewtopic.php?t=1168
