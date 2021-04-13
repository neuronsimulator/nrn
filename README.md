[![Build Status](https://api.travis-ci.org/neuronsimulator/nrn.svg?branch=master)](https://travis-ci.org/neuronsimulator/nrn) [![Build Status](https://dev.azure.com/neuronsimulator/nrn/_apis/build/status/neuronsimulator.nrn?branchName=master)](https://dev.azure.com/neuronsimulator/nrn/_build/latest?definitionId=1&branchName=master) [![Actions Status](https://github.com/neuronsimulator/nrn/workflows/Windows%20Installer/badge.svg)](https://github.com/neuronsimulator/nrn/actions) [![Actions Status](https://github.com/neuronsimulator/nrn/workflows/NEURON%20CI/badge.svg)](https://github.com/neuronsimulator/nrn/actions) [![codecov](https://codecov.io/gh/neuronsimulator/nrn/branch/master/graph/badge.svg?token=T7PIDw6LrC)](https://codecov.io/gh/neuronsimulator/nrn) [![Documentation Status](https://readthedocs.org/projects/nrn/badge/?version=latest)](http://nrn.readthedocs.io/?badge=latest)

# NEURON
NEURON is a simulator for models of neurons and networks of neuron. See [http://neuron.yale.edu](http://neuron.yale.edu) for installers, source code, documentation, tutorials, announcements of
courses and conferences, and a discussion forum.

## Installing NEURON

NEURON provides binary installers for Linux, Mac and Windows platforms. You can find the latest
installers for Mac and Windows [here](https://neuron.yale.edu/ftp/neuron/versions/alpha/). For
Linux and Mac you can install the official Python 3 wheel with:

```
pip3 install neuron
```

If you want to build the latest version from source, currently we are supporting two build systems:

- CMake (__recommended__)
- Autotools (legacy, minimum support - __will be DROPPED in the next release__)

Note that starting with the 8.0 release, CMake is used as the primary build system for NEURON.
We would be grateful for any feedback or issues you encounter using the CMake-based build system.
Please [report any issue here](https://github.com/neuronsimulator/nrn/issues) and we will be
happy to help.

**If you are using autotools, we strongly recommend switching to CMake as of now.**

For detailed installation instructions see [docs/install/install_instructions.md](docs/install/install_instructions.md) file.

## Documentation

* See documentation section of the [NEURON website](https://neuron.yale.edu/neuron/docs)
* See [docs/README.md](docs/README.md) for developers documentation
* See [http://neuronsimulator.github.io/nrn/](http://neuronsimulator.github.io/nrn/) for latest, nightly snapshot
