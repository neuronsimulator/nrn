[![Build Status](https://dev.azure.com/neuronsimulator/nrn/_apis/build/status/neuronsimulator.nrn?branchName=master)](https://dev.azure.com/neuronsimulator/nrn/_build/latest?definitionId=1&branchName=master) [![Actions Status](https://github.com/neuronsimulator/nrn/actions/workflows/windows.yml/badge.svg?branch=master)](https://github.com/neuronsimulator/nrn/actions) [![Actions Status](https://github.com/neuronsimulator/nrn/workflows/NEURON%20CI/badge.svg)](https://github.com/neuronsimulator/nrn/actions) [![codecov](https://codecov.io/gh/neuronsimulator/nrn/branch/master/graph/badge.svg?token=T7PIDw6LrC)](https://codecov.io/gh/neuronsimulator/nrn) [![Documentation Status](https://readthedocs.org/projects/nrn/badge/?version=latest)](http://nrn.readthedocs.io/?badge=latest)

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

If you want to build the latest version from source, we support **CMake** as build system. **Autotools** build system has been removed after 8.0 release.
See detailed installation instructions: [docs/install/install_instructions.md](docs/install/install_instructions.md).

It is possible to install the Linux Python wheels on Windows via the Windows Subsystem for Linux (WSL) - check the installation instructions above.

## Documentation

* See documentation section of the [NEURON website](https://neuron.yale.edu/neuron/docs)
* See [https://nrn.readthedocs.io/en/latest/](https://nrn.readthedocs.io/en/latest/) for latest, nightly snapshot
* See [docs/README.md](docs/README.md) for information on documentation (local build, Read the Docs setup)

## Changelog

Refer to [docs/changelog.md](docs/changelog.md)

## Contributing to NEURON development

Refer to [NEURON contribution guidelines](CONTRIBUTING.md)

## Funding

NEURON development is supported by NIH grant R01NS11613 (PI M.L. Hines at Yale University).

Collaboration is provided by the Blue Brain Project, a research center of the École polytechnique fédérale de Lausanne (EPFL) with funding from the Swiss government's ETH Board of the Swiss Federal Institutes of Technology. Additional funding from the European Union Seventh Framework Program (FP7/20072013) under grant agreement no. 604102 (HBP) and the European Union's Horizon 2020 Framework Programme for Research and Innovation under Specific Grant Agreement no. 720270 (Human Brain Project SGA1), no. 785907 (Human Brain Project SGA2) and no. 945539 (Human Brain Project SGA3).

