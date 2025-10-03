Conditional termination of a simulation based on spike times in NEURON
==========================================================================

You can run a NEURON simulation and automatically stop it when a convergence criterion on inter-spike intervals (ISIs) is met, such as when the difference between the two most recent ISIs falls below a threshold. This can be achieved by using `NetCon.record()` to attach a callback function that tracks spike times, compares ISIs, and stops the simulation when the condition is fulfilled.

Key points:

- Use `NetCon` to detect spikes and record spike times via a callback function.
- Maintain variables (e.g., spike count and ISIs) in Python to evaluate termination condition.
- Use `FInitializeHandler` to initialize variables before each run for interactive usage.
- You do not need to write a new NMODL mechanism; Python or hoc code suffices.
- Global variables are a simple way to share state between calls; alternatively, pass and return variables explicitly from the callback.

Example in Python:
------------------

.. code-block:: python

    from neuron import h
    
    # Create a soma section with HH properties and surface area 100 um2
    soma = h.Section(name='soma')
    soma.L = soma.diam = 10  # roughly 100 um2
    h.hh.insert(soma)
    
    # Parameters
    TOL = h.dt  # convergence tolerance
    
    # Variables for spike counting and ISI tracking
    count = 0
    isi = 1e3      # initialize to large value
    previsi = 2e3  # initialize to different large value
    
    # Reset function called before each simulation run
    def resetstuff():
        global count, isi, previsi
        count = 0
        isi = 1e3
        previsi = 2e3
    
    # Register reset function to run at model initialization
    rc = h.FInitializeHandler(resetstuff)
    
    # NetCon to detect spikes, threshold set to -10 mV
    nc = h.NetCon(soma(0.5)._ref_v, None)
    nc.threshold = -10
    
    tsp = 0  # time of last spike
    
    # Called on each spike detected by NetCon
    def spikehappened():
        global tsp, count, isi, previsi
        count += 1
        if count == 1:
            tsp = h.t
        else:
            previsi = isi
            isi = h.t - tsp
            tsp = h.t
            if abs(isi - previsi) < TOL:
                h.stoprun = 1  # terminate simulation
        print(f"Spike {count} at {h.t} ms, ISI={isi:.3f}, prev ISI={previsi:.3f}")
    
    nc.record(spikehappened)
    
    # Example of stimulus:
    stim = h.IClamp(soma(0.5))
    stim.delay = 1
    stim.dur = 1e9
    stim.amp = 0.01
    
    # Run simulation
    h.load_file('stdrun.hoc')
    h.finitialize(-65)
    h.continuerun(10000)

Example in Hoc:
--------------

.. code-block:: hoc

    create soma
    access soma
    soma {
      insert hh
      L = 10
      diam = 10
    }
    
    objref nc
    proc resetstuff() {
      count = 0
      isi = 1e3
      previsi = 2e3
    }
    
    // Variables
    count = 0
    isi = 1e3
    previsi = 2e3
    tsp = 0
    
    proc spikehappened() {
      count += 1
      if (count == 1) {
        tsp = t
      } else {
        previsi = isi
        isi = t - tsp
        tsp = t
        if (fabs(isi - previsi) < dt) {
          stoprun = 1
        }
      }
      print "Spike at " + $o + " ms, count = " + count + " ISI = " + isi + " prev ISI = " + previsi
    }
    
    proc run_sim() {
      resetstuff()
      objref nc
      nc = new NetCon(&soma.v(0.5), nil)
      nc.threshold = -10
      nc.record(&spikehappened)
    
      objref stim
      stim = new IClamp(0.5)
      stim.del = 1
      stim.dur = 1e9
      stim.amp = 0.01
    
      finitialize(-65)
      run(10000)
    }
    
    run_sim()

Notes:

- Using `FInitializeHandler` (Python) or a reset function in hoc ensures variables are properly initialized before each run; this is especially helpful when running multiple simulations interactively.
- The `spikehappened` callback function updates spike count and ISIs, and sets `h.stoprun = 1` (or `stoprun = 1` in hoc) to end the simulation when the termination condition is met.
- Passing state variables as arguments and returning them is possible but more complex with `NetCon.record()`, which only supports callbacks without output. Global variables or object attributes are simpler in this context.

This approach provides a simple and effective way to conditionally terminate simulations based on spike time metrics without requiring additional NMODL mechanisms.

Original Thread: https://neuron.yale.edu/phpBB/viewtopic.php?t=4652
