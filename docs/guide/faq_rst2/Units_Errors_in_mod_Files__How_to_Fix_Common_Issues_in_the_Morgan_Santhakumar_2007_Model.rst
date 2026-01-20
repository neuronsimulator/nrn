Units Errors in mod Files: How to Fix Common Issues in the Morgan/Santhakumar 2007 Model
=========================================================================================

**Q:** When checking units in mod files from the Morgan/Santhakumar 2007 model, I get errors like unrecognized units, non-conformable units, or dimensionless variables incorrectly declared. How can I fix these units errors effectively?

**A:** The `modlunit` tool used by NEURON is very strict about unit consistency and the syntax of unit declarations. To fix units errors:

- Repeat running `modlunit` and fix errors one by one until there are no complaints.
- Use recognized standard units (e.g., `degC` instead of `DegC`).
- Declare dimensionless variables with `(1)` rather than units like `(ms)` if they represent pure numbers.
- Include explicit units in expressions when mixing units, for example indicating Celsius degrees with `(degC)` next to numeric constants.
- Keep `FUNCTION` return units consistent with their expressions.

**Example fixes:**

1. **Dimensionless variable declaration:**

In `ppsyn.mod`, change the variable `on` declaration from

.. code-block:: hoc

    on (ms)

to

.. code-block:: hoc

    on (1)

2. **Correct unit name capitalization (temperature):**

In `LcaMig.mod`, change all `DegC` to `degC` in the `UNITS` block and function arguments.

3. **Adding explicit units in expressions:**

Modify the function `KTF` in `LcaMig.mod` as follows to fix unit mismatches:

.. code-block:: hoc

    FUNCTION KTF(celsius (degC)) (mV) {
        KTF = ((25. (mV)/293.15 (degC))*(celsius + 273.15 (degC)))
    }

4. **Ensure dimensionless variables in expressions:**

For the function `ghk` where `nu = v/f` must be dimensionless, make sure `v` and the return value of `KTF` have consistent units so their ratio is dimensionless by assigning proper units to `KTF` as above.

---

**Python example** (using NEURON's Python interface and corrected MOD files):

.. code-block:: python

    from neuron import h, gui

    # Load the corrected mod files through nrnivmodl or equivalent

    # Create a segment and insert corrected mechanisms
    sec = h.Section(name='soma')
    sec.insert('CaBK')    # corrected Ca-dependent K channel
    sec.insert('tca')     # corrected T-type calcium channel
    sec.insert('ppsyn')   # corrected synaptic mechanism

    # Access the KTF function if needed
    # This requires the mod files compiled and loaded properly

    # Example initializations and simulations can be done here
    # e.g., h.finitialize(-65)
    #       h.continuerun(100)

---

**Hoc example snippet** to declare a dimensionless variable and correct units:

.. code-block:: hoc

    NEURON {
        SUFFIX ppsyn
        NONSPECIFIC_CURRENT i
        RANGE gs, gsbar, on
    }

    UNITS {
        (mV) = (millivolt)
        (ms) = (millisecond)
        (umho) = (micromho)
    }

    PARAMETER {
        gsbar = 0 (umho)
    }

    ASSIGNED {
        i (nA)
        gs (umho)
        on (1)  : dimensionless
    }

    BREAKPOINT {
        gs = gsbar * on
        i = gs * (v - erev)
    }

---

**Tip on removing strange files in Windows with Cygwin:**

If a file like `~$bgka.mod` appears and cannot be deleted normally, try escaping special characters in Cygwin:

.. code-block:: bash

    rm ~\$bgka.mod

This ensures the `$` is not interpreted as a variable.

---

By following these recommendations, you can systematically fix mod file units errors reported by `modlunit` for the Morgan/Santhakumar model or other NMODL files.

Original Thread: https://neuron.yale.edu/phpBB/viewtopic.php?t=3260
