# Ecole Polytechnique Federale de Lausanne
# Brain Mind Institute,
# Blue Brain Project
# (c) 2006-2013. All rights reserved.
#
# Author: Aleksandr Ovcharenko
# Core Bluron


# Initial Setup

CMAKE_MINIMUM_REQUIRED(VERSION 2.8)
PROJECT(CoreBluron)

set(VERSION_MAJOR "0")
set(VERSION_MINOR "3")
set(VERSION_PATCH "0")
set(VERSION_ABI 1)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/CMake ${CMAKE_SOURCE_DIR}/CMake/common)
include(GitExternal)

set(COREBLURON_DESCRIPTION "BBP CoreBluron simulator")

if(${CMAKE_SYSTEM_PROCESSOR} STREQUAL "ppc64")
  set(COREBLURON_LIBRARY_TYPE STATIC)
  set(Boost_USE_STATIC_LIBS ON)
endif()

# Threading
option (COREBLURON_OPENMP "Build the CORE BLURON with OpenMP implementation" OFF)
if(COREBLURON_OPENMP)
  find_package(OpenMP)
endif(COREBLURON_OPENMP)

include(Common)
include(FindPackages)

INCLUDE (CheckIncludeFiles)
CHECK_INCLUDE_FILES (malloc.h have_malloc_h)

if(have_malloc_h)
  add_definitions(-DHAVE_MALLOC_H)
endif()

if(MPI_FOUND)
  set(CMAKE_C_COMPILER ${MPI_C_COMPILER})
  set(CMAKE_CXX_COMPILER ${MPI_CXX_COMPILER})
endif(MPI_FOUND)

# Core library

add_subdirectory(corebluron)


# Main

option (COREBLURON_MAIN "Build the CORE BLURON main" ON)
if(COREBLURON_MAIN)
    add_subdirectory(apps)
endif(COREBLURON_MAIN)


# Tests

option(COREBLURON_TESTS "Compilation of coreBluron unit test" ON)
if(COREBLURON_TESTS)
  if(NOT Boost_FOUND)
    message("Disabling tests: Boost libraries not found")
    set(COREBLURON_TESTS OFF)
  else()
    add_subdirectory(tests)
  endif()
endif(COREBLURON_TESTS)


# Documentation

set(GIT_DOCUMENTATION_REPO BBPDocumentation)
set(COMMON_ORGANIZATION_NAME BBP)
set(COMMON_PROJECT_DOMAIN ch.epfl.bluebrain)
include(DoxygenRule)
include(CPackConfig)
