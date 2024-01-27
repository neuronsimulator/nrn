.. _using_nmodl_files:

Using NMODL files
=================

Step 0:
-------
Save :download:`hhkchan.mod <code/hhkchan.mod>` into an empty directory.

Step 1:
------
For all versions of NEURON on MacOS and Linux, and NEURON 7.7+ on Windows:

    In a terminal, navigate to the folder containing the :file:`hhkchan.mod` file and run the shell script:

    .. code:: 
        none

        nrnivmodl

On a PC using a version of NEURON before 7.7:

    Launch ``mknrndll`` from the icon in the NEURON program group.

    Navigate to the directory containing the desired mod files.

    Select "Make nrnmech.dll" and the "mknrndll" script will create a nrnmech.dll file which contains the HHk model.

Step 2:
-------

Using the location of :file:`hhkchan.mod` as the working directory, start NEURON by typing ``python`` and then

.. code::
    python

    from neuron import h, gui


Step 3:
-------
Bring up a single compartment model with surface area of 100 µm\ :sup:`2` (:menuselection:`Build --> single compartment`) and toggle the HHk button in the Distributed Mechanism Inserter ON. Verify that the new HHk model (along with the Na portion of the built-in HH channel) produces the same action potential as the built-in HH channel (using both its Na and K portions).

 

