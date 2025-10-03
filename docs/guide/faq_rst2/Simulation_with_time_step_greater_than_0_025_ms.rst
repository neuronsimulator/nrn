Simulation with time step greater than 0.025 ms
==================================================

**Question:**  
How can I use a simulation time step (`dt`) greater than 0.025 ms in NEURON without it being automatically reset?

**Answer:**  
NEURON may automatically reset `h.dt` during a simulation if the `steps_per_ms` variable is not adjusted accordingly. To use a time step greater than 0.025 ms, set both `h.dt` and `h.steps_per_ms` consistently. Specifically, `h.steps_per_ms` should be set to the reciprocal of `h.dt` (in ms). For example:

.. code-block:: python

    from neuron import h
    h.dt = 0.05          # set time step to 0.05 ms
    h.steps_per_ms = 1.0 / h.dt  # adjust steps_per_ms accordingly
    h.finitialize(-65)
    h.continuerun(100)

In Hoc:

.. code-block:: hoc

    dt = 0.05  // set time step to 0.05 ms
    steps_per_ms = 1.0 / dt  // adjust steps_per_ms accordingly
    finitialize(-65)
    continue_ 100  


Original Thread: https://neuron.yale.edu/phpBB/viewtopic.php?t=3693
