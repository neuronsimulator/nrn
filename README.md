[![Build Status](https://api.travis-ci.org/neuronsimulator/nrn.svg?branch=master)](https://travis-ci.org/neuronsimulator/nrn) [![Build Status](https://dev.azure.com/neuronsimulator/nrn/_apis/build/status/neuronsimulator.nrn?branchName=master)](https://dev.azure.com/neuronsimulator/nrn/_build/latest?definitionId=1&branchName=master) [![Actions Status](https://github.com/neuronsimulator/nrn/workflows/Windows%20Installer/badge.svg)](https://github.com/neuronsimulator/nrn/actions) [![Actions Status](https://github.com/neuronsimulator/nrn/workflows/NEURON%20CI/badge.svg)](https://github.com/neuronsimulator/nrn/actions)

# NEURON
NEURON is a simulator for models of neurons and networks of neuron.
See [http://neuron.yale.edu](http://neuron.yale.edu) for installers, source code,
documentation, tutorials, announcements of courses and conferences,
and a discussion forum.

## Installing Binary Distributions

NEURON provides binary installers for Linux, Mac and Windows platforms. You can find the latest installers for Mac and Windows [here](https://neuron.yale.edu/ftp/neuron/versions/alpha/). For Linux and Mac you can install the official Python 3 wheel with:

```
pip3 install neuron
```

## Installing from source

If you want to build the latest version from source, you can find instructions in the following. Currently we are supporting two build systems:

- [CMake](#build-cmake) (__recommended__)
- [Autotools](#build-autotools) (legacy, minimum support)

Note that starting with the 8.0 release, we recommend users to use CMake as the primary build system for NEURON. We would be grateful for any feedback or issues you encounter using the CMake-based build system. Please [report any issue here](https://github.com/neuronsimulator/nrn/issues) and we will be happy to help.

If you are using autotools, we highly recommend switching to CMake.


### Build Dependencies

In order to build NEURON from source, following packages must be available:

- Bison
- Flex
- C/C++ compiler suite
- CMake 3.8 or Autotools

Following packages are optional (see build options):

- Python >=2.7, or Python >=3.5 (for Python interface)
- Cython (for RXD)
- MPI (for parallel)
- X11 (Linux) or XQuartz (MacOS) (for GUI)

<a name="build-cmake"></a>
### Build using CMake

Starting with the 7.8.1 release, NEURON can be installed using the [CMake build system](https://cmake.org/). One of the primary advantages of a CMake-based build system is cross platform support and integration with other projects like [Interviews](https://github.com/neuronsimulator/iv), [CoreNEURON](https://github.com/BlueBrain/CoreNeuron/), [NMODL](https://github.com/BlueBrain/nmodl/) etc. These projects are now integrated into single a CMake-based build system and they can be installed together as shown below:


1. Clone latest version:

  ```
  git clone https://github.com/neuronsimulator/nrn
  cd nrn
  ```

2. Create a build directory:

  ```
  mkdir build
  cd build
  ```

3. If you are building on Cray systems with a GNU toolchain, you have to set following environmental variable:

```
export CRAYPE_LINK_TYPE=dynamic
```

4. Run `cmake` with the appropriate options (see below for a list of common options). \
A full list of options can be found in `nrn/CMakeLists.txt` and defaults are shown in `nrn/cmake/BuildOptionDefaults.cmake`. \
e.g. a bare-bones install:

  ```
  cmake .. \
   -DNRN_ENABLE_INTERVIEWS=OFF \
   -DNRN_ENABLE_MPI=OFF \
   -DNRN_ENABLE_RX3D=OFF \
   -DCMAKE_INSTALL_PREFIX=/path/to/install/directory
  ```

5. Build the code:

  ```
  make -j
  make install
  ```

6. Set PATH and PYTHONPATH environmental variables to use the installation:

  ```
  export PATH=/path/to/install/directory/bin:$PATH
  export PYTHONPATH=/path/to/install/directory/lib/python:$PYTHONPATH
  ```


Particularly useful CMake options are (use **ON** to enable and **OFF** to disable feature):

* **-DNRN\_ENABLE\_INTERVIEWS=OFF** : Disable Interviews (native GUI support)
* **-DNRN\_ENABLE\_PYTHON=OFF** : Disable Python support
* **-DNRN\_ENABLE\_MPI=OFF** : Disable MPI support for parallelization
* **-DNRN\_ENABLE\_RX3D=OFF** : Disable rx3d support
* **-DNRN\_ENABLE\_CORENEURON=ON** : Enable CoreNEURON support
* **-DNRN\_ENABLE\_TESTS=ON** : Enable unit tests
* **-DPYTHON\_EXECUTABLE=/python/binary/path** : Use provided Python binary to build Python interface
* **-DCMAKE_INSTALL_PREFIX=/install/dir/path** : Location for installing
* **-DCORENRN\_ENABLE\_NMODL=ON** : Use [NMODL](https://github.com/BlueBrain/nmodl/) instead of [MOD2C](https://github.com/BlueBrain/mod2c/) for code generation
with CoreNEURON
* **-DNRN\_ENABLE\_BINARY_SPECIAL=ON** : Build special as a binary instead of shell script


Please refer to [docs/cmake_doc/options.rst](docs/cmake_doc/options.rst) for more information on the CMake options.

#### Optimized CPU and GPU Support using CoreNEURON

NEURON now integrates [CoreNEURON library](https://github.com/BlueBrain/CoreNeuron/) for improved simulation performance on modern CPU and GPU architectures. CoreNEURON is designed as a library within the NEURON simulator and can transparently handle all spiking network simulations including gap junction coupling with the fixed time step method. You can find detailed instructions [here](docs/coreneuron/how-to/coreneuron.md) and [here](https://github.com/BlueBrain/CoreNeuron/#installation).

#### Building documentation

See [docs/README.md](docs/README.md) to install dependencies and build documentation.

<a name="build-autotools"></a>
### Build using Autotools

If you would like to have GUI support, you first need to install the Interviews package available from GitHub [here](http://github.com/neuronsimulator/iv) or the tarball provided [here](http://neuron.yale.edu/ftp/neuron/versions/alpha/). In case of the former, first you need to run `build.sh` script to create the automake, autoconf, and libtool generated files:

```bash
sh build.sh
```

And then run the standard `configure`, `make` and `make install` steps to install Interviews:

```bash
./configure --prefix=/path/to/install/directory
make
make install
```

To build NEURON we have to use the same steps as Interviews, i.e., if the source is obtained from the git repository, run `build.sh` script to create the automake, autoconf, and libtool generated files:

```bash
sh build.sh
```

and then run the standard `configure`, `make` and `make install` steps:

```bash
./configure --prefix=/path/to/install/directory
make
make install
```

You can set the following environmental variables to use the installation:

```bash
export PATH=/path/to/install/directory/<arch>/bin:$PATH              # replace <arch> with x86_64 or other platform directory
export PYTHONPATH=/path/to/install/directory/lib/python:$PYTHONPATH
```


If you want to customize the build, particularly useful configure options are:


- `--prefix=/some/path` : Install in this location of your filesystem.
- `--without-x` : If the InterViews graphics library is not installed, disable GUI.
- `--with-iv=<prefix>/../iv` : If InterViews was not installed in <prefix>/../iv
- `--with-paranrn` : Parallel models on cluster computers using MPI
- `--with-nrnpython` : Use Python as an alternative interpreter (as well as the native HOC interpreter).
- `--with-nmodl-only` : Build nmodl only (in case of cross compiling)
- `--disable-rx3d` : Do not compile the cython translated 3-d rxd features


For additional documentation see the [INSTALL file](INSTALL.md)
file.

For more installation information see: [https://neuron.yale.edu/neuron/download/getdevel](https://neuron.yale.edu/neuron/download/getdevel).

## Developer documentation

Please refer to [docs/README.md](docs/README.md)
