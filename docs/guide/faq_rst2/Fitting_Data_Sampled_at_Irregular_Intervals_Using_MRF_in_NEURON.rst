Fitting Data Sampled at Irregular Intervals Using MRF in NEURON
=================================================================

**Question:**  
When fitting voltage vs. time data sampled at irregular intervals using MRF (Multiple Run Fitter) in NEURON, does MRF evaluate the fitness only at the provided data points, or does it perform linear interpolation between points? Also, how should one choose between fixed step and adaptive integration methods when fitting models?

**Answer:**  
MRF evaluates the model output at the exact timestamps of the provided experimental data. To do this, it performs linear interpolation on the model’s simulated time series to obtain voltage values at these specified time points. The fitness error is calculated as a weighted sum of squared differences between the data points and these interpolated model points. The number of points used in error calculation equals the number of provided data points.

When using adaptive integration, simulation results may not be at regular intervals, so MRF interpolates these results to compare at data points. For fixed step integration, the model values can be directly sampled at the data points or interpolated if necessary.

Regarding choosing between fixed and adaptive integration for MRF fitting, there can be differences in optimized parameter sets because the numerical integration methods may produce slightly different voltage traces. It is generally advisable to use fixed step integration or to record the model voltage at fixed time points (using Vector.record with a specified timestep or time vector) to ensure consistent comparison with data sampled at fixed intervals.

**Tips for fitting digitized data:**

- It is best to obtain and use raw experimental data if available.
- If only digitized data from figures are available, ensure careful digitization and consider interpolation or resampling to regularize time points.
- Use your judgment to evaluate the quality of fits, especially for action potentials where mean squared error might not fully capture the fitting quality.

Example usage in Python:
------------------------

.. code-block:: python

    from neuron import h
    import numpy as np

    # Define a single compartment with HH channels
    soma = h.Section(name='soma')
    soma.L = soma.diam = 12.6157  # 500 um2 area
    soma.insert('hh')

    # Stimulus
    stim = h.IClamp(soma(0.5))
    stim.delay = 5  # ms
    stim.dur = 1
    stim.amp = 0.1  # nA

    # Record voltage and time at fixed intervals
    tvec = h.Vector()
    vvec = h.Vector()
    dt = 0.025  # fixed time step
    h.dt = dt
    h.finitialize(-65)

    tvec.record(h._ref_t)
    vvec.record(soma(0.5)._ref_v)

    # Run simulation
    h.continuerun(40)

    # Suppose experimental data points (irregular intervals) are:
    exp_times = np.array([5.0, 5.725, 7.625])
    exp_voltages = np.array([-76.2631, 43.6974, -76.8252])

    # Interpolate model voltage at experimental time points
    model_voltages = np.interp(exp_times, np.array(tvec), np.array(vvec))

    # Compute error (sum of squared differences)
    error = np.sum((model_voltages - exp_voltages)**2 / len(exp_times))
    print("Mean squared error =", error)


Example using Hoc:
-----------------

.. code-block:: hoc

    create soma
    access soma
    soma {
        L = 12.6157
        diam = 12.6157
        insert hh
    }

    objref stim, tvec, vvec
    stim = new IClamp(0.5)
    stim.delay = 5
    stim.dur = 1
    stim.amp = 0.1

    tvec = new Vector()
    vvec = new Vector()
    tvec.record(&t, 0.025)
    vvec.record(&v(0.5), 0.025)

    dt = 0.025
    tstop = 40
    finitialize(-65)
    run()

    // Experimental data points (irregular intervals)
    // Define arrays or vectors with experimental times and voltages for fitting

    // Perform linear interpolation of model voltage at experimental times using appropriate methods

    // Calculate mean squared error accordingly

The MRF tool internally performs interpolation of the model voltage to the data time points when computing the fitness error. When using adaptive integration, consider recording model output at fixed time points for consistent comparisons.

Original Thread: https://neuron.yale.edu/phpBB/viewtopic.php?t=3112
