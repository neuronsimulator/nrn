.. _compiling_new_mechanisms_under_unix_linux:

Compiling new mechanisms under UNIX/Linux
========================

1.
    cd to the directory that contains your mod files.

2.
    Run 
    
    .. code::
        python

        modlunit filename

    on each mod file to check its units. This is very important. Even the simplest mechanisms can involve parameters and variables that have strange and confusing combinations of units. Unless you're very careful or very lucky, it is all too easy to write code that looks OK but produces results that are wrong by orders of magnitude because of a missing or incorrect conversion factor.

    The modlunit utility is provided to help detect and correct such errors. For instructions about how to fix units errors and how to define new units, `read this <https://nrn.readthedocs.io/en/latest/guide/units.html>`_.

3.
    When you are ready to compile the mod files, just type ``nrnivmodl``
