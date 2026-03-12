Recording the Second Derivative of an Action Potential in NEURON
==================================================================

**How can I record and analyze the second derivative of an action potential in NEURON?**

You can record the time course of the membrane potential (`v`) and capacitive current (`i_cap`) during a simulation using `Vector.record()`. After the simulation completes, use the `Vector.deriv()` method to calculate derivatives (including the second derivative). It is recommended to perform the derivative calculation and plotting as a post-simulation step rather than inside NMODL (`.mod`) files.

Below is a typical workflow and example to help you implement this approach:

1. Record relevant variables (`v` and `i_cap`) into vectors during the simulation.
2. Run the simulation.
3. Post-process the recorded vectors to compute the derivatives.
4. Plot the results, such as phase plots of `d(i_cap)/dt` vs `i_cap`.

Example Hoc code snippet:

.. code-block:: hoc

    // Model setup
    create soma
    soma {
        nseg = 1
        L = 35
        diam = 15
        Ra = 150
        cm = 1
        insert pasnts
        insert kdrDA
        gbar_kdrDA = 100
        insert Na12
        Vmid_ac_Na12 = -35
        gbar_Na12 = 250
        insert ka
        gbar_ka = 2
        ena = 60
        ek = -90
    }

    tstop = 50
    dt = 0.025
    celsius = 32
    v_init = -55

    objref vvec, icvec, d2, g2

    // Instrumentation: record variables
    vvec = new Vector()
    vvec.record(&soma.v(0.5))
    icvec = new Vector()
    icvec.record(&soma.i_cap(0.5))

    d2 = new Vector()  // will hold derivative of i_cap

    // Post-processing to compute and plot
    proc postprocess() { 
      d2.deriv(icvec, dt)
      if (g2 == nil) {
        g2 = new Graph(0)
      } else {
        g2.exec_menu("Erase")
      }
      d2.plot(g2, icvec)
      g2.size(-0.5, 0.5, -15, 15)
      g2.view(-0.5, -15, 1, 30, 648, 296, 400.48, 300.32)
      g2.exec_menu("View = plot")
      g2.label(0.05, 0.9, "x-axis: i_cap(0.5)", 2, 1, 0, 0, 2)
      g2.label(0.05, 0.75, "y-axis: D_t(i_cap(0.5))", 2, 1, 0, 0, 3)
    }

    // Run simulation and postprocess
    proc myrun() {
      run()
      postprocess()
    }

    // Run simulation with postprocessing
    myrun()

Example Python code snippet (using NEURON's Python interface):

.. code-block:: python

    from neuron import h, gui
    import matplotlib.pyplot as plt

    soma = h.Section(name='soma')
    soma.L = 35
    soma.diam = 15
    soma.Ra = 150
    soma.cm = 1

    # Insert channels as needed, example:
    # soma.insert('pasnts')
    # ... (additional channel insertions and parameter settings)

    tstop = 50
    dt = 0.025
    h.dt = dt
    h.tstop = tstop
    h.celsius = 32
    h.v_init = -55

    vvec = h.Vector()
    icvec = h.Vector()
    tvec = h.Vector()

    vvec.record(soma(0.5)._ref_v)
    icvec.record(soma(0.5)._ref_i_cap)
    tvec.record(h._ref_t)

    def myrun():
        h.finitialize(h.v_init)
        h.run()
        # Calculate derivative of i_cap:
        deriv_ic = h.Vector()
        deriv_ic.size(len(icvec))
        for i in range(1, len(icvec)-1):
            deriv_ic.x[i] = (icvec.x[i+1] - icvec.x[i-1]) / (2*dt)
        deriv_ic.x[0] = (icvec.x[1] - icvec.x[0]) / dt
        deriv_ic.x[-1] = (icvec.x[-1] - icvec.x[-2]) / dt

        plt.figure()
        plt.plot(icvec, deriv_ic)
        plt.xlabel('i_cap(0.5)')
        plt.ylabel('d/dt(i_cap(0.5))')
        plt.title('Phase Plot: Second Derivative vs i_cap')
        plt.show()

    myrun()

**Tips:**

- Always record variables during the simulation rather than attempting to compute derivatives in NMODL files.
- Derivative computation must be done after `run()` because vectors are empty before simulation.
- If you use adaptive integration (variable dt), use `Vector.interpolate()` to resample signals at even time intervals before differentiation.
- To update plots after parameter changes, encapsulate the simulation and analysis inside procedures (`myrun` and `postprocess`) and clear or update graphs explicitly.
- To keep multiple traces for comparison, use NEURON's Vector display features or manage vectors and plots programmatically.

This workflow provides an accurate and flexible way to analyze derivatives and phase plots of action potentials in NEURON.

Original Thread: https://neuron.yale.edu/phpBB/viewtopic.php?t=3452
