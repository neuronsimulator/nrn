Setting the Basal Level for Intracellular Calcium Concentration (cai) in NEURON
=================================================================================

**Question:**  
How can I set or initialize the basal intracellular calcium concentration (cai) to a desired value (e.g., 116 nM) when using calcium accumulation mechanisms such as `cdp.mod` in NEURON?

**Answer:**  
Setting the basal level of intracellular calcium concentration (cai) in NEURON models using calcium accumulation mechanisms like `cdp.mod` is non-trivial because these mechanisms include pumps, buffers, and diffusion processes that dynamically regulate cai. Directly changing parameters like `cainf` in `cad.mod` or simply assigning a value to `cai` may not affect the steady-state basal calcium concentration due to interactions of these processes.

**Key Points:**

- **Avoid inserting incompatible calcium mechanisms simultaneously.** For example, do not insert both `cdp` and `cad` mechanisms in the same section as both write to `cai` and can cause conflicting computations.

- **Using a "constant calcium source/sink" mechanism** can help achieve a desired steady-state basal cai without altering pump or channel kinetics. This is implemented as a supplemental current that balances the calcium flux from all other sources and sinks.

- **Initialization strategy:**  
  1. Insert the custom constant calcium source mechanism after all other mechanisms affecting `cai`.  
  2. Initialize `cai` to the desired basal value prior to simulation.  
  3. Calculate the compensatory current (`amp`) for this mechanism segment-wise to offset the net calcium flux, ensuring zero net flux at rest.  

- **General advice:**  
  Build and debug a simple toy model (one compartment with essential mechanisms) first, then port the approach to your complex model.

---

Example NMODL implementation of the constant calcium source mechanism `casrc.mod`:

.. code-block:: 

   NEURON {
     SUFFIX casrc
     USEION ca WRITE ica
     NONSPECIFIC_CURRENT ix
     RANGE amp
   }

   UNITS {
     (mA) = (milliamp)
   }

   PARAMETER {
     amp (mA/cm2)
   }

   ASSIGNED {
     ica (mA/cm2)
     ix (mA/cm2)
   }

   BREAKPOINT {
     ica = amp
     ix = -ica
   }


**How to use this mechanism in Hoc:**

.. code-block:: hoc

   // After inserting all other mechanisms that WRITE to ica
   forall if (ismembrane("ca_ion")) insert casrc  // Insert only where needed

   proc init() {
     cai0_ca_ion = 116e-6  // desired basal [Ca2+] in mM
     forall if (ismembrane("casrc")) amp_casrc = 0
     finitialize(v_init)
     forall if (ismembrane("ca_ion")) {
       for (x,0) {
         amp_casrc(x) = -ica(x)  // compensate to ensure net zero calcium flux
       }
     }
     if (cvode.active()) {
       cvode.re_init()
     } else {
       fcurrent()
     }
   }


**Equivalent Python example:**

.. code-block:: python
  
   from neuron import h, gui

   # Assume cell is created and mechanisms inserted, including 'cdp' or similar

   # Insert casrc to all segments with ca ion mechanism
   for sec in h.allsec():
       if h.ismembrane("ca_ion", sec=sec):
           h.casrc.insert(sec=sec)

   def init():
       h.cai0_ca_ion = 116e-6  # desired basal [Ca2+], in mM
       for sec in h.allsec():
           if h.ismembrane("casrc", sec=sec):
               for seg in sec:
                   seg.casrc.amp = 0.0
       h.finitialize(h.v_init)
       for sec in h.allsec():
           if h.ismembrane("ca_ion", sec=sec):
               for seg in sec:
                   seg.casrc.amp = -seg.ica
       if h.cvode.active():
           h.cvode.re_init()
       else:
           h.fcurrent()

---

**Additional recommendations:**

- Carefully check your model structure and code organization. Insertion and parameter assignments should be done in an orderly fashion.

- Use odd numbers for `nseg` in sections to avoid numerical issues.

- For complex models with multiple calcium sources and pumps, consider a "warm-up" simulation period or use `SaveState` to capture and reuse steady-states.

- Consult example mechanisms such as `cabpmp.mod` (included in NEURON distribution) for alternative calcium handling models with buffering and pumps.

- Utilize NEURON’s tools such as `MultipleRunFitter` for parameter optimization related to calcium dynamics.

---

This approach allows precise control over basal calcium concentration without altering fundamental kinetics of calcium handling mechanisms, maintaining physiological realism in simulations.

Original Thread: https://neuron.yale.edu/phpBB/viewtopic.php?t=1905
