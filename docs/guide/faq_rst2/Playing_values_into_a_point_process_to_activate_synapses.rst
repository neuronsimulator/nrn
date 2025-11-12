Playing values into a point process to activate synapses
==========================================================

**Question:**  
How can I use a custom set of spike times to drive synaptic activation in NEURON, and what is the recommended method to connect these spike times to synapses without causing errors?

**Answer:**  
If you have a vector of spike times and want to activate synapses at those specific times, the best approach is to use the built-in `VecStim` class. `VecStim` acts like an artificial spike generator that produces spike events at times specified in a `Vector`. You can then connect `VecStim` to one or more synapses using `NetCon` objects to deliver events, which will avoid issues such as segmentation violations.

**Example in Hoc:**

.. code-block:: hoc

    objref spike_times, vs, nc, syn
    spike_times = new Vector()
    spike_times.append(10)    // spike at 10 ms
    spike_times.append(20)    // spike at 20 ms
    spike_times.append(30)    // spike at 30 ms

    vs = new VecStim()
    vs.play(spike_times)

    // Assuming syn is a synaptic point process on a postsynaptic section
    // syn = new ExpSyn(some_section(0.5))
    nc = new NetCon(vs, syn)
    nc.weight = 0.05

**Example in Python:**

.. code-block:: python

    from neuron import h

    spike_times = h.Vector([10, 20, 30])  # Spike times in ms
    vs = h.VecStim()
    vs.play(spike_times)

    # Assuming syn is a synaptic point process on a postsynaptic section
    # syn = h.ExpSyn(some_section(0.5))
    nc = h.NetCon(vs, syn)
    nc.weight[0] = 0.05

**Notes:**  
- Make sure your spike times are all positive and sorted in increasing order.  
- Fill the spike time vector before calling `run()`.  
- `VecStim` provides a flexible way to input arbitrary spike trains into the network, better than trying to `play()` into a generic point process that is not designed to generate spikes.

Original Thread: https://neuron.yale.edu/phpBB/viewtopic.php?t=3021
