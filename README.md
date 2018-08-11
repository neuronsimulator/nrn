[![Build Status](https://travis-ci.org/nrnhines/nrn.svg?branch=master)](https://travis-ci.org/nrnhines/nrn)

NEURON is a simulator for models of neurons and networks of neuron.
See http://neuron.yale.edu for installers, source code,
documentation, tutorials, announcements of courses and conferences,
and a discusson forum.

Basic installation on Linux from the nrn...tar.gz file is:

  Build InterViews first from the git repository at
  http://github.org/neuronsimulator/iv or the iv...tar.gz file at
  http://neuron.yale.edu/ftp/neuron/versions/alpha/

```
  ./configure
  make
  make install
```

If sources are obtained from the git repository,
http://github.org/neuronsimulator/nrn ,
create the automake, autoconf, libtool generated files by:
  sh build.sh

Particularly useful configure options:

```
  --prefix=`pwd`
    Install in place
  --without-x
    If the InterViews graphics library is not installed.
  --with-iv=/where/you/installed/it
    If iv was not installed in <prefix>/../iv
  --with-paranrn
    Parallel models on cluster computers using MPI (openmpi or mpich2)
  --with-nrnpython
    Use Python as an alternative interpreter (as well as the native
    HOC interpreter). This is required to use the Reaction-Diffusion
    extension.
```

For more installation information see:
http://neuron.yale.edu/neuron/download/getdevel

