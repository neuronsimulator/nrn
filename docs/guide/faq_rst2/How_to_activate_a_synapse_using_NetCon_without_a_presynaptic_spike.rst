How to activate a synapse using NetCon without a presynaptic spike?
===================================================================

In NEURON, you can activate a synapse via a `NetCon` object without requiring a presynaptic neuron to generate a spike. This can be achieved by creating a `NetCon` with a `None` (or `nil` in Hoc) source and directly triggering events with the `event()` method.

Example in Python:

.. code-block:: python

    from neuron import h

    syn = h.Exp2Syn(0.5, sec=h.Section())
    nc = h.NetCon(None, syn)   # No presynaptic source
    nc.weight[0] = 0.04        # Set synaptic weight
    nc.delay = 0               # No delay

    # Deliver an event at 5 ms
    nc.event(5.0)

For multiple events, you can use a `VecStim` to provide a spike train at arbitrary times.

Example in Hoc:

.. code-block:: hoc

    objref syn, nc
    syn = new Exp2Syn(0.5)
    create soma
    insert pas
    nc = new NetCon(nil, syn)  // No presynaptic source
    nc.weight[0] = 0.04
    nc.delay = 0

    // Deliver a single event at 5 ms
    nc.event(5)

To generate multiple events, use `VecStim` which plays a vector of spike times.

Original Thread: https://neuron.yale.edu/phpBB/viewtopic.php?t=4298
