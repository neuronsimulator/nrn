Environment Variables
=====================

With a bash shell, these are set by, e.g. (on ubuntu)

.. code-block:: shell

    extern NEURONHOME=/where/neuron/was/installed/share/nrn

NRNUNIT_USE_LEGACY
------------------
  When set to 1, legacy unit values for FARADAY, R, and a few other constants
  are used. See ``nrn/share/lib/nrnunits.lib.in`` lines which begin with
  ``@LegacyY@``, ``nrn/src/oc/hoc_init.c`` in the code section
  ``static struct { /* Modern, Legacy units constants */``, and
  ``nrn/src/nrnoc/eion.c``.

  When set to 0, (default), values from codata2018 are used.
  See ``nrn/share/lib/nrnunits.lib.in`` lines that begin with
  ``@LegacyN@`` and ``nrn/src/oc/nrnunits_modern.h``.

  Switching between legacy and modern units can also be done after launch
  with the top level HOC function :func:`nrnunit_use_legacy`.

  The purpose of allowing legacy unit values is to easily validate
  results of old models (double precision identity).

  This environment variable takes precedence over the CMake option
  ``NRN_DYNAMIC_UNITS_USE_LEGACY``.
