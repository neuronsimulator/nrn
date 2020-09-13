[![Build Status](https://api.travis-ci.org/neuronsimulator/nrn.svg?branch=master)](https://travis-ci.org/neuronsimulator/nrn) [![Build Status](https://dev.azure.com/neuronsimulator/nrn/_apis/build/status/neuronsimulator.nrn?branchName=master)](https://dev.azure.com/neuronsimulator/nrn/_build/latest?definitionId=1&branchName=master)

NEURON is a simulator for models of neurons and networks of neuron.
See [http://neuron.yale.edu](http://neuron.yale.edu) for installers, source code,
documentation, tutorials, announcements of courses and conferences,
and a discussion forum.

## Building NEURON

NEURON provided binary installers for Linux, Mac and Windows platform [here](https://neuron.yale.edu/ftp/neuron/versions/alpha/). If you want to build latest version from source, you can find instructions bellow.

### Build Dependencies

In order to build NEURON from source, following packages must be available:

- Bison
- Flex
- C/C++ compiler suite

Following packages are optional (see build options):

- Python >=2.7, or Python >=3.5 (for Python interface)
- Cython (for RXD)
- MPI (for parallel)
- X11 (Linux) or XQuartz (MacOS) (for GUI)

### Build using Autotools

If you would like to have GUI support, you first need to install Interviews package available from GitHub [here](http://github.com/neuronsimulator/iv) or tarball provided [here](http://neuron.yale.edu/ftp/neuron/versions/alpha/). In case of git repository, first you need to run `build.sh` script to create the automake, autoconf, libtool generated files:

```bash
sh build.sh
```

And then run standard `configure`, `make` and `make install` steps to install Interviews:

```bash
./configure
make
make install
```

To build NEURON we have to use same steps as Interviews i.e. if source is obtained from the git repository, run `build.sh` script to create the automake, autoconf, libtool generated files:

```bash
sh build.sh
```

and then run standard `configure`, `make` and `make install` steps:

```bash
./configure
make
make install
```

If you want to customize build, particularly useful configure options are:


- `--prefix=/some/path` : Install in this location of your filesystem.
- `--without-x` : If the InterViews graphics library is not installed, disable GUI.
- `--with-iv=<prefix>/../iv` : If InterViews was not installed in <prefix>/../iv
- `--with-paranrn` : Parallel models on cluster computers using MPI
- `--with-nrnpython` : Use Python as an alternative interpreter (as well as the native HOC interpreter).
- `--with-nmodl-only` : Build nmodl only (in case of cross compiling)
- `--disable-rx3d` : Do not compile the cython translated 3-d rxd features


For more details see the [INSTALL.md](https://github.com/neuronsimulator/nrn/blob/master/INSTALL.md)
file.

For more installation information see: [https://neuron.yale.edu/neuron/download/getdevel](https://neuron.yale.edu/neuron/download/getdevel).

### Build using CMake

NEURON can now also be built and installed using [CMake build system](https://cmake.org/). Currently we are supporting two build systems i.e. Autoconf and CMake. In the future, based on the feedback from the community we will decide on switching to CMake build system. Therefore, we would be grateful for any feedback or issues you encounter using CMake based build system. Please [report an issue here](https://github.com/neuronsimulator/nrn/issues) and we will be happy to help.

One of the primary advantage of CMake based build system is integration with other projects like [Interviews](https://github.com/neuronsimulator/iv), [CoreNEURON](https://github.com/BlueBrain/CoreNeuron/), [NMODL](https://github.com/BlueBrain/nmodl/) etc. Such projects are now integrated into single CMake based build system and they can be installed together as shown below:


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

3. Run cmake with the appropriate options (see below for list of common options). \
A full list of options can be found in *nrn/CMakeLists.txt* . Defaults are shown in *nrn/cmake/BuildOptionDefaults.cmake*), \
e.g. a bare-bones install:

  ```
  cmake .. \
   -DNRN_ENABLE_INTERVIEWS=OFF \
   -DNRN_ENABLE_MPI=OFF \
   -DNRN_ENABLE_RX3D=OFF
  ```

Note that if you are building on Cray system with GNU toolchain, you have to set following environmental variable **before** calling above CMake comamnd:

```
export CRAYPE_LINK_TYPE=dynamic
```

4. Build the code:

  ```
  make -j
  make install
  ```

Particularly useful CMake options are (use **ON** to enable and **OFF** to disable feature):

* **-DNRN\_ENABLE\_BINARY_SPECIAL=ON** : Build special as a binary instead of shell script
* **-DNRN\_ENABLE\_INTERVIEWS=OFF** : Disable Interviews (native GUI support)
* **-DNRN\_ENABLE\_PYTHON=OFF** : Disable Python support
* **-DNRN\_ENABLE\_MPI=OFF** : Disable MPI support for parallelization
* **-DNRN\_ENABLE\_RX3D=OFF** : Disable rx3d support
* **-DNRN\_ENABLE\_CORENEURON=ON** : Enable CoreNEURON support
* **-DNRN\_ENABLE\_TESTS=ON** : Enable unit tests
* **-DPYTHON\_EXECUTABLE=/python/binary/path** : Use provided Python binary to build Python interface
* **-DCMAKE_INSTALL_PREFIX=/install/dir/path** : Location for installing
* **-DCORENRN\_ENABLE\_NMODL=ON** : Use [NMODL](https://github.com/BlueBrain/nmodl/) instead of [MOD2C](https://github.com/BlueBrain/mod2c/) for code generation with CoreNEURON

Please refer to [docs/cmake_doc/options.rst](docs/cmake_doc/options.rst) for more information on the CMake options.

## Developer documentation

Please refer to [docs/README.md](docs/README.md)
