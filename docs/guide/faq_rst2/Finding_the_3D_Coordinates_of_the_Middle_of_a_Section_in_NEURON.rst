Finding the 3D Coordinates of the Middle of a Section in NEURON
=================================================================

**Question:**  
How can I accurately determine the 3D coordinates of the middle (e.g., 0.5 location) of a NEURON section for use in calculations like extracellular stimulation?

**Answer:**  
The 3D points defined by `pt3d` do not necessarily correspond to segment midpoints or the geometric center of the section because they define the shape and can be unevenly spaced. Simply averaging `pt3d` points or taking their midpoint may give incorrect results, especially for branched or curved sections.

The correct way is to use the *segment location* within the section and interpolate along the section’s path length to find the exact 3D coordinates of any location `x` along the section. NEURON provides `sec.arc3d(i)` for the arc length to each `pt3d` point, which can be used for interpolation.

A Python function that obtains the 3D coordinates of any segment (`seg`) center is shown below.

**Important notes:**

- Call `h.define_shape()` after setting up morphology and `nseg` for your model sections to ensure correct pt3d data alignment.
- The coordinate for `sec(0.5)` or `seg.x=0.5` corresponds to the middle of the section in normalized length units but not necessarily the center of any `pt3d` point.
- All segments are exposed to extracellular potentials; do not apply extracellular fields just at the "middle" segment.
- For models defined using `pt3dadd`, `h.define_shape()` does not add segment center points, so interpolation is necessary.

**Example Python code to get the 3D coordinate of the middle of a section:**

.. code-block:: python

    from collections import namedtuple
    import numpy as np
    from neuron import h

    Point = namedtuple('Point', ['x', 'y', 'z'])

    def coordinate(seg):
        sec = seg.sec
        n3d = sec.n3d()
        d = seg.x * sec.L  # arc distance along the section
        xs = [sec.x3d(i) for i in range(n3d)]
        ys = [sec.y3d(i) for i in range(n3d)]
        zs = [sec.z3d(i) for i in range(n3d)]
        arcs = [sec.arc3d(i) for i in range(n3d)]  # cumulative lengths
        x = np.interp(d, arcs, xs)
        y = np.interp(d, arcs, ys)
        z = np.interp(d, arcs, zs)
        return Point(x, y, z)

    # Usage example
    soma = h.Section(name='soma')
    soma.pt3dadd(0, 0, 0, 1)
    soma.pt3dadd(5, 5, 0, 1)
    soma.pt3dadd(10, 5, 0, 1)

    midpoint = coordinate(soma(0.5))
    print(f"Midpoint coordinates: {midpoint.x}, {midpoint.y}, {midpoint.z}")


**Equivalent example in Hoc:**

.. code-block:: hoc

    func coordinate() { local(sec, n3d, d, x, y, z, i, arcs, xs, ys, zs, xd, yd, zd)
        sec = $1.sec
        n3d = sec.n3d()
        d = $1.x * sec.L
        // Arrays for pt3d point coords and arc lengths
        objref arcs, xs, ys, zs
        arcs = new Vector(n3d)
        xs = new Vector(n3d)
        ys = new Vector(n3d)
        zs = new Vector(n3d)
        for i=0, n3d-1 {
            arcs.x[i] = sec.arc3d(i)
            xs.x[i] = sec.x3d(i)
            ys.x[i] = sec.y3d(i)
            zs.x[i] = sec.z3d(i)
        }
        // Interpolate along the arc for x, y, z
        xd = arcs.linearinterp(d, xs)
        yd = arcs.linearinterp(d, ys)
        zd = arcs.linearinterp(d, zs)
        return xd, yd, zd
    }

    // Usage example
    create soma
    soma.pt3dadd(0, 0, 0, 1)
    soma.pt3dadd(5, 5, 0, 1)
    soma.pt3dadd(10, 5, 0, 1)

    objref pt
    pt = new Vector(3)
    pt.x[0], pt.x[1], pt.x[2] = coordinate(soma(0.5))
    printf("Midpoint coordinates: %g %g %g\n", pt.x[0], pt.x[1], pt.x[2])


This method properly returns the 3D location corresponding to the normalized position on a section, such as 0.5 for the middle, regardless of the distribution of `pt3d` points.

----------

Summary:

- Do **not** use the average of pt3d points or simple midpoints between 3D points to find the middle of a section.
- Use linear interpolation of pt3d coordinates along the path length (`arc3d`) based on segment position.
- Call `h.define_shape()` after all morphology and discretization to guarantee correct shape information.
- Treat each segment discretization point separately; extracellular potentials must be calculated for every segment, not just the midpoint.


Original Thread: https://neuron.yale.edu/phpBB/viewtopic.php?t=4236
