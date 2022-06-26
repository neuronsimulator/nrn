Examples
########
Here are some test examples to illustrate the usage of CoreNEURON API with NEURON:

`test_direct.py <https://github.com/neuronsimulator/nrn/blob/master/test/coreneuron/test_direct.py>`_:
   This is a simple, single cell, serial Python example demonstrating the use of CoreNEURON.
   We first run a simulation with NEURON and record the voltage and membrane currents.
   Then, the same model is executed with CoreNEURON, and we make sure the same results are achieved.
   Note that in order to run this example you must build `these mod files <https://github.com/neuronsimulator/nrn/tree/master/test/coreneuron/mod>`_  with ``nrnivmodl -coreneuron``.
   You can run this example as:

   .. code-block:: shell

    nrnivmodl -coreneuron mod                # first compile mod files
    nrniv -python test_direct.py             # run via nrniv
    x86_64/special -python test_direct.py    # run via special
    python test_direct.py                    # run via python

`test_direct.hoc <https://github.com/neuronsimulator/nrn/blob/master/test/coreneuron/test_direct.hoc>`_:
  This is the same example as above (``test_direct.py``) but written using HOC.

`test_spikes.py <https://github.com/neuronsimulator/nrn/blob/master/test/coreneuron/test_spikes.py>`_:
  This is similar to ``test_direct.py`` above, but it can be run with MPI where each MPI process creates a single cell and connects it with a cell on another rank.
  Each rank records spikes and compares them between NEURON execution and CoreNEURON execution.
  It also demonstrates usage of the `mpi4py <https://github.com/mpi4py/mpi4py>`_ Python module and NEURON's native MPI API.

  You can run this MPI example in different ways:

  .. code-block:: shell

    mpiexec -n <num_process> python test_spikes.py mpi4py               # using mpi4py
    mpiexec -n <num_process> x86_64/special -mpi -python test_spikes.py # neuron internal MPI

`Ring network test <https://github.com/neuronsimulator/ringtest>`_:
  This is a ring network model of Ball-and-Stick neurons which can be scaled arbitrarily for testing and benchmarking purpose.
  You can use this as a reference for porting your model, see the `README <https://github.com/neuronsimulator/ringtest/blob/master/README.md>`_ file for detailed instructions.

`3D Olfactory Bulb Model <https://github.com/HumanBrainProject/olfactory-bulb-3d>`_:
  `Migliore et al. (2014) <https://www.frontiersin.org/articles/10.3389/fncom.2014.00050>`_ model of the olfactory bulb ported with CoreNEURON on GPU.
  See the `README <https://github.com/HumanBrainProject/olfactory-bulb-3d/blob/master/README.md>`_ file for detailed instructions.
