# Ecole Polytechnique Federale de Lausanne
# Brain Mind Institute,
# Blue Brain Project
# (c) 2006-2013. All rights reserved.
#
# Author: Aleksandr Ovcharenko
# Core Neuron


# Initial Setup

CMAKE_MINIMUM_REQUIRED(VERSION 2.8)
PROJECT(coreneuron)

set(VERSION_MAJOR "0")
set(VERSION_MINOR "6")
set(VERSION_PATCH "0")
set(VERSION_ABI 1)

list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/CMake ${PROJECT_SOURCE_DIR}/CMake/common)
include(GitExternal)

set(CORENEURON_DESCRIPTION "BBP CoreNeuron simulator")

if(${CMAKE_SYSTEM_PROCESSOR} STREQUAL "ppc64")
  set(CORENEURON_LIBRARY_TYPE STATIC)
  set(Boost_USE_STATIC_LIBS ON)
endif()

# Threading
option (CORENEURON_OPENMP "Build the CORE NEURON with OpenMP implementation" ON)
if(CORENEURON_OPENMP)
  find_package(OpenMP)
endif(CORENEURON_OPENMP)

include(Common)
include(FindPackages)

INCLUDE (CheckIncludeFiles)
CHECK_INCLUDE_FILES (malloc.h have_malloc_h)

if(have_malloc_h)
  add_definitions(-DHAVE_MALLOC_H)
endif()

if(${CMAKE_SYSTEM_PROCESSOR} STREQUAL "ppc64")
  CHECK_INCLUDE_FILES("spi/include/kernel/memory.h" have_memory_h)
  if(have_memory_h)
    add_definitions(-DHAVE_MEMORY_H)
  endif()
endif()


if(MPI_FOUND)
  set(CMAKE_C_COMPILER ${MPI_C_COMPILER})
  set(CMAKE_CXX_COMPILER ${MPI_CXX_COMPILER})
endif(MPI_FOUND)

# Core library

add_subdirectory(coreneuron)


# Main

option (CORENEURON_MAIN "Build the CORE NEURON main" ON)
if(CORENEURON_MAIN)
    add_subdirectory(apps)
endif(CORENEURON_MAIN)


# Tests

option(CORENEURON_TESTS "Compilation of CoreNeuron unit test" ON)
if(CORENEURON_TESTS)
  if(NOT Boost_FOUND)
    message("Disabling tests: Boost libraries not found")
    set(CORENEURON_TESTS OFF)
  else()
    add_subdirectory(tests)
  endif()
endif(CORENEURON_TESTS)


# Documentation

set(GIT_DOCUMENTATION_REPO BBPDocumentation)
set(COMMON_ORGANIZATION_NAME BBP)
set(COMMON_PROJECT_DOMAIN ch.epfl.bluebrain)
include(DoxygenRule)
include(CPackConfig)
