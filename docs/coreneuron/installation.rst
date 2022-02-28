.. _getting-coreneuron:

Getting CoreNEURON
##################
CoreNEURON is integrated into the NEURON repository, and it is straightforward to enable it when compiling from source using CMake as described in :ref:`Installing Source Distributions`.

Starting with version 8.1, NEURON also provides Python wheels that include CoreNEURON and, optionally, GPU support. These binary distributions are described in :ref:`Installing Binary Distribution`.

.. warning::
   These wheels are not yet released, and must currently be installed using ``pip install neuron-nightly`` and/or ``pip install neuron-gpu-nightly``.

Installing with ``pip``
***********************
This should be as simple as ``pip install neuron-nightly``.
You may want to use ``virtualenv`` to manage your Python package installations.

If you want to use the GPU-enabled wheel then you should run ``pip install neuron-gpu-nightly``.
This binary wheel does not include all the NVIDIA dependencies that are required to build and execute GPU code, so you should install the `NVIDIA HPC SDK <https://developer.nvidia.com/hpc-sdk>`_ on your machine.

.. warning::
   It is safest to use the same version of the HPC SDK as was used to build the binary wheels.
   This is currently defined `in this file <https://github.com/neuronsimulator/nrn/blob/master/packaging/python/Dockerfile_gpu>`_ in the NEURON repository.


Installing from source
**********************
To enable CoreNEURON when building NEURON, simply pass the ``-DNRN_ENABLE_CORENEURON=ON`` option to CMake.

By default, CoreNEURON will use the `mod2c <https://github.com/BlueBrain/mod2c>`_ source-to-source compiler to translate MOD files into C++ code.
To use the more modern `NMODL <https://github.com/BlueBrain/nmodl>`_ source-to-source compiler, you should additionally pass ``-DCORENRN_ENABLE_NMODL=ON`` to CMake.

Most of CoreNEURON's build dependencies (Bison, Flex, CMake, Python, MPI (optional), ...) are already dependencies of NEURON, but you may want to use a specialised compiler to get optimal performance.
To enable GPU support the `NVIDIA HPC SDK <https://developer.nvidia.com/hpc-sdk>`_. is also required.

Compiler Selection
==================
CoreNEURON relies on compiler `auto-vectorisation <https://en.wikipedia.org/wiki/Automatic_vectorization>`_ to achieve better performance on modern CPUs.
With this release we recommend compilers from **Intel**, **Cray** and, for GPU support, **NVIDIA** (formerly **PGI**).
These compilers are able to vectorise the code better than **GCC** or **Clang**, achieving the best possible performance gains.

Computer clusters will typically provide the Intel and/or Cray compilers as modules.
You can also install the Intel compiler by downloading the `oneAPI HPC Toolkit <https://software.intel.com/content/www/us/en/develop/tools/oneapi/hpc-toolkit.html>`_.

CoreNEURON supports also GPU execution based on an `OpenACC <https://en.wikipedia.org/wiki/OpenACC>`_ backend.
Currently, the best supported compiler for the OpenACC backend is NVIDIA's ``nvc++``, which is available in the `NVIDIA HPC SDK <https://developer.nvidia.com/hpc-sdk>`_.

.. note::
   Experimental GPU support using OpenMP target offload is also available.
   This is the default if the NMODL source-to-source compiler is used, to use OpenACC with NMODL then you should pass ``-DCORENRN_ENABLE_OPENMP_OFFLOAD=OFF`` to CMake.

.. warning::
   Support for non-NVIDIA GPUs is not currently tested.

Step-by-step instructions
=========================
As stated above, the simplest way to enable CoreNEURON is to build it as a git submodule of the NEURON repository.
This section outlines how to do that on a typical system, using the ``master`` version of NEURON.

First, we clone the latest version of NEURON:

.. code-block::

   git clone https://github.com/neuronsimulator/nrn
   cd nrn

Then we create a build directory:

.. code-block::

   mkdir build
   cd build

In order to configure NEURON, we need to load the required software dependencies.
If compilers and necessary dependencies are already available in the default paths then you do not need to do anything.
In a cluster or HPC environment a module system is often used to select software.
For example, you might be able to load the compiler, cmake, and python dependencies using ``module``:

.. code-block::

   module load intel openmpi python cmake

And to enable GPU support then you might additionally load NVIDIA HPC SDK and CUDA modules:

.. code-block::

   module load nvidia-hpc-sdk cuda

.. warning::
   Module names are not standardised across different systems, and you will almost certainly need to adjust these names for your system
   ``module avail`` and ``module spider`` may help you to discover the module names that you need to use.


.. warning::
   The NVIDIA HPC SDK bundles up to four different versions of CUDA.
   You should make sure that the version of CUDA in your environment is compatible with the CUDA driver on your system, otherwise you may encounter errors like
   ``nvlink fatal: Input file '[snip]' newer than toolkit (112 vs 110) (target: sm_60)``

Once the required dependencies are loaded, you are ready to run CMake. See :ref:`Install NEURON using CMake` for more information.
To enable CoreNEURON, don't forget to add the ``-DNRN_ENABLE_CORENEURON=ON`` option.

.. code-block::

   cmake .. \
     -DNRN_ENABLE_CORENEURON=ON \
     -DNRN_ENABLE_INTERVIEWS=OFF \
     -DNRN_ENABLE_RX3D=OFF \
     -DCMAKE_INSTALL_PREFIX=$HOME/install \
     -DCMAKE_C_COMPILER=icc \
     -DCMAKE_CXX_COMPILER=icpc

Make sure to replace ``icc`` and ``icpc`` with the C/C++ compiler that you are using.
Also change `$HOME/install` to desired installation directory.
CMake tries to find MPI libraries automatically but if needed you can set MPI compiler options ``-DMPI_C_COMPILER=<mpi C compiler>`` and ``-DMPI_CXX_COMPILER=<mpi CXX compiler>``.

If you would like to enable GPU support with OpenACC, make sure to use ``-DCORENRN_ENABLE_GPU=ON`` option and to use the PGI/NVIDIA HPC SDK compilers with CUDA.
For example,

.. code-block::

   cmake .. \
     -DNRN_ENABLE_CORENEURON=ON \
     -DCORENRN_ENABLE_GPU=ON \
     -DNRN_ENABLE_INTERVIEWS=OFF \
     -DNRN_ENABLE_RX3D=OFF \
     -DCMAKE_INSTALL_PREFIX=$HOME/install \
     -DCMAKE_C_COMPILER=nvc \
     -DCMAKE_CUDA_COMPILER=nvcc \
     -DCMAKE_CXX_COMPILER=nvc++

.. note::
  ``nvcc`` is provided both by the NVIDIA HPC SDK and by CUDA toolkit
   installations, which can lead to fragile and surprising behaviour.
   See, for example, `this issue <https://forums.developer.nvidia.com/t/nvcc-only-partially-respects-cuda-home-input-file-newer-than-toolkit/182599>`_.
   On some systems it is necessary to load the ``nvhpc`` module before
   the ``cuda`` module, thereby ensuring that ``nvcc`` comes from a
   CUDA toolkit installation, but your mileage may vary.

By default the GPU code will be compiled for NVIDIA devices with compute capability 7.0 or 8.0.
This can be steered by passing, for example, ``-DCMAKE_CUDA_ARCHITECTURES:STRING=60;70;80`` to CMake.

You can change C/C++ optimisation flags using the  ``-DCMAKE_C_FLAGS``, ``-DCMAKE_CUDA_FLAGS`` and ``-DCMAKE_CXX_FLAGS`` options.
To make sure your custom flags are not modified, you should also set ``-DCMAKE_BUILD_TYPE=Custom``, for example:

.. code-block::

   -DCMAKE_C_FLAGS="-O3 -g" \
   -DCMAKE_CUDA_FLAGS="-O3" \
   -DCMAKE_CXX_FLAGS="-O3 -g" \
   -DCMAKE_BUILD_TYPE=Custom \

.. warning::
   If the CMake command fails, make sure to delete temporary CMake cache files (``CMakeCache.txt`` and ``CMakeFiles``, or the entire build directory) before re-running CMake.

Once the configure step is done, you can build and install the project by running

.. code-block::

   cmake --build . --parallel
   cmake --build . --target install

To use your new installation, you need to modify the ``PATH`` and ``PYTHONPATH`` environment varaibles:

.. code-block::

   export PATH=$HOME/install/bin:$PATH
   export PYTHONPATH=$HOME/install/lib/python:$PYTHONPATH

Now you should be able to import neuron module as

.. code-block::

   python -c "from neuron import h; from neuron import coreneuron"

If you get ``ImportError`` then make sure ``PYTHONPATH`` is set correctly, and that ``python`` is the same version that CMake was configured to use.
You can use ``-DPYTHON_EXECUTABLE=/path/to/python`` to force CMake to use a particular version.
