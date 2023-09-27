# NEURON 8.2

## 8.2.3
_Release Date_ : 15-09-2023

### What's New

- The primary purpose of 8.2.3 is to fix the HOC cursor control problems of
  the wheel and windows installed versions.
- Many fragments from current master to allow building of installers.
  with current compiler toolchains and github actions.


### Bug Fixes

- Fix MacOS linux wheel HOC backspace.
- Fix Windows HOC cursor issues.
- Fix Windows 11 HOC icon.
- Fix Windows and MacOS segfault on multiline HOC statements input from terminal.
- Fix build issues with current compiler toolchains and github actions.
- Deal with .inputrc if missing on Windows

## 8.2.2
_Release Date_ : 15-12-2022

### What's New

- Python 3.11 support
- Re-enabled MUSIC support in NEURON (#1896)
- CVode.poolshrink(1) deletes unused mechanism pools. (#2033)

### Bug Fixes

- Bugfix for rxd reactions involving rxd parameters. #1933

### Improvements /  Other Changes

- Documentation
  - windows build updates (#1991)
    - how to build NEURON
    - adapt scripts for local usage
  - added NMODL documentation in various constructs in hoc and python (#2011)
- SETUPTOOLS_USE_DISTUTILS=stdlib (#1924)
- Update CoreNEURON submodule to latest version

For the complete list of commits, see the list in [GitHub Issue #2109](https://github.com/neuronsimulator/nrn/issues/2109)

## 8.2.1
_Release Date_ : 12-08-2022

### What's New

- Change mcomplex.dat normalization to hh/5. (#1895)
- Jupyter support for ModelView #1907

### Bug Fixes

- nrnivmodl_core_makefile: fix SDKROOT (#1942)
- First time declaration of section in template must be at command level. (#1914)
- setup.exe installer must distribute env.exe (https://github.com/neuronsimulator/nrn/pull/1941)

### Improvements /  Other Changes

- Documentation
  - added new INCF/CNS 2022 online material (#1932)
  - updates (dealing with sims, generating movie, modelview, more #1925 )
  - transfer from Yale website (#1867) 
- nrnmpi_load: drop printf for already loaded lib (#1938)

For the complete list of commits, see the list in [GitHub Issue #1944](https://github.com/neuronsimulator/nrn/issues/1944)

## 8.2.0
_Release Date_ : 01-07-2022

### What's New
* Allow multiple BEFORE/AFTER blocks of same type in a MOD file. #1722
* Several documentation updates, including randomness in NEURON models #1727,
  NEURON course exercise sets from 2018 #1735 and publications using NEURON #1819.
* CMake: improved documentation targets. (#1725)  

### Breaking Changes
* Support for Python 3.6 was dropped, as it has reached end-of-life (#1733).
* In this release, more declarations of NEURON methods (typically `nrn_` and
  `hoc_` functions) are implicitly included in translated MOD files. This can
  cause compilation errors with MOD files that include incorrect declarations
  of these methods in `VERBATIM` blocks. (#1755, #1811, #1825).
* Declaring STATE variables as GLOBAL is now disallowed. (#1723)

### Deprecations & future changes
* NEURON is in the process of being fully migrated to a `C++` codebase.  
  Starting with next major release `9.0.0`, `MOD` files will be converted to `C++` instead of `C`.
  This will break compatibility with some legacy MOD files containing VERBATIM blocks and code may have to be updated
  given that some valid C code is not valid C++.
  A migration [guide](https://github.com/neuronsimulator/nrn/blob/80778700048f3aeefd35477308151bf7dd118941/docs/guide/porting_mechanisms_to_cpp.rst)
  is being compiled in the latest online documentation (note that it may change post release).
* `Random123` will be the default random number generator in NEURON and most of the random number distributions implementations will be
  replaced by those in the C++ standard library, while a few will be discontinued. See  [#1330](https://github.com/neuronsimulator/nrn/issues/1330) for follow-up and questions.

### Bug Fixes
* Fix for #335: Return proper exit code in hoc execution #1633
* Build on Ubuntu21.10 WSL: escape special characters (#1862)

### Improvements /  Other Changes
* Support for using `mallinfo2()` (#1805)
* HOC domain for Sphinx `5.0.1` 

### Upgrade Steps
* If your MOD files contain VERBATIM blocks you may need to refer to the aforementioned
  [C++ migration guide](https://github.com/neuronsimulator/nrn/blob/80778700048f3aeefd35477308151bf7dd118941/docs/guide/porting_mechanisms_to_cpp.rst)
  and make minor changes to prepare for the upcoming `9.0.0` release.
  Remaining compatible with `8.2.0` and older versions is typically straightforward.

For the complete list of features and bug fixes, see the list in [GitHub Issue #1879](https://github.com/neuronsimulator/nrn/issues/1879)

# NEURON 8.1

## 8.1.0
_Release Date_ : 25-03-2022

### What's New

* Python wheels
  * Support for ARM (Aarch64) Platform wheels via CircleCI - Python 3.6 to 3.10 (#1676)
  * Apple M1 Platform wheels - Python 3.8 to 3.10 (#1649)
  * NVIDIA GPU enabled wheels
    * First iteration released under a separate package: `neuron-gpu`
      * Must use NVHPC 21.1 SDK when you compile your MOD files (#1657)
  * Improved CoreNEURON support (#1705)
    * Use vendor compilers like Intel, Cray or NVHPC for better performance when you compile MOD files using `nrnivmodl`.
  * Support for HPE MPT MPI library (#1498)
  * Linux wheels are now `manylinux2014` (#1365)
* MacOS `universal2` package installer now available - Python 3.8 to 3.10
* CoreNEURON
  * Tight integration for both in-memory and as well as file transfer mode
  * Extend CoreNEURON POINTER transfer to any RANGE variable in a NRN_THREAD (#1622)
  * Support for BEFORE/AFTER constructs in MOD file (#1581)
* RXD
  * SaveState support added (#1586)
  * Dynamic extracellular tortuosity and volume fraction (#1260)
  * 3D support for importing multiple morphologies and for moving imported morphologies (#1147)

* Dedicated CI repository [nrn-build-ci](https://github.com/neuronsimulator/nrn-build-ci) to check builds on different distributions. Check this if you have build issues.
* Documentation
  * Integrated various documentation & tutorials from neuron.yale.edu website to the readthedocs (#1674)
  * Added training videos, guides and a list of NEURON publications
  * CoreNEURON: usage, MOD files compatibility/migration, profiling and (GPU) performance benchmarking
  * RxD programmer's reference expanded and rewritten (#1680)
  * Propose changes directly from ReadTheDocs - click on `Edit on GitHub`

### Breaking Changes
* Models using `_NrnThread` in a VERBATIM block will fail now. Change it to `NrnThread`.[See #1609](https://github.com/neuronsimulator/nrn/pull/1609).
* Minimum CMake version supported is now 3.15.0 (#1408)
* Python2 and Python3.5 support dropped
* Legacy internal `readline` source code is removed, must have Readline installed (#1371)
* Python wheels are now manylinux2014 (#1365)

### Deprecations
* NEURON is being migrated to a C++ codebase and beginning with the next release `MOD` files will be converted to `C++` instead of `C`.
  This might break compatibility with some legacy MOD files containing `VERBATIM` blocks.
  We will update the NEURON documentation describing how one can make MOD files compatible with this change.
* Python 3.6 Is End-of-Life - drop support after 8.1 release (#1651)

### Bug Fixes
* Can use ParallelContext.mpiabort_on_error(0) to say do not call to MPI_Abort on error (#1567)
* OpenMPI initialisation crash: argv[argc] should be null. (#1682)
* Avoid segfault if error during construction of POINT_PROCESS (#1627)
* Fixes a 1D/3D voxelization problem where frusta are outside the 3D grid. (#1227)
* Allow for two point (single section) SWC somas (#1144)
* Fix for current response in 3D reaction-diffusion simulations (#1721)
* `rxdmath.abs` no longer raises an exception (#1545)
* Fixes for 3D reaction-diffusion grid alignment edge cases (#1471, #1227)
* Update coreneuron : gpu bugfix for the BBP models (#1374)
* Fix UndefinedBehaviourSanitizer errors. (#1518)
* import3d_gui bugfix for HOC classes and top-level (#1159)

### Improvements / Other Changes
* Documentation
  * sphinx jupyter notebooks integration via `nbsphinx` (#1684)
  * Update CoreNEURON documentation. (#1650)
  * Building python wheels & managing NEURON Docker Images (#1469)
  * Short tutorial on using the profiler interface (#1453)
  * docs: Updated several rxd tutorials (#1061)
  * docs: WSL & Linux Python Wheel (#1496)
  * Update GPU profiling documentation (#1538)
* Testing and coverage
  * Integration of tqperf model for CI and CoreNEURON testing (#1556)
  * Add olfactory bulb mode as external test (#1541)
  * Add reduced dentate model tests (#1515)
  * Add BBP channel-benchmark to CTest suite. (#1439)
  * Make neuronsimulator/nrntest part of the CI (#1429)
* Refactorings
  * Move codebase to CPP  (#763)
  * Remove all code related to NRN_REALTIME and NRN_DAQ  (#1401)
  * Drop legacy hash implementations: nrnhash (#1403)
  * Remove absolute paths from generated code for ccache builds (#1574)
  * Using std::unordered_map as Gid2PreSyn/Int2TarList (#1568)
  * nrnivmodl succeeds when there are no mod files (#1583)
  * Fix UndefinedBehaviourSanitizer errors. (#1518)
  * Remove copy of readline (#1396)
  * Remove multisend specific for BlueGene (DMA) (#1294)
* Improvements
  * Possible to prefer python from $PATH when building from source with CMake #1659
  * User can use MPI_LIB_NRN_PATH to provide MPI library on linux platform (#1172)
  * MPI Multisend available in NEURON by default (#1537)
  * modlunit: Add CHARGE keyword and map it to VALENCE. (#1508)
  * Allow user to specify dialog popup location. (#1487)
  * Allow python section.disconnect(). (#1451)
  * Implement BBSaveState for Python cells (#1355)
  * macos build_wheels: add /usr/x11 to CMAKE_PREFIX_PATH (#1565)
  * Add NRN_NMODL_CXX_FLAGS to facilitate cross compilation (#1174)
* RXD
  * Faster convergence rate for surface voxel partial volume estimation (#1555)
  * Internal API for saving/restoring 3D voxelization (#1476)
  * Prevent RxD keeping objects alive (#1270, #1103, #1072)
  * Improved assignment of 3D voxels to segments (#1149)
  
### Upgrade Steps
* Linux wheels are now `manylinux2014`: upgrade your `pip`
* Legacy internal `readline` source code is removed: install `readline` on your system
* Python2 and Python3.5 support dropped: migrate to Python 3.7+ (3.6 is deprecated)

For the complete list of features and bug fixes, see `Commits going into 8.1.0` in [GitHub Issue #1719](https://github.com/neuronsimulator/nrn/issues/1719)

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
