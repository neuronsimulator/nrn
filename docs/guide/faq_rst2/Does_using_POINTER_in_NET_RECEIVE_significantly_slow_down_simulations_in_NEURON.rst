Does using POINTER in NET_RECEIVE significantly slow down simulations in NEURON?
================================================================================

When separating state variables into different NMODL mechanisms and linking them via POINTERs, users may observe a significant slowdown, especially when combining NET_RECEIVE, WATCH statements, and variable time-step integration. This is often not caused by POINTER itself, but by the adaptive integrator handling many frequent self-events triggered by WATCH, due to fast oscillations and resetting of state variables.

Key points and recommendations:

- Using WATCH to enforce hard boundaries (e.g., resetting a variable when it drops below zero) with variable time-step integrators can lead to many very small time steps and numerous self-events, causing a dramatic slowdown.

- POINTER usage alone generally does not cause a major computational penalty; the slowdown seen is mostly due to interaction with variable time-step and event handling.

- A better approach for enforcing boundary conditions with variable time-step integration is to replace hard resets with smooth "squashing" functions (e.g., logistic functions) applied to the forcing terms, to avoid rapid state variable resets.

- Fixed time-step integration allows simpler enforcement of boundary limits (checking and resetting state variables in BREAKPOINT) without the overhead of many self-events.

Example of declaring two equations in one NMODL mechanism (no POINTER), with WATCH to reset 'a' if it goes below zero:

.. code-block:: hoc

    create model
    access model
    objref obj
    obj = new equations(0.5)
    objref cvode
    cvode = new CVode(1)
    cvode_active(1)

.. code-block:: python

    NEURON {
        POINT_PROCESS equations
    }

    PARAMETER {
        b0 = 1.0
        w = 0.628
    }

    STATE { a b }

    BREAKPOINT {
        SOLVE states METHOD derivimplicit
    }

    INITIAL {
        a = 1.0
        b = 1
        net_send(0, 1)
    }

    DERIVATIVE states {
        b = b0 * cos(w * t)
        a' = a * (1 - a) - b
    }

    NET_RECEIVE(dummy_weight) {
        if (flag == 1) {
            WATCH (a < 0.0) 2
        } else if (flag == 2) {
            net_event(t)
            a = 0.0
        }
    }

Example of splitting into two mechanisms (using POINTER) — note that with WATCH + variable time-step, this can cause severe slowdown:

.. code-block:: python

    NEURON {
        POINT_PROCESS brain
        POINTER b
    }

    ASSIGNED { b }

    STATE { a }

    BREAKPOINT {
        SOLVE states METHOD derivimplicit
    }

    INITIAL {
        a = 1.0
        net_send(0, 1)
    }

    DERIVATIVE states {
        a' = a * (1 - a) - b
    }

    NET_RECEIVE(dummy_weight) {
        if (flag == 1) {
            WATCH (a < 0.0) 2
        } else if (flag == 2) {
            net_event(t)
            a = 0.0
        }
    }

.. code-block:: python

    NEURON {
        POINT_PROCESS body
    }

    PARAMETER {
        b0 = 1.0
        w = 0.628
    }

    STATE { b }

    BREAKPOINT {
        b = b0 * cos(w * t)
    }

    INITIAL {
        b = 1
    }

.. code-block:: hoc

    create model
    access model
    objref objBrain, objBody
    objBrain = new brain(0.5)
    objBody = new body(0.5)

    // Set pointer
    setpointer objBrain.b, objBody.b

    objref cvode
    cvode = new CVode(1)
    cvode_active(1)

Best practices for boundary enforcement under variable time step:

- Avoid hard resets with WATCH and NET_RECEIVE if possible.

- Use smooth squashing functions on forcing terms to prevent state variables from becoming negative, for example:

.. code-block:: python

    FUNCTION expit(x) {
      expit = 1 / (1 + exp(-x))
    }

    PARAMETER {
      k = 200
      c = 0.03
    }

    DERIVATIVE states {
      b = b0 * cos(w * t)
      a' = a * (1 - a) - b * expit(k * (a - c))
    }

- This approach avoids frequent event generation and small time steps, improving simulation efficiency with adaptive integrators.

Summary:

- POINTERs themselves do not inherently cause slowdowns.

- Slowness arises from WATCH-triggered frequent events combined with variable time-step integration resetting state variables.

- Using fixed time-step or smooth nonlinear functions to enforce boundaries improves performance.

- For models requiring boundary enforcement on state variables and efficient simulation, prefer smooth boundary conditions over hard resets when using variable time-step integrators.

Original Thread: https://neuron.yale.edu/phpBB/viewtopic.php?t=4499
