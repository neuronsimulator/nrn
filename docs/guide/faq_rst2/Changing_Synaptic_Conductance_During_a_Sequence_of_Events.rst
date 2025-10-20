Changing Synaptic Conductance During a Sequence of Events
==========================================================

**Question:**  
How can I change synaptic conductance dynamically during a sequence of synaptic activation events in NEURON?

**Answer:**  
In event-driven synaptic models such as those using `ExpSyn` or similar mechanisms, synaptic strength is typically controlled by the weight of the `NetCon` delivering the event, not by changing a `gmax` parameter inside the synapse mechanism. Changing `gmax` of a synapse during a simulation can cause non-physiological effects and is generally discouraged.

To implement a specific sequence of synaptic activations with different strengths at each event time, the recommended approach is:

1. Use a `VecStim` object to generate a spike train at the specified times.
2. Create one `NetCon` per synapse, setting its source to the `VecStim`.
3. To change synaptic weights at each event:
   - Adjust the weights *before* events occur.
   - Use an offset delay (`actdel`) and an additional `NetCon` to call a procedure that changes weights according to a pre-computed vector of weights.

Below is an example of how to implement this strategy in HOC:

.. code-block:: hoc

   // Assume stvec contains event times, swvec contains weights for each event,
   // and synlist is a List of synaptic point processes (e.g. ExpSyn instances).

   // Find shortest inter-event interval / 2
   objref tmpvec
   tmpvec = new Vector()
   tmpvec = stvec.c
   tmpvec.deriv()
   double actdel = tmpvec.min()/2.0
   if (stvec.x[0] < actdel) {
     actdel = stvec.x[0] / 2.0
   }
   
   // Shift event times earlier by actdel
   stvec.sub(actdel)
   
   // Create VecStim to generate events at shifted times
   objref vecstim
   vecstim = new VecStim()
   vecstim.play(stvec)
   
   // Create NetCons connecting VecStim to synapses with delay = actdel
   objref nclist
   nclist = new List()
   for (int i=0; i < synlist.count(); i++) {
     nclist.append(new NetCon(vecstim, synlist.o(i)))
     nclist.o(i).delay = actdel
   }
   
   // Initialize event counter
   int count = 0
   objref fih
   fih = new FInitializeHandler("count = 0")
   
   // Procedure to update weights before event occurs
   proc setweights() {
     for (int i=0; i < nclist.count(); i++) {
       nclist.o(i).weight = swvec.x[count]
     }
     count += 1
   }
   
   // Create NetCon to trigger setweights() at the shifted event times (no delay)
   objref ncx
   ncx = new NetCon(vecstim, nil)
   ncx.delay = 0
   ncx.record("setweights()")

**Python equivalent example:**

.. code-block:: python

   from neuron import h

   # Assume stvec is a h.Vector of event times
   # swvec is a h.Vector of synaptic weights for each event
   # synlist is a list of synaptic point processes, e.g., [exp_syn1, exp_syn2, ...]
   
   # Find shortest inter-event interval divided by 2
   tmpvec = stvec.c()
   tmpvec.deriv()
   actdel = tmpvec.min() / 2.0
   if stvec.x[0] < actdel:
       actdel = stvec.x[0] / 2.0
   
   # Adjust event times
   stvec.sub(actdel)
   
   # Create VecStim
   vecstim = h.VecStim()
   vecstim.play(stvec)
   
   # Create NetCons from VecStim to synapses
   nclist = []
   for syn in synlist:
       nc = h.NetCon(vecstim, syn)
       nc.delay = actdel
       nclist.append(nc)
   
   count = 0
   
   def setweights():
       nonlocal count
       for nc in nclist:
           nc.weight[0] = swvec.x[count]
       count += 1
   
   # Use a NetCon to call setweights() at event times with zero delay
   ncx = h.NetCon(vecstim, None)
   ncx.delay = 0
   ncx.record(setweights)

**Additional Notes:**  
- Each synapse requires its own `NetCon`.  
- Using `VecStim` simplifies managing sequences of multiple synaptic activations.  
- Avoid implementing a `gmax` parameter to scale conductance within event-driven synaptic mechanisms, as changing `gmax` affects all events on that mechanism instance and can break the causality of synaptic transmission.  
- The synaptic strength should be changed by adjusting the `NetCon` weight before the corresponding event fires.

Original Thread: https://neuron.yale.edu/phpBB/viewtopic.php?t=2840
