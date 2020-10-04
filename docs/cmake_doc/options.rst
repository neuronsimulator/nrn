Introduction
============
The NEURON build system now uses cmake as of version 7.8 circa Nov 2019.
The previous autotools (./configure) build system is still supported for
the time being but any features that use submodules would need to build
those separately.

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

Sadly, there is no equivalent in cmake to the autotool's ``./configure --help``
to list all the options. The closest is
``cmake .. -LH``
which runs ``cmake ..`` as above and lists the cache variables along with help
strings which are not marked as INTERNAL or ADVANCED. Alternatively,

.. code-block:: shell

  ccmake ..

allows one to interactively inspect cached variables.
In the build folder, ``cmake -LH`` (missing <path-to-source) will not
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

    -DNRN_PYTHON_DYNAMIC="python2.7;python3.6;python3.7;python3.8"

  This option is ignored unless NRN_ENABLE_PYTHON_DYNAMIC=ON

PYTHON_EXECUTABLE:PATH=
-----------------------
  Use provided python binary instead of the one found by CMake.
  This must be a full path. We generally use

  .. code-block:: shell

    -DPYTHON_EXECUTABLE=`which python3.7`

NRN_ENABLE_MODULE_INSTALL:BOOL=ON
---------------------------------
  Enable installation of NEURON Python module.

  By default, the neuron module is installed in CMAKE_INSTALL_PREFIX/lib/python.

NRN_MODULE_INSTALL_OPTIONS:STRING=--home=/usr/local
---------------------------------------------------
  setup.py options, everything after setup.py install  

  To install in site-packages use an empty string

  .. code-block:: shell

    -DNRN_MODULE_INSTALL_OPTIONS=""

  This option is (or should be) ignored unless NRN_ENABLE_MODULE_INSTALL=ON.

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

  If ON and no argument pointing to an external installation, CoreNEURON
  will be cloned as a submodule along with all its NMODL submodule dependencies.

NRN_ENABLE_MOD_COMPATIBILITY:BOOL=OFF
-------------------------------------
  Enable CoreNEURON compatibility for MOD files  

  CoreNEURON does not allow the common NEURON THREADSAFE promotion of
  GLOBAL variables that appear on the right hand side of assignment statements
  to become thread specific variables. This option is
  automatically turned on if NRN_ENABLE_CORENEURON=ON.

  There are a large number of cmake arguments specific to a CoreNEURON
  build that are explained in ???.

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

Readline_ROOT_DIR:PATH=/usr
---------------------------
  Path to a file.  

  If cmake can't find readline and you don't want the nrn internal version, you
can give this hint as to where it is.

NRN_ENABLE_TESTS:BOOL=OFF
-------------------------
  Enable unit tests  

  Clones the submodule catch2 from https://github.com/catchorg/Catch2.git and after a build using 
  ``make`` can run the tests with ``make test``.
  May also need to ``pip install pytest``.
  ``make test`` is quite terse. To get the same verbose output that is
  seen with the travis-ci tests, use ``ctest -VV`` (executed in the
  build folder). One can also run individual test files
  with ``python3 -m pytest <testfile.py>`` or all the test files in that
  folder with ``python3 -m pytest``. Note: It is helpful to ``make test``
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
    ctest -VV
    cd ../test/pynrn
    python3 -m pytest
    python3 -m pytest test_currents.py

NEURON_CMAKE_FORMAT:BOOL=OFF
----------------------------
  Enable CMake code formatting  

  Clones the submodule coding-conventions from https://github.com/BlueBrain/hpc-coding-conventions.git.
  Also need to ``pip install cmake-format=0.6.0 --user``.
  After a build using ``make`` can reformat cmake files with ``make cmake-format``
  See nrn/CONTRIBUTING.md for further details.
  How does one reformat a specific cmake file?

Miscellaneous Rarely used options specific to NEURON:
=====================================================

NRN_ENABLE_DISCRETE_EVENT_OBSERVER:BOOL=ON
------------------------------------------
  Enable Observer to be a subclass of DiscreteEvent  
  Can save space but a lot of component destruction may not notify other components that are watching it to no longer use that component. Useful only if one builds a model without needing to eliminate pieces of the model.

NRN_ENABLE_LEGACY_FR:BOOL=ON
----------------------------
  Use original faraday, R, etc. instead of 2019 nist constants  

  The default for version 8.0 onward is likely to become OFF in order to use the latest physical constants (at the cost of slight changes to legacy results). 

NRN_ENABLE_MECH_DLL_STYLE:BOOL=ON
---------------------------------
  Dynamically load nrnmech shared library  

NRN_ENABLE_MEMACS:BOOL=OFF
--------------------------
  Enable use of memacs  

  Microemacs is a tiny emacs like editor I have been using since the mid-eighties. I might be the only one in the world who uses it now.

NRN_ENABLE_SHARED:BOOL=ON
-------------------------
  Build shared libraries (otherwise static library)  

  This must be ON if python is launched and imports neuron. If OFF and one wants to use python it will be
  necessary to launch

  .. code-block:: shell

    nrniv -python

NRN_ENABLE_THREADS:BOOL=ON
--------------------------
  Allow use of Pthreads  

NRN_USE_REL_RPATH=OFF
---------------------
  Turned on when creating python wheels.  

NRN_ENABLE_BINARY_SPECIAL:BOOL=OFF
----------------------------------
  Build binary special  

  nrnivmodl by default creates shell script called 'special' which runs nrniv and specifies the argument
  ``-dll /path/to/libnrnmech.so`` or whatever the name is of the shared library created by nrnivmodl.
  This option forces nrnivmodl to create a binary version of special that can be run, for example, with gdb or valgrind.

  It is not often needed as nrniv has a ``-dll <path/to/libnrnmech.so>`` option.
  Also by default, if the current working directory on launch has a folder named
  ``x86_64`` (or whatever the CPU happens to be), the nrnmech library in that
  folder will be automatically loaded.


NRN_ENABLE_INTERNAL_READLINE:BOOL=OFF
-------------------------------------
  Use internal Readline library shipped with NEURON  

Forces use of the readline code distributed with NEURON even if there is a system supplied readline.

