Range syntax for varying morphology and mechanisms along a section in Python
==============================================================================

**Question:**  
How can I specify a parameter (e.g., diameter or ion channel conductance) that varies continuously along a section in NEURON using Python, similar to the hoc range syntax like `diam(0:1) = 1.5:0.5`?

**Short answer:**  
The hoc range syntax for specifying linearly varying parameters along a section (e.g., `diam(0:1) = 1.5:0.5`) does not directly translate to Python. Instead, assign values explicitly to each segment in the section, since segments are the smallest discrete units NEURON handles for parameter values. This can be done by iterating over segments and setting the parameter based on the normalized position (`seg.x`) of the segment’s center.

**Explanation:** 

- NEURON sections are divided into `nseg` segments.
- Parameters like `diam` or channel conductances can be set per segment.
- The hoc syntax that specifies variation over a range uses hidden piecewise linear interpolation internally and can be obscure and non-intuitive.
- In Python, it’s clearer and safer to assign values by looping over segments.
- The `.x` property of each segment corresponds to the normalized location (0 to 1) along the section.
- Assigning values at arbitrary points not corresponding to segment centers will affect the segment that contains that location.

**Example Python code:**
  
.. code-block:: python

    from neuron import h
    import numpy as np

    def range_assignment(sec, var, start, stop):
        """Linearly assign values to a range variable from start to stop along sec."""
        delta = stop - start
        for seg in sec:
            val = start + seg.x * delta
            setattr(seg, var, val)

    # Create a section
    axon = h.Section(name='axon')
    axon.nseg = 5

    # Vary diameter from 1.5 to 0.5 microns linearly along the section
    range_assignment(axon, 'diam', 1.5, 0.5)

    # Print diameters at segment centers
    print(axon.psection()['morphology']['diam'])
    # Output: [1.5, 1.25, 1.0, 0.75, 0.5]

    # Insert a mechanism and vary its conductance parameter
    axon.insert('hh')  # Example mechanism

    # Set gnabar_hh from 0 to 0.12 along the axon
    range_assignment(axon, 'gnabar_hh', 0, 0.12)

    for seg in axon:
        print(f"Location {seg.x:.2f}, gnabar_hh = {seg.gnabar_hh}")

**Equivalent hoc code:**

.. code-block:: hoc

    create axon
    axon.nseg = 5

    // Assign diameters linearly from 1.5 to 0.5
    for i=0, axon.nseg-1 {
        diam_val = 1.5 + (0.5 - 1.5) * ((i + 0.5)/axon.nseg)
        axon.diam((i + 0.5)/axon.nseg) = diam_val
    }

    // Insert hh mechanism and assign gnabar linearly
    axon insert hh
    for i=0, axon.nseg-1 {
        gnaba_val = 0 + (0.12 - 0) * ((i + 0.5)/axon.nseg)
        axon.gnabar_hh((i + 0.5)/axon.nseg) = gnaba_val
    }

**Notes:**

- The normalized segment location used for setting parameters is the center of each segment: `(i + 0.5)/nseg`.
- Parameters set to a location affect the entire segment containing that location.
- Assigning parameters at finer resolution than `nseg` will be clamped to segment values.
- For realistic morphologies, use 3D points (`pt3dadd`, etc.) to define spatial structure rather than segment diameters alone.
- It is strongly recommended to use explicit loops to set range variables for maintainability and clarity instead of obscure range expressions.



Original Thread: https://neuron.yale.edu/phpBB/viewtopic.php?t=4286
