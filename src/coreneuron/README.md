# CoreNEURON
> Optimised simulator engine for [NEURON](https://www.neuron.yale.edu/neuron/)

CoreNEURON is a simplified engine for the [NEURON](https://www.neuron.yale.edu/neuron/) simulator optimised for both memory usage and computational speed. Its goal is to simulate massive cell networks with minimal memory footprint and optimal performance.

If you are a new user and would like to use CoreNEURON, [this tutorial](https://github.com/nrnhines/ringtest) will be a good starting point to understand complete workflow of using CoreNEURON with NEURON.

# Features

CoreNEURON supports limited features provided by [NEURON](https://www.neuron.yale.edu/neuron/). Contact Michael Hines for detailed information.

# Dependencies
* [CMake 2.8.12+](https://cmake.org)
* [MOD2C](http://github.com/BlueBrain/mod2c)
* [MPI 2.0+](http://mpich.org) [Optional]
* [PGI OpenACC Compiler >=16.3](https://www.pgroup.com/resources/accel.htm) [Optional, for GPU systems]
* [CUDA Toolkit >=6.0](https://developer.nvidia.com/cuda-toolkit-60) [Optional, for GPU systems]

# Installation

First, install mod2c using the instructions provided [here](http://github.com/BlueBrain/mod2c). Make sure to install mod2c and CoreNEURON in the same installation directory (using CMAKE\_INSTALL\_PREFIX).

Set the appropriate MPI wrappers for the C and C++ compilers, e.g.:

```bash
export CC=mpicc
export CXX=mpicxx
```

If you don't have MPI, you can disable MPI dependency using the CMake option `-DENABLE_MPI=OFF`:

```bash
export CC=gcc
export CXX=g++
cmake .. -DENABLE_MPI=OFF
```

##### About MOD files

The workflow for building CoreNEURON is different from that of NEURON, especially considering the use of **nrnivmodl**. Currently we do not provide **nrnivmodl** for CoreNEURON. If you have MOD files from a NEURON model, you have to explicitly specify those MOD file directory paths during CoreNEURON build using the `-DADDITIONAL_MECHPATH` option:

```bash
cmake .. -DADDITIONAL_MECHPATH="/path/of/mod/files/directory/"
```
This directory should have only mod files that are compatible with CoreNEURON. You can also provide multiple directories separated by a semicolon :

```bash
-DADDITIONAL_MECHPATH="/path/of/folder/with/mod_files;/path/of/another_folder/with/mod_files" 
```


# Building with GPU support

CoreNEURON has support for GPUs using the OpenACC programming model when enabled with `-DENABLE_OPENACC=ON`. Below are the steps to compile with PGI compiler:

```bash
module purge
module load pgi/pgi64/16.5 pgi/mpich/16.5   #change pgi and cuda modules
module load cuda/6.0

export CC=mpicc
export CXX=mpicxx

cmake .. -DADDITIONAL_MECHPATH="/path/of/folder/with/mod_files" -DCMAKE_C_FLAGS:STRING="-O2" -DCMAKE_CXX_FLAGS:STRING="-O2" -DCOMPILE_LIBRARY_TYPE=STATIC -DCMAKE_INSTALL_PREFIX=$EXPER_DIR/install/ -DCUDA_HOST_COMPILER=`which gcc` -DCUDA_PROPAGATE_HOST_FLAGS=OFF -DENABLE_SELECTIVE_GPU_PROFILING=ON -DENABLE_OPENACC=ON
```

Note that the CUDA Toolkit version should be compatible with PGI compiler installed on your system. Otherwise you have to add extra C/C++ flags. For example, if we are using CUDA Toolkit 7.5 installation but PGI default target is CUDA 7.0 then we have to add :

```bash
-DCMAKE_C_FLAGS:STRING="-O2 -ta=tesla:cuda7.5" -DCMAKE_CXX_FLAGS:STRING="-O2 -ta=tesla:cuda7.5"
```

CoreNEURON uses the Random123 library written in CUDA. If you are **not using `NrnRandom123`** in your model and have issues with CUDA compilation/linking (or CUDA Toolkit is not installed), you can disable the CUDA dependency using the CMake option `-DENABLE_CUDA_MODULES=OFF` :

```bash
cmake ..  -DADDITIONAL_MECHPATH="/path/of/folder/with/mod_files" -DCMAKE_C_FLAGS:STRING="-O2" -DCMAKE_CXX_FLAGS:STRING="-O2" -DCOMPILE_LIBRARY_TYPE=STATIC -DCMAKE_INSTALL_PREFIX=$EXPER_DIR/install/ -DENABLE_CUDA_MODULES=OFF -DENABLE_OPENACC=ON
```

You have to run GPU executable with the `--gpu` or `-gpu` option as:

```bash
mpirun -n 1 ./bin/coreneuron_exec -d ../tests/integration/ring -mpi -e 100 --gpu
```

Make sure to enable cell re-ordering mechanism to improve GPU performance using `--cell_permute` option (permutation types : 1 or 2):

```bash
mpirun -n 1 ./bin/coreneuron_exec -d ../tests/integration/ring -mpi -e 100 --gpu --cell_permute 2
```

Note that if your model is using Random123 random number generator, you can't use same executable for CPU and GPU runs. We suggest to build separate executable for CPU and GPU simulations. This will be fixed in future releases.

# Building on Cray System

On a Cray system the user has to provide the path to the MPI library as follows:

```bash
export CC=`which cc`
export CXX=`which CC`
cmake -DMPI_C_INCLUDE_PATH=$MPICH_DIR/include -DMPI_C_LIBRARIES=$MPICH_DIR/lib
```

We have tested the build process on the following platforms:

* Blue Gene/Q: XLC/GCC
* x86: Intel, PGI, GCC, Cray
* OS X: Clang, GCC


# Optimization Flags

* One can specify C/C++ optimization flags specific to the compiler and architecture with `-DCMAKE_CXX_FLAGS` and `-DCMAKE_C_FLAGS` options to the CMake command. For example, on a BG-Q:

```bash
cmake .. -DCMAKE_CXX_FLAGS="-O3 -qtune=qp -qarch=qp -q64 -qhot=simd -qsmp -qthreaded" -DCMAKE_C_FLAGS="-O3 -qtune=qp -qarch=qp -q64 -qhot=simd -qsmp -qthreaded"
```

* By default OpenMP threading is enabled. You can disable it with `-DCORENEURON_OPENMP=OFF`
* By default CoreNEURON uses the SoA (Structure of Array) memory layout for all data structures. You can switch to AoS using `-DENABLE_SOA=OFF`.
* If the default compiler flags are not supported, try `-DCMAKE_BUILD_TARGET=SOME_TARGET`


# RUNNING SIMULATION:

Note that the CoreNEURON simulator dependends on NEURON to build the network model: see [NEURON](https://www.neuron.yale.edu/neuron/) documentation for more information. Once you build the model using NEURON, you can launch CoreNEURON on the same or different machine by:

```bash
export OMP_NUM_THREADS=8     #set appropriate value
mpiexec -np 2 /path/to/isntall/directory/coreneuron_exec -e 10 -d /path/to/model/built/by/neuron -mpi
```

[This tutorial](https://github.com/nrnhines/ringtest) provide more information for parallel runs and performance comparison.

In order to see the command line options, you can use:

```bash
/path/to/isntall/directory/coreneuron_exec --help
-b, --spikebuf ARG       Spike buffer size. (100000)
-c, --threading          Parallel threads. The default is serial threads.
-d, --datpath ARG        Path containing CoreNeuron data files. (.)
-dt, --dt ARG            Fixed time step. The default value is set by
                         defaults.dat or is 0.025.
-e, --tstop ARG          Stop time (ms). (100)
-f, --filesdat ARG       Name for the distribution file. (files.dat)
-g, --prcellgid ARG      Output prcellstate information for the gid NUMBER.
-gpu, --gpu              Enable use of GPUs. The default implies cpu only run.
-h, --help               Print a usage message briefly summarizing these
                         command-line options, then exit.
-i, --dt_io ARG          Dt of I/O. (0.1)
-k, --forwardskip ARG    Forwardskip to TIME
-l, --celsius ARG        Temperature in degC. The default value is set in
                         defaults.dat or else is 34.0.
-mpi                     Enable MPI. In order to initialize MPI environment this
                         argument must be specified.
-o, --outpath ARG        Path to place output data files. (.)
-p, --pattern ARG        Apply patternstim using the specified spike file.
-R, --cell-permute ARG   Cell permutation, 0 No; 1 optimise node adjacency; 2
                         optimize parent adjacency. (1)
-r, --report ARG         Enable voltage report (0 for disable, 1 for soma, 2 for
                         full compartment).
-s, --tstart ARG         Start time (ms). (0)
-v, --voltage ARG        Initial voltage used for nrn_finitialize(1, v_init). If
                         1000, then nrn_finitialize(0,...). (-65.)
-W, --nwarp ARG          Number of warps to balance. (0)
-w, --dt_report ARG      Dt for soma reports (using ReportingLib). (0.1)
-x, --extracon ARG       Number of extra random connections in each thread to
                         other duplicate models (int).
-z, --multiple ARG       Model duplication factor. Model size is normal size *
                         (int).
--binqueue               Use bin queue.
--mindelay ARG           Maximum integration interval (likely reduced by minimum
                         NetCon delay). (10)
--ms-phases ARG          Number of multisend phases, 1 or 2. (2)
--ms-subintervals ARG    Number of multisend subintervals, 1 or 2. (2)
--multisend              Use Multisend spike exchange instead of Allgather.
--read-config ARG        Read configuration file filename.
--show                   Print args.
--spkcompress ARG        Spike compression. Up to ARG are exchanged during
                         MPI_Allgather. (0)
--write-config ARG       Write configuration file filename.

```

Default values for all parameters are printed by rank 0 on launch. E.g.
```bash
$ cpu/bin/coreneuron_exec --show
...

--spikebuf = 100000      --spkcompress = 0        --prcellgid = -1
--cell-permute = 1       --nwarp = 0              --ms-subintervals = 2
--ms-phases = 2          --report = 0             --multiple = 1
--extracon = 0           --pattern = not set      --datpath = .
--filesdat = files.dat   --outpath = .            --write-config = not set
--read-config = not set  --tstart = 0             --tstop = 100
--dt = -1000             --dt_io = 0.1            --voltage = -65
--celsius = -1000        --forwardskip = 0        --dt_report = 0.1
--mindelay = 10          --help = not set         --threading = not set
--gpu = not set          -mpi = not set           --show = set
--multisend = not set    --binqueue = not set

...
```
Note that celsius and dt, if not specified, will get their values from the models <datpath>/defaults.dat file.

# Results

Currently CoreNEURON only outputs spike data. When running the simulation, each MPI rank writes spike information
into a file `out.#mpi_rank`. These files should be combined and sorted to compare with NEURON spike output.

```
cat out[0-9]*.dat | sort -k 1n,1n -k 2n,2n > out.spk
```

# Running tests

Once you compile CoreNEURON, unit tests and a ring test will be compiled if Boost is available.
If you pass the path for Neurodamus channels, 10 cell tests will also be compile. You can run tests using

```bash
make test
```

If you have different mpi launcher, you can specify it during cmake configuration as:

```bash
cmake .. -DTEST_MPI_EXEC_BIN="mpirun" -DTEST_EXEC_PREFIX="mpirun;-n;2" -DTEST_EXEC_PREFIX="mpirun;-n;2" -DAUTO_TEST_WITH_SLURM=OFF -DAUTO_TEST_WITH_MPIEXEC=OFF
```

# Developer Notes
If you have installed `clang-format`, you can reformat/reindent generated .c files from mod2c using:

```
make formatbuild
```

The `.clang-format` file in the source repository is compatible with version 3.9.

## License
* See LICENSE.txt
* See [NEURON](https://www.neuron.yale.edu/neuron/)
* The mechanisms and test datasets appearing in the CoreNeuron repository are subject to separate licenses.
  More information is available on the NMC portal website [NMC portal](https://bbp.epfl.ch/nmc-portal/copyright),
  the specific licenses are described in the ME-type model packages downloadable from that website.

## Contributors
To facilitate the future distributions of the software the Blue Brain Project wishes to remain the sole
owner of the copyright. Therefore we will ask contributors to not modify the existing copyright.
Contributors will however be gratefully acknowledged in the corresponding CREDIT.txt file.
