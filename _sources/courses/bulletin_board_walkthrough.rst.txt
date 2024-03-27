.. _bulletin_board_walkthrough:

Bulletin board code walkthroughs
================================

.. _initonerun_walkthrough:

initonerun.py
-------------

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

:download:`initonerun.py <code/initonerun.py>`

Code walkthrough
################

:download:`initonerun.py <code/initonerun.py>` is organized in a modular fashion. Only highlights are mentioned.

Simulation parameters
>>>>>>>>>>>>>>>>>>>>>

Firing frequency should be determined after the model has settled into a stable firing pattern. Tests show that the first few interspike intervals vary slightly, so the first NSETTLE=5 ISIs are ignored and frequency is computed from the last 10 ISIs in a simulation. The slowest sustained repetitive firing is > 40 Hz (longest ISI < 25 ms), so TSTOP = 375 ms would allow at least 15 ISIs. TSTOP has been set to 500 ms so that repetitive firing produces > 15 ISIs, and runs with < 15 are ignored.

Model specification
>>>>>>>>>>>>>>>>>>>
loads the cell's source code

Instrumentation
>>>>>>>>>>>>>>>

stimulus--attaches an :hoc:class:`IClamp` to ``soma(0.5)``

data recording and analysis--uses a :hoc:class:`NetCon` to record the times at which spikes reach ``dend(1)``

get_frequency(spvec) verifies that enough spikes have occurred, then calculates freq from the last NINVL=10 recorded ISIs.

Simulation control and reporting of results
>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

``onerun()`` expects a single numerical argument that specifies the stimulus amplitude. It creates a graph that will show ``dend(1).v`` vs. time, runs a simulation, analyzes the results, and prints out the stimulus amplitude and firing frequency.

.. _initbatser_walkthrough:

initbatser.py
-------------

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

Code walkthrough
################
:download:`initbatser.py <code/initbatser.py>` is based on :file:`initonerun.py`. Only significant differences are mentioned.

Simulation parameters
>>>>>>>>>>>>>>>>>>>>>
If PLOTRESULTS is True, an f-i curve will be generated at the end of program execution; if not, the program simply exits when done.
AMP0, D_AMP, and NRUNS specify the stimulus current in the first run, the increment from one run to the next, and the number of simulations that are executed, respectively.

Instrumentation
>>>>>>>>>>>>>>>
``setparams(run_id)`` assigns values to the parameters that differ from run to run. In this example, it sets stimulus amplitude to a value that depends on its argument. Its argument is the "run index", a whole number that ranges from 0 to NRUNS-1 (see batchrun(n) in the following discussion of "Simulation control").

Simulation control
>>>>>>>>>>>>>>>>>>
This has been separated from reporting of results.

``trun = time.time()`` records system time at the beginning of the code whose run time will be evaluated.

``batchrun()`` contains a ``for`` loop that iterates the run counter ``run_id`` from 0 to NRUNS - 1. Each pass through this loop results in a new simulation with a new stimulus amplitude, finds the spike frequency, and saves the stimulus amplitude and frequency to a pair of lists. It also prints a message to the terminal to indicate progress.

Reporting of results
>>>>>>>>>>>>>>>>>>>>
``saveresults()`` writes the stimulus and frequency vectors to a text file in the format used by :menuselection:`NEURON Main Menu --> Vector --> Save to File` and :menuselection:`Retrieve from File`.

After this is done, the program reports run time.

Then it plots an f-i curve or quits, depending on PLOTRESULTS.


.. _initbatpar_walkthrough:

initbatpar.py
-------------

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

Code walkthrough
################
:download:`initbatpar.py <code/initbatpar.py>` is based on :file:`initbatser.py`. Only key differences are mentioned below.

ParallelContext
>>>>>>>>>>>>>>>

An instance of the :class:`ParallelContext` class is created near the start of the program. Print statements inserted after this point to monitor program execution can report not only what code is being executed in the course of which simulation, but also the identity (``pc.id``) of the host that is executing the code.

Simulation control
>>>>>>>>>>>>>>>>>>

This is where most of the changes have been made.

The speedup of bulletin board style parallelization depends on keeping the workers as busy as possible, while minimizing communication (data exchange via the bulletin board) as much as possible. To this end, the master should post as little data as necessary to the bulletin board. The workers should do as much work as possible, and then return as little data as necessary to the bulletin board.

The serial program :file:`initbatser.py` has a batchrun(n) that uses this for loop to execute a series of simulations, one at a time, on a single processor:

.. code-block:: 
    python

    for run_id in range(n):
        set_params(run_id)
        h.run()
        stims.append(stim.amp)
        freqs.append(get_frequency())
        print('Finished %d of %d.' % (run_id + 1, n))


In :file:`initbatpar.py`, everything that can be offloaded to the workers has been taken out of batchrun() and inserted into a new function ``fi(run_id)`` that is defined prior to batchrun.

.. code-block::
    python

    def fi(run_id):
        """set params, execute a simulation, analyze and return results"""
        set_params(run_id)
        h.run()
        return (run_id, stim.amp, get_frequency(spvec))

Notice that ``fi`` contains the procedures that involve the most computational overhead. Also notice that ``fi`` expects a single numerical argument -- the run_id -- and returns a tuple with the run_id, the value of the stimulus, and the frequency obtained from the simulation. An alternative implementation could have reduced communication by returning only the frequency, unpacking the job index (equal to the run_id), and recomputing the stimulus amplitude. It is important to balance convenience, the aim of keeping the workers busy, and minimizing communication overhead.

Here is :file:`initbatpar.py`'s batchrun procedure:

.. code-block::
    python

    def batchrun(n):
        # preallocate lists of length n for storing parameters and results
        stims = [None] * n
        freqs = [None] * n
        for run_id in range(n):
            pc.submit(fi, run_id)
        count = 0
        while pc.working():
            run_id, amp, freq = pc.pyret()
            stims[run_id] = amp
            freqs[run_id] = freq
            count += 1
            print('Finished %d of %d.' % (count, n))
        return stims, freqs

There still is a ``for`` loop, but it uses :meth:`pc.submit() <ParallelContext.submit>` to post jobs to the bulletin board. Communication is minimized by passing only the function handle (``fi``) and the simulation index ``run_id`` for each run that is to be carried out.

Next comes a ``while`` loop in which the master checks the bulletin board for returned results. If nothing is found, the master picks a task from the bulletin board and executes it. If a result is present, the master retrieves it from the bulletin board: :meth:`pc.pyret() <ParallelContext.pyret>` gets the value returned by ``fi``, which is unpacked into its three components.

``run_id`` is used to place the results in the appropriate locations in the stims and freqs lists, as there is no guarantee that simulation results will be returned in any specific sequence.

After the last job has been completed, the master exits the while loop, and batchrun is finished. Then :meth:`pc.done() <ParallelContext.done>` releases the workers.

But the master still has to save the results.


