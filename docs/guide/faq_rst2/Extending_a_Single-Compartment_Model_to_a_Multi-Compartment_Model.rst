Extending a Single-Compartment Model to a Multi-Compartment Model
=====================================================================

**Question:**  
How can I properly extend a single-compartment Hodgkin-Huxley (HH) model to a multi-compartment model in NEURON?

**Answer:**  
Extending a single-compartment model to a multi-compartment model involves creating multiple connected sections (e.g., soma, dendrites, axon) each with appropriate properties and mechanisms. Key points to consider:

- **Sections and Properties:**  
  The axial resistance (`Ra`) and membrane capacitance (`cm`) are *attributes* of sections, not mechanisms, and should be set for all compartments to represent the passive electrical properties of neurites.

- **Mechanisms:**  
  - Use Hodgkin-Huxley (`hh`) or modified HH mechanisms in compartments where active conductances are required, typically soma and axon.
  - Use passive leak channels (`pas` mechanism) in dendritic sections if you want passive properties there.
  
- **Resting Potential Considerations:**  
  Mixing different mechanisms (e.g., `hh` with resting potential around -65 mV and `pas` with default parameters around -70 mV) can produce non-uniform resting potentials, causing the model not to initialize in a steady state. Adjust parameters like the `e_pas` reversal potential to match resting potentials across compartments to avoid this.

- **Current Injection:**  
  Use a Point Process (e.g., `IClamp`) to inject current for stimulation anywhere in the model.

- **Modeling Strategy:**  
  There is no one "correct" way; the model should be constructed based on your scientific question. Extending the model involves relaxing simplifying assumptions from the single-compartment version.

**Example: Python**

.. code-block:: python

    from neuron import h, gui

    # Create sections
    soma = h.Section(name='soma')
    apical = h.Section(name='apical')
    basal = h.Section(name='basal')
    axon = h.Section(name='axon')

    # Connect sections
    apical.connect(soma(1))
    basal.connect(soma(0))
    axon.connect(soma(0))

    # Set section properties
    for sec in [soma, apical, basal, axon]:
        sec.Ra = 100    # axial resistance in ohm*cm
        sec.cm = 1      # membrane capacitance in uF/cm^2

    # Insert mechanisms
    soma.insert('hh')         # modified HH can be inserted similarly
    axon.insert('hh')

    apical.insert('pas')      # passive mechanism
    basal.insert('pas')

    # Adjust passive parameters to match resting potential
    for sec in [apical, basal]:
        for seg in sec:
            seg.pas.e = -65  # match the HH resting potential

    # Add a current clamp to soma
    stim = h.IClamp(soma(0.5))
    stim.delay = 100
    stim.dur = 500
    stim.amp = 0.1

    h.tstop = 700
    h.run()

**Example: Hoc**

.. code-block:: hoc

    create soma, apical, basal, axon

    connect apical(0), soma(1)
    connect basal(0), soma(0)
    connect axon(0), soma(0)

    forall {
        Ra = 100
        cm = 1
    }

    soma {
        insert hh
    }

    axon {
        insert hh
    }

    apical {
        insert pas
        for (x=0) { e_pas = -65 }
    }

    basal {
        insert pas
        for (x=0) { e_pas = -65 }
    }

    objref stim
    stim = new IClamp(0.5)
    soma stim = stim
    stim.delay = 100
    stim.dur = 500
    stim.amp = 0.1

    tstop = 700
    run()

Original Thread: https://neuron.yale.edu/phpBB/viewtopic.php?t=4097