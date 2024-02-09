cmake_minimum_required(VERSION 3.15 FATAL_ERROR)
# Note that this needs to happen **before** the call to project(...). This is because CMake reads
# the CRAYPE_LINK_TYPE environment variable inside the call to project(...) and sets various flags
# and properties based on its value. Because we are so early in the CMake processing, we have to
# figure out for ourselves whether or not we are running on a Cray system by looking for Cray
# environment variables directly.
if(DEFINED ENV{CRAYPE_VERSION})
  # Older versions of the Cray software prefer static linking by default, which doesn't work with
  # NEURON with some compilers (GCC and Intel at the time of writing). Note that this variable will
  # not be set in the compiler and linker runtime environments, so we are relying on CMake baking in
  # the correct compiler flags based on the value we set here. In more recent versions of the Cray
  # software the default has changed; CMake knows about this in v3.15.3 and newer, but to support
  # older CMake versions we still need to set this explicitly. See:
  # https://github.com/neuronsimulator/nrn/issues/1153
  set(ENV{CRAYPE_LINK_TYPE} dynamic)
  message(STATUS "Cray system detected...setting CRAYPE_LINK_TYPE=dynamic")
endif()
project(
  NEURON
  VERSION 9.0.0
  LANGUAGES C CXX
  HOMEPAGE_URL "https://www.neuron.yale.edu/neuron/")

# =============================================================================
# CMake common project settings
# =============================================================================
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT "cxx_std_17" IN_LIST CMAKE_CXX_COMPILE_FEATURES)
  message(
    FATAL_ERROR
      "This compiler does not fully support C++17, choose a higher version or another compiler.")
endif()

# =============================================================================
# CMake common project settings
# =============================================================================
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# =============================================================================
# Include default build options
# =============================================================================
list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)
include(BuildOptionDefaults)

# =============================================================================
# Build options (boolean)
# =============================================================================
option(NRN_ENABLE_DOCS "Build documentation" ${NRN_ENABLE_DOCS_DEFAULT})
# This is useful for readthedocs-style builds and the documentation CI, where the actual
# installation of NEURON comes from an installed binary wheel.
option(
  NRN_ENABLE_DOCS_WITH_EXTERNAL_INSTALLATION
  "Build documentation without building NEURON. It will be assumed that import neuron works, nrnivmodl is in PATH, etc."
  ${NRN_ENABLE_DOCS_WITH_EXTERNAL_INSTALLATION_DEFAULT})
mark_as_advanced(NRN_ENABLE_DOCS_WITH_EXTERNAL_INSTALLATION)
option(NRN_ENABLE_SHARED "Build shared libraries (otherwise static library)"
       ${NRN_ENABLE_SHARED_DEFAULT})
option(NRN_ENABLE_INTERVIEWS "Enable GUI with INTERVIEWS" ${NRN_ENABLE_INTERVIEWS_DEFAULT})
option(NRN_ENABLE_MECH_DLL_STYLE "Dynamically load nrnmech shared library"
       ${NRN_ENABLE_MECH_DLL_STYLE_DEFAULT})
option(NRN_ENABLE_DISCRETE_EVENT_OBSERVER "Enable Observer to be a subclass of DiscreteEvent"
       ${NRN_ENABLE_DISCRETE_EVENT_OBSERVER_DEFAULT})
option(NRN_ENABLE_PYTHON "Enable Python interpreter support (default python3 fallback to python)"
       ${NRN_ENABLE_PYTHON_DEFAULT})
option(NRN_ENABLE_THREADS "Allow use of multithreading" ${NRN_ENABLE_THREADS_DEFAULT})
option(NRN_ENABLE_MPI "Enable MPI support" ${NRN_ENABLE_MPI_DEFAULT})
option(NRN_ENABLE_MUSIC "Enable MUSIC support" ${NRN_ENABLE_MUSIC_DEFAULT})
option(NRN_ENABLE_RX3D "Enable rx3d support" ${NRN_ENABLE_RX3D_DEFAULT})
option(NRN_ENABLE_CORENEURON "Enable CoreNEURON support" ${NRN_ENABLE_CORENEURON_DEFAULT})
option(NRN_ENABLE_BACKTRACE "Enable pretty-printed backtraces" ${NRN_ENABLE_BACKTRACE_DEFAULT})
option(NRN_ENABLE_TESTS "Enable unit tests" ${NRN_ENABLE_TESTS_DEFAULT})
option(NRN_ENABLE_MATH_OPT "Enable extra math optimisations (to enable SIMD)"
       ${NRN_ENABLE_MATH_OPT_DEFAULT})
set(NRN_ENABLE_MODEL_TESTS
    "${NRN_ENABLE_MODEL_TESTS_DEFAULT}"
    CACHE STRING "Comma-separated list of detailed models to enable tests of.")
# parse NRN_ENABLE_MODEL_TESTS as list
string(REPLACE "," ";" NRN_ENABLE_MODEL_TESTS "${NRN_ENABLE_MODEL_TESTS}")
option(
  NRN_ENABLE_PERFORMANCE_TESTS
  "Enable tests that measure performance. These are known to be unreliable when run on busy/oversubscribed machines such as CI runners."
  ${NRN_ENABLE_PERFORMANCE_TESTS_DEFAULT})
# This can be helpful in very specific CI build configurations, where ccache is used *and* different
# CI builds are built under different directories.
option(NRN_AVOID_ABSOLUTE_PATHS
       "Avoid embedding absolute paths in generated code (ccache optimisation)"
       ${NRN_AVOID_ABSOLUTE_PATHS_DEFAULT})
mark_as_advanced(NRN_AVOID_ABSOLUTE_PATHS)
# note that if CoreNEURON is enabled then it is not necessary to enable this option
option(NRN_ENABLE_MOD_COMPATIBILITY "Enable CoreNEURON compatibility for MOD files"
       ${NRN_ENABLE_MOD_COMPATIBILITY_DEFAULT})
option(NRN_ENABLE_REL_RPATH "Use relative RPATH in binaries. for relocatable installs/Python"
       ${NRN_ENABLE_REL_RPATH_DEFAULT})
option(NRN_BINARY_DIST_BUILD ${NRN_BINARY_DIST_BUILD_DEFAULT})
option(NRN_WHEEL_STATIC_READLINE "Use static readline libraries for the wheels."
       ${NRN_WHEEL_STATIC_READLINE_DEFAULT})
mark_as_advanced(NRN_ENABLE_REL_RPATH)
mark_as_advanced(NRN_BINARY_DIST_BUILD)

# =============================================================================
# Build options (string)
# =============================================================================
# ~~~
# NEURON module installation:
#   - OFF       : do not install
#   - ON        : install in ${CMAKE_INSTALL_PREFIX} (default)
#   NOTE: When building the wheel, this is set to OFF.
# Dynamic Python version support:
#   - NRN_ENABLE_PYTHON_DYNAMIC=OFF  : libnrniv is linked against a single Python version, which
#                                      can be steered with PYTHON_EXECUTABLE
#   - NRN_ENABLE_PYTHON_DYNAMIC=ON   : libnrniv is not linked against Python, distinct libnrnpython
#                                      libraries are built for each configured Python major.minor
#                                      version but are only linked against the relevant libpython
#                                      if NRN_LINK_AGAINST_PYTHON is true, which is by default only
#                                      the case on windows.
#                                      nrniv/special still works in the absence of Python, and
#                                      dynamically loads the appropriate libnrnpythonX.Y as needed
#   - NRN_PYTHON_DYNAMIC=pythonX;... : semicolon separated list of Python versions used when
#                                      NRN_ENABLE_PYTHON_DYNAMIC=ON, if not set then it defaults to
#                                      PYTHON_EXECUTABLE. This must include all major.minor
#                                      versions that you want nrniv -python to be able to load. If
#                                      this *is* set then the first entry will be used as the
#                                      default Python version.
# Extra options for extended Python tests:
#   - NRN_PYTHON_EXTRA_FOR_TESTS=... : semicolon separated list of Python executables that NEURON
#                                      is *not* built against. These are only used for testing, and
#                                      there must be no overlap in major.minor versions between
#                                      this list and the set of versions that NEURON *is* built
#                                      against. This allows tests of failure modes.
# Dynamic MPI support:
#   - OFF       : nrnmpi is linked into libnrniv.so
#   - ON        : nrnmpi interface consistent with default mpi is built and loaded dynamically
#                 at run time (nrniv still works in the absence of any mpi at all).
#   - <string>  : semicolon (;) separated list of MPI's bin directories to create a separate
#                 libnrnmpi_xxx.so interface for each. When nrniv is launched with the -mpi argument,
#                 the first mpi found will determine which interface is dynamically loaded."
# Rx3D Cython generated files compiler optimization level. 0 is default.
# Specific coverage files:
#   - OFF       : Disable code coverage.
#   - ON        : Collect code coverage for files (default all).
#   - <string>  : semicolon (;) separated list of files to collect coverage.
# ~~~
option(NRN_ENABLE_MODULE_INSTALL
       "Enable installation of NEURON Python module for regular CMake builds"
       ${NRN_ENABLE_MODULE_INSTALL_DEFAULT})
option(NRN_ENABLE_PYTHON_DYNAMIC "Enable dynamic Python version support"
       ${NRN_ENABLE_PYTHON_DYNAMIC_DEFAULT})
option(NRN_LINK_AGAINST_PYTHON
       "Link libnrnpythonX.Y against libpythonX.Y when NRN_ENABLE_PYTHON_DYNAMIC=ON"
       ${NRN_LINK_AGAINST_PYTHON_DEFAULT})
set(NRN_PYTHON_DYNAMIC
    ""
    CACHE
      STRING
      "semicolon (;) separated list of python executables to create interface for (default python3)"
)
set(NRN_PYTHON_EXTRA_FOR_TESTS
    ""
    CACHE STRING "semicolon (;) separated list of python executables to use for testing")

option(NRN_ENABLE_MPI_DYNAMIC "Enable dynamic MPI library support" OFF)
set(NRN_MPI_DYNAMIC
    ""
    CACHE
      STRING
      "semicolon (;) separated list of MPI include directories to build against (default to first found mpi)"
)

set(NRN_RX3D_OPT_LEVEL
    "${NRN_RX3D_OPT_LEVEL_DEFAULT}"
    CACHE STRING "Optimization level for Cython generated files (non-zero may compile slowly)")

option(NRN_ENABLE_COVERAGE "EnableCode Coverage (make cover_begin, make cover_html)" OFF)
set(NRN_COVERAGE_FILES
    ""
    CACHE STRING "semicolon (;) separated list of files to collect code coverage")

set(NRN_NMODL_CXX_FLAGS
    "${NRN_NMODL_CXX_FLAGS_DEFAULT}"
    CACHE STRING "space separated list of flags to be added to host tools (nocmodl, modlunit")
separate_arguments(NRN_NMODL_CXX_FLAGS UNIX_COMMAND ${NRN_NMODL_CXX_FLAGS})

set(NRN_EXTRA_CXX_FLAGS
    ""
    CACHE STRING "Extra compiler flags for NEURON source files")
separate_arguments(NRN_EXTRA_CXX_FLAGS)
list(APPEND NRN_COMPILE_FLAGS ${NRN_EXTRA_CXX_FLAGS})

set(NRN_EXTRA_MECH_CXX_FLAGS
    ""
    CACHE STRING "Extra compiler flags for translated NEURON mechanisms")
separate_arguments(NRN_EXTRA_MECH_CXX_FLAGS)

option(NRN_ENABLE_PROFILING "Enable profiling" ${NRN_ENABLE_PROFILING_DEFAULT})
set(NRN_PROFILER
    "${NRN_PROFILER_DEFAULT}"
    CACHE STRING "Set which profiler to build against ('caliper', 'likwid')")

# =============================================================================
# Include cmake modules
# =============================================================================
# sub-directorty containing project submodules
set(THIRD_PARTY_DIRECTORY "${PROJECT_SOURCE_DIR}/external")
list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake/modules)
include(cmake/PlatformHelper.cmake)
include(cmake/CompilerHelper.cmake)
include(cmake/MacroHelper.cmake)
include(cmake/RpathHelper.cmake)
include(cmake/ExternalProjectHelper.cmake)
include(cmake/modules/FindPythonModule.cmake)
include(cmake/Coverage.cmake)

# set CMAKE_BUILD_TYPE and associated flags using allowableBuildTypes and CMAKE_BUILD_TYPE_DEFAULT
set(allowableBuildTypes Custom Debug Release RelWithDebInfo Fast FastDebug)
include(ReleaseDebugAutoFlags)

# Try and emit an intelligent warning if the version number currently set in the CMake project(...)
# call is inconsistent with the output of git describe.
include(cmake/CheckGitDescribeCompatibility.cmake)

# Save the version information from project(...) to an nrnsemanticversion.h header in the build
# directory. This separate from nrnversion.h, which includes information about the latest git
# commit, because it should change less frequently, but it is included in many more files.
set(NRN_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
set(NRN_VERSION_MINOR ${PROJECT_VERSION_MINOR})
set(NRN_VERSION_PATCH ${PROJECT_VERSION_PATCH})
configure_file(src/nrnoc/nrnsemanticversion.h.in include/nrnsemanticversion.h @ONLY)

# =============================================================================
# Add coding-conventions submodule so we can use helper functions defined there
# =============================================================================
set(CODING_CONV_PREFIX "NRN")
set(${CODING_CONV_PREFIX}_3RDPARTY_DIR "external")
set(CODING_CONV_CMAKE "${PROJECT_SOURCE_DIR}/${NRN_3RDPARTY_DIR}/coding-conventions/cpp/cmake")
if(NOT EXISTS "${CODING_CONV_CMAKE}/3rdparty.cmake")
  # Abort with a helpful message if the current source tree lacks .git information, as in that case
  # we're not going to be able to initialise the submodule.
  nrn_submodule_file_not_found("3rdparty.cmake")
  nrn_initialize_submodule(external/coding-conventions)
endif()
include("${CODING_CONV_CMAKE}/3rdparty.cmake")
cpp_cc_git_submodule(Random123)
cpp_cc_git_submodule(eigen)

# =================================================================================================
# Enable sanitizer support if the NRN_SANITIZERS variable is set. Comes befores PythonHelper.cmake.
# =================================================================================================
include(cmake/SanitizerHelper.cmake)
# Make a CMake list NRN_SANITIZERS_LIST
string(REPLACE "," ";" NRN_SANITIZERS_LIST "${NRN_SANITIZERS}")

# =================================================================================================
# Juggle PYTHON_EXECUTABLE and NRN_ENABLE_PYTHON_DYNAMIC to make sure that:
#
# * NRN_DEFAULT_PYTHON_EXECUTABLE is the default Python version, and NRN_DEFAULT_PYTHON_INCLUDES and
#   NRN_DEFAULT_PYTHON_LIBRARIES are its include direcory and library
# * NRN_PYTHON_EXECUTABLES is a list of absolute paths to all Python executables to be built against
#   (length >=1 if NRN_ENABLE_PYTHON_DYNAMIC else == 1)
# * NRN_PYTHON_VERSIONS (3.8, 3.11, ...), NRN_PYTHON_INCLUDES and NRN_PYTHON_LIBRARIES are populated
#   with values patching NRN_PYTHON_EXECUTABLES
# * NRN_PYTHON_COUNT is set to the number of entries in those lists (i.e. number of Pythons), and
#   NRN_PYTHON_ITERATION_LIMIT is set to ${NRN_PYTHON_COUNT} - 1
# =================================================================================================
set(NRN_MINIMUM_PYTHON_VERSION 3.8)
include(cmake/PythonHelper.cmake)

# This needs NRN_DEFAULT_PYTHON_EXECUTABLE, which comes from PythonHelper.cmake
if(NRN_SANITIZERS)
  configure_file(bin/nrn-enable-sanitizer.in bin/nrn-enable-sanitizer @ONLY)
  install(PROGRAMS ${PROJECT_BINARY_DIR}/bin/nrn-enable-sanitizer
          DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)
endif()

# =============================================================================
# Find required packages
# =============================================================================
find_package(BISON REQUIRED)
find_package(FLEX 2.6 REQUIRED)

# When shipping the wheels we want to link readline libs statically.
if(NRN_WHEEL_STATIC_READLINE)
  # CMake will prefer dynamic libraries over static ones. With the following construct we make sure
  # we get them.
  set(ORIGINAL_CMAKE_FIND_LIBRARY_SUFFIXES ${CMAKE_FIND_LIBRARY_SUFFIXES})
  set(CMAKE_FIND_LIBRARY_SUFFIXES ".a")
endif()

find_package(Readline REQUIRED)
if(READLINE_FOUND)
  # MAC libedit.3.dylib does not have rl_event_hook Readline_LIBRARY may be a binary lib or (on the
  # MAC) a tbd file that points to the library and mentions all its definitions. this part has to be
  # prior to adding the nrniv subdirectory.
  execute_process(COMMAND grep -q rl_event_hook ${Readline_LIBRARY} RESULT_VARIABLE result)
  if(NOT result EQUAL 0)
    # define for src/oc/hoc.cpp
    set(DEF_RL_GETC_FUNCTION use_rl_getc_function)
  endif()
  # If we are not using self contained, static readline library created for building wheel then only
  # look for curses and termcap
  if(NOT NRN_WHEEL_STATIC_READLINE)
    find_package(Curses QUIET)
    find_package(Termcap QUIET)
  endif()
endif()

# Reset original CMake library suffixes
if(ORIGINAL_CMAKE_FIND_LIBRARY_SUFFIXES)
  set(CMAKE_FIND_LIBRARY_SUFFIXES ${ORIGINAL_CMAKE_FIND_LIBRARY_SUFFIXES})
endif()

if(NRN_ENABLE_RX3D)
  if(NOT NRN_ENABLE_PYTHON)
    message(SEND_ERROR "NRN_ENABLE_RX3D requires NRN_ENABLE_PYTHON feature.")
  else()
    find_package(Cython REQUIRED)
    if(CYTHON_VERSION VERSION_GREATER_EQUAL 3)
      message(FATAL_ERROR "Cython 3+ is not supported")
    endif()
  endif()
endif()
if(MINGW)
  find_package(Termcap REQUIRED)
endif()

# =============================================================================
# Enable MPI
# =============================================================================
if(NRN_ENABLE_MPI)

  # find_package(MPI REQUIRED) has a CMAKE_OSX_ARCHITECTURES edge case
  nrn_mpi_find_package()
  set(NRNMPI 1)

  # avoid linking to C++ bindings
  add_compile_definitions(MPI_NO_CPPBIND=1 OMPI_SKIP_MPICXX=1 MPICH_SKIP_MPICXX=1)

  # Launching mpi executable with full path can mangle different python versions and libraries (see
  # issue #894). ${MPIEXEC_NAME} would reinsert the full path, but ${CMAKE_COMMAND} -E env
  # ${MPIEXEC_NAME} does not.
  get_filename_component(MPIEXEC_NAME ${MPIEXEC_EXECUTABLE} NAME)

  # Detect if we have an OpenMPI v2 or older.
  execute_process(
    COMMAND "${MPIEXEC_EXECUTABLE}" --version
    RESULT_VARIABLE OPENMPI_TEST_RESULT
    OUTPUT_VARIABLE OPENMPI_TEST_OUTPUT)
  set(NRN_HAVE_OPENMPI2_OR_LESS OFF)
  if(${OPENMPI_TEST_RESULT} EQUAL 0 AND "${OPENMPI_TEST_OUTPUT}" MATCHES
                                        "^mpiexec \\(OpenRTE\\) ([0-9\.]+)")
    set(NRN_OPENMPI_VERSION "${CMAKE_MATCH_1}")
    message(STATUS "Detected OpenMPI ${NRN_OPENMPI_VERSION}")
    if("${NRN_OPENMPI_VERSION}" VERSION_LESS 3)
      set(NRN_HAVE_OPENMPI2_OR_LESS ON)
      message(STATUS "OpenMPI<v3: `mpirun python` may not work, try `mpirun special -python`.")
      message(STATUS "See also: https://www.neuron.yale.edu/phpBB/viewtopic.php?t=4297")
    endif()
  endif()
else()
  set(NRNMPI 0)
endif()

# =============================================================================
# Enable MUSIC MUlti SImulation Coordinator
# =============================================================================
if(NRN_ENABLE_MUSIC)
  if(NOT NRN_ENABLE_MPI)
    message(FATAL "MUSIC requires -DNRN_ENABLE_MPI=ON")
  endif()
  if(NOT NRN_ENABLE_PYTHON)
    message(FATAL "MUSIC requires -DNRN_ENABLE_PYTHON=ON")
  endif()
  find_package(Cython REQUIRED)
  if(CYTHON_VERSION VERSION_GREATER_EQUAL 3)
    message(FATAL_ERROR "Cython 3+ is not supported")
  endif()
  find_package(MUSIC REQUIRED)
  set(NRN_MUSIC 1)
endif()

# =============================================================================
# Enable backward
# =============================================================================
if(NRN_ENABLE_BACKTRACE)
  set(NRN_USE_BACKWARD 1)
  nrn_add_external_project(backward)
else()
  set(NRN_USE_BACKWARD 0)
endif()
# =============================================================================
# Enable Interviews
# =============================================================================
if(NRN_ENABLE_INTERVIEWS)
  # x11 is not required on windows
  if(NOT NRN_WINDOWS_BUILD)
    find_package(X11 QUIET)
    if(NOT X11_FOUND)
      if(APPLE)
        message(SEND_ERROR "You must install XQuartz from https://www.xquartz.org/ to build iv")
      else()
        message(
          SEND_ERROR
            "You must install X11 to build iv e.g. 'apt install libx11-dev libxcomposite-dev' on Ubuntu"
        )
      endif()
    endif()
    include_directories(${X11_INCLUDE_DIR})
  endif()

  find_package(iv QUIET PATHS ${IV_DIR}/lib/cmake ${IV_DIR})
  if(iv_FOUND)
    message(STATUS "Using external Interviews from ${IV_DIR}")
    get_target_property(IV_INCLUDE_DIR interviews INTERFACE_INCLUDE_DIRECTORIES)
  else()
    nrn_add_external_project(iv)
    include_directories(SYSTEM external/iv/src/include)
    set(IV_DIR ${PROJECT_SOURCE_DIR}/external/iv)
    set(IV_INCLUDE_DIR ${PROJECT_SOURCE_DIR}/external/iv/src/include)
  endif()
  set(HAVE_IV 1)
else()
  set(HAVE_IV 0)
endif()

# =============================================================================
# Enable Python support
# =============================================================================
if(NRN_ENABLE_PYTHON)
  # Make sure the USE_PYTHON macro is defined in the C++ code
  list(APPEND NRN_COMPILE_DEFS USE_PYTHON)
endif()

# =============================================================================
# Enable Threads support
# =============================================================================
if(NRN_ENABLE_THREADS)
  # Note that at least GCC requires -pthread to be passed even when using std::thread.
  set(THREADS_PREFER_PTHREAD_FLAG ON)
  find_package(Threads REQUIRED)
  list(APPEND NRN_COMPILE_DEFS NRN_ENABLE_THREADS)
endif()

# =============================================================================
# Profiler/Instrumentation Options
# =============================================================================
if(NRN_ENABLE_PROFILING)
  add_definitions("-DNRN_PROFILING")
  if(NRN_PROFILER STREQUAL "caliper")
    find_package(caliper REQUIRED)
    include_directories(${caliper_INCLUDE_DIR})
    add_definitions("-DNRN_CALIPER")
    set(CALIPER_LIB "caliper")
  elseif(NRN_PROFILER STREQUAL "likwid")
    include(GetLIKWID)
    set(LIKWID_LIB nrn_likwid)

  endif()
endif()

# =============================================================================
# Set library type
# =============================================================================
if(NRN_ENABLE_SHARED)
  set(NRN_LIBRARY_TYPE "SHARED")
else()
  set(NRN_LIBRARY_TYPE "STATIC")
  set(CMAKE_POSITION_INDEPENDENT_CODE ON)
endif()

# =============================================================================
# Add helper CMake modules AFTER setting options
# =============================================================================
include(NeuronFileLists)
include(MPIDynamicHelper)

# Set variable to include mpi headers
set(NRN_INCLUDE_MPI_HEADERS OFF)
list(LENGTH NRN_MPI_LIBNAME_LIST num_mpi)
if(NOT NRN_ENABLE_MPI_DYNAMIC OR ${num_mpi} EQUAL 1)
  set(NRN_INCLUDE_MPI_HEADERS ON)
endif()

# initialize CLI11 submodule
cpp_cc_git_submodule(CLI11 BUILD PACKAGE CLI11 REQUIRED)

# =============================================================================
# Enable CoreNEURON support
# =============================================================================
if(NRN_ENABLE_CORENEURON)
  message(STATUS "Building CoreNEURON")

  # =============================================================================
  # Include cmake modules path
  # =============================================================================
  list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake/coreneuron
       ${PROJECT_SOURCE_DIR}/cmake/coreneuron/packages)

  # If NEURON tests are enabled then enable CoreNEURON tests too
  set(CORENRN_ENABLE_UNIT_TESTS
      ${NRN_ENABLE_TESTS}
      CACHE BOOL "" FORCE)
  if(NRN_ENABLE_PROFILING)
    if(NRN_PROFILER STREQUAL "caliper")
      set(CORENRN_ENABLE_CALIPER_PROFILING ON)
    elseif(NRN_PROFILER STREQUAL "likwid")
      set(CORENRN_ENABLE_LIKWID_PROFILING ON)
    endif()
  endif()
  set(CORENEURON_DIR ${PROJECT_SOURCE_DIR}/src/coreneuron)

  add_subdirectory(${PROJECT_SOURCE_DIR}/src/coreneuron)

  # By default `nrnivmodl` should look for `nrnivmodl-core` in the NEURON install prefix.
  set(cnrn_prefix "${CMAKE_INSTALL_PREFIX}")
  # CoreNEURON exports this list of flags as a property; turn it into a variable in this scope. If
  # CoreNEURON is installed externally then this is exported into coreneuron-config.cmake.
  get_property(CORENRN_LIB_LINK_FLAGS GLOBAL PROPERTY CORENRN_LIB_LINK_FLAGS)
  get_property(CORENRN_NEURON_LINK_FLAGS GLOBAL PROPERTY CORENRN_NEURON_LINK_FLAGS)
  get_property(CORENRN_ENABLE_SHARED GLOBAL PROPERTY CORENRN_ENABLE_SHARED)
  get_property(CORENRN_NMODL_BINARY GLOBAL PROPERTY CORENRN_NMODL_BINARY)
  # NEURON tests that link against CoreNEURON need to depend on it.
  set(CORENEURON_TARGET_TO_DEPEND coreneuron-for-tests)

  # When we are using CoreNEURON, OpenACC and shared libraries then it is important that the first
  # library *compiled by nvc++* to be opened is linked against OpenACC, to avoid the NVIDIA runtime
  # being initialised without OpenACC. See:
  # https://forums.developer.nvidia.com/t/dynamically-loading-an-openacc-enabled-shared-library-from-an-executable-compiled-with-nvc-does-not-work/210968/5
  # When we build CoreNEURON with NVHPC then we often build NEURON with NVHPC too, and if we load
  # NEURON from Python then the first compiled-by-nvc++ thing to be loaded is the NEURON Python
  # module (hoc.XXX.so). It is, therefore, important that we link that with -acc.
  separate_arguments(CORENRN_NEURON_LINK_FLAGS)
  set(NRN_LINK_FLAGS_FOR_ENTRY_POINTS ${CORENRN_NEURON_LINK_FLAGS})

endif()

# This needs to be after all modifications to NRN_LINK_FLAGS
include(ConfigFileSetting)

# =============================================================================
# Set install location for libraries (before src/nrniv)
# =============================================================================
# ~~~
# Classically, the install destination of the share folder for mac/linux has
# been <prefix>/share/nrn but for linux it has been <prefix>. For now we keep
# this distinction.
# Also, the classic location for shared libraries has been <prefix>/<arch>/lib
# and for max/linux we have move this to <inst>/lib. But windows has classically
# expected these shared libraries in <prefix>/bin (reduces the PATH and expected
# by ctypes in the neuron module.) So for now we keep that distinction as
# well. Setting these here as setup.py.in needs it.
# ~~~
if(MINGW)
  set(NRN_INSTALL_SHARE_DIR ${CMAKE_INSTALL_PREFIX})
  set(NRN_BUILD_SHARE_DIR ${CMAKE_BINARY_DIR})
  set(NRN_INSTALL_SHARE_LIB_DIR ${CMAKE_INSTALL_PREFIX}/bin)
else()
  set(NRN_INSTALL_SHARE_DIR ${CMAKE_INSTALL_PREFIX}/share/nrn)
  set(NRN_BUILD_SHARE_DIR ${CMAKE_BINARY_DIR}/share/nrn)
  set(NRN_INSTALL_SHARE_LIB_DIR ${CMAKE_INSTALL_PREFIX}/lib)
endif()

# =============================================================================
# Add project directories AFTER CMake modules
# =============================================================================
add_subdirectory(src/sparse13)
add_subdirectory(src/gnu)
add_subdirectory(src/nrniv)

# Collect the environment variables that are needed to execute NEURON from the build directory. This
# is used when configuring tests, and when building documentation targets.
function(prepend_to_var var new_path)
  set(old_list "${NRN_RUN_FROM_BUILD_DIR_ENV}")
  set(new_val "${var}=${new_path}")
  if(DEFINED ENV{${var}} AND NOT "$ENV{${var}}" STREQUAL "")
    string(APPEND new_val ":$ENV{${var}}")
  endif()
  list(APPEND old_list "${new_val}")
  set(NRN_RUN_FROM_BUILD_DIR_ENV
      "${old_list}"
      PARENT_SCOPE)
endfunction()
set(NRN_RUN_FROM_BUILD_DIR_ENV "NEURONHOME=${PROJECT_BINARY_DIR}/share/nrn"
                               "NRNHOME=${PROJECT_BINARY_DIR}")
prepend_to_var(PATH "${PROJECT_BINARY_DIR}/bin")
prepend_to_var(LD_LIBRARY_PATH "${PROJECT_BINARY_DIR}/lib")
if(NRN_ENABLE_CORENEURON)
  list(APPEND NRN_RUN_FROM_BUILD_DIR_ENV "CORENRNHOME=${PROJECT_BINARY_DIR}")
endif()
if(NRN_ENABLE_PYTHON)
  prepend_to_var(PYTHONPATH "${PROJECT_BINARY_DIR}/lib/python:${PROJECT_SOURCE_DIR}/test/rxd")
endif()
add_subdirectory(bin)

if(NRN_ENABLE_PYTHON)
  add_subdirectory(src/nrnpython)
endif()

if(NRN_MACOS_BUILD)
  add_subdirectory(src/mac)
endif()

if(MINGW)
  add_subdirectory(src/mswin)
endif()

if(NRN_ENABLE_DOCS)
  if(APPLE AND "address" IN_LIST NRN_SANITIZERS)
    # IPython notebook execution doesn't have enough magic to preload ASan
    message(FATAL_ERROR "macOS + ASan + docs is not supported")
  endif()
  # Do we need to set extra environment variables to find NEURON?
  set(NRN_DOCS_COMMAND_PREFIX ${CMAKE_COMMAND} -E env)
  if(NOT NRN_ENABLE_DOCS_WITH_EXTERNAL_INSTALLATION)
    list(APPEND NRN_DOCS_COMMAND_PREFIX ${NRN_RUN_FROM_BUILD_DIR_ENV})
  endif()

  # Make sure all dependencies are available
  find_package(Doxygen REQUIRED)
  find_program(FFMPEG_EXECUTABLE ffmpeg REQUIRED)
  find_program(JUPYTER_EXECUTABLE jupyter REQUIRED)
  find_program(PANDOC_EXECUTABLE pandoc REQUIRED)
  find_package(Sphinx REQUIRED)
  set(docs_requirements_file "${PROJECT_SOURCE_DIR}/docs/docs_requirements.txt")
  file(STRINGS "${docs_requirements_file}" docs_requirements)
  # Make sure CMake reruns if docs_requirements.txt changeds.
  set_property(GLOBAL APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${docs_requirements_file}")
  foreach(docs_requirement ${docs_requirements})
    if(${skip_next})
      set(skip_next FALSE)
      continue()
    endif()
    # This is needed for ipython, which is pip installable but not importable.
    if("${docs_requirement}" STREQUAL "# do not check import of next line")
      set(skip_next TRUE)
    elseif("${docs_requirement}" MATCHES "^([a-zA-Z_][a-zA-Z0-9_]*)$")
      nrn_find_python_module(MODULE ${CMAKE_MATCH_0} REQUIRED)
    endif()
  endforeach()

  # =============================================================================
  # Setup Doxygen documentation
  # =============================================================================
  # generate Doxyfile with correct source paths
  configure_file(${PROJECT_SOURCE_DIR}/docs/Doxyfile.in ${PROJECT_BINARY_DIR}/Doxyfile)
  add_custom_target(
    doxygen
    COMMAND ${NRN_DOCS_COMMAND_PREFIX} ${DOXYGEN_EXECUTABLE} ${PROJECT_BINARY_DIR}/Doxyfile
    WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
    COMMENT "Generating API documentation with Doxygen"
    VERBATIM)

  # =============================================================================
  # Setup Sphinx documentation
  # =============================================================================

  # Target to execute && convert notebooks to html. Note that neuron must be available for python
  # import, so we use NRN_RUN_FROM_BUILD_DIR_ENV. See docs/README.md.
  add_custom_target(
    notebooks
    COMMAND ${NRN_DOCS_COMMAND_PREFIX} NEURON_MODULE_OPTIONS="-nogui" bash
            ${PROJECT_SOURCE_DIR}/docs/notebooks.sh
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/docs)
  if(NRN_ENABLE_DOCS_WITH_EXTERNAL_INSTALLATION)
    message(STATUS "**Not** making the notebooks target depend on the rest of the NEURON build.")
    message(STATUS "Documentation building will probably fail if you haven't installed NEURON.")
  else()
    # We need NEURON to execute the notebooks. If we're building documentation as part of a normal
    # build, this means we need to schedule the documenation targets sufficiently late in the build.
    add_dependencies(notebooks hoc_module rxdmath)
  endif()

  add_custom_target(
    sphinx
    COMMAND ${NRN_DOCS_COMMAND_PREFIX} ${SPHINX_EXECUTABLE} -j auto -b html
            "${PROJECT_SOURCE_DIR}/docs" "${PROJECT_SOURCE_DIR}/docs/_build"
    COMMAND echo "Copy/Paste to Browser file://${PROJECT_SOURCE_DIR}/docs/_build/index.html"
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/docs
    COMMENT "Generating documentation with Sphinx")

  # Executing notebooks will add outputs in-place. We don't want those committed to the repo. This
  # commands cleans them out.
  add_custom_target(
    notebooks-clean
    COMMAND ${NRN_DOCS_COMMAND_PREFIX} bash ${PROJECT_SOURCE_DIR}/docs/notebooks.sh --clean
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/docs)

  # =============================================================================
  # Build full docs
  # =============================================================================
  add_custom_target(
    docs
    COMMAND ${CMAKE_COMMAND} --build ${PROJECT_BINARY_DIR} --target doxygen
    COMMAND ${CMAKE_COMMAND} --build ${PROJECT_BINARY_DIR} --target notebooks
    COMMAND ${CMAKE_COMMAND} --build ${PROJECT_BINARY_DIR} --target sphinx
    COMMAND ${CMAKE_COMMAND} --build ${PROJECT_BINARY_DIR} --target notebooks-clean
    COMMAND echo "The HTML docs are at file://${PROJECT_SOURCE_DIR}/docs/_build/index.html"
    COMMENT "Generating full documentation")
endif()

# =============================================================================
# Black, Clang-format, Cmake-format (see $PROJECT_SOURCE_DIR/.bbp-project.yaml)
# =============================================================================
add_custom_target(
  format
  COMMAND ${PROJECT_SOURCE_DIR}/external/coding-conventions/bin/format
  WORKING_DIRECTORY ${PROJECT_SOURCE_DIR})

# Prepare a shell script to format only files modified with respect to master branch
file(
  WRITE ${CMAKE_CURRENT_BINARY_DIR}/format-pr.sh
  "\
#!bash\n\
set -e\n\
cmd='cd ${PROJECT_SOURCE_DIR} && external/coding-conventions/bin/format `git diff --name-only master`'\n\
echo $cmd\n\
cd ${PROJECT_SOURCE_DIR} && external/coding-conventions/bin/format `git diff --name-only master`\n\
")

add_custom_target(
  format-pr
  COMMAND bash ${CMAKE_CURRENT_BINARY_DIR}/format-pr.sh
  COMMENT "Format only files modified with respect to master branch."
  WORKING_DIRECTORY ${PROJECT_SOURCE_DIR})

# =============================================================================
# ~~~
# Update hh.mod for CoreNEURON compatibility
# - Replace GLOBAL variable by RANHE
# - Comment out TABLE
# ~~~
# =============================================================================
if(NRN_ENABLE_CORENEURON OR NRN_ENABLE_MOD_COMPATIBILITY)
  set(GLOBAL_VAR_TOGGLE_COMMAND "'s/ GLOBAL minf/ RANGE minf/'")
  set(TABLE_VAR_TOGGLE_COMMAND "'s/ TABLE minf/ :TABLE minf/'")
else()
  set(GLOBAL_VAR_TOGGLE_COMMAND "'s/ RANGE minf/ GLOBAL minf/'")
  set(TABLE_VAR_TOGGLE_COMMAND "'s/ :TABLE minf/ TABLE minf/'")
endif()
separate_arguments(GLOBAL_VAR_TOGGLE_COMMAND UNIX_COMMAND "${GLOBAL_VAR_TOGGLE_COMMAND}")
separate_arguments(TABLE_VAR_TOGGLE_COMMAND UNIX_COMMAND "${TABLE_VAR_TOGGLE_COMMAND}")
add_custom_target(
  hh_update
  COMMAND sed ${GLOBAL_VAR_TOGGLE_COMMAND} ${CMAKE_SOURCE_DIR}/src/nrnoc/hh.mod >
          ${CMAKE_BINARY_DIR}/hh.mod.1
  COMMAND sed ${TABLE_VAR_TOGGLE_COMMAND} ${CMAKE_BINARY_DIR}/hh.mod.1 >
          ${CMAKE_BINARY_DIR}/hh.mod.2
  COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_BINARY_DIR}/hh.mod.2
          ${CMAKE_SOURCE_DIR}/src/nrnoc/hh.mod
  COMMAND ${CMAKE_COMMAND} -E remove ${CMAKE_BINARY_DIR}/hh.mod.1 ${CMAKE_BINARY_DIR}/hh.mod.2
  COMMENT "Update hh.mod for CoreNEURON compatibility"
  VERBATIM)
add_dependencies(nrniv_lib hh_update)

# =============================================================================
# Generate help_data.dat
# =============================================================================
if(NRN_ENABLE_PYTHON)
  add_custom_target(
    help_data_dat
    COMMAND ${CMAKE_COMMAND} -E make_directory ${PROJECT_BINARY_DIR}/lib/python/neuron
    COMMAND ${NRN_DEFAULT_PYTHON_EXECUTABLE} ${PROJECT_SOURCE_DIR}/docs/parse_rst.py
            ${PROJECT_SOURCE_DIR}/docs/python ${PROJECT_BINARY_DIR}/lib/python/neuron/help_data.dat
    COMMENT "Generating help_data.dat"
    VERBATIM)
  add_dependencies(nrniv_lib help_data_dat)
  # also ensure help_data.dat is copied to the install location
  install(FILES ${PROJECT_BINARY_DIR}/lib/python/neuron/help_data.dat DESTINATION lib/python/neuron)
endif()

include("${CODING_CONV_CMAKE}/build-time-copy.cmake") # cpp_cc_build_time_copy
function(copy_share_subdir_at_build_time)
  cmake_parse_arguments(opt "" "SUBDIR;TARGET" "" ${ARGN})
  set(src_dir "${PROJECT_SOURCE_DIR}/share/${opt_SUBDIR}")
  set(tgt_dir "${NRN_BUILD_SHARE_DIR}/${opt_SUBDIR}")
  # Note CONFIGURE_DEPENDS causes GLOB mismatch for MINGW, see #2277
  file(
    GLOB_RECURSE files_to_copy
    LIST_DIRECTORIES false
    RELATIVE "${src_dir}"
    CONFIGURE_DEPENDS "${src_dir}/*")
  foreach(file_to_copy ${files_to_copy})
    cpp_cc_build_time_copy(
      INPUT "${src_dir}/${file_to_copy}"
      OUTPUT "${tgt_dir}/${file_to_copy}"
      NO_TARGET)
    list(APPEND files_to_depend_on "${tgt_dir}/${file_to_copy}")
  endforeach()
  add_custom_target(
    ${opt_TARGET}
    COMMENT "Copying ${src_dir} to ${tgt_dir}"
    DEPENDS ${files_to_depend_on})
endfunction()
copy_share_subdir_at_build_time(SUBDIR demo TARGET copy_share_demo_to_build)
copy_share_subdir_at_build_time(SUBDIR lib TARGET copy_share_lib_to_build)

# =============================================================================
# Add tests if enabled
# =============================================================================
if(NRN_ENABLE_TESTS)
  # If CoreNEURON + NMODL are enabled, Catch2 will already have been set up...
  if(NOT TARGET Catch2::Catch2)
    nrn_add_external_project(catch2)
    set(CATCH_DIR ${PROJECT_SOURCE_DIR}/external/catch2)
    list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/external/catch2/contrib)
    include(Catch)
  endif()
  include(CTest)
  if(NRN_ENABLE_PYTHON)
    # Need this for *all* Python versions we try and run tests with
    nrn_find_python_module(MODULE pytest ALL REQUIRED)
    nrn_find_python_module(MODULE pytest_cov ALL)
    if(NOT PYTEST_COV_FOUND)
      message(STATUS "pytest-cov package not installed. Python coverage will not be generated.")
    endif()
  endif()
  add_dependencies(nrniv_lib copy_share_demo_to_build)
  # Execute neurondemo as part of the build because it lazily calls nrnivmodl. If we don't do this
  # then test_neurondemo.py will run nrnivmodl in a clean build, and report compilation warnings as
  # unexpected stderr.
  set(neurondemo_prefix
      "${PROJECT_BINARY_DIR}/share/nrn/demo/release/${CMAKE_HOST_SYSTEM_PROCESSOR}")
  set(neurondemo_files
      "${neurondemo_prefix}/special"
      "${neurondemo_prefix}/${CMAKE_SHARED_LIBRARY_PREFIX}nrnmech${CMAKE_SHARED_LIBRARY_SUFFIX}")
  add_custom_command(
    OUTPUT ${neurondemo_files}
    COMMAND ${CMAKE_COMMAND} -E env ${NRN_RUN_FROM_BUILD_DIR_ENV}
            ${NRN_SANITIZER_ENABLE_ENVIRONMENT} "${PROJECT_BINARY_DIR}/bin/nrnivmodl"
    WORKING_DIRECTORY ${PROJECT_BINARY_DIR}/share/nrn/demo/release
    VERBATIM)

  add_custom_target(generate-neurondemo-mechanism-library ALL DEPENDS ${neurondemo_files} nrniv
                                                                      copy_share_lib_to_build)
  # Initialize the submodule *before* including the test/CMakeLists.txt that uses it. This ensures
  # that the test infrastructure can find the names of the input data files and set up rules to copy
  # them into the test working directories.
  set(backup "${${CODING_CONV_PREFIX}_3RDPARTY_DIR}")
  set(${CODING_CONV_PREFIX}_3RDPARTY_DIR "test")
  cpp_cc_git_submodule(rxd/testdata)
  set(${CODING_CONV_PREFIX}_3RDPARTY_DIR "${backup}")
  add_subdirectory(test)
endif()

# =============================================================================
# Install targets
# =============================================================================
install(
  DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/share/lib
  DESTINATION ${NRN_INSTALL_SHARE_DIR}
  PATTERN "python" EXCLUDE)
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/share/demo DESTINATION ${NRN_INSTALL_SHARE_DIR})
install(FILES ${PROJECT_BINARY_DIR}/share/nrn/lib/nrnunits.lib
              ${PROJECT_BINARY_DIR}/share/nrn/lib/nrn.defaults
        DESTINATION ${NRN_INSTALL_SHARE_DIR}/lib)
install(PROGRAMS ${CMAKE_CURRENT_SOURCE_DIR}/share/lib/cleanup
        DESTINATION ${NRN_INSTALL_SHARE_DIR}/lib)

# Copy NEURON headers that will be included in the installation into the build directory.
set(headers_in_build_dir)
foreach(header_below_src ${HEADER_FILES_TO_INSTALL})
  get_filename_component(header_name "${header_below_src}" NAME)
  set(output_name "${PROJECT_BINARY_DIR}/include/${header_name}")
  cpp_cc_build_time_copy(
    INPUT "${PROJECT_SOURCE_DIR}/src/${header_below_src}"
    OUTPUT "${output_name}"
    NO_TARGET)
  list(APPEND headers_in_build_dir "${output_name}")
endforeach()
foreach(header_below_src ${STRUCTURED_HEADER_FILES_TO_INSTALL})
  set(output_name "${PROJECT_BINARY_DIR}/include/${header_below_src}")
  cpp_cc_build_time_copy(
    INPUT "${PROJECT_SOURCE_DIR}/src/${header_below_src}"
    OUTPUT "${output_name}"
    NO_TARGET)
  list(APPEND headers_in_build_dir "${output_name}")
endforeach()
add_custom_target(
  copy_headers_to_build
  COMMENT "Copying headers to build directory"
  DEPENDS ${headers_in_build_dir})
add_dependencies(nrniv_lib copy_headers_to_build)
install(DIRECTORY ${PROJECT_BINARY_DIR}/include DESTINATION ${CMAKE_INSTALL_PREFIX})

if(NRN_MACOS_BUILD)
  # universal build for neurondemo needs to be after, or at end of, install
  nrn_macos_after_install()
endif()

# =============================================================================
# Copy bash executable for windows
# =============================================================================
if(MINGW)
  # ~~~
  # nrniv.cpp calls nrnpyenv.sh with absolute path to bash.exe
  # this is sufficient on the build machine since a full
  # development environment exists. On the users install machine
  # using a setup.exe distribution, the setup.ex will contain a
  # minimal development environment with sufficient mingw programs
  # to allow nrnpyenv.sh to work. (see nrn/mingw_files/nrnmingwenv.sh)
  # ~~~
  find_file(BASH_EXE bash.exe DOC "DOS path to bash.exe")
  message(STATUS "Found bash.exe at ${BASH_EXE}")
  if("${BASH_EXE}" STREQUAL "BASH_EXE-NOTFOUND")
    set(BASH_EXE "f:/msys64/usr/bin/bash.exe")
    message(WARNING "Can not find bash.exe, trying to use ${BASH_EXE}")
  endif()
  install(PROGRAMS ${BASH_EXE} DESTINATION ${CMAKE_INSTALL_PREFIX}/mingw/usr/bin)
endif()

# =============================================================================
# Installation steps for backward compatibility
# =============================================================================
if(NOT NRN_WINDOWS_BUILD)
  # create arch folder under prefix with symlink to bin and lib
  nrn_install_dir_symlink(${CMAKE_INSTALL_PREFIX}/bin
                          ${CMAKE_INSTALL_PREFIX}/${CMAKE_HOST_SYSTEM_PROCESSOR}/bin)
  nrn_install_dir_symlink(${CMAKE_INSTALL_PREFIX}/lib
                          ${CMAKE_INSTALL_PREFIX}/${CMAKE_HOST_SYSTEM_PROCESSOR}/lib)
endif()

# =============================================================================
# Print build status
# =============================================================================

# just for printing the compiler flags in the build status
string(TOUPPER ${CMAKE_BUILD_TYPE} BUILD_TYPE_UPPER)
if(BUILD_TYPE_UPPER MATCHES "CUSTOM")
  set(COMPILER_FLAGS "${CMAKE_CXX_FLAGS}")
else()
  set(COMPILER_FLAGS "${CMAKE_CXX_FLAGS_${BUILD_TYPE_UPPER}}")
endif()
string(JOIN " " COMPILER_FLAGS "${COMPILER_FLAGS}" ${NRN_COMPILE_FLAGS} ${CMAKE_CXX_FLAGS})

message(STATUS "")
message(STATUS "Configured NEURON ${PROJECT_VERSION}")
message(STATUS "")
message(STATUS "You can now build NEURON using:")
message(STATUS "  cmake --build . --parallel 8 [--target TARGET]")
message(STATUS "You might want to adjust the number of parallel build jobs for your system.")
message(STATUS "Some non-default targets you might want to build:")
message(STATUS "--------------+--------------------------------------------------------------")
message(STATUS " Target       |   Description")
message(STATUS "--------------+--------------------------------------------------------------")
message(STATUS "install       | Will install NEURON to: ${CMAKE_INSTALL_PREFIX}")
message(STATUS "              | Change the install location of NEURON using:")
message(STATUS "              |   cmake <src_path> -DCMAKE_INSTALL_PREFIX=<install_path>")
message(
  STATUS
    "docs          | Build full docs. Calls targets: doxygen, notebooks, sphinx, notebooks-clean")
message(STATUS "uninstall     | Removes files installed by make install (todo)")
message(STATUS "--------------+--------------------------------------------------------------")
message(STATUS " Build option | Status")
message(STATUS "--------------+--------------------------------------------------------------")
message(STATUS "C COMPILER    | ${CMAKE_C_COMPILER}")
message(STATUS "CXX COMPILER  | ${CMAKE_CXX_COMPILER}")
message(STATUS "BUILD_TYPE    | ${CMAKE_BUILD_TYPE} (allowed: ${allowableBuildTypes})")
message(STATUS "COMPILE FLAGS | ${COMPILER_FLAGS}")
message(STATUS "Shared        | ${NRN_ENABLE_SHARED}")
message(STATUS "MPI           | ${NRN_ENABLE_MPI}")
if(NRN_ENABLE_MPI)
  message(STATUS "  DYNAMIC     | ${NRN_ENABLE_MPI_DYNAMIC}")
  if(NRN_ENABLE_MPI_DYNAMIC)
    list(LENGTH NRN_MPI_LIBNAME_LIST _num_mpi)
    math(EXPR num_mpi "${_num_mpi} - 1")
    foreach(val RANGE ${num_mpi})
      list(GET NRN_MPI_LIBNAME_LIST ${val} libname)
      list(GET NRN_MPI_INCLUDE_LIST ${val} include)
      message(STATUS "    LIBNAME   | ${libname}")
      message(STATUS "    INC       | ${include}")
    endforeach(val)
  else()
    if(NRN_INCLUDE_MPI_HEADERS)
      message(STATUS "  INC         | ${MPI_C_INCLUDE_DIRS}")
    else()
      message(STATUS "  INC         | N/A")
    endif()
    message(STATUS "  LIB         | ${MPI_C_LIBRARIES}")
  endif()
endif()
if(NRN_ENABLE_MUSIC)
  message(STATUS "MUSIC         | ${MUSIC_LIBDIR}")
endif()
message(STATUS "Python        | ${NRN_ENABLE_PYTHON}")
if(NRN_ENABLE_PYTHON)
  message(STATUS "  DYNAMIC     | ${NRN_ENABLE_PYTHON_DYNAMIC}")
  message(STATUS "  MODULE      | ${NRN_ENABLE_MODULE_INSTALL}")
  foreach(val RANGE ${NRN_PYTHON_ITERATION_LIMIT})
    list(GET NRN_PYTHON_EXECUTABLES ${val} exe)
    list(GET NRN_PYTHON_VERSIONS ${val} pyver)
    list(GET NRN_PYTHON_INCLUDES ${val} pyinc)
    list(GET NRN_PYTHON_LIBRARIES ${val} pylib)
    unset(suffix)
    if(val EQUAL 0)
      set(suffix " (default)")
    endif()
    message(STATUS " python${pyver}${suffix}")
    message(STATUS "  EXE         | ${exe}")
    message(STATUS "  INC         | ${pyinc}")
    message(STATUS "  LIB         | ${pylib}")
  endforeach()
endif()
if(READLINE_FOUND)
  message(STATUS "Readline      | ${Readline_LIBRARY}")
endif()
if(CURSES_FOUND)
  message(STATUS "Curses        | ${CURSES_LIBRARIES}")
elseif(TERMCAP_FOUND)
  message(STATUS "Termcap       | ${TERMCAP_LIBRARIES}")
endif()
message(STATUS "RX3D          | ${NRN_ENABLE_RX3D}")
if(${NRN_ENABLE_RX3D})
  message(STATUS "  OptLevel    | ${NRN_RX3D_OPT_LEVEL}")
endif()
message(STATUS "Interviews    | ${NRN_ENABLE_INTERVIEWS}")
if(NRN_ENABLE_INTERVIEWS)
  message(STATUS "  PATH        | ${IV_DIR}")
  message(STATUS "  INC         | ${IV_INCLUDE_DIR}")
  message(STATUS "  X11 (INC)   | ${X11_INCLUDE_DIR}")
  message(STATUS "      (LIBDIR)| ${X11_LIBRARY_DIR}")
  if(IV_ENABLE_X11_DYNAMIC)
    message(STATUS "              | IV_ENABLE_X11_DYNAMIC ${IV_ENABLE_X11_DYNAMIC}")
    message(STATUS "              | IV_ENABLE_SHARED ${IV_ENABLE_SHARED}")
  endif()
endif()
message(STATUS "CoreNEURON    | ${NRN_ENABLE_CORENEURON}")
if(NRN_ENABLE_CORENEURON)
  message(STATUS "  PATH        | ${CORENEURON_DIR}")
  message(STATUS "  LINK FLAGS  | ${CORENRN_LIB_LINK_FLAGS}")
endif()
if(NRN_UNIVERSAL2_BUILD)
  message(STATUS "CMAKE_OSX_ARCH| ${CMAKE_OSX_ARCHITECTURES}")
endif()
message(STATUS "Tests         | ${NRN_ENABLE_TESTS}")
if(NRN_ENABLE_COVERAGE)
  message(STATUS "Coverage      | Enabled")
  if(NRN_COVERAGE_FILES)
    message(STATUS "  Files       | ${NRN_COVERAGE_FILES}")
  endif()
endif()
if(NRN_ENABLE_PROFILING)
  message(STATUS "Profiling     | ON")
  if(NRN_PROFILER STREQUAL "caliper")
    message(STATUS "  Caliper     | ${caliper_DIR}")
  endif()
endif()
message(STATUS "--------------+--------------------------------------------------------------")
message(STATUS " See documentation : https://www.neuron.yale.edu/neuron/")
message(STATUS "--------------+--------------------------------------------------------------")
message(STATUS "")
