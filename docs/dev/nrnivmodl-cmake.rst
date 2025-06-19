Mechanism building with CMake
=============================

.. note::

   This API is **experimental** and subject to change.

Helper functions for generating (core)NEURON mechanism libraries directly in CMake.
The basic idea is to replicate all of the previous functionality of the Makefiles,
but without having to deal with reading Makefiles or having to worry about dependencies.

Overview
--------

What ``nrnivmodl`` and ``nrnivmodl -coreneuron`` were doing was essentially:

- create a subdir equivalent to ``CMAKE_HOST_SYSTEM_PROCESSOR`` in the current working directory
- translate a given list of mod files to cpp files (using either NOCMODL or NMODL)
- create a file ``mod_func.cpp`` which dynamically (that is, upon running ``nrniv`` or similar) registers the mechanisms in NEURON
- create a ``nrnmech`` library in the ``CMAKE_HOST_SYSTEM_PROCESSOR`` subdirectory from all of the above listed cpp files
- create a ``special`` executable in the ``CMAKE_HOST_SYSTEM_PROCESSOR`` subdirectory from the ``nrnmain.cpp`` file
- link the above executable to the ``nrnmech`` library

In case the ``-coreneuron`` option is given, it additionally does the following:

- create a ``corenrnmech`` library in the ``CMAKE_HOST_SYSTEM_PROCESSOR`` subdirectory from all of the above listed cpp files, except with a different ``mod_func.cpp`` which correctly registers it under the ``coreneuron`` cpp namespace
- create a ``special-core`` executable in the ``CMAKE_HOST_SYSTEM_PROCESSOR`` subdirectory from the ``coreneuron.cpp`` file
- link the above executable to the ``corenrnmech`` library

Reference
---------

.. cmake:command:: create_nrnmech

    **Options**

    ``NEURON``
        (*optional*) whether a library compatible with NEURON should be created.

    ``CORENEURON``
        (*optional*) whether a library compatible with coreNEURON should be created. At least one of ``NEURON`` or ``CORENEURON`` must be specified.

    ``SPECIAL``
        (*optional*) whether a ``special`` (or ``special-core`` in case of coreNEURON) executable should be created.

    ``NMODL_NEURON_CODEGEN``
        (*optional*) whether to use NMODL to generate files compatible with NEURON.

    ``TARGET_LIBRARY_NAME``
        (*optional*, default: ``nrnmech``) the name of the CMake target for the library. Note that ``core`` is prepended to the coreNEURON target.

    ``TARGET_EXECUTABLE_NAME``
        (*optional*, default: ``special``) the name of the CMake target for the executable. Note that ``-core`` is appended to the coreNEURON target.

    ``ARTIFACTS_OUTPUT_DIR``
        (*optional*) the path where the CPP files will be placed at build-time.

    ``NOCMODL_EXECUTABLE``
        (*optional*) the path to the NOCMODL executable. If not specified, attempts to find deduce the location of the executable from the NEURON CMake configuration.

    ``NMODL_EXECUTABLE``
        (*optional*) the path to the NMODL executable. If not specified, attempts to find deduce the location of the executable from the NEURON CMake configuration.

    ``MOD_FILES``
        list of mod files to convert.

    ``NMODL_NEURON_EXTRA_ARGS``
        (*optional*, default: None) list of additional arguments to pass to NMODL for NEURON codegen.

    ``NMODL_CORENEURON_EXTRA_ARGS``
        (*optional*, default: ``passes --inline host --c`` if CUDA disabled, ``passes --inline host --c acc --oacc`` if CUDA enabled) list of additional arguments to pass to NMODL for coreNEURON codegen.

    ``EXTRA_ENV``
        (*optional*, default: None) list of additional environmental variables to pass when building the targets.
