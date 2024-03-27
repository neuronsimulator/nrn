.. _mpi_parallelization:

Parallel Computing with MPI
==========

Getting Started 
---------

The first step is to get to the place where you can run the "hello world" level program :download:`test0.hoc <code/test0.hoc.txt>` by launching in a terminal window with

.. code::
    c++

    mpiexec -n 4 nrniv -mpi test0.hoc

and see the output

.. code::
    c++

    ...
    I am 1 of 4
    I am 3 of 4
    I am 0 of 4
    I am 2 of 4

MSWIN
++++++

The NEURON setup installer has already pu all the required sofware on your machine to use MPI.

1.
    Start a bash terminal window.

2.
    Launch the program (mpiexec command above) in the directory containing ``test0.hoc`` (or give a full pathto ``test0.hoc``).

Mac OS X and Linux
+++++++++++

Unfortunately MPI can't be a part of the binary installation because I don't know if, which, or where MPI was installed on your machine. So you have to install MPI yourself, check that it works, and build NEURON from the sources with the configure option '--with-paranrn'. See the "installing and testing MPI" section of the Hines and Carnevale (2008) paper, "Translating network models to parallel hardware in NEURON", J. Neurosci. Meth. 169: 425-465. The paper is reprinted in your handout booklet. Or see `the ModelDB entry <https://senselab.med.yale.edu/ModelDB/ShowModel?model=96444#tabs-1>`_

Going Further 
----------

The ring model from the `above ModelDB entry <https://senselab.med.yale.edu/ModelDB/ShowModel?model=96444#tabs-1>`_ is a good next step. See also the documentation for the `ParallelContext <https://nrn.readthedocs.io/en/latest/hoc/modelspec/programmatic/network/parcon.html?highlight=parallelcontext>`_ class, especialy the subset of methods gathered under the `ParallelNetwork <https://nrn.readthedocs.io/en/latest/hoc/modelspec/programmatic/network/parcon.html?highlight=parallelcontext>`_ heading. A large portion of the `ParallelNetManager <https://nrn.readthedocs.io/en/latest/hoc/modelspec/programmatic/network/parnet.html?highlight=parallelnetmanager>`_ wrapper is better off done directly from the underlying ParallelContext though it can be mined for interesting pieces. A good place to find the most recent idioms is the NEURON implementation of the Vogels and Abbott model found in the `Brette et al. ModelDB entry <https://senselab.med.yale.edu/ModelDB/ShowModel?model=83319#tabs-1>`_. However, to run in parallel, the NetCon delay between cells needs to be set greater than zero.


