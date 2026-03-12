Vector.play memory efficiency: Can I apply scaled versions of one vector to many synaptic conductances without storing multiple copies?
=======================================================================================================================================

If you have a sampled time course of synaptic conductance and want to apply scaled versions of that same time course to many synapses efficiently, you can use a single `Vector` to drive all synapses and scale the conductance within the synapse mechanism itself.

This approach requires modifying (or writing) a synaptic mechanism in NMODL that separates the input conductance time course (`gnorm`) from the scaling factor (`k`). The `Vector.play` is called once on `gnorm`, while each synapse has its own `k` parameter.

Example NMODL mechanism `VecSyn`:

.. code-block:: hoc

   COMMENT
   VecSyn, a "synaptic mechanism" whose conductance
   is driven by Vector.play.

   Actual synaptic conductance is gs.
   gs is the product of a scale factor k and gnorm,
   where gnorm is driven by a Vector.

   PARAMETER defaults:
     gnorm = 0 (microsiemens)
     k = 1 (dimensionless)
     erev = 0 (millivolt)

   gs and current i are updated each timestep.
   ENDCOMMENT

   NEURON {
      POINT_PROCESS VecSyn
      GLOBAL gnorm
      RANGE k, erev, gs
      NONSPECIFIC_CURRENT i
   }

   PARAMETER {
      gnorm = 0 (microsiemens)
      k = 1 (1)
      erev = 0 (millivolt)
   }

   ASSIGNED {
      gs (microsiemens)
      i (nanoamp)
      v (millivolt)
   }

   BREAKPOINT {
      gs = k * gnorm
      if (gs < 0) {
        i = 0
      } else {
        i = gs * (v - erev)
      }
   }

Using this mechanism, you create one `Vector` in hoc or Python with the conductance time course and play it into the shared parameter `gnorm` of each synapse instance. The scaling factor `k` is set differently for each synapse.

**Hoc example:**

.. code-block:: hoc

   objref v, synlist[10]
   v = new Vector()

   // load your conductance waveform into v here...

   for i=0, 9 {
     // Assume synlist[i] is a VecSyn instance on some section
     synlist[i].k = 1.0 / (i+1)  // example scaling factor
     v.play(&synlist[i].gnorm, dt)
   }

**Python example:**

.. code-block:: python

   from neuron import h
   import numpy as np

   signal = np.loadtxt('signal.txt')  # your conductance time course
   v = h.Vector(signal)

   synapses = []
   for i, cell in enumerate(cellList):
       syn = h.VecSyn(cell.soma(0.5))  # assuming VecSyn is loaded
       syn.k = 1.0 / cell.inDeg         # example scale factor
       v.play(syn._ref_gnorm, h.dt)
       synapses.append(syn)

This method allows you to store only one vector in memory regardless of how many synapses you have, reducing memory usage significantly while preserving the ability to apply individual scaling factors.

Original Thread: https://neuron.yale.edu/phpBB/viewtopic.php?t=4303
