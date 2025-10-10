Initialization of Calcium Concentration in a Segment with Attached Point Process
====================================================================================

**Q: How can I properly initialize calcium concentrations (cai, cao) in segments that have calcium dynamics and attached point processes reading or writing calcium ions in NEURON?**

When using mechanisms that read and/or write calcium ion concentrations (`cai`, `cao`), such as calcium dynamics or synaptic point processes, the initial calcium concentrations may be set unexpectedly due to NEURON’s internal handling of ionic concentrations and the `ion_style` rules.

Key points:
- The default initial calcium concentrations used by NEURON are `h.cai0_ca_ion` and `h.cao0_ca_ion` (default values are typically 5.0e-5 mM for `cai` and 2.0 mM for `cao`).
- If a mod file does not explicitly initialize these ion concentrations (via an `INITIAL` block), NEURON overrides user settings for segments with mechanisms that use calcium ions.
- Using `ion_style` with `cinit=0` can be “promoted” internally if a later inserted mechanism requires calcium concentrations as state variables, causing unexpected reinitialization.
- To control initialization explicitly, it is recommended to add initialization statements in your NMODL files and/or set the NEURON default initial ionic concentration variables in your Python or Hoc code.

Recommended steps to ensure correct initialization:
1. **Modify your calcium dynamics NMODL (.mod) file**, e.g. `CaDynamics_E2.mod`:
  - Add a parameter for initial internal calcium concentration:

  .. code-block:: mod

     PARAMETER {
         cai0 = 1e-3 (mM)
         ...
     }

  - Add an INITIAL block to set the starting internal calcium concentration:
  
  .. code-block:: mod

     INITIAL {
         cai = cai0
     }
  
   - Optionally for extracellular calcium `cao`, either control externally or:

     - Add `cao` as a state variable with zero derivative if you want it fixed.

2. **Set the NEURON default initial extracellular calcium concentration in your Python or Hoc code**, for example:

  .. code-block:: python

    h.cao0_ca_ion = 1.2  # Set desired initial extracellular calcium concentration (mM)

3. **Avoid redundant or conflicting calls to `ion_style` with `cinit=0` on segments with calcium mechanisms**, because adding mechanisms that use calcium ions later may “promote” the ion style and override your settings.

---

Example Python snippet to initialize calcium concentrations:

.. code-block:: python

    # Set default extracellular calcium concentration before simulation initialization
    h.cao0_ca_ion = 1.2  # mM

    # Create your cell and insert mechanisms with calcium dynamics
    cell = YourCellModel()

    # Avoid calling ion_style with cinit=0 if later inserting calcium synapses
    # Instead, rely on INITIAL blocks and this default

    # Attach synapse onto a segment that uses calcium concentration
    syn = h.AMPAandNMDAplastic(0.5, sec=cell.apic)

    # Initialize simulation
    h.finitialize()

---

Example Hoc snippet illustrating modifying the default calcium initialization:

.. code-block:: hoc

    // Set default extracellular calcium concentration to 1.2 mM
    cai0_ca_ion = 0.0001  // Default internal concentration can remain or be set in mod file
    cao0_ca_ion = 1.2     // Change default extracellular calcium concentration

    create soma
    access soma
    insert cadynamics_e2
    // Do not override ion_style with cinit=0 after insertion of mechanisms reading/writing ca

    objref syn
    syn = new AMPAandNMDAplastic(0.5)
    syn.segment(&soma(0.5))

    // Initialize the simulation
    finitialize()

---

By using these methods, you ensure consistent and stable initialization of calcium concentrations in compartments with calcium dynamics and point processes that access `cai` and `cao`.

Original Thread: https://neuron.yale.edu/phpBB/viewtopic.php?t=4325
