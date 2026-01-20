How to change parameters used in NMODL for channel kinetics from hoc or Python?
================================================================================

When modifying parameters defined in a MOD file that affect channel kinetics (e.g., voltage-dependence parameters), changing the parameter values from hoc or Python alone may not immediately affect the channel kinetics if the mechanism uses `TABLE` statements to cache voltage-dependent variables.

Key points:
- Variables and parameters declared in the `PARAMETER` block and exposed via `RANGE` or `GLOBAL` can be accessed and modified from hoc or Python using the syntax `paramname_mechname`.
- If your mechanism uses `TABLE` statements without the appropriate `DEPEND` clauses, changing parameters from hoc or Python will not update the cached tables, and the channel kinetics will not change during simulation.
- To make parameter changes effective immediately, you have two options:
  1. Disable table usage by setting `usetable_mechname = 0` from hoc or Python. This forces the kinetics to be recalculated at each step but can slow down simulations.
  2. Modify your `TABLE` statements in the MOD file to include `DEPEND` clauses listing all parameters that affect each table. This ensures NEURON automatically recalculates tables when those parameters change.

Example of disabling tables from hoc or Python:

.. code-block:: hoc

    usetable_kv1 = 0  // Disable tables for kv1 mechanism
    kv1.vhkv1ninf_kv1 = -70
    kv1.kkv1ninf_kv1 = 20

.. code-block:: python

    sec = h.Section()
    sec.insert('kv1')
    h.usetable_kv1 = 0  # Disable tables
    sec(0.5).kv1.vhkv1ninf_kv1 = -70
    sec(0.5).kv1.kkv1ninf_kv1 = 20

Example modification of TABLE statements in NMODL to include DEPEND (conceptual):

.. code-block:: none

    TABLE ninf FROM -100 TO 100 WITH 200 DEPEND vhkv1ninf, kkv1ninf
    TABLE ntau FROM -100 TO 100 WITH 200 DEPEND akv1ntaul, bkv1ntaul, ckv1ntaul, dkv1ntaul, akv1ntaur, bkv1ntaur, ckv1ntaur, dkv1ntaur
    TABLE hinf FROM -100 TO 100 WITH 200 DEPEND akv1hinf, bkv1hinf, vhkv1hinf, kkv1hinf
    TABLE htau FROM -100 TO 100 WITH 200 DEPEND akv1htaul, bkv1htaul, ckv1htaul, dkv1htaul, akv1htaur, bkv1htaur, ckv1htaur, dkv1htaur

Note: Because NMODL allows only one `TABLE` statement per function, it may be necessary to separate the voltage-dependent variables into different FUNCTIONS or PROCEDURES to assign different `DEPEND` clauses. Alternatively, consider removing the `TABLE` statements when parameters change frequently to avoid inconsistency.

Tips:

- Avoid redundant naming in variables and parameters relative to the mechanism SUFFIX for easier access (e.g., use `gbar` instead of `gkv1bar`).
- After modifying parameters that affect tables, remember to recompile the MOD files.
- Refer to "The NEURON simulation environment" and "Expanding NEURON's repertoire of mechanisms with NMODL" papers for comprehensive understanding.

Summary:
Changing parameters that influence voltage-dependent kinetics requires attention to how `TABLE` statements are defined in MOD files. Disabling tables during parameter sweeps or properly using `DEPEND` clauses ensures parameter changes take immediate effect in simulations.

Original Thread: https://neuron.yale.edu/phpBB/viewtopic.php?t=4033
