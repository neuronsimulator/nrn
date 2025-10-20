Simple Internal Calcium Concentration: How to Correctly Implement and Initialize Cai in NEURON?
===============================================================================================

This FAQ entry addresses common issues encountered when implementing a simple internal calcium concentration mechanism (`cai`) in NEURON, focusing on proper initialization, unit consistency, and scaling of calcium currents to concentration changes.

Question
--------
Why does the internal calcium concentration (`cai`) not change from its initial value despite substantial inward calcium current (`ica`), and how should the mechanism be correctly implemented and initialized in NEURON?

Answer
------
1. **Initialization and Parameters:**
   - Declare a distinct parameter for the initial calcium concentration (`cai0`) in the `NEURON` block using `RANGE cai0`.
   - Set default values for parameters like `tau` (time constant), `C0` (background calcium concentration), and `cai0` in the `PARAMETER` block.
   - Ensure all parameters are assigned non-zero and physically meaningful values in hoc or Python before running simulations.

2. **Unit Consistency and Geometric Scaling:**
   - Calcium current density `ica` is reported in mA/cm², but calcium concentration change depends on the current influx per unit volume.
   - Use the surface-to-volume ratio rather than explicit volume and length multiplications to scale the influence of calcium current on `cai`.
   - For a cylindrical compartment, the surface-to-volume ratio is `4/d` (where `d` is diameter).
   - The derivative of `cai` should account for calcium influx with physical constants like Faraday’s constant.

3. **Avoiding Incorrect Volume Multiplication:**
   - Do not multiply `ica` by volume (`pi*diam*L`) as it incorrectly increases calcium influx with compartment size.
   - Instead, scale by `4/(diameter)` * `(ica / (2 * FARADAY))` to convert current density to concentration change rate, considering valence and units.

4. **Initialization Best Practices:**
   - Avoid custom initialization routines that manipulate time (`t`) or timestep (`dt`) manually.
   - Use NEURON’s standard `finitialize()` and `fcurrent()` procedures to ensure consistent state variable initialization.
   - Explicitly initialize `cai` in the mod file’s `INITIAL` block to `cai0` with proper unit conversion.

Example MOD File (calcium accumulation mechanism)
-------------------------------------------------

.. code-block:: hoc

   NEURON {
     SUFFIX cacumst
     USEION ca READ ica WRITE cai
     RANGE tau, C0, cai0
   }

   UNITS {
     (mA) = (milliamp)
     (mM) = (millimolar)
     (uM) = (micromolar)
     (um) = (micrometer)
     FARADAY = (faraday) (coulombs)
   }

   PARAMETER {
     tau = 303 (ms)       : time constant (e.g., AB soma)
     C0 = 0.5 (uM)        : background calcium concentration
     cai0 = 0.5 (uM)      : initial internal calcium concentration
   }

   ASSIGNED {
     ica (mA/cm2)
     diam (um)
   }

   STATE {
     cai (mM)
   }

   INITIAL {
     cai = cai0 * 1e-3  : convert from uM to mM for NEURON units
   }

   BREAKPOINT {
     SOLVE states METHOD cnexp
   }

   DERIVATIVE states {
     : Convert C0 and cai units properly; factor 4 converts surface-to-volume ratio for a cylinder
     cai' = -(1e4)*2*ica/(FARADAY*diam) + (1e-3)*(C0*1e-3 - cai)/tau
   }

Example Hoc Simulation Setup
----------------------------

.. code-block:: hoc

   load_file("nrngui.hoc")
   create soma
   access soma

   soma {
     diam = 16.695
     L = 17159      : large length to keep surface-to-volume ratio correct
     insert cacumst
     tau_cacumst = 1e9   : large tau to reduce buffering effects (test scenario)
     C0_cacumst = 0.5
     cai0_cacumst = 0.5
   }

   objref capp
   capp = new CaPP(0.5)    : custom point process to deliver Ca current
   capp.del = 1            : delay in ms
   capp.dur = 1            : duration in ms
   capp.amp = -303         : amplitude in nA (negative for inward current)

   finitialize(-65)
   run()

   print "Initial cai (mM): ", soma.cai_cacumst
   print "Final cai (mM): ", soma.cai_cacumst

Example Python Simulation Setup
-------------------------------

.. code-block:: python

   from neuron import h, gui

   soma = h.Section(name='soma')
   soma.diam = 16.695
   soma.L = 17159
   soma.insert('cacumst')
   soma.tau_cacumst = 1e9    # reduce buffering for test
   soma.C0_cacumst = 0.5
   soma.cai0_cacumst = 0.5

   # Define CaPP point process (similar to hoc version)
   class CaPP(h.PointProcess):
       def __init__(self, sec):
           h.PointProcess.__init__(self, sec(0.5))
           self.del_ = 1
           self.dur = 1
           self.amp = -303
       def play(self, t):
           pass  # Implement as needed, or use a current clamp equivalent

   h.finitialize(-65)
   h.run()

   print("Initial cai (mM):", soma(0.5).cai_cacumst)
   print("Final cai (mM):", soma(0.5).cai_cacumst)

Summary
-------
- Always define initial concentrations (`cai0`) explicitly and make them accessible from hoc or Python via `RANGE`.
- Scale calcium currents properly using compartment geometry; use surface-to-volume ratio to convert current density to concentration change.
- Avoid custom initialization codes manipulating time stepping; use NEURON’s built-in initialization.
- Check units carefully: `ica` is in mA/cm², `cai` in mM; appropriate scaling with Faraday’s constant and geometry is critical.

This procedure aligns with the calcium accumulation model described in Soto-Trevino et al., J Neurophysiol 94: 590–604, 2005.

Original Thread: https://neuron.yale.edu/phpBB/viewtopic.php?t=1272
