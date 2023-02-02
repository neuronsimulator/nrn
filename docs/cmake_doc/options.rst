Introduction
============
The NEURON build system now uses cmake as of version 7.8 circa Nov 2019.
The previous autotools (./configure) build system has been removed after
8.0 release.

.. code-block:: shell

  git clone https://github.com/neuronsimulator/nrn nrn
  cd nrn
  mkdir build
  cd build
  cmake .. # default install to /usr/local
  make -j
  sudo make -j install

The ``-j`` option to make invokes a parallel make using all available cores.
This is often very much faster than a single process make. One can add a number
after the ``-j`` (e.g. ``make -j 6``) to specify the maximum number of processes
to use. This can be useful if there is the possibility of running out of memory.

The make targets that are made available by cmake can be listed with

.. code-block:: shell

  make help

You can list CMake options with
``cmake .. -LH``
which runs ``cmake ..`` as above and lists the cache variables along with help
strings which are not marked as INTERNAL or ADVANCED. Alternatively,

.. code-block:: shell

  ccmake ..

allows one to interactively inspect cached variables.
In the build folder, ``cmake -LH`` (missing <path-to-source>) will not
run cmake, but if there is a ``CMakeCache.txt`` file, the cache variables
will be listed.

The above default ``cmake ..`` specifies a default installation location
and build type, and includes (or leaves out) the following major
functional components.

.. code-block:: shell

  cmake .. -DCMAKE_INSTALL_PREFIX=/usr/local \
    -DCMAKE_BUILD_TYPE=RelWithDebInfo \
    -DNRN_ENABLE_INTERVIEWS=ON \
    -DNRN_ENABLE_MPI=ON \
    -DNRN_ENABLE_PYTHON=ON \
    -DNRN_ENABLE_CORENEURON=OFF

Cmake option values persist with subsequent invocations of cmake unless
explicitly changed by specifying arguments to cmake (or by modifying them
with ccmake). It is intended that all build dependencies are taken into
account so that it is not necessary to start fresh with an empty build
folder when modifying cmake arguments. However, there may be unknown
exceptions to this (bugs) so in case of problems it is generally sufficient
to delete all contents of the build folder and start again with the desired
cmake arguments.

General options
===============
First arg is always ``<path-to-source>`` which is the path (absolute or relative)
to the top level nrn folder (e.g. cloned from github). It is very common
to create a folder named ``build`` in the top level nrn folder and run cmake
in that. e.g.

 .. code-block:: shell

  cd nrn
  mkdir build
  cd build
  cmake .. <more args>

CMAKE_INSTALL_PREFIX:PATH=\<path-where-nrn-should-be-installed\>
----------------------------------------------------------------
  Install path prefix, prepended onto install directories.
  This can be a full path or relative. Default is /usr/local .
  A common install folder is 'install' in the build folder. e.g.

  .. code-block:: shell

    -DCMAKE_INSTALL_PREFIX=install

  so that the installation folder is ``.../nrn/build/install`` .
  In this case the user should prepend ``.../nrn/build/install/bin`` to PATH
  and it may be useful to

  .. code-block:: shell

    export PYTHONPATH=.../nrn/build/install/lib/python

  where in each case ``...`` is the full path prefix to nrn.

CMAKE_BUILD_TYPE:STRING=RelWithDebInfo
--------------------------------------
  Empty or one of Custom;Debug;Release;RelWithDebInfo;Fast.

  * RelWithDebInfo means to compile using -O2 -g options.
  * Debug means to compile with just -g (and optimization level -O0)
    This is very useful for debugging with gdb as, otherwise, local
    variables may be optimized away that are useful to inspect.
  * Release means to compile with -O2 -DNDEBUG.
    The latter eliminates assert statements.
  * Custom requires that you specify flags with CMAKE_C_FLAGS and CMAKE_CXX_FLAGS
  * Fast requires that you specify flags as indicated in nrn/cmake/ReleaseDebugAutoFlags.cmake

  Custom and Fast depend on specific compilers and (super)computers and are tailored to those
  machines. See ``nrn/cmake/ReleaseDebugAutoFlags.cmake``

Ninja
-----
  Use the Ninja build system (``make`` is the default CMake build system).

  .. code-block:: shell

    cmake .. -G Ninja ...
    ninja install

  Ninja can be faster than make during development when compiling
  just a few files. Some rough timings on a mac powerbook arm64 with and
  without -G Ninja for ``cmake .. -G Ninja -DCMAKE_INSTALL_PREFIX=install``
  are:

  .. code-block:: shell

    # Note: make executed in build-make folder, ninja executed in build-ninja folder.
    time make -j install) # 39s
    time ninja install    # 35s
    touch ../src/nrnoc/section.h
    time make -j          # 8.3s
    time ninja            # 7.4s

  On mac, install ninja with ``brew install ninja``

  ``ninja help`` prints the target names that can be built individually

  ``ninja -j 1`` does a non-parallel build.

  ``ninja -v`` shows each command.

InterViews options
==================

NRN_ENABLE_INTERVIEWS:BOOL=ON
-----------------------------
  Enable GUI with INTERVIEWS

  Unless you specify IV_DIR, InterViews will be automatically cloned as
  a subproject, built, and installed in CMAKE_INSTALL_PREFIX.

IV_DIR:PATH=<path-to-external-installation-of-interviews>
---------------------------------------------------------
  The directory containing a CMake configuration file for iv.

  IV_DIR is the install location of iv and the directory actually containing
  the cmake configuration files is ``IV_DIR/lib/cmake``.
  This is useful when you have many clones of nrn for different development
  purposes and wish to use a single independent InterViews installation
  for many/all of them. E.g. I generally invoke

.. code-block:: shell

  -DIV_DIR=$HOME/neuron/ivcmake/build/install

IV_ENABLE_SHARED:BOOL=OFF
-------------------------
  Build libraries shared or static

  I generally build InterViews static. The nrn build will then incorporate
  all of InterViews into libnrniv.so

IV_ENABLE_X11_DYNAMIC:BOOL=OFF
------------------------------
  dlopen X11 after launch

  This is most useful for building Mac distributions where XQuartz (X11) may
  not be installed on the user's machine and the user does not require
  InterViews graphics. If XQuartz is subsequently installed, InterViews graphics
  will suddenly be available.

IV_ENABLE_X11_DYNAMIC_MAKE_HEADERS:BOOL=OFF
-------------------------------------------
  Remake the X11 dynamic .h files.

  Don't use this. The scripts are very brittle and X11 is very stable.
  If it is ever necessary to remake the X11 dynamic .h files, I will
  do so and push them to the https://github.com/neuronsimulator/iv respository.

MPI options:
============

NRN_ENABLE_MPI:BOOL=ON
----------------------
  Enable MPI support

  Requires an MPI installation, e.g. openmpi or mpich. Note that the Python mpi4py module generally uses
  openmpi which cannot be mixed with mpich.

NRN_ENABLE_MPI_DYNAMIC:BOOL=OFF
-------------------------------
  Enable dynamic MPI library support

  This is mostly useful for binary distibutions where MPI may or may not
  exist on the target machine.

NRN_MPI_DYNAMIC:STRING=
-----------------------
  semicolon (;) separated list of MPI include directories to build against. Default to first found mpi)

  Cmake knows about openmpi, mpich, mpt, and msmpi. The dynamic loader for linux tries to load libmpi.so and if that fails, libmpich.so (the latter is good for cray mpi). The system then checks to see if a specific symbol exists in the libmpi... and determines whether to  load the libnrnmp_xxx.so for openmpi, mpich, or mpt. To make binary installers good for openmpi and mpich, I use

.. code-block:: shell

  -DNRN_MPI_DYNAMIC="/usr/local/include/;/home/hines/soft/mpich/include"

  This option is ignored unless NRN_ENABLE_MPI_DYNAMIC=ON

NRN_ENABLE_MUSIC:BOOL=OFF
-------------------------
  Enable MUSIC. MUlti SImulation Coordinator.

  MUSIC must already be installed. See https://github.com/INCF/MUSIC.
  Hints for MUSIC installation: use the switch-to-MPI-C-interface branch.
  Python3 must have mpi4py and cython modules. I needed a PYTHON_PREFIX, so
  on my Apple M1 used: ``./configure --prefix=`pwd`/musicinstall PYTHON_PREFIX=/Library/Frameworks/Python.framework/Versions/3.11 --disable-anysource``

  MPI and Python must be enabled.

  If MUSIC is installed but CMake cannot find its ``/path``, augment the
  semicolon separated list of paths ``-DCMAKE_PREFIX_PATH=...;/path;...``
  or pass the ``/path`` with ``-DMUSIC_ROOT=/path`` to cmake.
  CMake needs to find

  .. code-block:: shell

    /path/include/music.hh
    /path/lib/libmusic.so

  With the music installed above, cmake configuration example is
  ``build % cmake .. -G Ninja -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_INSTALL_PREFIX=install -DPYTHON_EXECUTABLE=`which python3.11` -DNRN_ENABLE_RX3D=OFF -DCMAKE_BUILD_TYPE=Debug -DNRN_ENABLE_TESTS=ON -DNRN_ENABLE_MUSIC=ON -DCMAKE_PREFIX_PATH=$HOME/neuron/MUSIC/musicinstall``

Python options:
===============

NRN_ENABLE_PYTHON:BOOL=ON
-------------------------
  Enable Python interpreter support
  (default python, fallback to python3, but see PYTHON_EXECUTABLE below)

NRN_ENABLE_PYTHON_DYNAMIC:BOOL=OFF
----------------------------------
  Enable dynamic Python version support

  This is mostly useful for binary distributions where it is unknown which
  version, if any, of python exists on the target machine.

NRN_PYTHON_DYNAMIC:STRING=
--------------------------
  semicolon (;) separated list of python executables to create interfaces. (default python3)

  If the string is empty use the python specified by PYTHON_EXECUTABLE
  or else the default python. Binary distributions often specify a list
  of python versions so that if any one of them is available on the
  target machine, NEURON + Python will be fully functional. Eg. the
  mac package build script on my machine, nrn/bldnrnmacpkgcmake.sh uses

  .. code-block:: shell

    -DNRN_PYTHON_DYNAMIC="python3.8;python3.9;python3.10;python3.11"

  This option is ignored unless NRN_ENABLE_PYTHON_DYNAMIC=ON

PYTHON_EXECUTABLE:PATH=
-----------------------
  Use provided python binary instead of the one found by CMake.
  This must be a full path. We generally use

  .. code-block:: shell

    -DPYTHON_EXECUTABLE=`which python3.8`

NRN_ENABLE_MODULE_INSTALL:BOOL=ON
---------------------------------
  Enable installation of the NEURON Python module. 
  By default, the NEURON module is installed in CMAKE_INSTALL_PREFIX/lib/python.

  Note: When building wheels, this must be set to OFF since the top-level `setup.py`
  is already building the extensions.


NRN_ENABLE_RX3D:BOOL=ON
-----------------------
  Enable rx3d support

  No longer any reason to turn this off as build time is not significantly
  increased due to compiling cython generated files with -O0 by default.

NRN_RX3D_OPT_LEVEL:STRING=0
---------------------------
  Optimization level for Cython generated files (non-zero may compile slowly)

  It is not clear to me if -O0 has significantly less performance than -O2.
  Binary distributions are (or should be) built with

  .. code-block:: shell

    -DNRN_RX3D_OPT_LEVEL=2

CoreNEURON options:
===================

NRN_ENABLE_CORENEURON:BOOL=OFF
------------------------------
  Enable CoreNEURON support

  If ON CoreNEURON will be built and any needed NMODL submodule dependencies
  cloned as external submodules.

NRN_ENABLE_MOD_COMPATIBILITY:BOOL=OFF
-------------------------------------
  Enable CoreNEURON compatibility for MOD files

  CoreNEURON does not allow the common NEURON THREADSAFE promotion of
  GLOBAL variables that appear on the right hand side of assignment statements
  to become thread specific variables. This option is
  automatically turned on if NRN_ENABLE_CORENEURON=ON.

Other CoreNEURON options:
-------------------------
  There are 20 or so cmake arguments specific to a CoreNEURON
  build that are listed in https://github.com/BlueBrain/CoreNeuron/blob/master/CMakeLists.txt.
  The ones of particular interest that can be used on the NEURON
  CMake configure line are `CORENRN_ENABLE_NMODL` and `CORENRN_ENABLE_GPU`.

Occasionally useful advanced options:
=====================================

  See all the options with ``ccmake ..`` in the build folder. They are
  also in the CMakeCache.txt file. Following is a definitely incomplete list.

CMAKE_C_COMPILER:FILEPATH=/usr/bin/cc
-------------------------------------
  C compiler

  On the mac, prior to knowing about
  ``export SDK_ROOT=$(xcrun -sdk macosx --show-sdk-path)``
  I got into the habit of

  .. code-block::

    -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++

  to avoid the problem of gcc not being able to find stdio.h when
  python was compiling inithoc.cpp

CMAKE_CXX_COMPILER:FILEPATH=/usr/bin/c++
----------------------------------------
  C plus plus compiler

NRN_ENABLE_DOCS:BOOL=OFF
------------------------
  Enable documentation targets in the build.
  This also makes all documentation dependencies into hard requirements, so
  CMake will report an error if anything is missing.
  There are five documentation targets:
    * ``doxygen`` generates Doxygen documentation from the NEURON source code.
    * ``notebooks`` executes the various Jupyter notebooks that are included in
      the documentation, so they contain both code and results, instead of just
      code. These are run in situ in the source tree, so if you run this target
      manually then make sure not to accidentally commit the results to git.
    * ``sphinx`` generates Sphinx documentation. This logically depends on
      ``notebooks``, as it generates HTML from the executed notebooks, but this
      dependency is not declared in the build system.
    * ``notebooks-clean`` removes the execution results from the Jupyter
      notebooks, leaving them in a clean state. This logically depends on
      ``sphinx``, as the execution results need to be converted to HTML before
      they are discarded, but this dependency is not declared in the build
      system.
    * ``docs`` is shorthand for building ``doxygen``, ``notebooks``, ``sphinx``
      and ``notebooks-clean`` in that order.

  .. warning::
    Executing the notebooks requires a functional NEURON installation.
    There are two possibilities here:
      * The default, which is sensible for local development, is that the
        ``notebooks`` target uses NEURON from the current CMake build directory.
        This implies that building the documentation builds NEURON too.
      * The alternative, which is enabled by setting
        ``-DNRN_ENABLE_DOCS_WITH_EXTERNAL_INSTALLATION=ON``, is that ``notebooks``
        does not depend on any other NEURON build targets. In this case you must
        provide an installation of NEURON by some other means. It will be assumed
        that commands like ``nrnivmodl`` work and that ``import neuron`` works
        in Python.

NRN_EXTRA_CXX_FLAGS:STRING=""
-----------------------------
  Compiler flags that are used to build NEURON code but not (unlike
  ``CMAKE_CXX_FLAGS``) code of dependencies built as submodules.
  This can be useful for tuning things like compiler warning flags.

NRN_EXTRA_MECH_CXX_FLAGS:STRING=""
----------------------------------
  Compiler flags that are used to build the C code generated by ``nocmodl`` but
  not source code files that are committed to the repository.

NRN_NMODL_CXX_FLAGS:STRING=""
-----------------------------
  Compiler flag to build tools like nocmodl, modlunit.

  In cluster environment with different architecture of login node
  and compute node, we need to compile tools like nocmodl and modlunit
  with different compiler options to run them on login/build nodes. This
  option appends provided flags to CMAKE_CXX_FLAGS.

  For example, with intel compiler compiling NEURON for KNL but building
  on a Skylake node:
  .. code-block::

    -DCMAKE_BUILD_TYPE=Custom -DCMAKE_CXX_FLAGS="-xMIC-AVX512" -DNRN_NMODL_CXX_FLAGS="-XHost"

Readline_ROOT_DIR:PATH=/usr
---------------------------
  Install directory prefix where readline is installed.

  If cmake can't find readline, you can give this hint with the directory
  path under which readline is installed. Note that on some platforms
  with multi-arch support (e.g. Debian/Ubuntu), CMake versions < 3.20 are not
  able to find readline library when NVHPC/PGI compiler is used (for GPU
  support). In this case you can install newer CMake (>= 3.20) or explicitly
  specify readline library using `-DReadline_LIBRARY=` option:
  .. code-block::

    -DReadline_LIBRARY=/usr/lib/x86_64-linux-gnu/libreadline.so

.. _cmake-nrn-enable-tests-option:

NRN_ENABLE_TESTS:BOOL=OFF
-------------------------
  Enable unit tests

  Clones the submodule catch2 from https://github.com/catchorg/Catch2.git and after a build using
  ``make`` can run the tests with ``make test``.
  May also need to ``pip install pytest``.
  ``make test`` is quite terse. To get the same verbose output that is
  seen with the CI tests, use ``ctest -VV`` (executed in the
  build folder) or an individual test with ``ctest -VV -R name_of_the_test``.
  One can also run individual test files
  with ``python3 -m pytest -s <testfile.py>`` or all the test files in that
  folder with ``python3 -m pytest -s``. (The ``-s`` shows all output on
  the terminal.) Note: It is helpful to ``make test``
  first to ensure any mod files needed are available to the tests. If
  running a test outside the folder where the test is located, it may be
  necessary to add the folder to PYTHONPATH. Note: The last python
  mentioned in the ``-DNRN_PYTHON_DYNAMIC=...`` (if the semicolon separated
  list is non-empty and ``-DNRN_ENABLE_PYTHON_DYNAMIC=ON``)
  is the one used for ``make test`` and ``ctest -VV``. Otherwise the
  value specified by ``PYTHON_EXECUTABLE`` is used.

  Example

  .. code-block:: shell

    mkdir build
    cmake .. -DNRN_ENABLE_TESTS=ON ...
    make -j
    make test
    ctest -VV -R parallel_tests
    cd ../test/pynrn
    python3 -m pytest
    python3 -m pytest test_currents.py

NRN_ENABLE_COVERAGE:BOOL=OFF
---------------------------
  Enable code coverage

  Requires ``lcov`` (e.g. ``sudo apt install lcov``).

  Provides two make targets to simplify the repeated "run tests, examine coverage"
  workflow.
    -- ``make cover_begin`` erases all previous coverage data
    (``*.gcda`` files), and creates a baseline report. (Note all files and
    folders are created in the ``CMAKE_BINARY_DIR`` where you ran cmake.)

    -- ``make cover_html`` creates a coverage report for the sum of all the
    software runs since the last ``cover_begin`` and prints a file url
    that you can paste into your browser to review the coverage.

  When using an iterative workflow to examine test coverage of a single
  or a few files, the above targets run much faster when this option is
  combined with `NRN_COVERAGE_FILES:STRING=`_

  Code coverage without the use of this option is explained in
  `Developer Builds: Code Coverage <../install/code_coverage.html>`_

NRN_COVERAGE_FILES:STRING=
-------------------------------------------------------------
  Coverage limited to semicolon (;) separated list of file paths
  relative to ``PROJECT_SOURCE_DIR``.

  ``-DNRN_COVERAGE_FILES="src/nrniv/partrans.cpp;src/nmodl/parsact.cpp;src/nrnpython/nrnpy_hoc.cpp"``

NRN_SANITIZERS:STRING=
----------------------
  Enable some combination of AddressSanitizer, LeakSanitizer and
  UndefinedBehaviorSanitizer. Accepts a comma-separated list of ``address``,
  ``leak`` and ``undefined``. See the "Diagnosis and Debugging" section for more
  information.

Miscellaneous Rarely used options specific to NEURON:
=====================================================

NRN_ENABLE_DISCRETE_EVENT_OBSERVER:BOOL=ON
------------------------------------------
  Enable Observer to be a subclass of DiscreteEvent
  Can save space but a lot of component destruction may not notify other components that are watching it to no longer use that component. Useful only if one builds a model without needing to eliminate pieces of the model.

NRN_DYNAMIC_UNITS_USE_LEGACY:BOOL=OFF
----------------------------
  Default is to use modern faraday, R, etc. from 2019 nist constants.
  When Off or ON, and in the absence of the ``NRNUNIT_USE_LEGACY=0or1``
  environment variable, the default dynamic value of ``h.nrnunit_use_legacy()``
  will be 0 or 1 respectively.

  At launch time (or import neuron),
  use of legacy or modern units can be specified with the
  ``NRNUNIT_USE_LEGACY=0or1`` environment variable. The use of legacy or
  modern units can be dynamically specified after launch with the
  ``h.nrnunit_use_legacy(0or1)`` function (with no args, returns the
  current use flag).

NRN_ENABLE_MECH_DLL_STYLE:BOOL=ON
---------------------------------
  Dynamically load nrnmech shared library

NRN_ENABLE_SHARED:BOOL=ON
-------------------------
  Build shared libraries (otherwise static library)

  This must be ON if python is launched and imports neuron. If OFF and one wants to use python it will be
  necessary to launch

  .. code-block:: shell

    nrniv -python

  Note that the top-level ``CMakeLists.txt`` file includes some custom configuration for Cray platforms.
  This may need to be adapted if you specify ``NRN_ENABLE_SHARED=OFF``.

NRN_ENABLE_THREADS:BOOL=ON
--------------------------
  Allow use of Pthreads

NRN_USE_REL_RPATH=OFF
---------------------
  Turned on when creating python wheels.

NRN_ENABLE_BACKTRACE:BOOL=OFF
-------------------------------------
  Generate a backtrace on floating, segfault, and bus exceptions.

  Avoids the need to use gdb to view the backtrace.

  Does not work with python.

  Note: floating exceptions are turned on with :func:`nrn_feenableexcept`.
