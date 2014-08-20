# Ecole Polytechnique Federale de Lausanne
# Brain Mind Institute,
# Blue Brain Project
# (c) 2006-2013. All rights reserved.
#
# Author: Aleksandr Ovcharenko
# Core Bluron

CMAKE_MINIMUM_REQUIRED(VERSION 2.8)
PROJECT(CoreBluron)

set(RELEASE_VERSION OFF)  # OFF or ABI version
set(LAST_RELEASE 0.1.0)
set (VERSION_MAJOR "0")
set (VERSION_MINOR "2")
set (VERSION_PATCH "0")

list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/CMake ${CMAKE_SOURCE_DIR}/CMake/common)
include(GitExternal)

set(COREBLURON_DESCRIPTION "BBP core simulator")
set(COMMON_PROJECT_DOMAIN ch.epfl.bluebrain)

if(${CMAKE_SYSTEM_PROCESSOR} STREQUAL "ppc64")
  set(COREBLURON_LIBRARY_TYPE STATIC)
  set(Boost_USE_STATIC_LIBS ON)
endif()

include(Common)
include(FindPackages)

if(MPI_FOUND)
  set(CMAKE_C_COMPILER ${MPI_C_COMPILER})
  set(CMAKE_CXX_COMPILER ${MPI_CXX_COMPILER})
endif(MPI_FOUND)

option (COREBLURON_TESTS "Build the CORE BLURON regression tests" OFF)
option (COREBLURON_MAIN "Build the CORE BLURON main" OFF)

########################################################################
#
# Core library
#
########################################################################

add_subdirectory(corebluron)

########################################################################
#
# Main
#
########################################################################

if(COREBLURON_MAIN)
    add_subdirectory(apps)
endif(COREBLURON_MAIN)


########################################################################
#
# Tests
#
########################################################################

set(COREBLURON_TESTS ON CACHE BOOL "Compilation of coreBluron unit test")
if(COREBLURON_TESTS)
  #this depends of the associated module
  set(BOOST_DIR $ENV{BOOST_DIR} CACHE PATH "Path to the Boost installation (or to the Boost source)")
  set(BOOST_ROOT ${BOOST_DIR})

  find_package (Boost COMPONENTS unit_test_framework system)
  if(Boost_FOUND)
    include_directories(${Boost_INCLUDE_DIRS})
  else()
    message(FATAL_ERROR "Boost libraries not found. Please specify location using the BOOST_DIR variable")
  endif()
  add_subdirectory(tests)
endif(COREBLURON_TESTS)

include(CPackConfig)
include(DoxygenRule)
