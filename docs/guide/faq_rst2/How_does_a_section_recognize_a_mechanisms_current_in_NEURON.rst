How does a section recognize a mechanism’s current in NEURON?
==============================================================

In NEURON, for a section to recognize and record the current produced by a custom mechanism, the mechanism’s NMODL file must include a `BREAKPOINT` block that assigns the current variable (`i`) a value. Merely declaring a current as `NONSPECIFIC_CURRENT` without computing it inside a `BREAKPOINT` block will not make it visible to the section's current recordings.

A minimal working example for a synaptic-like mechanism producing a current is as follows:

.. code-block:: hoc

    NEURON {
        POINT_PROCESS netcontest
        RANGE amp0, delta, i
        NONSPECIFIC_CURRENT i
    }

    UNITS {
        (nA) = (nanoamp)
        (uS) = (microsiemens)
    }

    PARAMETER {
        amp0 = 0 (nA)
        delta = -0.01 (nA)  : Negative for inward (depolarizing) current
    }

    ASSIGNED {
        amp (nA)
        i (nA)
    }

    INITIAL {
        amp = amp0
    }

    BREAKPOINT {
        i = amp
    }

    NET_RECEIVE(weight (uS)) {
        amp = amp + delta
    }

Example usage in Python:

.. code-block:: python

    from neuron import h
    import matplotlib.pyplot as plt

    h.load_file("stdrun.hoc")

    s0 = h.Section()
    s0.insert('pas')
    syn = h.netcontest(s0(0.5))  # use the custom mechanism compiled from above

    h.tstop = 1000
    h.dt = 0.1

    evec = h.Vector([200, 400, 700])
    stim = h.VecStim()
    stim.play(evec)

    nc = h.NetCon(stim, syn)
    nc.weight[0] = 1

    i_syn = h.Vector()
    i_syn.record(syn._ref_i)  # record synaptic current

    i_sec = h.Vector()
    i_sec.record(s0(0.5)._ref_i_pas)  # record section membrane current at 0.5

    h.run()

    plt.figure()
    plt.plot(i_syn, label='syn current')
    plt.plot(i_sec, label='section i_pas')
    plt.legend()
    plt.show()

This setup ensures the mechanism's current affects the membrane current of the section and can be recorded and analyzed.

Original Thread: https://neuron.yale.edu/phpBB/viewtopic.php?t=4105
