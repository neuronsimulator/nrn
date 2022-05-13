.. _batch_runs_bulletin_board_parallelization:

Batch runs with bulletin board parallelization
==============================================

Sooner or later most modelers find it necessary to grind out a large number of simulations, varying one or more parameters from one run to the next. The purpose of this exercise is to show you how bulletin board parallelization can speed this up.

This exercise could be done at several levels of complexity. Most challenging and rewarding, but also most time consuming, would be for you to develop all code from scratch. But learning by example also has its virtues, so we provide complete serial and parallel implementations that you can examine and work with.

    *The following instructions assume that you are using a Mac or PC, with at least NEURON 7.1 under UNIX/Linux, or NEURON 7.2 under macOS or MSWin. For UNIX, Linux, or macOS, be sure MPICH 2 or OpenMPI is installed. For Windows, be sure Microsoft MPI is installed. If you are using a workstation cluster or parallel supercomputer, some details will differ, so ask the system administrator how to get your NEURON source code (.py, .ses, .mod files) to where the hosts can use them, how to compile .mod files, and what commands are used to manage simulations.*

Physical system
---------------

You have a cell with an excitable dendritic tree. You can inject a steady depolarizing current into the soma and observe membrane potential in the dendrite. Your goal is to find the relationship between the amplitude of the current applied at the soma, and the spike frequency at the distal end of the dendritic tree.

Computational implementation
----------------------------

The model cell
~~~~~~~~~~~~~~

The model cell is a ball and stick with these properties:

**soma**

    .. list-table:: 

        * - L
          - 10 μm
        * - diam 
          - 3.1831 μm  (area 100 μm\ :sup:`2`)
        * - cm
          - 1 μF/cm\ :sup:`2`
        * - Ra
          - 100 ohm * cm
        * - nseg
          - 1
        * - kinetics
          - full Hodgkin-Huxley

**dend**

    .. list-table:: 

        * - L
          - 1000 μm
        * - diam 
          - 2 μm
        * - cm
          - 1 μF/cm\ :sup:`2`
        * - Ra
          - 100 ohm * cm
        * - nseg
          - 25
        * - kinetics
          - reduced Hodgkin-Huxley (all conductances ``/= 2``)

The implementation of this model is in :download:`cell.py <code/cell.py>`.

Code development strategy
~~~~~~~~~~~~~~~~~~~~~~~~~

Before trying to set up a program that runs multiple simulations, it is useful to have a program that executes a single simulation. This is helpful for exploring the properties of the model and collecting information needed to guide the development of a batch simulation program.

The next step is to create a program that performs serial execution of multiple simulations, i.e. executes them one after another. In addition to generating simulation results, it is useful for this program to report a measure of computational performance. For this example the measure will be the total time required to run all simulations and save results. The simulation results will be needed to verify that the parallel implementation is working properly. The performance measure will help us gauge the success of our efforts, and indicate whether we should look for additional ways to shorten run times.

The final step is to create a parallel implementation of the batch program. This should be tested by comparing its simulation results and performance against those of the serial batch program in order to detect errors or performance deficiencies that should be corrected.

In accordance with this development strategy, we provide the following three programs. For each program there is a brief description, plus one or more examples of usage. There are also links to each program's source code and code walkthroughs, which may be helpful in completing one of this exercise's assignments.

Finally, there is a fourth program for plotting results that have been saved to a file, but more about that later.

initonerun.py
~~~~~~~~~~~~~

Description
###########

Executes one simulation with a specified stimulus.

Displays response and reports spike frequency.

Usage
#####

.. code-block::

    python -i initonerun.py

A new simulation can be launched by entering the command

.. code-block::
    python

    onerun(x)

at the ``>>>`` prompt, where x is a number that specifies the stimulus current amplitude in nA.

Example:

.. code-block::
    python

    onerun(0.3)

Source
######

- :download:`initonerun.py <code/initonerun.py>`
- :ref:`code walkthrough <initonerun_walkthrough>`


initbatser.py
~~~~~~~~~~~~~

Description
###########

Executes a batch of simulations, one at a time, in which stimulus amplitude increases from run to run.

Then saves results, reports performance, and optionally plots an f-i graph.

Usage
#####

Either

.. code-block::

    python initbatser.py

or (to see the graph):

.. code-block::

    python -i initbatser.py


Source
######

- :download:`initbatser.py <code/initbatser.py>`
- :ref:`code walkthrough <initbatser_walkthrough>`

initbatpar.py
~~~~~~~~~~~~~

Description
###########

Performs the same task as :file:`initbatser.py`, i.e. executes a batch of simulations, but does it serially or in parallel, depending on how the program is launched.

Parallel execution uses NEURON's bulletin board.

Usage
#####

Serial execution: ``python initbatpar.py``
runs simulations one after another on a single processor, i.e. serially. Parallel execution: ``mpiexec -n N python initbatpar.py``
launches N processes that carry out the simulations. On a multicore PC or Mac, parallel execution with N equal to the number of cores can reduce total run time to about 1/N of the run time required by initbatser.py, serial execution of initbatpar.py, or parallel execution of initbatpar.py with N = 1.


Source
######

- :download:`initbatpar.py <code/initbatpar.py>`
- :ref:`code walkthrough <initbatpar_walkthrough>`

Things to do
------------

1. Run a few simulations with initonerun.py just to see how the cell responds to injected current. You might try to find the smallest and largest stimuli that elicit repetitive dendritic spiking.

2. 

    Compare results produced by serial and parallel simulations, to verify that parallelization hasn't broken anything. For example (on Linux and macOS):

    .. code-block::
        none

        python initbatser.py
        mv fi.dat fiser.dat
        python initbatpar.py
        mv fi.dat finompi.dat
        mpiexec -n 4 python initbatpar.py
        mv fi.dat fimpi4.dat
        cmp fiser.dat finompi.dat
        cmp fiser.dat fimpi4.dat
    
    Instead of ``cmp``, MSWin users will have to use ``fc`` in a "Command Prompt" (cmd) or Power Shell window.

3. 

    Evaluate and compare performance of the serial and parallel programs.
    Here are results of some tests I ran:

    .. code-block::
        none

        NEURON 7.5 (266b5a0) 2017-05-22 under Windows Subsystem for Linux
        on a quad core desktop.
        initbatser     6.16 s
        initbatpar
        without MPI  6.03
        with MPI            speedup
        n = 1        6.03   1 (performance baseline)
            2        3.41   1.77
            3        2.62   2.30
            4        2.05   2.94

4.
    Make a copy of initbatpar.py and edit it, inserting print calls that reveal the sequence of execution, i.e. which processor is doing what. These statements should report whatever you think would help you understand program flow. Here are some suggestions for things you might want to report:

    - the identity of the host process (i.e. the ``pc.id``)
    - the name of the proc or func that is being entered or exited
    - the value that is being passed or returned
    - the index of the simulation that is being run
    - anything else that you think might illuminate this dark little corner of computational neuroscience

    Focus on the parts of the program that are involved in the master's principal function (posting jobs and gathering results), and the workers' principal function (entering, executing, and exiting ``fi``).

    After inserting the print calls, change NRUNS to 3 or 4, then run a serial simulation and see what happens.

    Next run parallel simulations with -n 1, 2, 3 or 4 and see what happens. Do the monitor reports make sense?

5. 
    Examine an f-i curve from data saved to one of the dat files.

    .. code-block::
        bash
    
        python -i initplotfi.py

    then use its file browser to select one of the dat files.

    Examine :download:`initplotfi.py <code/initplotfi.py>` to see how it takes advantage of procs that are built into NEURON's standard run library :file:`stdlib.hoc` (view the source code for this file on the NEURON GitHub repository at `https://github.com/neuronsimulator/nrn/blob/master/share/lib/hoc/stdlib.hoc <https://github.com/neuronsimulator/nrn/blob/master/share/lib/hoc/stdlib.hoc>`_).
    

.. toctree::
    :hidden:

    bulletin_board_walkthrough.rst