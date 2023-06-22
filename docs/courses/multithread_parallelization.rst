.. _multithread_parallelization:

Multithreaded parallelization
=============================

If a model has more than a few thousand states, it may run faster with multiple threads.

Physical System
---------------

Purkinje Cell

Model
-----

`Miyasho et al. 2001 <https://modeldb.yale.edu/17664>`_

Simulation
----------

Launch the model, let it run for about 30 seconds of wall time and stop it.
What ms of simulation time did it get to?

Pop up the :menuselection:`Tools --> ParallelComputing` panel. How large is the model? (The "Total model complexity" value is approximately the number of states in the model). This is a large model that should be able to run faster on a multicore workstation if we can parallelize the simulation using threads.

Press the "Refresh" button to see how many useful processors your machine has. May have to press it several times to get a stable number. The value is determined by how much time it takes N threads to count to 1e8. If N is greater than the number of cores on your machine, then the total time will go up. Enter the number of processors into the "#Threads" field. Thread performance has a chance of being good only if the "Load imbalance" is less than 20%. That can only happen if there are more cells than threads and the cells can be distributed so that the total complexity on each thread is about the same. Here, there is only one cell so we have to split the cell into pieces and put the pieces into different threads. This is done by pressing the "Multisplit" button. On my 4-core computer, it splits the cell into 26 pieces and distributes the pieces on 4 threads for a load imbalance of just 4%. Unfortunately, an error message is printed to the terminal window saying:

.. code::
    none

    cad is not thread safe

A look at the NMODL translator messages shows that not all of the mod files are threadsafe. We need to repair those mod files (cells that use a non-threadsafe mechanism are placed onto the main thread unless you force them onto a different thread, as above, in which case NEURON will generate an error message). A script to aid in the repair is called "mkthreadsafe" and is run in a bash terminal window. On mswin machines, start a bash terminal using the rxvt icon. When executed in the prknj directory it first complains about

.. code::
    none

            VERBATIM
            return 0;
            ENDVERBATIM
    Translating K22.mod into K22.c
    Notice: VERBATIM blocks are not thread safe
    ...
    Force THREADSAFE? [y][n]: n

This VERBATIM block is an old and never needed idiom that some people use to return a value from a PROCEDURE or FUNCTION. You can edit the :file:`K22.mod` file to remove it but it does not affect thread safety so you can type: ``y`` so that the script adds the THREADSAFE keyword to the NEURON block. The script continues with the same messages for :file:`K23.mod`, :file:`K2.mod`, :file:`KC2.mod`, :file:`KC3.mod`, :file:`KC.mod`, for which it is safe to type ``y``. Something new comes up with

.. code::
    none

    NEURON {
            SUFFIX Khh
            USEION k WRITE ik
            RANGE   gk,  gkbar, ik
            GLOBAL  ninf, nexp
    }
    Translating Khh.mod into Khh.c
    Notice: This mechanism cannot be used with CVODE
    Notice: Assignment to the GLOBAL variable, "nexp", is not thread safe
    Notice: Assignment to the GLOBAL variable, "ninf", is not thread safe
    Warning: Default 37 of PARAMETER celsius will be ignored and set by NEURON.
    Force THREADSAFE? [y][n]: n

This is an even more common idiom that uses global variables to save space. I.e A block calls a rate procedure that computes rate values and temporarily stores them for use later in the block. The assumption was that between assignment and use, no other instance of the model assigns a value to those variables. That assumption is false when there are multiple threads. Type "y" for this case as well. The script will add the THREADSAFE keyword to the NEURON block of the mod file which will cause GLOBALs that are assigned values to become thread variables. That was the last problem mentioned by the script. Unfortunately, there is one other problem in CalciumP.mod which is not tested by the script and you will continue to get the "cad is not thread safe" error if you launch the model. The problem is

.. code::
    none

            SOLVE state METHOD euler

I never bothered to make euler thread safe since the best practical methods are "cnexp" for hh-like equations and "derivimplicit" for all the others. So change the "euler" to "cnexp" manually in :file:`CalciumP.mod`.

Now one should build the dll as normally done on your machine and try the "Parallel Computing" tool again. My computer runs the model in 76s with one thread and 12s with 4 threads. The reason for the superlinear speedup is that multisplit forces "Cashe Efficient" on. It is often worthwhile turning that on even with a single thread (in my case, 49s).

Note: Multisplit, divides the cell into many independent cells which are connected together internally (check with ":func:`h.topology() <topology>`"). When divided into pieces the cell as a whole is difficult to deal with (for example, :func:`h.distance() <distance>` and Shape tools don't work well. Even h.topology() gives an incomplete idea of what is going on). So it is best to turn off "Multisplit" to re-assemble the cell to its original condition before doing any GUI manipulation.

 

Let's try another case using a network model.

Physical System
---------------

Cortex integrates sensory information. What is a moment in time?

Model
-----

Transient synchrony. `Hopfield and Brody 2001 <https://modeldb.yale.edu/2798>`_ implemented by Michele Migliore.

Simulation
----------

This model has a home-brew interface that does not show elapsed walltime, but to run and time the "before training" simulation one can copy-paste the following into the Python prompt:

.. code::
    python

    import time
    from neuron import h, gui

    h.load_file('mosinit.hoc')

    start_time = time.time()
    h.run_u()
    print(f'wall time: {time.time() - start_time}s')

This model also has non-threadsafe mechanisms. So we need to repair with ``mkthreadsafe`` (Another case of using GLOBAL variables for temporary storage.) However, running a sim with two threads gives an error

.. code::
    none
    
    ...usable mindelay is 0 (or less than dt for fixed step method)

Sadly, threads cannot be used when any :attr:`NetCon.delay` is 0. Fortunately, this model is not critically sensitive to the delay, so try again by setting all delays to 0.5 ms . (Copy-paste the following into the Python terminal)

.. code::
    python
    
    for netcon in h.List('NetCon'):
        netcon.delay = 0.5

With two threads the run will be faster, but far from twice as fast. Try again with "Cache Efficient" checked.

 
