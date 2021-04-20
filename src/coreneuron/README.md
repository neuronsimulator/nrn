![CoreNEURON CI](https://github.com/BlueBrain/CoreNeuron/workflows/CoreNEURON%20CI/badge.svg) [![codecov](https://codecov.io/gh/BlueBrain/CoreNeuron/branch/master/graph/badge.svg?token=mguTdBx93p)](https://codecov.io/gh/BlueBrain/CoreNeuron)

# CoreNEURON
> Optimised simulator engine for [NEURON](https://github.com/neuronsimulator/nrn)

CoreNEURON is a compute engine for the [NEURON](https://www.neuron.yale.edu/neuron/) simulator optimised for both memory usage and computational speed. Its goal is to simulate large cell networks with small memory footprint and optimal performance.

## NEURON Models Compatibility

CoreNEURON is designed as a library within the NEURON simulator and can transparently handle all spiking network simulations including gap junction coupling with the **fixed time step method**. In order to run a NEURON model with CoreNEURON:

* MOD files should be THREADSAFE
* If random number generator is used then Random123 should be used instead of MCellRan4
* POINTER variables need to be converted to BBCOREPOINTER ([details here](docs/userdoc/MemoryManagement/bbcorepointer.md))

## Dependencies
* [CMake 3.7+](https://cmake.org)
* MPI Library [Optional, for MPI support]
* [PGI OpenACC Compiler / NVIDIA HPC SDK](https://developer.nvidia.com/hpc-sdk) [Optional, for GPU support]
* [CUDA Toolkit >=9.0](https://developer.nvidia.com/cuda-downloads) [Optional, for GPU support]

In addition to this, you will need other [NEURON dependencies](https://github.com/neuronsimulator/nrn) such as Python, Flex, Bison etc.

## Installation

**CoreNEURON** is now **integrated into** the **development version** of the **NEURON** simulator. If you are a NEURON user, **the preferred way** to **install CoreNEURON** is to **enable extra build options** during **NEURON installation** as follows:

1. Clone the latest version of NEURON:

  ```
  git clone https://github.com/neuronsimulator/nrn
  cd nrn
  ```

2. Create a build directory:

  ```
  mkdir build
  cd build
  ```

3. Load software dependencies

	Currently CoreNEURON relies on compiler auto-vectorisation and hence we advise to use one of Intel, Cray, or PGI compilers to ensure vectorized code is generated. These compilers are able to vectorize the code better than **GCC** or **Clang**, achieving the best possible performance gains. Note that Intel compiler can be installed by downloading [oneAPI HPC Toolkit](https://software.intel.com/content/www/us/en/develop/tools/oneapi/hpc-toolkit.html). CoreNEURON supports GPU execution using **OpenACC** programming model.  Currently the best supported compiler for the **OpenACC** backend is **PGI** (available via [NVIDIA-HPC-SDK](https://developer.nvidia.com/hpc-sdk)) and this is the recommended one for compilation.


	HPC systems often use a module system to select software. For example, you can load the compiler, cmake, and python dependencies using module as follows:

  ```
  module load intel intel-mpi python cmake
  ```

Note that if you are building on Cray system with the GNU toolchain, you have to set following environment variable:

  ```
  export CRAYPE_LINK_TYPE=dynamic
  ```

3. Run CMake with the appropriate [options](https://github.com/neuronsimulator/nrn/blob/master/docs/install/install_instructions.md#install-neuron-using-cmake) and additionally enable CoreNEURON with `-DNRN_ENABLE_CORENEURON=ON` option:

  ```
  cmake .. \
   -DNRN_ENABLE_CORENEURON=ON \
   -DNRN_ENABLE_INTERVIEWS=OFF \
   -DNRN_ENABLE_RX3D=OFF \
   -DCMAKE_INSTALL_PREFIX=$HOME/install
   -DCMAKE_C_COMPILER=icc \
   -DCMAKE_CXX_COMPILER=icpc
  ```

4. If you would like to enable GPU support with OpenACC, make sure to use `-DCORENRN_ENABLE_GPU=ON` option and use the PGI/NVIDIA HPC SDK compilers with CUDA. For example,

  ```
  cmake .. \
   -DNRN_ENABLE_CORENEURON=ON \
   -DCORENRN_ENABLE_GPU=ON
   -DNRN_ENABLE_INTERVIEWS=OFF \
   -DNRN_ENABLE_RX3D=OFF \
   -DCMAKE_INSTALL_PREFIX=$HOME/install
   -DCMAKE_C_COMPILER=nvc \
   -DCMAKE_CXX_COMPILER=nvc++
  ```

NOTE : If the CMake command fails, please make sure to delete temporary CMake cache files (`CMakeCache.txt`) before rerunning CMake.

4. Build and Install :  once the configure step is done, you can build and install the project as:

	```bash
	cmake --build . --parallel 8 --target install
	```
  Feel free to define the number of parallel jobs building by setting a number for the `--parallel` option.

## Building Model

Once NEURON is installed with CoreNEURON support, you need setup setup the `PATH` and `PYTHONPATH ` environment variables as:

```
export PYTHONPATH=$HOME/install/lib/python:$PYTHONPATH
export PATH=$HOME/install/bin:$PATH
```

As in a typical NEURON workflow, you can use `nrnivmodl` to translate MOD files:

```
nrnivmodl mod_directory
```

In order to enable CoreNEURON support, you must set the  `-coreneuron` flag. Make sure to necessary modules (compilers, CUDA, MPI etc) are loaded before using nrnivmodl:

```
nrnivmodl -coreneuron mod_directory
```

If you see any compilation error then one of the mod files might be incompatible with CoreNEURON. Please [open an issue](https://github.com/BlueBrain/CoreNeuron/issues) with an example and we can help to fix it.


## Running Simulations

With CoreNEURON, existing NEURON models can be run with minimal changes. For a given NEURON model, we typically need to adjust as follows:

1. Enable cache effficiency : `h.cvode.cache_efficient(1)`
2. Enable CoreNEURON :

	```
	from neuron import coreneuron
	coreneuron.enable = True
	```
3. If GPU support is enabled during build, enable GPU execution using :
	```
	coreneuron.gpu = True
  ```

4. Use `psolve` to run simulation after initialization :

	```
	h.stdinit()
	pc.psolve(h.tstop)
	```

Here is a simple example model that runs with NEURON first, followed by CoreNEURON and compares results between NEURON and CoreNEURON execution:


```python
import sys
from neuron import h, gui

# setup model
h('''create soma''')
h.soma.L=5.6419
h.soma.diam=5.6419
h.soma.insert("hh")
h.soma.nseg = 3
ic = h.IClamp(h.soma(.25))
ic.delay = .1
ic.dur = 0.1
ic.amp = 0.3

ic2 = h.IClamp(h.soma(.75))
ic2.delay = 5.5
ic2.dur = 1
ic2.amp = 0.3

h.tstop = 10

# make sure to enable cache efficiency
h.cvode.cache_efficient(1)

pc = h.ParallelContext()
pc.set_gid2node(pc.id()+1, pc.id())
myobj = h.NetCon(h.soma(0.5)._ref_v, None, sec=h.soma)
pc.cell(pc.id()+1, myobj)

# First run NEURON and record spikes
nrn_spike_t = h.Vector()
nrn_spike_gids = h.Vector()
pc.spike_record(-1, nrn_spike_t, nrn_spike_gids)
h.run()

# copy vector as numpy array
nrn_spike_t = nrn_spike_t.to_python()
nrn_spike_gids = nrn_spike_gids.to_python()

# now run CoreNEURON
from neuron import coreneuron
coreneuron.enable = True

# for GPU support
# coreneuron.gpu = True

coreneuron.verbose = 0
h.stdinit()
corenrn_all_spike_t = h.Vector()
corenrn_all_spike_gids = h.Vector()
pc.spike_record(-1, corenrn_all_spike_t, corenrn_all_spike_gids )
pc.psolve(h.tstop)

# copy vector as numpy array
corenrn_all_spike_t = corenrn_all_spike_t.to_python()
corenrn_all_spike_gids = corenrn_all_spike_gids.to_python()

# check spikes match between NEURON and CoreNEURON
assert(nrn_spike_t == corenrn_all_spike_t)
assert(nrn_spike_gids == corenrn_all_spike_gids)

h.quit()
```

We can run this model as:

```
python test.py
```

You can find [HOC example](https://github.com/neuronsimulator/nrn/blob/master/test/coreneuron/test_direct.hoc) here.

## FAQs

#### What results are returned by CoreNEURON?

At the end of the simulation CoreNEURON transfers by default : spikes, voltages, state variables, NetCon weights, all Vector.record, and most GUI trajectories to NEURON. These variables can be recorded using regular NEURON API (e.g. [Vector.record](https://www.neuron.yale.edu/neuron/static/py_doc/programming/math/vector.html#Vector.record) or [spike_record](https://www.neuron.yale.edu/neuron/static/new_doc/modelspec/programmatic/network/parcon.html#ParallelContext.spike_record)).

#### How can I pass additional flags to build?

One can specify C/C++ optimization flags specific to the compiler with `-DCMAKE_CXX_FLAGS` and `-DCMAKE_C_FLAGS` options to the CMake command. For example:

```bash
cmake .. -DCMAKE_CXX_FLAGS="-O3 -g" \
         -DCMAKE_C_FLAGS="-O3 -g" \
         -DCMAKE_BUILD_TYPE=CUSTOM
```

By default, OpenMP threading is enabled. You can disable it with `-DCORENRN_ENABLE_OPENMP=OFF`

For other errors, please [open an issue](https://github.com/BlueBrain/CoreNeuron/issues).

## Developer Build

#### Running Unit and Integration Tests

As **CoreNEURON** is mostly used as a compute library of **NEURON** it needs to be incorporated with **NEURON** to test most of its functionality. Consequently its tests are included in the NEURON repository. To enable and run all the tests of **CoreNEURON** you need to add the `-DNRN_ENABLE_TESTS=ON` CMake flag in **NEURON**.
Those tests include:
* Unit tests of NEURON
* Unit tests of CoreNEURON
* Integration tests comparing NEURON, CoreNEURON and reference files
* [ringtest](https://github.com/neuronsimulator/ringtest) tests with NEURON and CoreNEURON
* [testcorenrn](https://github.com/neuronsimulator/testcorenrn) tests with NEURON and CoreNEURON
Some of those tests are going to be also run with various backends in case that those are enabled (for example with GPUs).
To build NEURON with CoreNEURON and run the tests you need to:

```bash
cd nrn/build
cmake .. \
  -DNRN_ENABLE_CORENEURON=ON \
  -DNRN_ENABLE_INTERVIEWS=OFF \
  -DNRN_ENABLE_RX3D=OFF \
  -DCMAKE_INSTALL_PREFIX=$HOME/install
  -DCMAKE_C_COMPILER=icc \
  -DCMAKE_CXX_COMPILER=icpc \
  -DNRN_ENABLE_TESTS=ON
cmake --build . --parallel 8
ctest # use --parallel for speed, -R to run specific tests
```

#### Building standalone CoreNEURON without NEURON

If you want to build the standalone CoreNEURON version, first download the repository as:

```
git clone https://github.com/BlueBrain/CoreNeuron.git

```

Once the appropriate modules for compiler, MPI, CMake are loaded, you can build CoreNEURON with:

```bash
mkdir CoreNeuron/build && cd CoreNeuron/build
cmake .. -DCMAKE_INSTALL_PREFIX=$HOME/install
cmake --build . --parallel 8 --target install
```

If you don't have MPI, you can disable the MPI dependency using the CMake option `-DCORENRN_ENABLE_MPI=OFF`.

#### Compiling MOD files

In order to compile mod files, one can use **nrnivmodl-core** as:

```bash
/install-path/bin/nrnivmodl-core mod-dir
```

This will create a `special-core` executable under `<arch>` directory.

#### Building with GPU support

CoreNEURON has support for GPUs using the OpenACC programming model when enabled with `-DCORENRN_ENABLE_GPU=ON`. Below are the steps to compile with PGI compiler:

```bash
module purge all
module load nvidia-hpc-sdk/20.11 cuda/11 cmake openmpi # change pgi, cuda and mpi modules
cmake .. -DCORENRN_ENABLE_GPU=ON -DCMAKE_INSTALL_PREFIX=$HOME/install -DCMAKE_C_COMPILER=nvc -DCMAKE_CXX_COMPILER=nvc++
cmake --build . --parallel 8 --target install
```

You have to run GPU executable with the `--gpu` flag. Make sure to enable cell re-ordering mechanism to improve GPU performance using `--cell_permute` option (permutation types : 2 or 1):

```bash
mpirun -n 1 ./bin/nrniv-core --mpi --gpu --tstop 100 --datpath ../tests/integration/ring --cell-permute 2
```

Note: If your model is using Random123 random number generator, you cannot use the same executable for CPU and GPU runs. We suggest to install separate NEURON with CoreNEURON for CPU and GPU simulations. This will be fixed in future releases.


#### Running tests with SLURM

If you have a different mpi launcher (than `mpirun`), you can specify it during cmake configuration as:

```bash
cmake .. -DTEST_MPI_EXEC_BIN="mpirun" \
         -DTEST_EXEC_PREFIX="mpirun;-n;2" \
         -DTEST_EXEC_PREFIX="mpirun;-n;2" \
         -DAUTO_TEST_WITH_SLURM=OFF \
         -DAUTO_TEST_WITH_MPIEXEC=OFF \
```
You can disable tests using with options:

```
cmake .. -CORENRN_ENABLE_UNIT_TESTS=OFF
```

#### CLI Options

To see all CLI options for CoreNEURON, see `./bin/nrniv-core -h`.

#### Formatting CMake and C++ Code

In order to format code with `cmake-format` and `clang-format` tools, before creating a PR, enable below CMake options:

```
cmake .. -DCORENRN_CLANG_FORMAT=ON -DCORENRN_CMAKE_FORMAT=ON
cmake --build . --parallel 8 --target install
```

and now you can use `cmake-format` or `clang-format` targets:

```
cmake --build . --target cmake-format
cmake --build . --target clang-format
```

## Citation

If you would like to know more about CoreNEURON or would like to cite it, then use the following paper:

* Pramod Kumbhar, Michael Hines, Jeremy Fouriaux, Aleksandr Ovcharenko, James King, Fabien Delalondre and Felix Schürmann. CoreNEURON : An Optimized Compute Engine for the NEURON Simulator ([doi.org/10.3389/fninf.2019.00063](https://doi.org/10.3389/fninf.2019.00063))


## Support / Contribuition

If you see any issue, feel free to [raise a ticket](https://github.com/BlueBrain/CoreNeuron/issues/new). If you would like to improve this library, see [open issues](https://github.com/BlueBrain/CoreNeuron/issues).

You can see current [contributors here](https://github.com/BlueBrain/CoreNeuron/graphs/contributors).


## License
* See LICENSE.txt
* See [NEURON](https://github.com/neuronsimulator/nrn)


## Funding

CoreNEURON is developed in a joint collaboration between the Blue Brain Project and Yale University. This work has been funded by the EPFL Blue Brain Project (funded by the Swiss ETH board), NIH grant number R01NS11613 (Yale University), the European Union Seventh Framework Program (FP7/20072013) under grant agreement n◦ 604102 (HBP) and the European Union’s Horizon 2020 Framework Programme for Research and Innovation under Specific Grant Agreement n◦ 720270 (Human Brain Project SGA1), n◦ 785907 (Human Brain Project SGA2) and n◦ 945539 (Human Brain Project SGA3).

Copyright (c) Blue Brain Project/EPFL 2016 - 2021
