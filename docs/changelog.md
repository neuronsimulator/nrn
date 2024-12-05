# NEURON 9.0

## NEURON 9.0.0
_Release Date_ : 24-12-2024

NEURON 9.0 marks a major milestone with significant under-the-hood updates.
This release features a redesign of internal data structures using the
Structure-Of-Arrays (SoA) memory layout, migration of the codebase and translation
of MOD files to C++, and the introduction of random number generator construct natively
in the NMODL language. Additionally, many legacy NMODL constructs have been removed,
and legacy code and libraries have been replaced with modern alternatives
like Eigen, ensuring improved performance and maintainability. DataHandles (pointers) into
the SoA memory layout are valid (point to the correct value) regardless of how memory is
internally reorganized or reallocated for purposes of higher simulation performance. 

### What’s New

- Use of SoA memory layout for internal data structures and DataHandles instead of pointers into that memory. ([#2027](https://github.com/neuronsimulator/nrn/issues/2027), [#2381](https://github.com/neuronsimulator/nrn/issues/2381), [#2712](https://github.com/neuronsimulator/nrn/issues/2712))
- Support for random number generator language construct (RANDOM) in NMODL ([#2627](https://github.com/neuronsimulator/nrn/issues/2627))
- Replace pthread-based threading implementation with std::thread ([#1859](https://github.com/neuronsimulator/nrn/issues/1859))
- Introduce meaningful Python types: HOC classes associated with Python types ([#1858](https://github.com/neuronsimulator/nrn/issues/1858))
- Formalization of C API for NEURON (first version) ([#2357](https://github.com/neuronsimulator/nrn/issues/2357))
- Introduce nanobind to gradually replace C Python API usage ([#2545](https://github.com/neuronsimulator/nrn/issues/2545))
- Requires C++17 compiler and flex >= 2.6 ([#1893](https://github.com/neuronsimulator/nrn/issues/1893))
- CoreNEURON repository is merged into NEURON ([#2055](https://github.com/neuronsimulator/nrn/issues/2055))
- Support for `numpy>=2` added ([#3040](https://github.com/neuronsimulator/nrn/issues/3040))
- Replace legacy Meschach source copy with Eigen library ([#2470](https://github.com/neuronsimulator/nrn/issues/2470))

### Breaking Changes

- MOD files are compiled as C++ instead of C (see [migration guide](guide/porting_mechanisms_to_cpp.rst#porting-mechanisms-to-cpp) for VERBATIM blocks)
- API changes in the functions related to random numbers, see [#2618](https://github.com/neuronsimulator/nrn/issues/2618)
- Restored usage of TABLE statement in hh.mod, results are now same as 8.0.2 (see [#1764](https://github.com/neuronsimulator/nrn/issues/1764))
- NMODL: Disallow use of ion variable in the CONSTANT block ([#1955](https://github.com/neuronsimulator/nrn/issues/1955))
- NMODL: Disallow declaring variables and functions with the same name ([#1992](https://github.com/neuronsimulator/nrn/issues/1992))
- Usage of Eigen library could introduce floating point differences, but they are very small, validated and can be ignored


### Removal / Deprecation

- Removed usage of legacy OS X carbon libraries ([#1869](https://github.com/neuronsimulator/nrn/issues/1869))
- Removed legacy LINDA code and Java bindings ([#1919](https://github.com/neuronsimulator/nrn/issues/1919), [#1937](https://github.com/neuronsimulator/nrn/issues/1937))
- Removed unused NMODL methods: adams, heun, clsoda, seidel, simplex, gear ([#2032](https://github.com/neuronsimulator/nrn/issues/2032))
- Removed support for Python 3.7 and 3.8 ([#2194](https://github.com/neuronsimulator/nrn/issues/2194), [#3108](https://github.com/neuronsimulator/nrn/issues/3108))
- Removed support for legacy units and dynamic units selection ([#2539](https://github.com/neuronsimulator/nrn/issues/2539))
- Removed unused NEMO simulator compatibility ([#3035](https://github.com/neuronsimulator/nrn/issues/3035))
- Removed checkpoint feature (`h.checkpoint()`), superseded by SaveState ([#3092](https://github.com/neuronsimulator/nrn/issues/3092))
- Removed legacy Random.MLCG and Random.ACG random number generators ([#3189](https://github.com/neuronsimulator/nrn/issues/3189), [#3190](https://github.com/neuronsimulator/nrn/issues/3190))
- Removed support for mod2c transpiler for CoreNEURON ([#2247](https://github.com/neuronsimulator/nrn/issues/2247))
- Removed unused NMODL constructs: PUTQ, GETQ, RESET, MATCH, TERMINAL, SECTION,
  IFERROR, MODEL_LEVEL, PLOT, SENS, etc. ([#1956](https://github.com/neuronsimulator/nrn/issues/1956), [#1974](https://github.com/neuronsimulator/nrn/issues/1974), [#1975](https://github.com/neuronsimulator/nrn/issues/1975), [#2001](https://github.com/neuronsimulator/nrn/issues/2001), [#2004](https://github.com/neuronsimulator/nrn/issues/2004), [#2005](https://github.com/neuronsimulator/nrn/issues/2005))
- Removed `__MWERKS__` (CodeWarrior compiler) related code ([#2655](https://github.com/neuronsimulator/nrn/issues/2655))
- Removed obsolete uxnrnbbs code ([#2203](https://github.com/neuronsimulator/nrn/issues/2203))
- Removed old tqueue implementations ([#2225](https://github.com/neuronsimulator/nrn/issues/2225), [#2740](https://github.com/neuronsimulator/nrn/issues/2740))


### Bug Fixes

- Fix segfault from unref hoc_obj, see [#1857](https://github.com/neuronsimulator/nrn/issues/1857) ([#1917](https://github.com/neuronsimulator/nrn/issues/1917))
- Fix bug in rxd reactions involving rxd parameters ([#1933](https://github.com/neuronsimulator/nrn/issues/1933))
- Fix CoreNEURON bug when checkpointing VecPlay instances ([#2148](https://github.com/neuronsimulator/nrn/issues/2148))
- Fix nocmodl bug with unused STATE + COMPARTMENT variable in KINETIC block ([#2210](https://github.com/neuronsimulator/nrn/issues/2210))
- Fix for dynamic ECS diffusion characteristics ([#2229](https://github.com/neuronsimulator/nrn/issues/2229))
- Various fixes for compatibility with newer NVHPC compiler releases ([#2239](https://github.com/neuronsimulator/nrn/issues/2239), [#2591](https://github.com/neuronsimulator/nrn/issues/2591))
- Fix Windows and Mac crash on multiline HOC statements input from terminal ([#2258](https://github.com/neuronsimulator/nrn/issues/2258))
- Fix for Vector.max_ind ([#2251](https://github.com/neuronsimulator/nrn/issues/2251))
- Fix for detecting ION channel type in LoadBalance ([#2310](https://github.com/neuronsimulator/nrn/issues/2310))
- Fix for RangeVarPlot adding extra point at section ends when using plotly ([#2410](https://github.com/neuronsimulator/nrn/issues/2410))
- Fixed mod file array variable assignment (seg.mech.array[i] = x) ([#2504](https://github.com/neuronsimulator/nrn/issues/2504))
- Various fixes in RxD features ([#2593](https://github.com/neuronsimulator/nrn/issues/2593), [#2588](https://github.com/neuronsimulator/nrn/issues/2588))
- Fix CoreNEURON bug causing in-memory model copy with file mode ([#2767](https://github.com/neuronsimulator/nrn/issues/2767))
- Fix CoreNEURON bug when using array variables in MOD files ([#2779](https://github.com/neuronsimulator/nrn/issues/2779))
- Fix lexer for ONTOLOGY parsing ([#3091](https://github.com/neuronsimulator/nrn/issues/3091))
- Fix issue while using Anaconda Python on MacOS ([#3088](https://github.com/neuronsimulator/nrn/issues/3088))

### Other Improvements and Changes

- Allow `segment.mechanism.func(args)` to call a FUNCTION or PROCEDURE ([#2475](https://github.com/neuronsimulator/nrn/issues/2475))
- Simplify INDEPENDENT statement as only `t` variable is accepted ([#2013](https://github.com/neuronsimulator/nrn/issues/2013))
- Fix various memory leaks and related improvements ([#3255](https://github.com/neuronsimulator/nrn/issues/3255), [#3257](https://github.com/neuronsimulator/nrn/issues/3257), [#3243](https://github.com/neuronsimulator/nrn/issues/3243), [#2456](https://github.com/neuronsimulator/nrn/issues/2456))
- Support for reading reporting related information with in-memory mode ([#2555](https://github.com/neuronsimulator/nrn/issues/2555))
- NMODL language documentation update ([#2152](https://github.com/neuronsimulator/nrn/issues/2152), [#2771](https://github.com/neuronsimulator/nrn/issues/2771), [#2885](https://github.com/neuronsimulator/nrn/issues/2885))
- Documentation improvements including transfer from nrn.yale.edu to nrn.readthedocs.io ([#1901](https://github.com/neuronsimulator/nrn/issues/1901), [#2922](https://github.com/neuronsimulator/nrn/issues/2922), [#2971](https://github.com/neuronsimulator/nrn/issues/2971), [#3106](https://github.com/neuronsimulator/nrn/issues/3106), [#3187](https://github.com/neuronsimulator/nrn/issues/3187))
- Various test, CI, and build infrastructure improvements ([#1991](https://github.com/neuronsimulator/nrn/issues/1991), [#2260](https://github.com/neuronsimulator/nrn/issues/2260), [#2474](https://github.com/neuronsimulator/nrn/issues/2474))
- Migrate many C code parts to C++ ([#2083](https://github.com/neuronsimulator/nrn/issues/2083), [#2305](https://github.com/neuronsimulator/nrn/issues/2305))
- Support online LFP calculation in CoreNEURON (via SONATA reports for BBP use cases) ([#2118](https://github.com/neuronsimulator/nrn/issues/2118), [#2360](https://github.com/neuronsimulator/nrn/issues/2360))
- Adopt CoreNEURON cell permute functionality into NEURON ([#2598](https://github.com/neuronsimulator/nrn/issues/2598))
- Allow legacy `a.b(i)` syntax for 1d arrays (extend to `PointProcess.var[i]` and `ob.dblarray[i]`) ([#2256](https://github.com/neuronsimulator/nrn/issues/2256))
- Add an ability to run NEURON ModelDB CI using label ([#2108](https://github.com/neuronsimulator/nrn/issues/2108))
- Remove unused code parts under various preprocessor variables ([#2007](https://github.com/neuronsimulator/nrn/issues/2007))
- Return error/exit codes properly in `nrniv -c` and nrnivmodl ([#1871](https://github.com/neuronsimulator/nrn/issues/1871))
- No limit on number of ion mechanisms that can be used ([#3089](https://github.com/neuronsimulator/nrn/issues/3089))
- Added Jupyter support for ModelView ([#1907](https://github.com/neuronsimulator/nrn/issues/1907))
- Make notebooks work with `bokeh>=3` ([#3061](https://github.com/neuronsimulator/nrn/issues/3061))
- Added new method `ParallelContext.get_partition()` ([#2351](https://github.com/neuronsimulator/nrn/issues/2351))

# NEURON 8.2

## 8.2.6
_Release Date_ : 24-07-2024

This release pins numpy to <2 and includes backports for several fixes.

### Bug Fixes
- Informative error when cannot import hoc module
- ParallelContext: hoc_ac_ encodes global id of submitted process.
- Python 3.12 compatibility with Windows installer (#2963)
- Windows 11 fix for nrniv -python (#2946)
- Fix for dynamic ECS diffusion characteristics.
- python38 is back. For testing can use rx3doptlevel=0 bash bldnrnmacpkgcmake.sh
- Fix cvode.use_fast_imem(1) error with electrode time varying conductance.
- Apple M1 cmake failure in cloning subrepository. See #2326.
- Update iv

## 8.2.4
_Release Date_ : 08-02-2024


### What's New

This release brings no new features, just bugfixes and minor improvements.


### Bug Fixes
- Python 3.12 compatibility fixes
    - replace distutils with setuptools
    - fix segfault on exit
- updates to CI
    - move from MacOS 11 to MacOS 12
    - add MUSIC
    - bugfix for coverage CI
- small bugfix for edge case in RX3D
- unified setup.py
- misc cmake improvements

### Improvements /  Other Changes
- Disabled RXD code in notebook to avoid breaking docs build (see [GitHub Issue #2710](https://github.com/neuronsimulator/nrn/issues/2710))
- Disabled RXD tests on Windows (see [GitHub Issue #2585](https://github.com/neuronsimulator/nrn/issues/2585))


For the complete list of commits check  [GitHub Issue #2700](https://github.com/neuronsimulator/nrn/issues/2700)

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
