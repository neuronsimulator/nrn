.. _launching_hoc_and_python_scripts:

Running Python and HOC scripts
==============================

This section describes the various ways that you can **run** Python and HOC
scripts.
You may also want to refer to the :ref:`Python <python_prog_ref>` and
:ref:`HOC <hoc_prog_ref>` reference sections for documentation of how to write
scripts using the two languages.

HOC scripts are always run using one of the NEURON executables, such as
``nrniv``, ``special``, ``neurondemo`` or ``neurongui``.
Python scripts can also be run using these executables, but may additionally be
run using the standard ``python`` and ``ipython`` commands.

Files passed to the NEURON executables are identified based on their file
extensions, so:

.. code-block:: shell

    nrniv script.hoc
    nrniv script.py
    nrniv -python script.hoc
    nrniv -python script.py
    python script.py

Will all work as expected.
It is also possible to execute code from the commandline, for example:

.. code-block:: shell

    nrniv -c "hoc_statement"
    nrniv -python -c "python_statement"
    python -c "python_statement"

In this case, it is necessary to tell NEURON whether or not the code being
passed to ``-c`` is HOC or Python code, using the ``-python`` option.

When using Python, it is recommended that you organise your scripts so that
there is no need to pass more than one ``.py`` file or ``-c`` argument to
NEURON.
This is consistent with the ``python`` command, which only accepts a single
script file or ``-c`` option, and respecting this rule minimises the number of
possible differences between ``python script.py`` and ``nrniv script.py``.

In addition, Python scripts can execute HOC code internally using
:ref:`h <python_HocObject_class>`, and HOC scripts can execute Python code
internally using :ref:`nrnpython <hoc_function_nrnpython>`.

Advanced usage
~~~~~~~~~~~~~~

This section describes some of the caveats and differences between different
ways of running HOC and Python scripts.
In most cases, these details will not be important.

Use of custom MOD files:
    If you :ref:`use custom MOD files <extending_neuron_with_nmodl>` to extend
    NEURON, then some additional details may apply.

    In this case, you will have run the :ref:`nrnivmodl
    <FAQ_how_do_I_compile_mod_files>` command, which will have produced a new
    executable called ``special`` in a subdirectory named according to your
    computer architecture (*e.g.* ``x86_64/special``).

    ``special`` accepts the same commandline arguments as ``nrniv``, and it can
    be convenient or necessary (*e.g.* :ref:`when using GPU support
    <enabling_coreneuron>`) to launch scripts using it.

Multiple script files and commands:
    In Python mode (``-python``) the NEURON executables will only process one
    input file or ``-c`` command, whereas in HOC mode it will execute several.

    This means that ``nrniv a.py b.py`` will execute both scripts, but
    ``nrniv -python a.py b.py`` will only execute ``a.py`` and pass ``b.py`` as
    an argument in ``sys.argv``.

    Similarly, ``nrniv -c "hoc_code_1" -c "hoc_code_2"`` will execute both
    fragments of HOC code, but ``nrniv -python -c "pycode1" -c "pycode2"`` will
    only execute the first expression, ``pycode1``.

    It is best to organise your Python scripts to have a single entry point and
    to not rely on executing multiple files.
    This is consistent with the regular ``python`` command.
    
``sys.path``:
    NEURON aims to provide the same Python environment with ``nrniv -python``
    as you would obtain with ``python`` directly.
    This includes the behaviour for the first entry in ``sys.path``, which is
    an empty string when ``-c`` as used, and the script directory after
    resolving symlinks if a script is passed.
    See also: `the corresponding section of the Python documentation
    <https://docs.python.org/3/library/sys.html#sys.path>`_.
    If you try and execute multiple Python scripts, the ``sys.path`` behaviour
    may be surprising.

    One intentional difference is that if the path to the ``neuron`` module
    does not exist in ``sys.path`` then ``nrniv -python`` will automatically
    append it, while if you were to run ``python`` then an attempt to ``import
    neuron`` would simply fail.


``-pyexe`` and ``NRN_PYTHONEXE``:
    The NEURON executables also accept a ``-pyexe`` argument, which governs
    which Python interpreter NEURON will try and launch.
    The ``NRN_PYTHONEXE`` environment variable has the same effect, but if both
    are used then ``-pyexe`` takes precedence.

    This is typically only relevant in a build of NEURON that uses dynamic
    Python support (:ref:`NRN_ENABLE_PYTHON_DYNAMIC
    <cmake_nrn_enable_python_dynamic>`), which typically means the macOS and
    Windows binary installers.

    In this situation, ``nrniv -python`` searches for a Python installation in
    the following order:

    * The argument to ``-pyexe``.
    * The ``NRN_PYTHONEXE`` environment variable.
    * ``python``, ``python3``, ``pythonA.B`` ... ``pythonX.Y`` in ``$PATH``,
      where the set of ``pythonX.Y`` names corresponds to all the Python
      versions supported by the NEURON installation.
      The search order matches the :ref:`NRN_PYTHON_DYNAMIC
      <cmake_nrn_python_dynamic>` setting that was used at build time.
    * On Windows, some other heuristics are applied as a last resort.

    NEURON will exit with an error if you try to force it to use an unsupported
    Python version using ``-pyexe`` or ``NRN_PYTHONEXE``.
    If these are not passed, it will accept the first Python that is supported
    by the installation.

    On a system with multiple Python versions, this can lead to differences
    between ``python`` and ``nrniv -python``:

    .. code-block:: shell
        
        python -c "import neuron"        # fails, NEURON not installed
        python3.10 -c "import neuron"    # succeeds, NEURON installed for 3.10
        nrniv -python -c "import neuron" # succeeds, search ignores `python`
                                         # and continues to find `python3.10`

    Installations using Python wheels (``pip install neuron``) explicitly set
    the ``NRN_PYTHONEXE`` variable, so this section is unlikely to be relevant
    for those installations.
