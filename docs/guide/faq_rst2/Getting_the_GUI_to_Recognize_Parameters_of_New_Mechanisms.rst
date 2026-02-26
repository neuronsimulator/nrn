Getting the GUI to Recognize Parameters of New Mechanisms
==========================================================

**Question:**  
How can I make a parameter of a new NEURON mechanism appear as an editable field in the GUI tools like CellBuilder or the Distributed Mechanism Viewer?

**Answer:**  
Parameters declared in the `PARAMETER` block of a MOD file are global by default, and global parameters do **not** automatically show up as editable fields ("buttons") in GUI tools such as CellBuilder or section parameter panels. Instead, global parameters can be accessed via the menu:

Tools / Distributed Mechanisms / Viewers / Mechanisms (Globals)

If you want a parameter to appear as an editable field in the GUI and be settable on a per-section or per-segment basis, you must declare it as a `RANGE` variable in the `NEURON` block of your MOD file. For example:

.. code-block:: 

    NEURON {
        SUFFIX foo
        USEION cl WRITE cl
        RANGE cli0
    }
    PARAMETER {
        cli0 = 12 (mM)  : This will be editable in GUI per section/segment
    }


Declaring a parameter as `RANGE` makes it a variable that can have different values at different locations and automatically creates GUI widgets for it.

In contrast, global parameters (the default) do not generate GUI widgets but are accessible globally and can be edited only via the mechanisms globals viewer.

This concept applies both to **distributed mechanisms** and **point processes**. A parameter must be declared as `RANGE` to appear as an editable GUI element. For point processes, ensure there are no typos in the `RANGE` declaration.

---

**Example MOD snippet (distributed mechanism):**

.. code-block:: hoc

    NEURON {
        SUFFIX clman
        USEION cl READ cli WRITE cl
        RANGE cli0
    }
    PARAMETER {
        cli0 = 12 (mM)
    }
    ASSIGNED {
        cl (mM)
    }
    INITIAL {
        cl = cli0
    }

---

**Using GUI tools to modify parameters**:

- For `RANGE` variables: Edit directly in CellBuilder, PointProcessManager, or Section Parameter panels.
- For global parameters: Use  
  `Tools / Distributed Mechanisms / Viewers / Mechanisms (Globals)`  
  to access and modify.

---

**Example usage in hoc:**

.. code-block:: hoc
    
    forall {
        insert clman
        // Set cli0 at the section level
        clman.cli0 = 10
    }


**Example usage in Python:**

.. code-block:: python

    for sec in h.allsec():
        sec.insert("clman")
        sec.cli0 = 10  # sets cli0 for this section if declared as RANGE



Original Thread: https://neuron.yale.edu/phpBB/viewtopic.php?t=2742
