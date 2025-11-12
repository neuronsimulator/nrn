Is it possible to isolate a section from a neuron and reproduce its voltage response as if it were still connected?
===================================================================================================================

To isolate a section (e.g., section Y) from a neuron and reproduce the voltage response recorded when it was connected to other sections (e.g., section X with IClamp), follow these three steps:

1. **Record the voltage waveform at the section Y when it is connected.**

2. **Use a voltage clamp on the isolated section Y to replay the recorded voltage waveform.**

3. **Inject the clamp current recorded from step 2 into the isolated section Y using a current clamp to reproduce the voltage response.**

This approach recognizes that simply removing sections or setting KCL=0 and trying to mimic the input current at Y will not replicate the original voltage waveform due to the complex spatial and membrane properties.

Example workflow and code snippets:

- **Step 1: Record voltage at Y and time vector**

.. code-block:: hoc

    objref vy0_vec, t_vec
    vy0_vec = new Vector()
    t_vec = new Vector()
    Y { vy0_vec.record(&v(0)) }
    Y { t_vec.record(&t) }
    // Run simulation to record voltage at Y(0)

- **Step 2: Disconnect Y and apply voltage clamp to replay voltage waveform**

.. code-block:: hoc

    Y { disconnect() }        // Isolate section Y
    objref clamp
    Y { clamp = new SEClamp(0) }
    clamp.dur1 = 1e9            // Very long duration
    clamp.rs = 1e-3             // Very low series resistance
    clamp.amp1 = -65            // Holding voltage (initial)
    Y { vy0_vec.play(&clamp.amp1) }   // Play recorded voltage waveform into clamp

- **Step 3: Record clamp current, then use it in a current clamp to reproduce voltage**

.. code-block:: hoc

    objref i_clamp_current
    i_clamp_current = new Vector()
    clamp.current(&t)            // Record clamp current (example, actual vector recording code needed)

    // Now use an IClamp to inject the recorded current into isolated Y
    objref iclamp
    Y { iclamp = new IClamp(0) }
    iclamp.del = 0
    iclamp.dur = 1e9
    i_clamp_current.play(&iclamp.amp)   // Play recorded current into current clamp

Corresponding Python usage:

.. code-block:: python

    from neuron import h

    # Step 1: Record voltage at Y(0)
    y = h.Section(name='Y')
    vy0_vec = h.Vector()
    t_vec = h.Vector()
    vy0_vec.record(y(0)._ref_v)
    t_vec.record(h._ref_t)
    # Run simulation for connected model
    h.finitialize(-65)
    h.continuerun(100)

    # Step 2: Disconnect Y and apply SEClamp to replay recorded voltage
    y.disconnect()
    clamp = h.SEClamp(y(0))
    clamp.dur1 = 1e9
    clamp.rs = 1e-3
    clamp.amp1 = -65  # initial voltage
    vy0_vec.play(clamp._ref_amp1)

    h.finitialize(-65)
    h.continuerun(100)

    # Step 3: Record clamp current and use IClamp with recorded current to reproduce voltage
    i_clamp_current = h.Vector()
    # i_clamp_current.record(clamp._ref_i)  # example: attach record to clamp current, run sim, save vector
    
    iclamp = h.IClamp(y(0))
    iclamp.del = 0
    iclamp.dur = 1e9
    i_clamp_current.play(iclamp._ref_amp)

    h.finitialize(-65)
    h.continuerun(100)

Please note that if the section Y contains active voltage-gated channels, the exact voltage responses may differ due to nonlinear dynamics. This method works best for passive or linear regimes. For more details, consult the NEURON Programmer’s Reference for `SEClamp`, `IClamp`, and the `Vector` class methods `record()` and `play()`.

Original Thread: https://neuron.yale.edu/phpBB/viewtopic.php?t=2926
