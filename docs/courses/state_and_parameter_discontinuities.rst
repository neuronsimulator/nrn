.. _state_and_parameter_discontinuities:

State and parameter discontinuities
===================================

Physical System
---------------

Transient voltage clamp to assess action potential stability.

Model
-----

Force a discontinuous change in potential during an Action Potential.

Simulation
----------

To work properly with variable time step methods, models that change states and/or parameters discontinuously during a simulation must notify NEURON when such events take place. This exercise illustrates the kinds of problems that occur when a model is changed without reinitializing the variable step integrator.


1.

    Start with a current pulse stimulated HH patch. We recommend that you try creating this yourself with a brief current pulse at t = 0.1, either in Python or with the GUI tools. Our Python solution is :download:`hh_patch.py <code/hh_patch.py>`.

2.

    Discontinuously change the voltage by +20 mV via

    .. code::
        python

        def change():
            print(f'change at {h.t}')
            soma.v += 20

        def setup_discontinuities():
            h.cvode.event(2, change)

        fih = h.FInitializeHandler(setup_discontinuities)
    
    Note the difference between the fixed and variable step methods.


3.

    Replace the ``change()`` function with the following and try again:

    .. code::
        python

        def change():
            print(f'change at {h.t}')
            soma.v += 20
            h.cvode.re_init()

4.

    What happens if you discontinuously change a parameter such as ``gnabar_hh`` during the interval 2-3 ms without notifying the variable time step method?

    .. code::
        python

        def change(action):
            print(f'change at {h.t}: {action}')
            if action == 'raise':
                soma(0.5).hh.gnabar *= 2
            else:
                soma(0.5).hh.gnabar /= 2
            # h.cvode.re_init()   # should be here for cvode, but see below

        def setup_discontinuities():
            h.cvode.event(2, (change, 'raise'))
            h.cvode.event(3, (change, 'lower'))

        fih = h.FInitializeHandler(setup_discontinuities)

    It will be helpful to use the Crank-Nicholson fixed step method and compare the variable step method with and without the ``cvode.re_init()``. Zoom in around the discontinuity at 2 ms.