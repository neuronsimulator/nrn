Times of spikes vector
======================

**Question:**  
How can I record or extract the times of spikes (threshold crossings) from a single neuron simulation in NEURON instead of raw voltage traces?

**Answer:**  
Instead of recording the full membrane potential trace, you can use the `NetCon` class's `record()` method to directly capture the times when the membrane potential crosses a specified threshold, effectively giving you a vector of spike times.

If you already have recorded voltage traces, you can post-process these using the `Vector.indwhere()` method to find the indices where the voltage crosses your threshold in the positive direction (spike times).

**Example in Python:**

.. code-block:: python

    from neuron import h, gui
    
    soma = h.Section(name='soma')
    stim = h.IClamp(soma(0.5))
    stim.amp = 0.1
    stim.delay = 5
    stim.dur = 1
    
    v_vec = h.Vector().record(soma(0.5)._ref_v)
    t_vec = h.Vector().record(h._ref_t)
    
    spike_times = h.Vector()
    nc = h.NetCon(soma(0.5)._ref_v, None, sec=soma)
    nc.threshold = 0
    nc.record(spike_times)
    
    h.finitialize(-65)
    h.continuerun(40)
    
    print("Spike times recorded by NetCon:", list(spike_times))

**Example in Hoc:**

.. code-block:: hoc

    create soma
    access soma
    stim = new IClamp(0.5)
    stim.amp = 0.1
    stim.del = 5
    stim.dur = 1
    
    objref vvec, tvec, spikevec, nc
    vvec = new Vector()
    tvec = new Vector()
    spikevec = new Vector()
    
    vvec.record(&v(0.5))
    tvec.record(&t)
    
    nc = new NetCon(&v(0.5), nil)
    nc.threshold = 0
    nc.record(spikevec)
    
    initialization(-65)
    run(40)
    
    spikevec.printf()  // print spike times

This approach provides a convenient way to analyze spike timing without dealing with full voltage trace data.

Original Thread: https://neuron.yale.edu/phpBB/viewtopic.php?t=3303
