Testing GPU functionality
#########################
This section provides information and links that help with testing :ref:`CoreNEURON`'s GPU support.
Other sections of the documentation that may be relevant are:

- The :ref:`getting-coreneuron` section, which documents both building from source with CoreNEURON support and installing Python wheels.
- The :ref:`coreneuron-running-a-simulation` section, which explains the basics of porting a NEURON model to use CoreNEURON.
- The :ref:`Running GPU benchmarks` section, which outlines how to use profiling tools such as Caliper, NVIDIA NSight Systems, and NVIDIA NSight Compute.

This section aims to add some basic information about how to test if GPU execution is working.
This might be useful if, for example, you need to test GPU execution on a new system.

Accessing GPU resources
***********************
If your local system has an (NVIDIA) GPU installed then you can probably skip this section.
The ``nvidia-smi`` tool may be useful to check this; it will show the GPUs attached to a system:

.. code-block:: console

  $ nvidia-smi
  +-----------------------------------------------------------------------------+
  | NVIDIA-SMI 460.91.03    Driver Version: 460.91.03    CUDA Version: 11.2     |
  |-------------------------------+----------------------+----------------------+
  | GPU  Name        Persistence-M| Bus-Id        Disp.A | Volatile Uncorr. ECC |
  | Fan  Temp  Perf  Pwr:Usage/Cap|         Memory-Usage | GPU-Util  Compute M. |
  |                               |                      |               MIG M. |
  |===============================+======================+======================|
  |   0  Quadro P2200        Off  | 00000000:01:00.0 Off |                  N/A |
  | 45%   33C    P8     4W /  75W |     71MiB /  5049MiB |      2%      Default |
  |                               |                      |                  N/A |
  +-------------------------------+----------------------+----------------------+

On a university cluster or supercomputer system then you will typically need to pass some kind of extra constraint to the job scheduler.
For example on the BlueBrain5 system, which uses Slurm, you can allocate a GPU node using the ``volta`` constraint:

.. code-block:: console

  [login node] $ salloc -A <account> -C volta
  salloc: Granted job allocation 294001
  ...
  [compute node] $ nvidia-smi
  +-----------------------------------------------------------------------------+
  | NVIDIA-SMI 470.57.02    Driver Version: 470.57.02    CUDA Version: 11.4     |
  |-------------------------------+----------------------+----------------------+
  | GPU  Name        Persistence-M| Bus-Id        Disp.A | Volatile Uncorr. ECC |
  | Fan  Temp  Perf  Pwr:Usage/Cap|         Memory-Usage | GPU-Util  Compute M. |
  |                               |                      |               MIG M. |
  |===============================+======================+======================|
  |   0  Tesla V100-SXM2...  Off  | 00000000:1A:00.0 Off |                  Off |
  ...

Running NEURON tests
********************
If you have configured NEURON with CoreNEURON, CoreNEURON GPU support and tests (:ref:`-DNRN_ENABLE_TESTS=ON <cmake-nrn-enable-tests-option>`) enabled then simply running

.. code-block:: console

  $ ctest --output-on-failure

in your CMake build directory will execute a large number of tests, many of them including GPU execution.
You can filter which tests are run by name using the ``-R`` option to CTest, for example:

.. code-block:: console

  $ ctest --output-on-failure -R gpu
  Test project /path/to/your/build
  Start  42: coreneuron_modtests::direct_py_gpu
   1/53 Test  #42: coreneuron_modtests::direct_py_gpu .............................   Passed    1.98 sec
        Start  43: coreneuron_modtests::direct_hoc_gpu
   2/53 Test  #43: coreneuron_modtests::direct_hoc_gpu ............................   Passed    1.03 sec
        Start  44: coreneuron_modtests::spikes_py_gpu
   ...

Running tests manually
**********************
It is sometimes convenient to run basic tests outside the CTest
infrastructure.
A particularly useful test case is the ``ringtest`` that is included in
the CoreNEURON repository.
This is very convenient because binary input data files for CoreNEURON
are committed to the repository -- meaning that the test can be run
without NEURON, Python, HOC, and friends -- and the required mechanisms
are compiled as part of the standard NEURON build.
To run this test on CPU you can, from your build directory, run:

.. code-block:: console

  $ ./bin/x86_64/special-core -d ../external/coreneuron/tests/integration/ring
  ...

where it is assumed that ``..`` is the source directory.
To enable GPU execution, add the ``--gpu`` option:

.. code-block:: console

  $ ./bin/x86_64/special-core -d ../external/coreneuron/tests/integration/ring --gpu
  Info : 4 GPUs shared by 1 ranks per node
  ...

You should see that the statistics printed at the end of the simulation
are the same.
It can also be useful to enable some basic profiling, for example by using
NVIDIA's NSight Systems utility ``nsys``:

.. code-block:: console

  $ nsys nvprof ./bin/x86_64/special-core -d ../external/coreneuron/tests/integration/ring --gpu
  WARNING: special-core and any of its children processes will be profiled.

  Collecting data...
  Info : 4 GPUs shared by 1 ranks per node
  ...
  Number of spikes: 37
  Number of spikes with non negative gid-s: 37
  Processing events...
  ...
  CUDA API Statistics:

  Time(%)  Total Time (ns)  Num Calls  Average (ns)   Minimum (ns)  Maximum (ns)  StdDev (ns)             Name
  -------  ---------------  ---------  -------------  ------------  ------------  -----------  --------------------------
     42.7    2,127,723,623    136,038       15,640.7         3,630    10,224,640     59,860.5  cuLaunchKernel
  ...

  CUDA Kernel Statistics:

  Time(%)  Total Time (ns)  Instances  Average (ns)  Minimum (ns)  Maximum (ns)  StdDev (ns)                                                  Name
  -------  ---------------  ---------  ------------  ------------  ------------  -----------  ----------------------------------------------------------------------------------------------------
     32.3      346,133,763      8,000      43,266.7        42,175        50,080      1,435.3  nvkernel__ZN10coreneuron18solve_interleaved1Ei_F1L653_4
     12.7      136,155,806      8,002      17,015.2         3,615     1,099,738     90,544.0  nvkernel__ZN10coreneuron14nrn_cur_ExpSynEPNS_9NrnThreadEPNS_9Memb_listEi_F1L375_7
     10.4      111,258,439      8,002      13,903.8         3,199     1,314,489     73,556.3  nvkernel__ZN10coreneuron11nrn_cur_pasEPNS_9NrnThreadEPNS_9Memb_listEi_F1L274_4
     10.1      108,647,844      8,000      13,581.0         3,391     1,274,394     70,309.4  nvkernel__ZN10coreneuron16nrn_state_ExpSynEPNS_9NrnThreadEPNS_9Memb_listEi_F1L418_10
  ...

This can be helpful to confirm that compute kernels are really being
launched on the GPU.
Substrings such as ``solve_interleaved1``, ``solve_interleaved2``,
``nrn_cur_`` and ``nrn_state_`` in these kernel names indicate that the
computationally heavy parts of the simulation are indeed being executed
on the GPU.
This test dataset is extremely small, so you should not pay much
attention to the simulation time in this case.

.. note::
   The kernel names, which start with ``nvkernel__ZN10coreneuron``
   above, are implementation details of the OpenACC or OpenMP
   implementation being used.
   They can also depend on whether you use MOD2C or NMODL to translate
   MOD files.
   If you want to do any more sophisticated profiling then you should
   use a profiling tool such as Caliper that can access the
   well-defined human-readable names for these kernels that NEURON and
   CoreNEURON define.
