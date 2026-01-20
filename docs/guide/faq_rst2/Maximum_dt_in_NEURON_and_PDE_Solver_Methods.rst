Maximum dt in NEURON and PDE Solver Methods
=============================================

**Q: Why can't I increase `dt` above 0.025 ms in NEURON, and what PDE solver methods does NEURON use?**

In NEURON's standard fixed time step integration, the time step `dt` is constrained to be compatible with the plotting interval parameter `steps_per_ms`. Specifically, `dt` is set according to the relation:

```
dt = 1 / (N * steps_per_ms)
```

where `N` is a positive integer (1, 2, 3, ...). This means `dt` can only take certain discrete values, effectively allowing you to reduce `dt` by integer factors but not increase it arbitrarily above the value determined by `steps_per_ms`.

Regarding the PDE solvers, NEURON approximates the second spatial derivative using the central difference method. Each section has an integer parameter `nseg` defining the number of segments (compartments) used to discretize the section. Solutions are calculated at the centers of these segments, so it is recommended to use odd values of `nseg` to have a node at the midpoint of the section. The default integration method is implicit Euler, but other methods such as Crank-Nicholson and adaptive order/time-step integrators are also available.

Example usage in Python:

.. code-block:: python

    from neuron import h

    # Set steps_per_ms
    h.steps_per_ms = 40  # for example

    # dt will be set automatically to 1/(N*steps_per_ms) when calling h.run()
    h.dt = 0.025  # initial dt

    # Running simulation
    h.finitialize(-65)
    h.continuerun(100)

Example usage in Hoc:

.. code-block:: hoc

    steps_per_ms = 40  // set steps per millisecond
    dt = 0.025         // initial dt
    initialize(-65)
    run()              // dt is adjusted internally to be compatible with steps_per_ms

To summarize:
- `dt` values are restricted by `steps_per_ms` and must satisfy `dt = 1/(N*steps_per_ms)`.
- The PDE solver uses central difference spatial discretization with adjustable `nseg`.
- Implicit Euler is the default solver method; others are available for advanced usage.

Original Thread: https://neuron.yale.edu/phpBB/viewtopic.php?t=1976
