Interfacing NEURON with other software at runtime
=================================================

**Q: How can I execute Python functions at every simulation time step to interface NEURON with other software, for example to control an artificial cell’s spike times dynamically?**

Instead of using a custom `.mod` file with a Python callback pointer, NEURON provides built-in Python callback mechanisms that are simpler and more reliable:

- `FInitializeHandler`: called once at initialization
- `CVode.event`: schedules a one-time callback event at a specified simulation time
- `CVode.extra_scatter_gather`: called at every time step (when CVODE variable time step integrator is used)

These callback facilities allow you to run Python functions during simulation and control parameters dynamically.

Example in Python using `CVode.extra_scatter_gather` to update `NetStim.interval` every time step:

.. code-block:: python

    from neuron import h
    import matplotlib.pyplot as plt

    h.load_file('stdrun.hoc')

    soma = h.Section(name='soma')
    soma.insert('hh')

    syn = h.ExpSyn(soma(0.5))
    syn.tau = 20
    syn.e = 0

    ns = h.NetStim()
    ns.interval = 20  # ms
    ns.start = 0
    ns.noise = 0
    ns.number = 1e9

    nc = h.NetCon(ns, syn)
    nc.delay = 0
    nc.weight[0] = 0.1

    vx = h.Vector().record(soma(0.5)._ref_v)
    t = h.Vector().record(h._ref_t)

    def update_interval():
        if int(h.t) % 50 == 0:
            ns.interval += 10

    esf = h.CVode().extra_scatter_gather(0, update_interval)

    h.tstop = 500
    h.run()

    plt.plot(t, vx)
    plt.show()

Example in Hoc demonstrating the registration of an event and an initialization handler:

.. code-block:: hoc

    proc called_once() {
        printf("called_once, t=%g\n", $t)
    }

    proc called_on_event() {
        printf("called_on_event, t=%g\n", $t)
    }

    objref fih, cvode
    fih = new FInitializeHandler(0, "called_once()")
    cvode = new CVode()
    cvode.event(0.1, "called_on_event()")

    initialize()
    cvode.continuerun(0.2)

**Note:** Be careful not to pass the result of calling a Python function as a callback (e.g., `set_callback(foo(ns))`) because it will evaluate immediately and pass `None`. Instead, pass a callable object such as a lambda or just the function reference.

This approach avoids the complexities of writing custom `.mod` files and integrates seamlessly with NEURON’s simulation loop.

Original Thread: https://neuron.yale.edu/phpBB/viewtopic.php?t=4242
