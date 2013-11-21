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
set(LAST_RELEASE 0.0.0)
set (VERSION_MAJOR "0")
set (VERSION_MINOR "1")
set (VERSION_PATCH "0")

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/CMake")

include(Common)
include(FindPackages)

if(MPI_FOUND)
  set(CMAKE_C_COMPILER ${MPI_C_COMPILER})
  set(CMAKE_CXX_COMPILER ${MPI_CXX_COMPILER})
endif(MPI_FOUND)

option (COREBLURON_TESTS "Build the CORE BLURON regression tests" ON)
option (COREBLURON_MAIN "Build the CORE BLURON main" ON)

########################################################################
#
# Core and mech libraries 
#
########################################################################

add_subdirectory (src/simcore)
add_subdirectory (src/mech)

########################################################################
#
# Main
#
########################################################################

if(COREBLURON_MAIN)
    add_subdirectory (src/main)
endif(COREBLURON_MAIN)


#this depends of the associated module
#set(Boost_NO_SYSTEM_PATHS TRUE)
set(BOOST_DIR $ENV{BOOST_DIR} CACHE PATH "Path to the Boost installation (or to the Boost source)")
set(BOOST_ROOT ${BOOST_DIR})
#set(CMAKE_FIND_LIBRARY_SUFFIXES ".a")
find_package (Boost COMPONENTS unit_test_framework system)
if(Boost_FOUND)
    include_directories(${Boost_INCLUDE_DIRS})
else()
    message(FATAL_ERROR "Boost libraries not found. Please specify location using the BOOST_DIR variable")
endif()

########################################################################
#
# Tests
#
########################################################################

if(COREBLURON_TESTS)
    find_package(PythonInterp 2.6 REQUIRED )
    include(CTest)
    enable_testing()
    set(BUILDNAME "${PROJECT_VERSION}" CACHE STRING "Name of build on the dashboard")
      mark_as_advanced(BUILDNAME)
    add_subdirectory (test/integration)
endif(COREBLURON_TESTS)


########################################################################
#
# Install
#
########################################################################
install(DIRECTORY src DESTINATION include FILES_MATCHING PATTERN "*.h" PATTERN "*.hpp" PATTERN ".git" EXCLUDE)
install(FILES ${PROJECT_BINARY_DIR}/src/simcore/libcorebluron.a DESTINATION lib)
install(FILES ${PROJECT_BINARY_DIR}/src/mech/libcoremech.a DESTINATION lib)

############ Packaging & Release ###################
#set(CPACK_PACKAGING_INSTALL_PREFIX "/usr/bin" CACHE PATH "")

include(CPackConfig)
include(DoxygenRule)
include(PackageConfig)
include(BBPRelease)
