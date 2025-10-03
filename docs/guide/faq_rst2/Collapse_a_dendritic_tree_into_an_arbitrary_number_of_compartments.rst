Collapse a dendritic tree into an arbitrary number of compartments
==================================================================

**Question:**  
*How can I collapse a detailed dendritic tree into a specified number of equivalent compartments in NEURON, preserving the axial resistance?*

**Answer:**  
The original `collapse.hoc` script collapses a dendritic tree into three compartments: proximal, middle, and distal, based on axial resistance cutoffs. To increase or change the number of compartments (e.g., to 4, 10, or any arbitrary number), the code needs to be refactored to use a vector of cutoff values and loop over the number of compartments dynamically.

The key points for generalizing the code are: 
 
- Use a `Vector` object `CUTOFF` to store cutoff values that define compartment boundaries by axial resistance.  
- Use a variable `n_comp` that depends on the size of `CUTOFF` to define the number of compartments.  
- Replace all hard-coded occurrences of compartment counts and cutoff variables with loops and vector indexing using `n_comp` and `CUTOFF`.  
- Initialize arrays to the correct size according to `n_comp`.  

This approach preserves the axial resistance by merging dendritic branches into compartments where the total axial resistance falls within the cutoff intervals.

Below is a simplified example illustrating essential modifications in HOC to collapse a dendritic tree into `n_comp` compartments:

.. code-block:: hoc

   // Define cutoffs for any number of compartments (monotonically increasing)
   objref CUTOFF
   CUTOFF = new Vector()
   // Append cutoff values defining compartment boundaries, including 0 and a large number at the end
   CUTOFF.append(0, 7.3, 10, 20, 30, 40, 50, 60, 65, 71, 1e10)
   n_comp = CUTOFF.size() - 1  // number of compartments

   // Initialize vectors sized by n_comp+1 to accommodate 1-based indexing in some Vectors
   proc initcyc() {
       cycL = new Vector(n_comp + 1, 0)
       cycdiam = new Vector(n_comp + 1, 0)
       cycnin = new Vector(n_comp + 1, 0)
   }

   // Assign each section to a compartment based on axial resistance using the cutoffs
   proc cycmake() {
       local sec, i
       initto()
       initcyc()
       setmrg()

       // Assign top compartment (highest cutoff)
       for sec = 1, NSEC - 1 {
           if (secpathri.x(sec) >= CUTOFF.x[n_comp] && secpathri.x(sec) < CUTOFF.x[n_comp+1]) {
               tocyc.set(sec, -1)
           }
       }
       // Assign intermediate compartments
       for i = n_comp - 1, 1, -1 {
           for sec = 1, NSEC - 1 {
               if (secpathri.x(sec) >= CUTOFF.x[i] && secpathri.x(sec) < CUTOFF.x[i + 1]) {
                   tocyc.set(sec, i - n_comp)
               }
           }
       }
   }
   
   // Example to print collapsed compartments
   for ii = 1, n_comp {
       print "Section ", ii, "\tLength (um): ", cycL.x(ii), "\tDiameter (um): ", cycdiam.x(ii)
   }

Python users can control the number of compartments similarly by setting `CUTOFF` values and calling the equivalent functions from Python hoc calls or by manipulating compiled mechanisms.

Example Python snippet to define cutoffs and set `n_comp`:

.. code-block:: python

   from neuron import h

   # Define cutoffs as a NEURON Vector
   CUTOFF = h.Vector([0, 7.3, 10, 20, 30, 40, 50, 60, 65, 71, 1e10])
   n_comp = CUTOFF.size() - 1

   # Access hoc procedures to initialize and collapse dendritic tree
   h.initcyc()
   h.arborcharacterize()
   h.cycmake()

   for i in range(1, n_comp + 1):
       length = h.cycL.x(i)
       diameter = h.cycdiam.x(i)
       print(f"Section {i}: Length = {length}, Diameter = {diameter}")

---

**Summary:**  

To collapse a dendritic tree into more than the original three compartments:  

- Replace scalar cutoff variables with a vector of cutoffs.  
- Use the size of this vector as the number of compartments.  
- Modify loops and conditionals to index over this vector dynamically.  
- Initialize data structures sized accordingly.  

This generalization maintains the principles of the original collapse algorithm based on conservation of axial resistance.

---

**References:**  

- Bush and Sejnowski (1993) algorithm for collapsing dendritic trees by preserving axial resistance.  
- Original `collapse.hoc` by Neubig & Destexhe (1997; CNRS 2000).

Original Thread: https://neuron.yale.edu/phpBB/viewtopic.php?t=589
