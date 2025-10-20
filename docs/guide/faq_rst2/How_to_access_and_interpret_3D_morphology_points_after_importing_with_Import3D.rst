How to access and interpret 3D morphology points after importing with Import3D
=================================================================================

When you import a morphology using Import3D, the 3D points of a section are stored as sequences of (x, y, z, diameter) tuples accessible through the `pt3d` interface. Each section’s 3D shape is defined by these points, but they are not necessarily aligned with the segment endpoints, and segments themselves are temporary objects defined by the section’s discretization (`nseg`).

Key points to understand:

- A section’s first and last 3D points correspond to the 0 end of the first segment and the 1 end of the last segment.
- Intermediate 3D points do not necessarily coincide with segment boundaries.
- Segments are ephemeral entities defined by the number of segments (`nseg`) in a section and can be iterated directly with `for seg in section:`.
- The connection point of a child section to its parent section is given by `section.parentseg()`, which returns a location on the parent section as a normalized path length (`x` between 0 and 1), but this does not directly correspond to a particular 3D point index.
- Sections can be connected to any node (including internal nodes) of a parent section, not necessarily just the ends, which explains why their 3D points may not align.
- Shape plots translate child section 3D points to appear connected to the parent in visualization, which may differ from the original coordinates.

Example: Accessing 3D points in Python and Hoc
-----------------------------------------------

.. code-block:: python

    from neuron import h

    # Create a section with explicit 3D points
    sec = h.Section(name='axon')
    sec.pt3dclear()
    sec.pt3dadd(0, 0, 0, 1)    # x, y, z, diameter
    sec.pt3dadd(100, 0, 0, 1)

    # Iterate over all 3D points
    for i in range(sec.n3d()):
        x = sec.x3d(i)
        y = sec.y3d(i)
        z = sec.z3d(i)
        diam = sec.diam3d(i)
        print(f"Point {i}: ({x}, {y}, {z}), diameter={diam}")

    # Iterate over segments and access their normalized position x
    for seg in sec:
        print(f"Segment at normalized position {seg.x}")

.. code-block:: hoc

    create axon
    axon pt3dclear()
    axon pt3dadd(0, 0, 0, 1)
    axon pt3dadd(100, 0, 0, 1)

    // Iterate 3D points
    for i=0, axon.n3d()-1 {
        print i, axon.x3d(i), axon.y3d(i), axon.z3d(i), axon.diam3d(i)
    }

    // Iterate segments, print normalized position on section
    forall {
        for i=0, nseg-1 {
            objref s
            s = SecRef(secname())
            print secname(), s.x[i]
        }
    }

Understanding the connection point (`parentseg().x`):

- The `x` returned by `parentseg()` is a normalized path length (arc length) from 0 to 1 along the parent section where the child connects.
- This value does not necessarily correspond to an exact 3D point index.
- Sections in NEURON can be connected at any node along the parent, not just at endpoints.
- Visualization routines translate 3D points to reflect connectivity for display purposes but do not alter original coordinates.

For detailed understanding, reviewing the NEURON Programmer’s Reference sections on `pt3d` functions, segmentation, `parentseg()`, `connect()`, and 3D morphology representation is recommended.

Original Thread: https://neuron.yale.edu/phpBB/viewtopic.php?t=4190
