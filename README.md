[![Build Status](https://api.travis-ci.org/neuronsimulator/nrn.svg?branch=master)](https://travis-ci.org/neuronsimulator/nrn) [![Build Status](https://dev.azure.com/neuronsimulator/nrn/_apis/build/status/neuronsimulator.nrn?branchName=master)](https://dev.azure.com/neuronsimulator/nrn/_build/latest?definitionId=1&branchName=master) [![Actions Status](https://github.com/neuronsimulator/nrn/workflows/Windows%20Installer/badge.svg)](https://github.com/neuronsimulator/nrn/actions) [![Actions Status](https://github.com/neuronsimulator/nrn/workflows/NEURON%20CI/badge.svg)](https://github.com/neuronsimulator/nrn/actions)

# NEURON
NEURON is a simulator for models of neurons and networks of neuron. See [http://neuron.yale.edu]
(http://neuron.yale.edu) for installers, source code, documentation, tutorials, announcements of
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
- Autotools (legacy, minimum support)

Note that starting with the 8.0 release, we recommend users to use CMake as the primary build system
for NEURON. We would be grateful for any feedback or issues you encounter using the CMake-based build
system. Please [report any issue here](https://github.com/neuronsimulator/nrn/issues) and we will be
happy to help.

**If you are using autotools, we highly recommend switching to CMake.**

For detailed installation instructions see [INSTALL file](INSTALL.md) file.

## Documentation

* See documentation section of the [NEURON website](https://neuron.yale.edu/neuron/docs)
* See [docs/README.md](docs/README.md) for developers documentation
* See [github.io](http://neuronsimulator.github.io/nrn/) for latest, nightly snapshot
