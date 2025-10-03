Action Potential Propagation and Intracellular Voltage in NEURON
==================================================================

1. **Does NEURON support point neuron models, or is it always based on cable theory?**

NEURON supports both cable theory-based compartmental models and point neuron models. For biophysically detailed kinetics, use a section with `nseg=1`. For simpler integrate-and-fire type models, use built-in artificial cell classes like `IntFire`, or create custom artificial cells using MOD files with the `ARTIFICIAL_CELL` keyword.

2. **Why don't I see a time delay in action potential propagation when I plot `axon.v(0.1)` and `axon.v(0.9)`?**

If an axon has only one segment (`nseg = 1`), all locations in that section belong to the same compartment, so no propagation delay is observed. To see propagation:

- Increase `nseg` (e.g., 101) to subdivide the axon.
- Use a smaller axon diameter (e.g., 2 microns) for more realistic propagation delays.
- Use a brief current clamp stimulus (e.g., 1 ms duration).

Example Python code demonstrating propagation delay:

.. code-block:: python

    import matplotlib.pyplot as plt
    from neuron import h
    from neuron.units import ms, mV

    h.load_file('stdrun.hoc')

    soma = h.Section(name='soma')
    axon = h.Section(name='axon')
    axon.connect(soma(0))

    soma.nseg = 1
    soma.diam = soma.L = 18.8

    axon.nseg = 101
    axon.diam = 2
    axon.L = 500

    for sec in [soma, axon]:
        sec.Ra = 123
        sec.insert('hh')

    stim = h.IClamp(soma(0.5))
    stim.delay = 100 * ms
    stim.dur = 1 * ms
    stim.amp = 0.9

    t = h.Vector().record(h._ref_t)
    v1 = h.Vector().record(axon(0.1)._ref_v)
    v2 = h.Vector().record(axon(0.9)._ref_v)

    h.finitialize(-65 * mV)
    h.continuerun(300 * ms)

    plt.plot(t, v1, label='axon(0.1).v')
    plt.plot(t, v2, label='axon(0.9).v')
    plt.xlim((99, 110))
    plt.xlabel('Time (ms)')
    plt.ylabel('Membrane potential (mV)')
    plt.legend()
    plt.show()

Equivalent hoc code snippet:

.. code-block:: hoc

    create soma, axon
    access soma

    soma.nseg = 1
    soma.diam = 18.8
    soma.L = 18.8
    soma.Ra = 123
    soma insert hh

    axon.nseg = 101
    axon.diam = 2
    axon.L = 500
    axon.Ra = 123
    axon insert hh

    connect axon(0), soma(0)

    objref stim
    soma stim = new IClamp(0.5)
    stim.del = 100
    stim.dur = 1
    stim.amp = 0.9

    tstop = 300
    init(-65)
    run()

3. **How can I compute intracellular voltage as \( V_i = V_m + V_e \) when using the extracellular mechanism?**

When the `extracellular` mechanism is inserted, `vext[0](x)` gives the extracellular potential at location `x`. The intracellular potential relative to ground is:

.. math::

    V_i = V_m + V_e

where:

- \( V_m \) is the membrane potential at \( x \) (e.g., `foo(x).v` in Python or `v` in HOC),
- \( V_e \) is the extracellular potential at the same location (e.g., `foo(x).vext[0]` in Python or `vext[0](x)` in HOC).

It is efficient to record both potentials during simulation and then perform the sum after the simulation:

Python example:

.. code-block:: python

    foov = h.Vector().record(foo(x)._ref_v)
    foovext = h.Vector().record(foo(x).vext[0])
    time = h.Vector().record(h._ref_t)

    h.finitialize(-65 * mV)
    h.continuerun(tstop * ms)

    vintracell = foov + foovext  # Vector addition of membrane and extracellular potentials

HOC example:

.. code-block:: hoc

    objref foov, foovext, tvec
    foov = new Vector()
    foovext = new Vector()
    tvec = new Vector()

    foov.record(&foo(x).v)
    foovext.record(&foo(x).vext[0])
    tvec.record(&t)

    // run simulation here

    // intracellular potential can be computed externally by adding corresponding vector elements

This method avoids costly recalculations during simulation and ensures accurate intracellular voltage measurement.

Original Thread: https://neuron.yale.edu/phpBB/viewtopic.php?t=4264
