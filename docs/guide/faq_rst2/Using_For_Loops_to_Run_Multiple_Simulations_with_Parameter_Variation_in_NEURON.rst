Using For Loops to Run Multiple Simulations with Parameter Variation in NEURON
==============================================================================

**Question:**  
How can I run multiple simulation trials in NEURON, iterating over a parameter such as event times, using hoc or Python, without encountering syntax errors or simulation failures?

**Answer:**  
When running multiple simulation runs in NEURON where a parameter (e.g., a vector of spike times) changes each run, it is best to:

- Properly initialize and resize vectors at the start of each run instead of appending to them.
- Use NEURON’s standard run system (`run()`), which handles initialization and integration.
- Call `run()` inside a loop over the desired repetitions.
- Avoid repeatedly loading and unloading hoc files; there is no explicit hoc file close command.
- Set simulation parameters such as `dt` or integration method before calling `run()`.

Below are example implementations in hoc and Python.

**Hoc Example:**

.. code-block:: hoc

    objref svec, spkcnts
    svec = new Vector()
    spkcnts = new Vector()

    // Creates spike times starting at start, increment dt, with n intervals
    proc before_run() { 
        svec.resize($3 + 1)      // $1=start, $2=dt, $3=n_intervals
        svec.indgen($2)          // fills with 0, dt, 2*dt, ...
        svec.add($1)             // adds start to all elements
    }

    proc counter() {
        objref grcspk
        grcspk = new Vector()
        objref nc
        nc = new NetCon(&v(0.5), nil)
        nc.threshold = 0
        nc.record(grcspk)
        run()
        if (grcspk.size > 1) {
            spkfreq = (grcspk.size - 1) / (grcspk.x[grcspk.size-1] - grcspk.x[0])
        } else {
            spkfreq = 0
        }
        spkcnts.append(spkfreq)
    }

    proc batrun() { local i
        for i=1,$1 {
            before_run(3000, i, 100)
            counter()
        }
        print "Completed ", $1, " runs."
    }

    // Run 5 simulation trials
    batrun(5)

**Python Example:**

.. code-block:: python

    from neuron import h

    svec = h.Vector()
    spkcnts = h.Vector()

    def before_run(start, dt, n_intervals):
        svec.resize(n_intervals + 1)
        svec.indgen(dt)  # 0, dt, 2*dt, ...
        svec.add(start)

    def counter():
        grcspk = h.Vector()
        nc = h.NetCon(h.v(0.5)._ref_v, None)
        nc.threshold = 0
        nc.record(grcspk)
        h.run()
        if grcspk.size() > 1:
            spkfreq = (grcspk.size() - 1) / (grcspk.x[grcspk.size() - 1] - grcspk.x[0])
        else:
            spkfreq = 0
        spkcnts.append(spkfreq)

    def batrun(num_runs):
        for i in range(1, num_runs + 1):
            before_run(3000, i, 100)
            counter()
        print(f"Completed {num_runs} runs.")

    # Set simulation parameters
    h.dt = 0.025
    h.tstop = 10000
    h.v_init = -60

    # Run simulations
    batrun(5)

**Additional Notes:**

- Make sure variables such as `svec` are declared as `objref` (hoc) or instances of `Vector` in Python.
- Do not append to the vector of event times (`svec`) across runs without clearing or resizing; use `.resize()` and `.indgen()` to initialize.
- Use `load_file("nrngui.hoc")` or `load_file("stdgui.hoc")` at the start of your hoc file to access the standard NEURON run system.
- Adjust `dt` or specify integration methods (e.g., CVODE) before calling `run()` in hoc or `h.run()` in Python.
- To close files opened with `File` or `wopen`, use the appropriate `.close()` method; there is no hoc command for closing hoc source files.
- For adaptive integration and error control, use VariableTimeStep tools accessible via the GUI or session files.

This approach avoids syntax errors, negative time events, and other common pitfalls encountered when looping simulation runs with parameter changes in NEURON.

Original Thread: https://neuron.yale.edu/phpBB/viewtopic.php?t=2962
