What are the units of `ina` and `ik` in the Hodgkin-Huxley mechanism in NEURON?
=================================================================================

In the Hodgkin-Huxley (`hh`) mechanism, the variables `ina` and `ik` represent the sodium and potassium transmembrane currents, respectively. These currents are expressed as *current densities* with units of **mA/cm²**.

Note that although the `hh` mechanism calculates and writes to `ina` and `ik`, these variables are not accessible as Python or Hoc variables via `axon(0.5).hh.ina` or `axon(0.5).hh.ik`, because they are not declared as `RANGE` variables in the mod file. However, the total transmembrane current densities can be accessed directly from the segment, e.g., `axon(0.5).ina` and `axon(0.5).ik`.

Example in Python
-----------------

.. code-block:: python

    from neuron import h, gui

    axon = h.Section(name='axon')
    axon.nseg = 1
    axon.diam = 18.8
    axon.L = 18.8
    axon.Ra = 123.0
    axon.insert('hh')

    stim = h.SEClamp(0.5, sec=axon)
    stim.dur1 = 10
    stim.amp1 = -64.9737
    stim.dur2 = 90
    stim.amp2 = 90
    stim.dur3 = 0
    stim.amp3 = 0

    h.finitialize(-65)
    h.continuerun(100)

    print('Sodium current density (ina):', axon(0.5).ina, 'mA/cm²')
    print('Potassium current density (ik):', axon(0.5).ik, 'mA/cm²')

Example in Hoc
--------------

.. code-block:: hoc

    create axon
    axon {
        nseg = 1
        diam = 18.8
        L = 18.8
        Ra = 123.0
        insert hh
    }

    objref stim
    stim = new SEClamp(0.5, axon)

    stim.dur1 = 10
    stim.amp1 = -64.9737
    stim.dur2 = 90
    stim.amp2 = 90
    stim.dur3 = 0
    stim.amp3 = 0

    finitialize(-65)
    tstop = 100
    run()

    print "Sodium current density (ina): ", axon.ina(0.5), " mA/cm²"
    print "Potassium current density (ik): ", axon.ik(0.5), " mA/cm²"

Summary
-------

- `ina` and `ik` are current densities (mA/cm²), not total currents.
- They are accessible only through the segment (e.g., `axon(0.5).ina`), not through the mechanism instance (e.g., `axon(0.5).hh.ina`).
- The leak current `il` is accessible from the mechanism interface as `axon(0.5).hh.il`.

Original Thread: https://neuron.yale.edu/phpBB/viewtopic.php?t=4364
