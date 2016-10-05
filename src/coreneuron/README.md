# CoreNEURON
> Optimised simulator engine for [NEURON](https://www.neuron.yale.edu/neuron/)

CoreNEURON is a simplified engine for the [NEURON](https://www.neuron.yale.edu/neuron/) simulator optimised for both memory usage and computational speed. Its goal is to simulate massive cell networks with minimal memory footprint and optimal performance.

# Features

CoreNEURON supports limited features provided by [NEURON](https://www.neuron.yale.edu/neuron/). Contact Michael Hines for detailed information.

# Dependencies
* [CMake 2.8.12+](https://cmake.org)
* [MOD2C](http://github.com/BlueBrain/mod2c)
* [MPI 2.0+](http://mpich.org) [Optional]
* [PGI OpenACC Compiler >=16.3](https://www.pgroup.com/resources/accel.htm) [Optional, for GPU systems]

# Installation

First, install mod2c using the instructions provided [here](http://github.com/BlueBrain/mod2c). Make sure to install mod2c and CoreNEURON in the same installation directory (using CMAKE\_INSTALL\_PREFIX).

Set the appropriate MPI wrappers for the C and C++ compilers, e.g.:

```bash
export CC=mpicc
export CXX=mpicxx
```

If you don't have MPI, you can disable MPI dependency using CMake option *-DENABLE_MPI=OFF*:
```bash
export CC=gcc
export CXX=g++
cmake .. -DENABLE_MPI=OFF
```

The workflow for building CoreNEURON is slightly different from that of NEURON, especially considering the use of **nrnivmodl**. Currently we do not provide **nrnivmodl** for CoreNEURON and hence the user needs to provide paths of mod file directories (semicolon separated) at the time of the build process using the *ADDITIONAL_MECHPATH* variable:

```bash
cd CoreNeuron
mkdir build && cd build
cmake .. -DADDITIONAL_MECHPATH="/path/of/folder/with/mod/file1dir;/path/of/folder/with/mod/file2dir" -DCMAKE_INSTALL_PREFIX=/path/to/isntall/directory
make
make install
```

# Building with GPU support

CoreNEURON has support for GPUs using OpenACC programming model when enabled with *-DENABLE_OPENACC=ON*.

Here are the steps to compile with PGI compiler:

```bash
module purge
module load pgi/pgi64/16.5 pgi/mpich/16.5
module load cuda/6.0

export CC=mpicc
export CXX=mpicxx

cmake .. -DCMAKE_C_FLAGS:STRING="-acc -Minfo=acc -Minline=size:200,levels:10 -O3 -DSWAP_ENDIAN_DISABLE_ASM -DDISABLE_HOC_EXP" -DCMAKE_CXX_FLAGS:STRING="-acc -Minfo=acc -Minline=size:200,levels:10 -O3 -DSWAP_ENDIAN_DISABLE_ASM -DDISABLE_HOC_EXP" -DCOMPILE_LIBRARY_TYPE=STATIC -DCMAKE_INSTALL_PREFIX=$EXPER_DIR/install/ -DCUDA_HOST_COMPILER=`which gcc` -DCUDA_PROPAGATE_HOST_FLAGS=OFF -DENABLE_SELECTIVE_GPU_PROFILING=ON -DENABLE_OPENACC=ON
```

And now you can run with --gpu option as:

```bash
export CUDA_VISIBLE_DEVICES=0   #if needed
mpirun -n 1 ./bin/coreneuron_exec -d ../tests/integration/ring -mpi -e 100 --gpu --celsius=6.3
```

Additionally you can enable cell reordering mechanism to improve GPU performance using cell_permute option:
```bash
mpirun -n 1 ./bin/coreneuron_exec -d ../tests/integration/ring -mpi -e 100 --gpu --celsius=6.3 --cell_permute=1
```

Note that if your model is using Random123 random number generator, you can't use same executable for CPU and GPU runs.
This will be fixed in next version.

# Using ReportingLib
If you want enable use of ReportingLib for the soma reports, install ReportingLib first and enable it using -DENABLE_REPORTINGLIB (use same install path for ReportingLib as CoreNeuron).

# Using Neurodamus / Additional MOD files

If you have MOD files from the NEURON model, then you have to explicitly build those MOD files with CoreNEURON using *ADDITIONAL_MECHPATH* option:
```bash
cmake .. -DADDITIONAL_MECHPATH="/path/of/mod/files/directory/"
```
This directory should have only mod files compatible with CoreNEURON.

For BPP Users: If you are building CoreNeuron with Neurodamus, you have to set *ADDITIONAL_MECHPATH* and *ADDITIONAL_MECHS* as:
```bash
cmake .. -DADDITIONAL_MECHPATH="/path/of/neurodamus/lib/modlib" -DADDITIONAL_MECHS="/path/of/neurodamus/lib/modlib/coreneuron_modlist.txt"
```
Make sure to switch to appropriate branch of Neurodamus (based on your dataset/experiment, e.g. coreneuronsetup).

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

* One can specify C/C++ optimization flags spcecific to the compiler and architecture with -DCMAKE_CXX_FLAGS and -DCMAKE_C_FLAGS options to the CMake command. For example, on a BG-Q:

```bash
cmake .. -DCMAKE_CXX_FLAGS="-O3 -qtune=qp -qarch=qp -q64 -qhot=simd -qsmp -qthreaded" -DCMAKE_C_FLAGS="-O3 -qtune=qp -qarch=qp -q64 -qhot=simd -qsmp -qthreaded"
```

* By default OpenMP threading is enabled. You can disable it with -DCORENEURON_OPENMP=OFF
* By default CoreNEURON uses the SoA (Structure of Array) memory layout for all data structures. You can switch to AoS using -DENABLE_SOA=OFF.
* If the default compiler flags are not supported, try -DCMAKE_BUILD_TARGET=SOME_TARGET
* NEURON wraps `exp` function with hoc_Exp; disable this using "-DDISABLE_HOC_EXP"


# RUNNING SIMULATION:

Note that the CoreNEURON simulator dependends on NEURON to build the network model: see [NEURON](https://www.neuron.yale.edu/neuron/) documentation for more information. Once you build the model using NEURON, you can launch CoreNEURON on the same or different machine by:
```bash
export OMP_NUM_THREADS=8     #set appropriate value

mpiexec -np 2 /path/to/isntall/directory/coreneuron_exec -e 10 -d /path/to/model/built/by/neuron -mpi
```
See below for information on the different input flags.

In order to see the command line options, you can use:

/path/to/isntall/directory/coreneuron_exec --help

```bash
       -s TIME, \--tstart=TIME
              Set the start time to TIME (double). The default value is '0.'
       -e TIME, \--tstop=TIME
              Set the stop time to TIME (double). The default value is '100.'
       -t TIME, \--dt=TIME
              Set the dt time to TIME (double). The default value is '0.025'.
       -l NUMBER, --celsius=NUMBER
              Set the celsius temperature to NUMBER (double). The default value is '34.'.
       -p FILE, \--pattern=FILE
              Apply patternstim with the spike file FILE (char*). The default value is 'NULL'.
       -b SIZE, \--spikebuf=SIZE
              Set the spike buffer to be of the size SIZE (int). The default value is '100000'.
       -g NUMBER, \--prcellgid=NUMBER
              Output prcellstate information for the gid NUMBER (int). The default value is '-1'.
       -d PATH, \--datpath=PATH
              Set the path to PATH (char*) containing data generated by NEURON. The default value is '.'.
       -f FILE, \--filesdat=FILE
              Absolute path with the name for the required file FILE (char*). The default value is 'files.dat'.
       -o PATH, --outpath=PATH
              Set the path for the output data to PATH (char*). The default value is '.'.
       -k TIME, --forwardskip=TIME
              Set forwardskip to TIME (double). The default value is '0.'.
       -r, --report
              Enable soma report.
       -w TIME, --dt_report=TIME
              Set the dt for soma reports (using ReportingLib) to TIME (double). The default value is '0.1'.
       -z MULTIPLE, --multiple=MULTIPLE
              Model duplication factor. Model size is normal size * MULTIPLE (int). The default value is '1'.
       -x EXTRACON, --extracon=EXTRACON
              Number of extra random connections in each thread to other duplicate models (int). The default value is '0'.
       -R TYPE, --cell_permute=TYPE
              Permutation of cells for efficient execution of solver on GPU (TYPE could be 1 or 2).
       -mpi
              Enable MPI. In order to initialize MPI environment this argument must be specified.
```

# Results

Currently CoreNEURON only outputs spike data. When running the simulation, each MPI rank writes spike information
into a file `out.#mpi_rank`. These files should be combined and sorted to compare with NEURON spike output.

# Running tests

Once you compile CoreNEURON, unit tests and ring test will be compile by if Boost is available.
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

## Contributors
To facilitate the future distributions of the software the Blue Brain Project wishes to remain the sole
owner of the copyright. Therefore we will ask contributors to not modify the existing copyright.
Contributors will however be gratefully acknowledged in the corresponding CREDIT.txt file.
