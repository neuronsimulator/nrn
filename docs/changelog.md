# NEURON 8.0

## 8.0.2
_Release Date_ : 02-02-2022


### Bug Fixes

Python 3.10 bugfixes for RXD and Windows, more details in [GitHub PR #1614](https://github.com/neuronsimulator/nrn/pull/1614)


## 8.0.1
_Release Date_ : 28-01-2022


### What's New

- Python 3.10 support & wheels

### Bug Fixes

For the complete list of bug fixes, see the list in [GitHub PR #1603](https://github.com/neuronsimulator/nrn/pull/1603)

### Improvements /  Other Changes

- Introduce a Sphinx HOC domain for NEURON documentation 


## 8.0.0
_Release Date_ : 30-04-2021


### What's New

- Dynamic selection of legacy vs modern units using HOC/Python API (**default** : modern)
- Faster reaction-diffusion support
- Initial GPU support using integration of CoreNEURON
- Binary installer for new Apple M1 platform
- Binary wheel distribution for Python 3.9 and Python 2.7
- Release of NMODL version 0.3 (available as python wheel)
- Versioned documentation available via [nrn.readthedocs.io](https://nrn.readthedocs.io/en/latest/)
- CMake as a primary build system for NEURON and Interviews


### Breaking Changes
- `h.Section` now interprets positional arguments as `name`, `cell`. Previously positional arguments were interpreted in the other order. (Calling it with keyword arguments is unchanged.)
-  For 3d reaction-diffusion simulations, the voxelization and segment mapping algorithms have been adjusted, especially around the soma. Voxel indices and sometimes counts will change from previous versions.

### Deprecations
- Five functions in the `neuron` module: `neuron.init`, `neuron.run`, `neuron.psection`, `neuron.xopen`, and `neuron.quit`. 
- Autotools build is deprecated and will be removed in the next release. Use CMake instead.
- Python 2 and Python 3.5 support  is deprecated and will be removed in the next release. Use `Python >= 3.6`

### Bug Fixes

For the complete list of bug fixes, see the list on the [GitHub here](https://github.com/neuronsimulator/nrn/issues/1211#issuecomment-826919173).

### Improvements /  Other Changes
- Allow for two point (single section) SWC somas 
- GitHub Actions and Azure as primary CI systems. Travis CI removed.
- GitHub [Releases](https://github.com/neuronsimulator/nrn/releases) provides full source tarballs, binary installers and python wheels.
- Improved testing and CI infrastructure including GPUs
- [nrn-build-ci](https://github.com/neuronsimulator/nrn-build-ci) repository test nightly builds for Ubuntu 18.04, Ubuntu 20.04, Fedora 32, Fedora 33, CentOS7, CentOS8, Debian Buster (10) and macOS 10.15 platforms.
- Improved integration of CoreNEURON
- Support for recent numpy version
- Various build improvements on Linux, MacOS and HPC platforms
- Documentation from various repositories is consolidated under `nrn` repository 
- New releases via Spack and Easybuild package managers 
- Fix deadlock when compiling NEURON with AVX-512
- Add backward-cpp for better backtraces
- NEURON_MODULE_OPTIONS environment variable to pass in nrniv options before `neuron import` 

### Upgrade Steps

Existing models should work without any changes.  In order to upgrade NEURON version you can:
- Use python wheels provided for Linux or Mac OS platform
- Use binary installer provided for windows
- Install from source, preferably using CMake build system 
- For new version, it's always a good idea to start over from scratch with nrnivmodl (deleting existing directory like `x86_64`)

See `Installation` section under [nrn.readthedocs.io/](nrn.readthedocs.io/). In the very rare case that numerical differences exist, check selection of legacy vs modern units.

# Contributors

See the list of contributors on respective GitHub projects:
- NEURON : [https://github.com/neuronsimulator/nrn/graphs/contributors](https://github.com/neuronsimulator/nrn/graphs/contributors)
- CoreNEURON : [https://github.com/BlueBrain/CoreNeuron/graphs/contributors](https://github.com/BlueBrain/CoreNeuron/graphs/contributors)
- NMODL : [https://github.com/BlueBrain/nmodl/graphs/contributors](https://github.com/BlueBrain/nmodl/graphs/contributors)

# Feedback / Help

- Software related issues should be reported on GitHub : [https://github.com/neuronsimulator/nrn/issues/new/choose](https://github.com/neuronsimulator/nrn/issues/new/choose)
- For developing models and any scientific questions, see NEURON forum : [https://www.neuron.yale.edu/phpBB/](https://www.neuron.yale.edu/phpBB/)
- If you want to participate / contribute to the development of NEURON, you can join monthly developers meeting : [https://github.com/neuronsimulator/nrn/wiki](https://github.com/neuronsimulator/nrn/wiki)
