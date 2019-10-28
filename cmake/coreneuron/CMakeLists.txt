# =============================================================================
# Copyright (C) 2016-2019 Blue Brain Project
#
# See top-level LICENSE file for details.
# =============================================================================

cmake_minimum_required(VERSION 3.7 FATAL_ERROR)
project(coreneuron)

# =============================================================================
# CMake common project settings
# =============================================================================
set(VERSION_MAJOR "0")
set(VERSION_MINOR "16")
set(VERSION_PATCH "0")
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(VERSION_STRING "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}")

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
list(APPEND CMAKE_MODULE_PATH
            ${CORENEURON_PROJECT_SOURCE_DIR}/CMake
            ${CORENEURON_PROJECT_SOURCE_DIR}/CMake/packages
            ${CORENEURON_PROJECT_SOURCE_DIR}/CMake/config)

# =============================================================================
# Include common cmake modules
# =============================================================================
include(CheckIncludeFiles)
include(ReleaseDebugAutoFlags)
include(CrayPortability)
include(SetRpath)
include(CTest)

# =============================================================================
# Build options
# =============================================================================
option(CORENRN_ENABLE_OPENMP "Build the CORE NEURON with OpenMP implementation" ON)
option(CORENRN_ENABLE_TIMEOUT "Enable nrn_timeout implementation" ON)
option(CORENRN_ENABLE_REPORTINGLIB "Enable use of ReportingLib for soma reports" OFF)
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

set(CORENRN_NMODL_DIR "" CACHE PATH "Path to nmodl source-to-source compiler installation")
set(LIKWID_DIR "" CACHE PATH "Path to likwid performance analysis suite")

set(CORENRN_FRONTEND_C_COMPILER gcc CACHE FILEPATH "C compiler for building mod2c [frontend]")
set(CORENRN_FRONTEND_CXX_COMPILER g++ CACHE FILEPATH "C++ compiler for building mod2c [frontend]")

# =============================================================================
# Include cmake modules after cmake options
# =============================================================================
include(OpenAccHelper)
include(ClangFormatHelper)

# =============================================================================
# Common build options
# =============================================================================
# build mod files for coreneuron
add_definitions(-DCORENEURON_BUILD)

check_include_files(malloc.h have_malloc_h)
if(have_malloc_h)
  add_definitions("-DHAVE_MALLOC_H")
endif()

# =============================================================================
# Build option specific compiler flags
# =============================================================================
if(${CMAKE_C_COMPILER_ID} STREQUAL "PGI")
  add_definitions(-DSWAP_ENDIAN_DISABLE_ASM)
endif()

if(CORENRN_ENABLE_ISPC)
  enable_language(ISPC)
  add_definitions("-DISPC_INTEROP=1")
  set(CORENRN_ENABLE_NMODL ON)
endif()

if(CORENRN_ENABLE_MPI)
  find_package(MPI REQUIRED)
  add_definitions("-DNRNMPI=1")
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

if(CORENRN_ENABLE_REPORTINGLIB)
  find_package(reportinglib REQUIRED)
  include_directories(${reportinglib_INCLUDE_DIR})
  add_definitions("-DENABLE_REPORTING")
endif()

if(MINGW)
  add_definitions("-DMINGW")
endif()

# =============================================================================
# NMODL specific options
# =============================================================================
if(CORENRN_ENABLE_NMODL)
  find_package(nmodl)
  if (nmodl_FOUND)
    set(CORENRN_NMODL_BINARY ${nmodl_BINARY})
    set(CORENRN_NMODL_INCLUDE ${nmodl_INCLUDE})
    # path to python interface
    set(ENV{PYTHONPATH} "${nmodl_PYTHONPATH}:$ENV{PYTHONPATH}")
    set(CORENRN_NMODL_PYTHONPATH $ENV{PYTHONPATH})
  else()
    include(AddNmodlSubmodule)
    set(CORENRN_NMODL_BINARY ${CMAKE_BINARY_DIR}/bin/nmodl${CMAKE_EXECUTABLE_SUFFIX})
    set(CORENRN_NMODL_INCLUDE ${CMAKE_BINARY_DIR}/include)
  endif()
  include_directories(${CORENRN_NMODL_INCLUDE})
  set(CORENRN_NMODL_FLAGS "" CACHE STRING "Extra NMODL options such as passes")
  separate_arguments(NMODL_EXTRA_FLAGS_LIST UNIX_COMMAND "${CORENRN_NMODL_FLAGS}")
else()
  include(AddMod2cSubmodule)
  set(CORENRN_NMODL_BINARY ${CMAKE_BINARY_DIR}/bin/mod2c_core${CMAKE_EXECUTABLE_SUFFIX})
  set(CORENRN_NMODL_INCLUDE ${CMAKE_BINARY_DIR}/include)
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
# Add main directories
# =============================================================================
add_subdirectory(coreneuron)
add_subdirectory(apps)
add_subdirectory(tests)
add_subdirectory(extra)

# =============================================================================
# Install cmake modules
# =============================================================================
install(FILES CMake/coreneuron-config.cmake DESTINATION share/cmake)
install(EXPORT coreneuron DESTINATION share/cmake)

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
  message(STATUS "make uninstall      | Removes files installed by make install")
  message(STATUS "--------------------+--------------------------------------------------------")
  message(STATUS " Build option       | Status")
  message(STATUS "--------------------+--------------------------------------------------------")

  message(STATUS "MPI                 | ${CORENRN_ENABLE_MPI}")
  if(CORENRN_ENABLE_MPI)
    message(STATUS "  INC               | ${MPI_C_INCLUDE_PATH}")
  endif()
  message(STATUS "OpenMP              | ${CORENRN_ENABLE_OPENMP}")
  message(STATUS "NMODL               | ${CORENRN_ENABLE_NMODL}")
  if(CORENRN_ENABLE_NMODL)
    message(STATUS "  PATH              | ${CORENRN_NMODL_BINARY}")
    message(STATUS "  ISPC              | ${CORENRN_ENABLE_ISPC}")
  else()
    message(STATUS "MOD2C               | ${MOD2C_PROJ}")
  endif()
  message(STATUS "GPU Support         | ${CORENRN_ENABLE_GPU}")
  message(STATUS "CUDA Unified Memory | ${CORENRN_ENABLE_CUDA_UNIFIED_MEMORY}")
  message(STATUS "Auto Timeout        | ${CORENRN_ENABLE_TIMEOUT}")
  message(STATUS "Wrap exp()          | ${CORENRN_ENABLE_HOC_EXP}")
  message(STATUS "SplayTree Queue     | ${CORENRN_ENABLE_SPLAYTREE_QUEUING}")
  message(STATUS "NetReceive Buffer   | ${CORENRN_ENABLE_NET_RECEIVE_BUFFER}")
  message(STATUS "Caliper             | ${CORENRN_ENABLE_CALIPER_PROFILING}")
  message(STATUS "Likwid              | ${CORENRN_ENABLE_LIKWID_PROFILING}")
  message(STATUS "Unit Tests          | ${CORENRN_ENABLE_UNIT_TESTS}")
  message(STATUS "ReportingLib        | ${CORENRN_ENABLE_REPORTINGLIB}")
  if(CORENRN_ENABLE_REPORTINGLIB)
    message(STATUS "  INC               | ${reportinglib_INCLUDE_DIR}")
  endif()

  message(STATUS "--------------+--------------------------------------------------------------")
  message(STATUS " See documentation : https://github.com/BlueBrain/CoreNeuron/")
  message(STATUS "--------------+--------------------------------------------------------------")
endif()
message(STATUS "")
