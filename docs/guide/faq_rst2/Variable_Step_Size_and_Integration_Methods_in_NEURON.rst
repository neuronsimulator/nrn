Variable Step Size and Integration Methods in NEURON
====================================================

**Q:** Does NEURON use variable step size integration by default, and how does it achieve correct solutions for membrane voltage dynamics, such as in an RC circuit with a large conductance?

**A:** NEURON uses a fixed time step by default, but its default numerical integration method is the implicit (backward) Euler method, not variable step size. The implicit Euler method is unconditionally stable and effectively captures the dynamics of systems even when the time step dt is large compared to the system time constants. This allows NEURON to accurately solve equations like

.. math::

   C \frac{dv}{dt} = -g v

without the unrealistic results that the explicit (forward) Euler method might produce.

Example in Python:

.. code-block:: python

    from neuron import h

    h.dt = 0.1  # ms
    soma = h.Section(name='soma')
    soma.L = soma.diam = 1  # arbitrary dimensions to get 1 segment
    soma.nseg = 1

    # Insert a passive mechanism with Cm = 1 uF/cm2 typically
    soma.insert('pas')
    soma.cm = 1  # membrane capacitance in uF/cm2

    # Point process injecting current i = g * v
    # For illustration, a current clamp could be used but here we simulate a conductance:
    # Create a large conductance that clamps to zero voltage
    g = 1e6  # S (very large)
    v_init = 0.01  # 10 mV in Volts is 0.01 V, but NEURON uses mV internally, so set v_init = 10
    
    h.v_init = 10  # mV

    # As this is a conceptual code snippet, you would model the current i = g*v by a point process or by modifying equations.
    # Then run simulation:
    h.finitialize(h.v_init)
    h.continuerun(0.1)

    print(f"Membrane potential at t=0.1 ms: {soma.v}")

Example in Hoc:

.. code-block:: hoc

    create soma
    access soma
    soma {
        nseg = 1
        L = 10 // arbitrary length
        diam = 10 // arbitrary diameter
        insert pas
        cm = 1
    }

    // Set initial voltage
    v_init = 10

    // Simulate large conductance current: i = g * v, g = 1e6 S
    // This can be implemented as a POINT_PROCESS or mechanism injecting current proportional to voltage.

    dt = 0.1
    tstop = 0.1

    finitialize(v_init)
    run()

    printf("Membrane potential at t=0.1 ms: %g mV\n", v(0.5))

**Notes:**

- NEURON’s implicit Euler scheme allows the voltage to rapidly reach its steady state even with large dt.
- This differs from explicit Euler methods that may overshoot and produce unstable or unrealistic voltages.
- NEURON does not use variable time step integration by default; variable time step is available but must be enabled explicitly with `cvode`.

Original Thread: https://neuron.yale.edu/phpBB/viewtopic.php?t=136
