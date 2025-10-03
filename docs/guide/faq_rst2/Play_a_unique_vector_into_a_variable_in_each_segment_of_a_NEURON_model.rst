Play a unique vector into a variable in each segment of a NEURON model
=======================================================================

**Question:**  
How can I play a unique vector into an arbitrary variable in each segment of a section in NEURON?

**Answer:**  
To play a unique vector into a variable for each segment, the variable must be declared as `RANGE` in the NMODL mechanism. When using `Vector.play()`, you must use the pointer reference to the variable (e.g., `seg.playtest._ref_xx`), not just `seg.playtest.xx`.

`GLOBAL` variables share the same value across all segments, and `POINTER` cannot be used as a per-segment storage for playing vectors.

---

**Example MOD file:**

.. code-block:: hoc

    NEURON {
        SUFFIX playtest
        RANGE xx
        NONSPECIFIC_CURRENT ixi
    }

    ASSIGNED {
        xx (mA)
        ixi (mA)
    }

    INITIAL {
        ixi = 0
        xx = 0
    }

    BREAKPOINT {
        ixi = xx + 1
    }

---

**Example Python script:**

.. code-block:: python

    from neuron import h
    import matplotlib.pyplot as plt

    h.load_file("stdrun.hoc")

    # Vectors of data and time points
    data = h.Vector([0, 1, 5, 17, 2, 4, 38, 22, 43, 1, 1])
    time = h.Vector([1, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10])

    # Create a section with multiple segments
    cell = h.Section("neurite")
    cell.nseg = 3  # use an odd number of segments for accuracy
    cell.L = 30
    cell.diam = 5

    # Insert the mechanism and play data into each segment's xx variable
    for sec in cell.wholetree():
        sec.insert(h.playtest)
        for seg in sec:
            data.play(seg.playtest._ref_xx, time, True)

    # Record time and xx variable at two segment locations
    t = h.Vector().record(h._ref_t)
    xxp1 = h.Vector().record(cell(0.1).playtest._ref_xx)
    xxp2 = h.Vector().record(cell(0.9).playtest._ref_xx)

    h.finitialize(-65)
    h.continuerun(10)

    plt.plot(t, xxp1, label="xx at 0.1")
    plt.plot(t, xxp2, label="xx at 0.9")
    plt.legend()
    plt.show()

Original Thread: https://neuron.yale.edu/phpBB/viewtopic.php?t=4623
