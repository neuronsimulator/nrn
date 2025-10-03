How to access ionic currents defined in NMODL mechanisms from HOC or Python?
========================================================================

To access ionic current variables from an NMODL mechanism (e.g., a sodium current `ina`), it is important to properly declare the ionic currents in the `NEURON` block of the mod file using `USEION`. For ionic species other than the default `na`, `k`, `ca`, you must declare them with `VALENCE` and use new ion names.

1. **Declare ionic currents in the NMODL file:**

Use the `USEION` statement with appropriate WRITE variables and VALENCE (for new ions):

.. code-block:: mod

    NEURON {
        POINT_PROCESS ghkampa
        USEION na WRITE inas VALENCE 1
        USEION k WRITE iks VALENCE 1
  
        RANGE taur, taud
        RANGE iampa, inas, iks
        RANGE P, Pmax
    }

`inas` and `iks` are now modifiable ionic current variables for new ions `nas` and `ks`.

2. **Calculating the ionic currents inside BREAKPOINT:**

Use these assigned variables as your ionic currents in the BREAKPOINT block:

.. code-block:: mod

    BREAKPOINT {
        SOLVE state METHOD cnexp
        P = B - A
        inas = P * ghk(v, nai, nao, 1) * Area
        iks = P * ghk(v, ki, ko, 1) * Area
        iampa = inas + iks
    }

3. **Access the NMODL ionic current variables in HOC or Python:**

Because these ionic currents are associated with membrane segments, you can record or access them by indexing the segment:

- In **HOC**, use the variable name with segment index `x`:

.. code-block:: hoc

    if (ismembrane("nas_ion")) {
        ina_syn = inas(x) * area(x) / 100  // converts mA/cm2 to nA
    }

- In **Python**, access currents similarly:

.. code-block:: python

    for sec in h.allsec():
        for seg in sec:
            if "nas_ion" in seg.psection()['mechanisms']:
                ina_syn = seg.inas * h.area(seg.x) / 100  # mA/cm2 * um2 / 100 = nA

4. **Summing the total ionic currents over all segments:**

Example HOC procedure to calculate total current (nA):

.. code-block:: hoc

    func total_inas() { local total
        total = 0
        forall {
            for (x, 0) {
                if (ismembrane("nas_ion")) {
                    total += inas(x) * area(x) / 100   // convert mA/cm2 to nA
                }
            }
        }
        return total
    }

Example Python function:

.. code-block:: python

    def total_inas():
        total = 0.0
        for sec in h.allsec():
            for seg in sec:
                if "nas_ion" in seg.psection()['mechanisms']:
                    total += seg.inas * h.area(seg.x) / 100  # nA
        return total

5. **About point processes and arrays:**

- Point processes (e.g., synapses) are **not** inherently array variables in HOC. You cannot index them as `AMPAsyn[j]` unless you explicitly create an `objref` array to hold them.
- Be sure to declare arrays before use, for example:

.. code-block:: hoc

    objref AMPAsyn[80]
    for (i=0; i < 80; i+=1) {
        AMPAsyn[i] = new ghkampa(segment)
    }

- This allows you to loop over them in HOC.

6. **Avoid iterating over point processes to get currents:**

- It is better to declare new ionic species (e.g., `nas`, `ks`) and write currents to `inas` and `iks`.
- Then, each segment automatically sums the currents from all mechanisms using that ionic species.
- This removes the need for complex iteration over point processes.

Summary
-------

- Use `USEION ion WRITE current VALENCE n` to declare new ionic species in NMODL.
- Assign currents to these variables inside BREAKPOINT.
- Access these ionic currents via `ina(segment_index)` in HOC or `seg.ina` in Python.
- Sum over segments converting units appropriately.
- Create arrays of point processes explicitly in HOC before indexing.
- Prefer ionic currents over iterating point processes for simpler, more robust code.

This approach ensures unit consistency and proper tracking of ionic currents from synaptic or other mechanisms in NEURON simulations.

Original Thread: https://neuron.yale.edu/phpBB/viewtopic.php?t=4720
