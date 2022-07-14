.. _batch_runs_with_bulletin_board_parallelization:

Batch runs with bulletin board parallelization
=============

Sooner or later most modelers find it necessary to grind out a large number of simulations, varying one or more parameters from one run to the next. The purpose of this exercise is to show you how bulletin board parallelization can speed this up.

This exercise could be done at several levels of complexity. Most challenging and rewarding, but also most time consuming, would be for you to develop all code from scratch. But learning by example also has its virtues, so we provide complete serial and parallel implementations that you can examine and work with.

The following instructions assume that you are using a Mac or PC, with at least NEURON 7.1 under UNIX/Linux, or the latest alpha version of NEURON 7.2 under OS X or MSWin. Also be sure that MPICH 2 is installed if you have UNIX/Linux (OS X already has it, and it comes with NEURON's installer for MSWin). If you are using a workstation cluster or parallel supercomputer, some details will differ, so ask the system administrator how to get your NEURON source code (.hoc, .ses, .mod, .py files) to where the hosts can use them, how to compile .mod files, and what commands are used to manage simulations.

Physical system
-------

You have a cell with an excitable dendritic tree. You can inject a steady depolarizing current into the soma and observe membrane potential in the dendrite. Your goal is to find the relationship between the amplitude of the current applied at the soma, and the spike frequency at the distal end of the dendritic tree.

Computational implementation
----------

The model cell
++++++++

The model cell is a ball and stick with these properties:

**soma**

    L 10 um, diam 3.1831 um (area 100 um2)
    
    cm 1 uf/cm2, Ra 100 ohm cm
    
    nseg 1
    
    full hh

**dend**
    
    L 1000 um, diam 2 um

    cm 1 uf/cm2, Ra 100 ohm cm

    nseg 25 (appropriate for d_lambda = 0.1 at 100 Hz)

    reduced hh (all conductances /=2)

The implementation of this model is in :download:`cell.hoc <code/cell.ho.txt>`

Code development strategy
+++++++++

Before trying to set up a program that runs multiple simulations, it is useful to have a program that executes a single simulation. This is helpful for exploring the properties of the model and collecting information needed to guide the development of a batch simulation program.

The next step is to create a program that performs serial execution of multiple simulations, i.e. executes them one after another. In addition to generating simulation results, it is useful for this program to report a measure of computational performance. For this example the measure will be the total time required to run all simulations and save results. The simulation results will be needed to verify that the parallel implementation is working properly. The performance measure will help us gauge the success of our efforts, and indicate whether we should look for additional ways to shorten run times.

The final step is to create a parallel implementation of the batch program. This should be tested by comparing its simulation results and performance against those of the serial batch program in order to detect errors or performance deficiencies that should be corrected.

In accordance with this development strategy, we provide the following three programs. For each program there is a brief description, plus one or more examples of usage. There are also links to each program's source code and code walkthroughs, which may be helpful in completing one of this exercise's assignments.

Finally, there is a fourth program for plotting results that have been saved to a file, but more about that later.

initonerun.hoc
+++++++++++

**Description**

- Executes one simulation with a specified stimulus.

- Displays response and reports spike frequency.

**Usage**

.. code::
    c++

    nrngui initbatser.hoc

A new simulation can be launched by entering the command

.. code::
    c++

    onerun(x)

at the oc> prompt, where x is a number that specifies the stimulus current amplitude in nA.

Example:

.. code::
    c++

    onerun(0.3)

**Source** 

:download:`initonerun.hoc <code/initonerun.ho.txt>`

`code walkthrough <https://nrn.readthedocs.io/en/latest/courses/batch_runs_bulletin_board_parallelization.html?.py#initonerun-py>`_

initbatser.hoc
+++++++++

**Description**

- Executes a batch of simulations, one at a time, in which stimulus amplitude increases from run to run.

- Then saves results, reports performance, and optionally plots an f-i graph.

**Usage**

.. code::
    c++

    nrngui initbatser.hoc

**Source**

:download:`initbatseer.hoc <code/initbatser.ho.txt>`

`code walkthrough <https://nrn.readthedocs.io/en/latest/courses/batch_runs_bulletin_board_parallelization.html?.py#initbatser-py>`_

initbatpar.hoc
++++++++

**Description**

- Performs the same task as initbatser.hoc, i.e. executes a batch of simulations, but does it serially or in parallel, depending on how the program is launched.

- Parallel execution uses NEURON's bulletin board.

**Usage**

Serial execution:

.. code::
    c++

    initbatpar.hoc

runs simulations one after another on a single processor, i.e. serially. Parallel execution:

.. code::
    c++

    mpiexec -n N nrniv -mpi initbatpar.hoc

launches N processes that carry out the simulations. On a multicore PC or Mac, parallel execution with N equal to the number of cores can reduce total run time to about 1/N of the run time required by initbatser.hoc, serial execution of initbatpar.hoc, or parallel execution of initbatpar.hoc with N = 1.

**Source**

:download:`initbatpar.hoc <code/initbatpar.ho.txt>`

`code walkthrough <https://nrn.readthedocs.io/en/latest/courses/batch_runs_bulletin_board_parallelization.html?.py#initbatpar-py>`_

Things to do
------

1.
    Run a few simulations with initonerun.hoc just to see how the cell responds to injected current. You might try to find the smallest and largest stimuli that elicit repetitive dendritic spiking.

2.
    Compare results produced by serial and parallel simulations, to verify that parallelization hasn't broken anything. For example:

.. code::
    c++

    nrngui initbatser.hoc
    mv fi.dat fiser.dat
    nrngui initbatpar.hoc
    mv fi.dat finompi.dat
    mpd &
    mpiexec -n 4 nrniv -mpi initbatpar.hoc
    mv fi.dat fimpi4.dat
    cmp fiser.dat finompi.dat
    cmp fiser.dat fimpi4.dat

Instead of cmp, MSWin users will have to use fc in a "Command prompt" window.

3. 
    Evaluate and compare performance of the serial and parallel programs.

    Here are results of some tests I ran on a couple of PCs.

.. code::
    c++

    NEURON 7.2 (508:9756f32df7d0) 2011-03-16 under Linux
    on a quad core desktop.
    initbatser     10.4 s
    initbatpar
        without MPI  10.2
        with MPI            speedup
        n = 1        10.2   1 (performance baseline)
            2         5.3   1.9
            3         3.5   2.9
            4         2.8   3.6

    NEURON 7.2 (426:7b4f020b29e8) 2010-03-12 under Linux
    on a dual core laptop
    initbatser      7.7 s
    initbatpar
        without MPI   7.5
        with MPI            speedup
        n = 1         7.7   1 (performance baseline)
            2         4.1   1.9

4.
    Make a copy of ``initbatpar.hoc`` and edit it, inserting printf statements that reveal the sequence of execution, i.e. which processor is doing what. These statements should report whatever you think would help you understand program flow. Here are some suggestions for things you might want to report:

- the identity of the host process (i.e. the pc.id)

- the name of the proc or func that is being entered or exited

- the value that is being passed or returned

- the index of the simulation that is being run

- anything else that you think might illuminate this dark little corner of computational neuroscience

Focus on the parts of the program that are involved in the master's principal function (posting jobs and gathering results), and the workers' principal function (entering, executing, and exiting ``fi``).

After inserting the printf statements, change NRUNS to 3 or 4, then run a serial simulation and see what happens.

Next run parallel simulations with -n 1, 2, 3 or 4 and see what happens. Do the monitor reports make sense?

5.
    Examine an f-i curve from data saved to one of the dat files.

.. code::
    c++

    nrngui initplotfi.hoc

then use its file browser to select one of the dat files.

Examine :download:`initplotfi.hoc <code/initplotfi.ho.txt>` to see how it takes advantage of procs that are built into NEURON's standard run library (UNIX/Linux users see ``nrn/share/nrn/lib/hoc/stdlib.hoc``, MSWin users see ``c:\nrnxx\lib\hoc\stdlib.hoc``).







