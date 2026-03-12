Difference between weight and reversal potential for synaptic connections
===========================================================================

**Question:**  
What is the difference between the synaptic weight and the reversal potential (.e) when using ExpSyn or Exp2Syn mechanisms in NEURON? Can the weight be negative to represent inhibitory synapses?

**Answer:**  
In NEURON, for biophysical model cells with conductance-based synapses such as ExpSyn or Exp2Syn, the *reversal potential* (`.e`) defines the equilibrium potential of the synaptic current, typically positive for excitatory synapses and negative for inhibitory synapses. The *weight* of the `NetCon` specifies the peak amplitude of the conductance transient evoked by a presynaptic spike and should be a non-negative value.

- The reversal potential controls the direction and driving force of the synaptic current.
- The weight controls the size (strength) of the conductance change due to a spike.

A negative weight is not used and is physically unrealistic because conductances (probability of ion channel opening) cannot be negative. Instead, inhibition is represented by setting the reversal potential to a hyperpolarized value (e.g., -75 mV) and using a positive weight. You **cannot** simply invert the sign of the weight and reversal potential to obtain equivalent results.

**Example in Hoc:**

.. code-block:: hoc

    // Create an inhibitory Exp2Syn synapse on soma(0.5)
    objref GABA
    soma {
        GABA = new Exp2Syn(0.5)
        GABA.tau1 = 30
        GABA.tau2 = 100
        GABA.e = -75  // reversal potential for inhibitory synapse
    }

    // Create a NetCon with positive weight targeting the synapse
    // Assuming v(0.5) is the presynaptic source variable
    objref nc
    nc = new NetCon(&v(0.5), GABA, -40, 1, 0.5)  // weight = 0.5 (positive)

**Example in Python:**

.. code-block:: python

    from neuron import h

    soma = h.Section(name='soma')

    # Create inhibitory Exp2Syn synapse at soma(0.5)
    gaba = h.Exp2Syn(soma(0.5))
    gaba.tau1 = 30
    gaba.tau2 = 100
    gaba.e = -75  # inhibitory reversal potential

    # Create NetCon with positive weight delivering spikes
    nc = h.NetCon(soma(0.5)._ref_v, gaba, sec=soma)
    nc.threshold = -40
    nc.weight[0] = 0.5  # weight must be positive

**Summary:**  

- Use positive weights in `NetCon` to set synaptic strength.  
- Use the reversal potential `.e` to define excitatory or inhibitory synapse type.  
- Do not use negative weights; inhibition is achieved via reversal potential.

Original Thread: https://neuron.yale.edu/phpBB/viewtopic.php?t=4001
