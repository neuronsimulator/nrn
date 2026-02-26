Parallel assignment of large arrays in multiple instances of an NMODL mechanism
====================================================================================

**Q: How can I efficiently share a large constant array across many instances of the same NMODL mechanism without repeating the array assignment for each instance?**

When you have multiple instances of a mechanism with a large array of constant values that should be identical across all instances, assigning the array values in a loop for each instance is very inefficient. Instead, you can store the array data in a single NumPy array on the Python side and pass a pointer to that data to all mechanism instances using a POINTER in NMODL and a `VERBATIM` block for array access.

This method avoids copying data multiple times and allows all mechanism instances to reference the same array in memory.

**Example mechanism (NMODL):**

.. code-block:: hoc

    NEURON {
        POINT_PROCESS ARRAY_TEST
        POINTER foo
        RANGE val
    }

    ASSIGNED {
        v       (mV)
        foo
        val     (1)
    }

    BREAKPOINT {
        VERBATIM
        val = _p_foo[(int) _v + 100];
        ENDVERBATIM
    }

- Here, `foo` is a pointer to an external array.
- The `VERBATIM` block accesses the array via pointer `_p_foo`.
- Indexing uses the segment voltage cast to int plus an offset (example usage).

**Python usage:**

.. code-block:: python

    import numpy as np
    from neuron import h, numpy_element_ref

    # Create sections and mechanisms
    soma = h.Section(name='soma')
    dend = h.Section(name='dend')
    pps = [h.ARRAY_TEST(seg) for seg in [soma(0.5), dend(0.5)]]

    # Create large numpy array of data
    data = np.random.random(200)

    # Get pointer to the numpy array data
    ptr = numpy_element_ref(data, 0)

    # Assign the pointer to each mechanism instance's `foo` POINTER variable
    for pp in pps:
        pp._ref_foo = ptr

    # Example to demonstrate use
    soma(0.5).v = 14
    dend(0.5).v = -43
    h.fadvance()

    for pp in pps:
        print(pp.get_segment(), pp.val, data[int(pp.get_segment().v) + 100])

**Notes:**

- This requires NEURON version 7.7.2 or newer for the `_ref_foo` pointer assignment in Python.
- For older versions, you can use `h.setpointer` instead:

  .. code-block:: python

      for pp in pps:
          h.setpointer(ptr, 'foo', pp)

- The method allows multiple mechanism instances to share the same large array without copying.
- The `VERBATIM` block lets you directly use C syntax to dereference the pointer inside NMODL.

This technique efficiently supports embedding large constant arrays in many mechanism instances with minimal overhead.

Original Thread: https://neuron.yale.edu/phpBB/viewtopic.php?t=4167
