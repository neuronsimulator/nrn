Arrays as RANGE variables in NEURON
=====================================

**Q: How can I access and record STATE variables that are arrays in a NEURON mechanism from hoc or Python?**

In NEURON, STATE variables declared as arrays in NMODL are automatically available as RANGE variables and can be accessed in hoc or Python without declaring them as RANGE explicitly. Each element of the array corresponds to a RANGE variable with the suffix `_mechanismname`, allowing indexed access.

For example, given the following NMODL snippet:

.. code-block:: hoc

    NEURON {
      SUFFIX cadifus
      DEFINE Nannuli 4
      STATE {
        ca[Nannuli]       (mM) <1e-10>
        CaBuffer[Nannuli] (mM)
        Buffer[Nannuli]   (mM)
      }
    }

You can access each element of the `ca` array from hoc or Python with the syntax:

.. code-block:: hoc

    ca_cadifus[0](x)
    ca_cadifus[1](x)
    ca_cadifus[2](x)
    ca_cadifus[3](x)

where `x` is a segment or position along the section.

Similarly, in Python:

.. code-block:: python

    sec = h.Section()
    mech = h.cadifus(sec(0.5))
    print(mech.ca_cadifus[0](0.5))
    print(mech.ca_cadifus[1](0.5))

**Note on Recording:**  
Recording from such arrayed STATE variables does not multiplex automatically into a single Vector; each array element must be recorded individually.

This approach avoids the need to create separate RANGE variables for each shell or molecule, facilitating code reuse and scalability.

Original Thread: https://neuron.yale.edu/phpBB/viewtopic.php?t=1436
