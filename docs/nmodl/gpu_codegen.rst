NMODL codegen for GPU-enabled NEURON builds
===========================================

When NEURON is configured with ``NRN_ENABLE_GPU=ON``, the CMake API
``create_nrnmech`` (and the installed ``nrnivmodl`` CMake wrapper) default to
the **NMODL** transpiler with ``--neuron`` (option ``NMODL_NEURON_CODEGEN``).
The legacy **NOCMODL** path cannot emit OpenACC code and is rejected when
``create_nrnmech`` is invoked without ``NMODL_NEURON_CODEGEN`` on GPU builds.

The shell ``nrnivmodl`` makefile workflow still uses NOCMODL during the Phase A
transition so existing GPU ctests keep passing; use ``create_nrnmech`` with
``NMODL_NEURON_CODEGEN`` (or ``NRN_ENABLE_NMODL=ON``) for new GPU-targeted
mechanism work.

``create_nrnmech`` / CMake workflow
-----------------------------------

.. code-block:: cmake

   create_nrnmech(
     NEURON
     NMODL_NEURON_CODEGEN   # implied when NRN_ENABLE_GPU=ON
     MOD_FILES hh.mod)

``nrnivmodl`` shell workflow
----------------------------

.. code-block:: bash

   nrnivmodl mod                 # NOCMODL (Phase A transition; see above)
   nrnivmodl -coreneuron mod     # CoreNEURON mechanisms via NMODL + OpenACC
   nrnivmodl -nmodl $(which nmodl) mod   # explicit NMODL NEURON codegen

Passing ``-nocmodl`` or pointing ``-nmodl`` at a ``nocmodl`` binary fails on
GPU-enabled builds.

NMODL vs NOCMODL feature gaps (NEURON)
--------------------------------------

Use NMODL for new GPU-targeted mechanisms. Known construct coverage differences
that affect porting legacy MOD files:

+------------------+----------------------------+----------------------------------+
| MOD construct    | NOCMODL (legacy)           | NMODL ``--neuron``               |
+==================+============================+==================================+
| KINETIC          | Supported                  | Supported; preferred for new code|
+------------------+----------------------------+----------------------------------+
| TABLE            | Supported                  | Supported; verify table ranges   |
+------------------+----------------------------+----------------------------------+
| POINTER          | Supported                  | Supported with handle migration  |
+------------------+----------------------------+----------------------------------+
| NET_RECEIVE      | Supported                  | Supported; GPU delivery host-side|
|                  |                            | in Phase B native GPU            |
+------------------+----------------------------+----------------------------------+
| SOLVE methods    | Full legacy set            | Subset; check solver compatibility|
+------------------+----------------------------+----------------------------------+

OpenACC emission for NEURON mechanisms (``acc --oacc`` in
``NMODL_NEURON_EXTRA_ARGS``) is added in a follow-on PR; until then
``create_nrnmech`` defaults to NMODL so GPU CI can exercise the NMODL path
incrementally (ringtest mod set compiles with NMODL).

See also :doc:`transpiler/readme` and :doc:`language/nmodl_neuron_extension`.