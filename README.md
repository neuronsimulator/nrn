[![Build Status](https://api.travis-ci.org/neuronsimulator/nrn.svg?branch=master)](https://travis-ci.org/neuronsimulator/nrn)

NEURON is a simulator for models of neurons and networks of neuron.
See http://neuron.yale.edu for installers, source code,
documentation, tutorials, announcements of courses and conferences,
and a discusson forum.

## Building NEURON


### Building using autotools

Basic installation on Linux from the nrn...tar.gz file is:

  Build InterViews first from the git repository at
  http://github.com/neuronsimulator/iv or the iv...tar.gz file at
  http://neuron.yale.edu/ftp/neuron/versions/alpha/

```
  ./configure
  make
  make install
```

If sources are obtained from the git repository,
http://github.com/neuronsimulator/nrn ,
create the automake, autoconf, libtool generated files by:
  sh build.sh

Particularly useful configure options:


- `--prefix=/some/path` : Install in in this location of your filesystem
- `--without-x` : If the InterViews graphics library is not installed.
- `--with-iv=/where/you/installed/it` : If iv was not installed in <prefix>/../iv
- `--with-paranrn` : Parallel models on cluster computers using MPI (openmpi or mpich2)
- `--with-nrnpython` : Use Python as an alternative interpreter (as well as the native HOC interpreter).
  This is required to use the Reaction-Diffusion extension.


For more details see the [INSTALL.md](https://github.com/neuronsimulator/nrn/blob/master/INSTALL.md)
file.

### Building using cmake

NEURON can now also be built and installed using cmake. Currently we are evaluating the two build
system options but given positive feedback from the community we plan to depricate autotools and
completely switch to cmake. Therefor we would be grateful for any feedback.

To build with cmake we require following packages to be installed on the system:

- bison
- cmake >=3.3.0
- flex
- A C/C++ compiler suite (GCC, Intel Parallel Studio, clang,

Following packages are optional:

- python >=2.7, or python >=3.5
- cython
- MPI
- X11 (or XQuartz on macos)


1. Download the NEURON `.tar.gz` or clone from https://github.com/neuronsimulator/nrn

  ```
  git clone https://github.com/neuronsimulator/nrn
  cd nrn
  ```

2. Create a build directory

  ```
  mkdir build
  ```

3. Run cmake with the appropriate options

  ```
  cmake -DNRN_ENABLE_CORENEURON=OFF -DNRN_ENABLE_PYTHON=ON -DNRN_ENABLE_MPI=OFF -DNRN_ENABLE_IV=ON ..
  ```

4. Build the code

  ```
  make -j
  ```

For more installation information see:
http://neuron.yale.edu/neuron/download/getdevel

