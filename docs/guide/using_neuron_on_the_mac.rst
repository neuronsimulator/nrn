.. _using_neuron_on_the_mac:

Using NEURON on the Mac 
=======================

Under macOS, how do I compile mod files or start NEURON?
--------------------------------------------------------

The typical file organization is to have the python/hoc and mod files together in a folder (directory). The gui way to launch is then to drag the folder onto the mknrndll icon and then drag the seed python/hoc file onto the nrngui icon.

If you want to do everything with a UNIX-like terminal window, then in the folder containing the python/hoc and mod files type

.. code::
    bash

    nrnivmodl
    python your_starting_file.py

This assumes you have :file:`/Applications/NEURON-X.X/nrn/x86_64/bin` in your PATH (substitute the version number for the X.X).

