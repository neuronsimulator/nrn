# How to use CoreNEURON

[CoreNEURON](https://github.com/BlueBrain/CoreNeuron) is a compute engine for the NEURON simulator optimised for both memory usage and computational speed on modern CPU/GPU architectures. The goals of CoreNEURON are:

* Simulating large network models
* Reduce memory usage
* Support for GPUs
* Optimisations like vectorisation and memory layout (e.g. Structure-Of-Arrays)

CoreNEURON is designed as a library within the NEURON simulator and can transparently handle all spiking network simulations including gap junction coupling with the **fixed time step method**. In order to run a NEURON model with CoreNEURON:

* MOD files shall be [THREADSAFE](https://neuron.yale.edu/neuron/docs/multithread-parallelization)
* Random123 shall be used if a random generator is needed (instead of MCellRan4)
* POINTER variables need to be converted to BBCOREPOINTER ([details here](bbcorepointer.md))

For compatibility purposes, please make sure to refer to [non supported NEURON features list](../non_supported_features.rst).

## Build Dependencies
* Bison
* Flex
* CMake >=3.15.0
* Python >=3.6
* MPI Library [Optional, for MPI support]
* [PGI Compiler / NVIDIA HPC SDK](https://developer.nvidia.com/hpc-sdk) [Optional, for GPU support]
* [CUDA Toolkit >=9.0](https://developer.nvidia.com/cuda-downloads) [Optional, for GPU support]

#### Compiler Selection

CoreNEURON relies on compiler [auto-vectorisation](https://en.wikipedia.org/wiki/Automatic_vectorization) to achieve better performance on moder CPUs.
With this release we recommend compilers like **Intel / PGI / Cray  Compiler**.
These compilers are able to vectorise the code better than **GCC** or **Clang**, achieving the best possible performance gains.
If you are using any cluster platform, then Intel or Cray compiler should be available as a module.
You can also install the Intel compiler by downloading [oneAPI HPC Toolkit](https://software.intel.com/content/www/us/en/develop/tools/oneapi/hpc-toolkit.html).  
CoreNEURON supports also GPU execution based on an [OpenACC](https://en.wikipedia.org/wiki/OpenACC) backend.
Currently, the best supported compiler for the OpenACC backend is PGI, available as part of [NVIDIA-HPC-SDK](https://developer.nvidia.com/hpc-sdk).
You need to use this compiler for NVIDIA GPUs.
Note that AMD GPU support is not tested.

## Installation

CoreNEURON is a submodule of the NEURON git repository.
If you are a NEURON user, the preferred way to install CoreNEURON is to enable extra build options during NEURON installation as follows:

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
   If compilers and necessary dependencies are already available in the default paths then you do not need to do anything.
   In cluster or HPC environment we often use a module system to select software.
   For example, you can load the compiler, cmake, and python dependencies using module as follows:
   ```
   module load intel openmpi python cmake
   ```
   If you want to enable GPU support then you have to load PGI/NVIDIA-HPC-SDK and CUDA modules:
   ```
   module load nvidia-hpc-sdk cuda
   ```
   Make sure to change module names based on your system.
   > ⚠️⚠️
   > 
   > The NVIDIA HPC SDK also bundles CUDA, at least in recent versions. You should make sure that the version of CUDA in your environment is compatible with the CUDA driver on your system, otherwise you may encounter errors like
   > ```
   > nvlink fatal: Input file '[snip]' newer than toolkit (112 vs 110) (target: sm_60)
   > ```
   > ⚠️⚠️

4. Run CMake with the appropriate [options](https://github.com/neuronsimulator/nrn#build-using-cmake) and additionally enable CoreNEURON with `-DNRN_ENABLE_CORENEURON=ON`:

 	```
  	cmake .. \
   		-DNRN_ENABLE_CORENEURON=ON \
   		-DNRN_ENABLE_INTERVIEWS=OFF \
   		-DNRN_ENABLE_RX3D=OFF \
   		-DCMAKE_INSTALL_PREFIX=$HOME/install \
   		-DCMAKE_C_COMPILER=icc \
   		-DCMAKE_CXX_COMPILER=icpc
  	```

	Make sure to replace `icc` and `icpc` with C/CXX compiler you are using. Also change `$HOME/install` to desired installation directory. CMake tries to find MPI libraries automatically but if needed you can set MPI compiler options `-DMPI_C_COMPILER=<mpi C compiler>` and `-DMPI_CXX_COMPILER=<mpi CXX compiler>`.
 
	If you would like to enable GPU support with OpenACC, make sure to use `-DCORENRN_ENABLE_GPU=ON` option and use the PGI/NVIDIA HPC SDK compilers with CUDA. For example,

	```
	cmake .. \
		-DNRN_ENABLE_CORENEURON=ON \
		-DCORENRN_ENABLE_GPU=ON \
		-DNRN_ENABLE_INTERVIEWS=OFF \
		-DNRN_ENABLE_RX3D=OFF \
		-DCMAKE_INSTALL_PREFIX=$HOME/install \
		-DCMAKE_C_COMPILER=nvc \
		-DCMAKE_CXX_COMPILER=nvc++
	```
  	By default the GPU code will be compiled for NVIDIA devices with compute capability 6.0 or 7.0.
	This can be steered by passing, for example, `-DCORENRN_GPU_CUDA_COMPUTE_CAPABILITY:STRING=50;60;70` to CMake.

	You can change C/C++ optimization flags using `-DCMAKE_CXX_FLAGS` and `-DCMAKE_C_FLAGS` options to the CMake command.
	You have to add the following CMake options:
	```bash
		-DCMAKE_CXX_FLAGS="-O3 -g" \
	  	-DCMAKE_C_FLAGS="-O3 -g" \
	  	-DCMAKE_BUILD_TYPE=CUSTOM \
	```
	NOTE : If the CMake command fails, please make sure to delete temporary CMake cache files (`CMakeCache.txt` or build directory) before re-running CMake.


5. Once the configure step is done, you can build and install the project as:
   ```bash
   make -j
   make install
   ```

7. Set PATH and PYTHONPATH environmental variables to use the installation:
   ```bash
   export PATH=$HOME/install/bin:$PATH
   export PYTHONPATH=$HOME/install/lib/python:$PYTHONPATH
   ```

Now you should be able to import neuron module as:

```
python -c "from neuron import h; from neuron import coreneuron"
```

If you get `ImportError` then make sure `PYTHONPATH` is setup correctly and `python` version is same as the one used for NEURON installation.

## Building MOD files

As in a typical NEURON workflow, you can now use `nrnivmodl` to translate MOD files.
In order to enable CoreNEURON support, you must set the `-coreneuron` flag.
Make sure any necessary modules (compilers, CUDA, MPI etc) are loaded before using `nrnivmodl`:

```bash
nrnivmodl -coreneuron <directory containing mod files>
```

If you don't have additional mod files and are only using inbuilt mod files from NEURON then **you still need to use `nrnivmodl -coreneuron` to generate a CoreNEURON library**. For example, you can run:

```bash
nrnivmodl -coreneuron .
```

With above commands, NEURON will create a `x86_64/special` binary linked to CoreNEURON (here `x86_64` is the architecture name of your system).

If you see any compilation error then one of the mod files might be incompatible with CoreNEURON.
Please [open an issue](https://github.com/BlueBrain/CoreNeuron/issues) with mod file example.

## Running Simulations

With CoreNEURON, existing NEURON models can be run with minimal changes.For a given NEURON model, we typically need to do the following steps:

1. Enable cache efficiency :

	```python
	from neuron import h
	h.cvode.cache_efficient(1)
	```

2. Enable CoreNEURON :

	```python
	from neuron import coreneuron
	coreneuron.enable = True
	```

3. If GPU support is enabled during build, enable GPU execution using:
   ```python
   coreneuron.gpu = True
   ```
   > ⚠️⚠️
   > 
   > **In this case you must launch your script using the `special` binary!**
   > This is explained in more detail below.
   > 
   > ⚠️⚠️

4. Use `psolve` to run simulation after initialization :

	```python
	h.stdinit()
	pc.psolve(h.tstop)
	```

With the above steps, NEURON will build the model and will transfer it to CoreNEURON for simulation.
At the end of the simulation CoreNEURON transfers by default: spikes, voltages, state variables, NetCon weights, all Vector.record, and most GUI trajectories to NEURON.
These variables can be recorded using regular NEURON API (e.g. [Vector.record](https://www.neuron.yale.edu/neuron/static/py_doc/programming/math/vector.html#Vector.record) or [spike_record](https://www.neuron.yale.edu/neuron/static/new_doc/modelspec/programmatic/network/parcon.html#ParallelContext.spike_record)).

If you are primarily using HOC then before calling `psolve` you can enable CoreNEURON as:

```python
// make sure NEURON is compiled with Python
if (!nrnpython("from neuron import coreneuron")) {
	printf("NEURON not compiled with Python support\n")
    return
}

// access coreneuron module via Python object
py_obj = new PythonObject()
py_obj.coreneuron.enable = 1
```

Once you adapted your model with changes described above then you can execute your model like normal NEURON simulation. For example:

```bash
mpiexec -n <num_process> nrniv -mpi -python your_script.py       # python
mpiexec -n <num_process> nrniv -mpi your_script.hoc              # hoc
```

Alternatively, instead of `nrniv` you can use the `special` binary generated by `nrnivmodl` command.
Note that for GPU execution you **must** use the `special` binary to launch your simulation:

```bash
mpiexec -n <num_process> x86_64/special -mpi -python your_script.py       # python
mpiexec -n <num_process> x86_64/special -mpi your_script.hoc              # hoc
```
This is because the GPU-enabled build is statically linked [to avoid issues with OpenACC](https://forums.developer.nvidia.com/t/clarification-on-using-openacc-in-a-shared-library/136279/27), so `python` and `nrniv` cannot dynamically load CoreNEURON.

As CoreNEURON is used as a library under NEURON, it will use the same number of MPI ranks as NEURON. Also, if you enable threads using [ParallelContext.nthread()](https://www.neuron.yale.edu/neuron/static/py_doc/modelspec/programmatic/network/parcon.html#ParallelContext.nthread) then CoreNEURON will internally use the same number of OpenMP threads.

> NOTE: Replace mpiexec with an MPI launcher supported on your system (e.g. `srun` or `mpirun`).

## Examples

Here are some test examples to illustrate the usage of CoreNEURON API with NEURON:

1. [test_direct.py](https://github.com/neuronsimulator/nrn/blob/master/test/coreneuron/test_direct.py): This is a simple, single cell, serial Python example using demonstrating use of CoreNEURON. We first run simulation with NEURON and record voltage and membrane current. Then, the same model is executed with CoreNEURON, and we make sure the same results are achieved. Note that in order to run this example make sure to compile [these mod files](https://github.com/neuronsimulator/nrn/tree/master/test/coreneuron/mod) with `nrnivmodl -coreneuron`. You can this example as:

	```bash
	nrnivmodl -coreneuron mod                # first compile mod files
	nrniv -python test_direct.py             # run via nrniv
	x86_64/special -python test_direct.py    # run via special
	python test_direct.py                    # run via python
	```

2. [test_direct.hoc](https://github.com/neuronsimulator/nrn/blob/master/test/coreneuron/test_direct.hoc): This is the same example as above (`test_direct.py`) but written using HOC.

3. [test_spikes.py](https://github.com/neuronsimulator/nrn/blob/master/test/coreneuron/test_spikes.py): This is similar to above mentioned test_direct.py but can be run with MPI where each MPI process creates a single cell and connect it with a cell on another rank. Each rank records spikes and compares them between NEURON execution and CoreNEURON execution. It also demonstrates usage of [mpi4py](https://github.com/mpi4py/mpi4py) python module or NEURON's native MPI API.

	You can run this MPI example in different ways:

	```bash
	mpiexec -n <num_process> python test_spikes.py mpi4py                        # using mpi4py
	mpiexec -n <num_process> x86_64/special -mpi -python test_spikes.py          # neuron internal MPI
	```

4. [Ring network test](https://github.com/neuronsimulator/ringtest): This is a ring network model of Ball-and-Stick neurons which can be scaled arbitrarily for testing and benchmarking purpose. You can use this as reference for porting your model, see [README](https://github.com/neuronsimulator/ringtest/blob/master/README.md) file for detailed instructions.

5. [3D Olfactory Bulb Model](https://github.com/HumanBrainProject/olfactory-bulb-3d): [Migliore et al. (2014)](https://www.frontiersin.org/articles/10.3389/fncom.2014.00050/) model of the olfactory bulb ported with CoreNEURON on GPU. See [README](https://github.com/HumanBrainProject/olfactory-bulb-3d/blob/master/README.md) for detailed instructions.
