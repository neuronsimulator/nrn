Recording Synaptic Currents (GABA, AMPA, NMDA) to Approximate LFP in NEURON
===============================================================================

**Question:**  
How can I record GABAergic, AMPAergic, and NMDAergic synaptic currents from a population of cells in NEURON, and efficiently sum them to approximate the Local Field Potential (LFP)? How can this be done in a memory-efficient way during long simulations?

**Answer:**  
Synapses in NEURON are typically implemented as point processes producing synaptic currents (units: nanoamperes). To record these currents for GABA, AMPA, and NMDA synapses, you must record the corresponding current variables from each synaptic mechanism instance using `Vector.record()`.  

However, synaptic mechanisms may have different variable names for their currents (e.g., `igaba`, `iampa`, `inmda`). For efficient recording and easy handling, it is recommended to modify each synaptic mechanism (`.mod` file) to define a `RANGE i` variable summing their respective currents, e.g.:

.. code-block:: hoc

  NEURON {
    POINT_PROCESS MySyn
    RANGE i
    ...
  }

  ASSIGNED {
    i (nA)
    igaba (nA)
    iampa (nA)
    inmda (nA)
    ...
  }

  BREAKPOINT {
    SOLVE release METHOD cnexp
    i = igaba + iampa + inmda  // sum actual synaptic currents here
    // (for specific synapses use i = igaba etc.)
  }

This standardizes recording by allowing `.i` to be recorded regardless of synapse type.

---

**Managing Large Numbers of Synaptic Instances**

If you have many synaptic instances (e.g., thousands), manually recording each current vector and summing at each timestep can be memory intensive. The recommended approach is:

1. Store all instances in a NEURON `List`.
2. After model setup but before simulation, create and record vectors for each synaptic current.
3. Run simulation in shorter segments.
4. After each segment, sum absolute values of all recorded synaptic currents and append the result to a cumulative Vector.
5. This approach avoids recording at every timestep and reduces memory usage.

---

**Example HOC code to record currents from a list of synapses:**

.. code-block:: hoc

  // Assume synlist is a List of synaptic point process instances

  objref isynvecs, tvec, sumabsisyn

  proc setup_recording() {
    int i
    objref tmpvec
    
    // Vector to record simulation time
    tvec = new Vector()
    tvec.record(&t)
    
    // List of Vectors to record synaptic currents
    isynvecs = new List()
    for i=0,synlist.count()-1 {
      tmpvec = new Vector()
      tmpvec.record(&synlist.o(i).i)
      isynvecs.append(tmpvec)
    }
    
    // Vector to store summed absolute currents per time point
    sumabsisyn = new Vector()
  }

---

**Example HOC snippet for breaking simulation into segments and summing synaptic currents:**

.. code-block:: hoc

  proc sumabsvecs() { local i localobj v, s
    v = new Vector(isynvecs.o(0).size())
    s = new Vector()
    v.assign(0)
    for i=0,isynvecs.count()-1 {
      s = isynvecs.o(i).c()
      s.abs()
      v.add(s)
    }
    return v
  }

  proc segrun() {
    // Parameters for simulation:
    // total_time = 1000  // ms
    // nsegments = 10
    local i
    local interval, t_end, lastpass
    localobj segtvec, segsumabsisyn

    interval = total_time / nsegments
    t_end = 0
    lastpass = 0

    // Initialize vectors for segment recording
    segtvec = new Vector()
    segtvec.record(&t)
    
    for i=0,nsegments-1 {
      t_end = t + interval
      if (t_end > total_time) {
        t_end = total_time
        lastpass = 1
      }
      continuerun(t_end)
      
      // Sum absolute synaptic currents for this segment
      segsumabsisyn = sumabsvecs()
      
      // Append segment data to cumulative vectors
      tvec.append(segtvec)
      sumabsisyn.append(segsumabsisyn)
      
      // Remove duplicate end points except last segment
      if (lastpass == 0) {
        tvec.remove(tvec.size()-1)
        sumabsisyn.remove(sumabsisyn.size()-1)
      }
      
      // Reset segment vectors for next interval
      frecord_init()
      segtvec.resize(0)
    }
    
    print "Simulation complete"
  }

---

**Python example for recording synaptic currents:**

.. code-block:: python

  from neuron import h

  # Assume synlist is a h.List object containing synaptic point process instances

  isynvecs = h.List()
  tvec = h.Vector()
  tvec.record(h._ref_t)

  def setup_recording(synlist):
      for i in range(synlist.count()):
          syn = synlist.o(i)
          vec = h.Vector()
          vec.record(syn._ref_i)  # assumes 'i' variable defined in mod files
          isynvecs.append(vec)

  # Later, after simulation run:
  # Sum absolute values of all recorded currents element-wise in Python
  import numpy as np

  def sum_absolute_currents(isynvecs):
      n = isynvecs.o(0).size()
      all_data = np.zeros((isynvecs.count(), n))
      for i in range(isynvecs.count()):
          all_data[i, :] = np.array(isynvecs.o(i).to_python())
      return np.sum(np.abs(all_data), axis=0)

---

**Summary:**

- Modify synaptic `.mod` files to define a common current variable `i` for ease of recording.
- Use NEURON `List` to hold synaptic instances.
- Record synaptic currents into vectors and sum their absolute values after simulation.
- To handle large simulations, divide simulation into segments, record within each segment, sum, and concatenate results.
- Avoid recording at every time step inside the simulation loop for performance and memory reasons.
- For clarity and maintainability, keep instrumentation and model specification separate.

This approach enables efficient recording and summation of synaptic currents for approximating LFP in NEURON simulations.

Original Thread: https://neuron.yale.edu/phpBB/viewtopic.php?t=3428
