Why does setting gbar=0 still produce current?
==============================================

When inserting a channel mechanism into a NEURON SectionList and setting its maximum conductance (`gbar`) to zero, you might still observe effects on the simulation. This is often due to misunderstanding what the SectionList contains and how the insertion and parameter assignment operates.

**Key points:**

- A SectionList (e.g., `dendritic`) may include sections you do not expect, such as the soma.
- Writing `dendritic { insert kv gbar_kv = 0 }` does **not** insert the `kv` mechanism into all dendritic sections; instead, it executes the block on a single section — usually the first section in the list (often `soma`).
- Therefore, setting `gbar_kv = 0` inside this block can inadvertently set the `gbar_kv` of the soma to zero, significantly altering the model’s behavior.
- To correctly insert mechanisms and set parameters on all sections in a SectionList, use the `forsec` iterator.

**Example usage in Hoc:**

.. code-block:: hoc

    // Incorrect: affects only first section in 'dendritic' SectionList (often soma)
    dendritic {
        insert kv
        gbar_kv = 0
    }

    // Correct: insert 'kv' and set gbar_kv=0 on all dendritic_only sections
    forsec dendritic_only {
        insert kv
        gbar_kv = 0
    }

**Example usage in Python:**

.. code-block:: python

    from neuron import h

    # Suppose h.dendritic_only is a SectionList without soma
    for sec in h.dendritic_only:
        sec.insert('kv')
        for seg in sec:
            seg.kv.gbar_kv = 0

**Tips:**

- Verify which sections are in your SectionList with:

  .. code-block:: hoc

      forsec dendritic print secname()

- If unwanted sections are included (e.g. soma in dendritic), consider creating a new SectionList excluding soma:

  .. code-block:: hoc

      dendritic_only = new SectionList()
      forsec dendritic dendritic_only.append()
      dendritic_only.remove(soma)

- Use `ifsec` for conditional execution on specific section names:

  .. code-block:: hoc

      forsec dendritic ifsec "som*" print secname()

Understanding the contents of SectionLists and correctly iterating over sections helps avoid unexpected simulation behavior when setting conductances to zero.

Original Thread: https://neuron.yale.edu/phpBB/viewtopic.php?t=2245
