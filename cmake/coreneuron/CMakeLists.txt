# =============================================================================
# Copyright (C) 2016-2020 Blue Brain Project
#
# See top-level LICENSE file for details.
# =============================================================================

cmake_minimum_required(VERSION 3.7 FATAL_ERROR)
project(coreneuron VERSION 0.21.0)

# ~~~
# It is a bad idea having floating point versions, since macros cant handle them
# We therefore, have version as an int, which is pretty much standard
# ~~~
math(EXPR CORENEURON_VERSION_COMBINED
     "${coreneuron_VERSION_MAJOR} * 100 + ${coreneuron_VERSION_MINOR}")

# =============================================================================
# CMake common project settings
# =============================================================================
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_BUILD_TYPE
    RelWithDebInfo
    CACHE STRING "Empty or one of Debug, Release, RelWithDebInfo")

# =============================================================================
# Settings to enable project as submodule
# =============================================================================
set(CORENEURON_PROJECT_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(CORENEURON_AS_SUBPROJECT OFF)
if(NOT CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
  set(CORENEURON_AS_SUBPROJECT ON)
endif()

# =============================================================================
# Include cmake modules path
# =============================================================================
list(APPEND CMAKE_MODULE_PATH ${CORENEURON_PROJECT_SOURCE_DIR}/CMake
            ${CORENEURON_PROJECT_SOURCE_DIR}/CMake/packages
            ${CORENEURON_PROJECT_SOURCE_DIR}/CMake/config)

# =============================================================================
# HPC Coding Conventions
# =============================================================================
set(CODING_CONV_PREFIX "CORENRN")
set(CORENRN_ClangFormat_EXCLUDES_RE
    ".*/external/.*$$"
    CACHE STRING "list of regular expressions to exclude C/C++ files from formatting" FORCE)
set(CORENRN_CMakeFormat_EXCLUDES_RE
    ".*/external/.*$$" ".*/CMake/packages/.*$$"
    CACHE STRING "list of regular expressions to exclude CMake files from formatting" FORCE)
include(AddHpcCodingConvSubmodule)
add_subdirectory(CMake/hpc-coding-conventions/cpp)

# =============================================================================
# Include common cmake modules
# =============================================================================
include(CheckIncludeFiles)
include(ReleaseDebugAutoFlags)
include(CrayPortability)
include(SetRpath)
include(CTest)
include(AddRandom123Submodule)
include(AddCLI11Submodule)
include(GitRevision)

add_subdirectory(${CORENEURON_PROJECT_SOURCE_DIR}/external/CLI11)

# =============================================================================
# Build options
# =============================================================================
option(CORENRN_ENABLE_OPENMP "Build the CORE NEURON with OpenMP implementation" ON)
option(CORENRN_ENABLE_TIMEOUT "Enable nrn_timeout implementation" ON)
option(CORENRN_ENABLE_REPORTING "Enable use of ReportingLib for soma reports" OFF)
option(CORENRN_ENABLE_MPI "Enable MPI-based execution" ON)
option(CORENRN_ENABLE_SOA "Enable SoA Memory Layout" ON)
option(CORENRN_ENABLE_HOC_EXP "Enable wrapping exp with hoc_exp()" OFF)
option(CORENRN_ENABLE_SPLAYTREE_QUEUING "Enable use of Splay tree for spike queuing" ON)
option(CORENRN_ENABLE_NET_RECEIVE_BUFFER "Enable event buffering in net_receive function" ON)
option(CORENRN_ENABLE_ISPC "Enable ispc interoperability structs and data" OFF)
option(CORENRN_ENABLE_NMODL "Enable external nmodl source-to-source compiler" OFF)
option(CORENRN_ENABLE_CALIPER_PROFILING "Enable Caliper instrumentation" OFF)
option(CORENRN_ENABLE_LIKWID_PROFILING "Enable LIKWID instrumentation" OFF)
option(CORENRN_ENABLE_CUDA_UNIFIED_MEMORY "Enable CUDA unified memory support" OFF)
option(CORENRN_ENABLE_UNIT_TESTS "Enable unit tests execution" ON)
option(CORENRN_ENABLE_GPU "Enable GPU support using OpenACC" OFF)
option(CORENRN_ENABLE_SHARED "Enable shared library build" ON)
option(CORENRN_ENABLE_LEGACY_UNITS "Enable legacy FARADAY, R, etc" OFF)

set(CORENRN_NMODL_DIR
    ""
    CACHE PATH "Path to nmodl source-to-source compiler installation")
set(LIKWID_DIR
    ""
    CACHE PATH "Path to likwid performance analysis suite")

if(CORENEURON_AS_SUBPROJECT)
  set(CORENRN_ENABLE_UNIT_TESTS OFF)
endif()

# =============================================================================
# Project version from git and project directories
# =============================================================================
set(CN_PROJECT_VERSION ${PROJECT_VERSION})

# generate file with version number from git and nrnunits.lib file path
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/coreneuron/config/config.cpp.in
               ${PROJECT_BINARY_DIR}/coreneuron/config/config.cpp @ONLY)

# =============================================================================
# Include cmake modules after cmake options
# =============================================================================
include(OpenAccHelper)

# =============================================================================
# Common dependencies
# =============================================================================
find_package(PythonInterp REQUIRED)
find_package(Perl REQUIRED)

# =============================================================================
# Common build options
# =============================================================================
# build mod files for coreneuron
add_definitions(-DCORENEURON_BUILD)

set(CMAKE_REQUIRED_QUIET TRUE)
check_include_files(malloc.h have_malloc_h)
if(have_malloc_h)
  add_definitions("-DHAVE_MALLOC_H")
endif()

# =============================================================================
# Build option specific compiler flags
# =============================================================================
if(${CMAKE_CXX_COMPILER_ID} STREQUAL "PGI")
  # PGI with llvm code generation doesn't have necessary assembly intrinsic headers
  add_definitions(-DEIGEN_DONT_VECTORIZE=1)
endif()

# ~~~
# OpenACC needs to build static library in order to have global/routines working.
# See https://www.pgroup.com/userforum/viewtopic.php?t=5350
# ~~~
if(CORENRN_ENABLE_GPU)
  set(CORENRN_ENABLE_SHARED OFF)
endif()

if(CORENRN_ENABLE_SHARED)
  set(COMPILE_LIBRARY_TYPE "SHARED")
else()
  set(COMPILE_LIBRARY_TYPE "STATIC")
endif()

if(CORENRN_ENABLE_ISPC)
  enable_language(ISPC)
  set(CORENRN_ENABLE_NMODL ON)
endif()

if(CORENRN_ENABLE_MPI)
  find_package(MPI REQUIRED)
  add_definitions("-DNRNMPI=1")
  # avoid linking to C++ bindings
  add_definitions("-DMPI_NO_CPPBIND=1")
  add_definitions("-DOMPI_SKIP_MPICXX=1")
  add_definitions("-DMPICH_SKIP_MPICXX=1")
else()
  add_definitions("-DNRNMPI=0")
  add_definitions("-DNRN_MULTISEND=0")
endif()

if(CORENRN_ENABLE_OPENMP)
  find_package(OpenMP QUIET)
  if(OPENMP_FOUND)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS} ${ADDITIONAL_THREADSAFE_FLAGS}")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS} ${ADDITIONAL_THREADSAFE_FLAGS}")
  endif()
endif()

if(CORENRN_ENABLE_SOA)
  add_definitions("-DLAYOUT=0")
else()
  add_definitions("-DLAYOUT=1")
endif()

if(NOT CORENRN_ENABLE_HOC_EXP)
  add_definitions("-DDISABLE_HOC_EXP")
endif()

# splay tree required for net_move
if(CORENRN_ENABLE_SPLAYTREE_QUEUING)
  add_definitions("-DENABLE_SPLAYTREE_QUEUING")
endif()

if(NOT CORENRN_ENABLE_NET_RECEIVE_BUFFER)
  add_definitions("-DNET_RECEIVE_BUFFERING=0")
endif()

if(NOT CORENRN_ENABLE_TIMEOUT)
  add_definitions("-DDISABLE_TIMEOUT")
endif()

if(CORENRN_ENABLE_REPORTING)
  find_package(reportinglib)
  find_package(sonata)
  find_program(H5DUMP_EXECUTABLE h5dump)
  if(NOT reportinglib_FOUND AND NOT sonata_FOUND)
    message(SEND_ERROR "Neither reportinglib nor SONATA libraries were found")
  elseif(reportinglib_FOUND AND sonata_FOUND)
    add_definitions("-DENABLE_BIN_REPORTS")
    add_definitions("-DENABLE_SONATA_REPORTS")
  elseif(reportinglib_FOUND)
    add_definitions("-DENABLE_BIN_REPORTS")
    set(ENABLE_BIN_REPORTS ON)
  else() # sonata_FOUND
    if(TARGET sonata::sonata_report)
      add_definitions("-DENABLE_SONATA_REPORTS")
      set(ENABLE_SONATA_REPORTS ON)
      set(reportinglib_INCLUDE_DIR "")
      set(reportinglib_LIBRARY "")
    else()
      message(SEND_ERROR "SONATA library was found but without reporting support")
    endif()
  endif()
  include_directories(${reportinglib_INCLUDE_DIR})
  include_directories(${sonatareport_INCLUDE_DIR})
endif()

if(CORENRN_ENABLE_LEGACY_UNITS)
  set(CORENRN_USE_LEGACY_UNITS 1)
else()
  set(CORENRN_USE_LEGACY_UNITS 0)
endif()
# Propagate Legacy Units flag to backends.
set(MOD2C_ENABLE_LEGACY_UNITS ${CORENRN_ENABLE_LEGACY_UNITS} CACHE BOOL "" FORCE)
set(NMODL_ENABLE_LEGACY_UNITS ${CORENRN_ENABLE_LEGACY_UNITS} CACHE BOOL "" FORCE)

if(MINGW)
  add_definitions("-DMINGW")
endif()

# =============================================================================
# NMODL specific options
# =============================================================================
if(CORENRN_ENABLE_NMODL)
  find_package(nmodl)
  if(nmodl_FOUND)
    set(CORENRN_MOD2CPP_BINARY ${nmodl_BINARY})
    set(CORENRN_MOD2CPP_INCLUDE ${nmodl_INCLUDE})
    # path to python interface
    set(ENV{PYTHONPATH} "${nmodl_PYTHONPATH}:$ENV{PYTHONPATH}")
    set(CORENRN_NMODL_PYTHONPATH $ENV{PYTHONPATH})
  else()
    set(NMODL_ENABLE_PYTHON_BINDINGS OFF CACHE BOOL "Disable NMODL python bindings")
    include(AddNmodlSubmodule)
    set(CORENRN_MOD2CPP_BINARY ${CMAKE_BINARY_DIR}/bin/nmodl${CMAKE_EXECUTABLE_SUFFIX})
    set(CORENRN_MOD2CPP_INCLUDE ${CMAKE_BINARY_DIR}/include)
    set(ENV{PYTHONPATH} "$ENV{PYTHONPATH}")
    set(nmodl_PYTHONPATH "${CMAKE_BINARY_DIR}/lib")
    set(CORENRN_NMODL_PYTHONPATH "${nmodl_PYTHONPATH}:$ENV{PYTHONPATH}")
    set(NMODL_TARGET_TO_DEPEND nmodl)
  endif()
  include_directories(${CORENRN_MOD2CPP_INCLUDE})
  # set correct arguments for nmodl for cpu/gpu target
  set(CORENRN_NMODL_FLAGS
      ""
      CACHE STRING "Extra NMODL options such as passes")
  if(CORENRN_ENABLE_GPU)
    string(APPEND CORENRN_NMODL_FLAGS " acc --oacc")
  endif()
else()
  include(AddMod2cSubmodule)
  set(CORENRN_MOD2CPP_BINARY ${CMAKE_BINARY_DIR}/bin/mod2c_core${CMAKE_EXECUTABLE_SUFFIX})
  set(CORENRN_MOD2CPP_INCLUDE ${CMAKE_BINARY_DIR}/include)
endif()

# =============================================================================
# Profiler/Instrumentation Options
# =============================================================================
if(CORENRN_ENABLE_CALIPER_PROFILING)
  find_package(caliper REQUIRED)
  include_directories(${caliper_INCLUDE_DIR})
  add_definitions("-DCORENEURON_CALIPER")
  set(CALIPER_LIB "caliper")
  if(CORENRN_ENABLE_MPI)
    set(CALIPER_MPI_LIB "caliper-mpi")
  endif()
endif()

if(CORENRN_ENABLE_LIKWID_PROFILING)
  find_package(likwid REQUIRED)
  include_directories(${likwid_INCLUDE_DIRS})
  add_definitions("-DLIKWID_PERFMON")
endif()

# =============================================================================
# Common CXX flags : ignore unknown pragma warnings
# =============================================================================
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${IGNORE_UNKNOWN_PRAGMA_FLAGS}")

# =============================================================================
# Add main directories
# =============================================================================
add_subdirectory(coreneuron)

include(MakefileBuildOptions)
add_subdirectory(extra)

if(CORENRN_ENABLE_UNIT_TESTS)
  add_subdirectory(tests)
endif()

# =============================================================================
# Install cmake modules
# =============================================================================
install(FILES CMake/coreneuron-config.cmake DESTINATION share/cmake)
install(EXPORT coreneuron DESTINATION share/cmake)

if(NOT CORENEURON_AS_SUBPROJECT)
  # =============================================================================
  # Setup Doxygen documentation
  # =============================================================================
  find_package(Doxygen QUIET)
  if(DOXYGEN_FOUND)
    # generate Doxyfile with correct source paths
    configure_file(${PROJECT_SOURCE_DIR}/docs/Doxyfile.in ${PROJECT_BINARY_DIR}/Doxyfile)
    add_custom_target(
      doxygen
      COMMAND ${DOXYGEN_EXECUTABLE} ${PROJECT_BINARY_DIR}/Doxyfile
      WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
      COMMENT "Generating API documentation with Doxygen"
      VERBATIM)
  endif()

  # =============================================================================
  # Setup Sphinx documentation
  # =============================================================================
  find_package(Sphinx QUIET)
  if(SPHINX_FOUND)
    set(SPHINX_SOURCE ${PROJECT_SOURCE_DIR}/docs)
    set(SPHINX_BUILD ${PROJECT_BINARY_DIR}/docs/)

    add_custom_target(
      sphinx
      COMMAND ${SPHINX_EXECUTABLE} -b html ${SPHINX_SOURCE} ${SPHINX_BUILD}
      WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
      COMMENT "Generating documentation with Sphinx")
  endif()

  # =============================================================================
  # Build full docs
  # =============================================================================
  if(DOXYGEN_FOUND AND SPHINX_FOUND)
    add_custom_target(
      docs
      COMMAND make doxygen
      WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
      COMMAND make sphinx
      WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
      COMMENT "Generating full documentation")
  else()
    add_custom_target(
      docs VERBATIM
      COMMAND echo "Please install docs requirements (see docs/README.md)!"
      COMMENT "Documentation generation not possible!")
  endif()
endif()
# =============================================================================
# Build status
# =============================================================================
message(STATUS "")
message(STATUS "Configured CoreNEURON ${PROJECT_VERSION}")
message(STATUS "")
string(TOLOWER "${CMAKE_GENERATOR}" cmake_generator_tolower)
if(cmake_generator_tolower MATCHES "makefile")
  message(STATUS "Some things you can do now:")
  message(STATUS "--------------------+--------------------------------------------------------")
  message(STATUS "Command             |   Description")
  message(STATUS "--------------------+--------------------------------------------------------")
  message(STATUS "make install        | Will install CoreNEURON to: ${CMAKE_INSTALL_PREFIX}")
  message(STATUS "make docs           | Build full docs. Calls targets: doxygen, sphinx")
  message(STATUS "--------------------+--------------------------------------------------------")
  message(STATUS " Build option       | Status")
  message(STATUS "--------------------+--------------------------------------------------------")

  message(STATUS "C COMPILER          | ${CMAKE_C_COMPILER}")
  message(STATUS "CXX COMPILER        | ${CMAKE_CXX_COMPILER}")
  message(STATUS "COMPILE FLAGS       | ${CORENRN_CXX_FLAGS}")
  message(STATUS "Build Type          | ${COMPILE_LIBRARY_TYPE}")
  message(STATUS "MPI                 | ${CORENRN_ENABLE_MPI}")
  if(CORENRN_ENABLE_MPI)
    message(STATUS "  INC               | ${MPI_C_INCLUDE_PATH}")
  endif()
  message(STATUS "OpenMP              | ${CORENRN_ENABLE_OPENMP}")
  message(STATUS "Use legacy units    | ${CORENRN_ENABLE_LEGACY_UNITS}")
  message(STATUS "NMODL               | ${CORENRN_ENABLE_NMODL}")
  if(CORENRN_ENABLE_NMODL)
    message(STATUS "  ISPC              | ${CORENRN_ENABLE_ISPC}")
    message(STATUS "  FLAGS             | ${CORENRN_NMODL_FLAGS}")
  endif()
  message(STATUS "MOD2CPP PATH        | ${CORENRN_MOD2CPP_BINARY}")
  message(STATUS "GPU Support         | ${CORENRN_ENABLE_GPU}")
  if(CORENRN_ENABLE_GPU)
    message(STATUS "  CUDA              | ${CUDA_TOOLKIT_ROOT_DIR}")
    message(STATUS "  Unified Memory    | ${CORENRN_ENABLE_CUDA_UNIFIED_MEMORY}")
    message(STATUS "  HOST COMPILER     | ${CUDA_HOST_COMPILER}")
  endif()
  message(STATUS "Auto Timeout        | ${CORENRN_ENABLE_TIMEOUT}")
  message(STATUS "Wrap exp()          | ${CORENRN_ENABLE_HOC_EXP}")
  message(STATUS "SplayTree Queue     | ${CORENRN_ENABLE_SPLAYTREE_QUEUING}")
  message(STATUS "NetReceive Buffer   | ${CORENRN_ENABLE_NET_RECEIVE_BUFFER}")
  message(STATUS "Caliper             | ${CORENRN_ENABLE_CALIPER_PROFILING}")
  message(STATUS "Likwid              | ${CORENRN_ENABLE_LIKWID_PROFILING}")
  message(STATUS "Unit Tests          | ${CORENRN_ENABLE_UNIT_TESTS}")
  message(STATUS "Reporting           | ${CORENRN_ENABLE_REPORTING}")
  if(CORENRN_ENABLE_REPORTING)
    message(STATUS "  sonatareport_INC  | ${sonatareport_INCLUDE_DIR}")
    message(STATUS "  sonatareport_LIB  | ${sonatareport_LIBRARY}")
    message(STATUS "  reportinglib_INC  | ${reportinglib_INCLUDE_DIR}")
    message(STATUS "  reportinglib_LIB  | ${reportinglib_LIBRARY}")
  endif()

  message(STATUS "--------------+--------------------------------------------------------------")
  message(STATUS " See documentation : https://github.com/BlueBrain/CoreNeuron/")
  message(STATUS "--------------+--------------------------------------------------------------")
endif()
message(STATUS "")
