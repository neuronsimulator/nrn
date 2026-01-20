Find all point processes in a NEURON model
============================================

**Question:**  
How can I find and remove all point processes (e.g., `IClamp`) in a NEURON model programmatically?

**Answer:**  
NEURON does not provide a built-in function to automatically find and remove all point processes from sections. Instead, it is recommended to keep track of created point process objects in a list when you create them. Later, you can remove references to these objects by clearing this list, allowing Python's garbage collector to free them.

Additionally, to inspect existing point process instances in the model, you can use `h.allobjects("ClassName")` which lists all instances of a given class along with their reference counts.

**Example Python usage:**

.. code-block:: python

    from neuron import h

    # Create sections
    soma = h.Section(name="soma")
    dend = h.Section(name="dend")

    # Keep references to point processes in a list
    doomed = []
    doomed.append(h.IClamp(soma(0.5)))
    doomed.append(h.IClamp(dend(0.1)))

    # When you want to remove them
    doomed = []  # Remove references so they can be garbage collected

    # To list all IClamp objects currently alive
    print(h.allobjects("IClamp"))  # e.g., prints IClamp[0] with 1 refs


**Example Hoc usage:**

.. code-block:: hoc

    create soma, dend

    objref doomed[2]
    doomed[0] = new IClamp(soma(0.5))
    doomed[1] = new IClamp(dend(0.1))

    // To delete, simply dereference or delete explicitly if desired
    // For example:
    doomed[0] = 0
    doomed[1] = 0

    // Use AllObjects("IClamp") to list existing IClamp instances (in hoc interpreter)


Original Thread: https://neuron.yale.edu/phpBB/viewtopic.php?t=4389
