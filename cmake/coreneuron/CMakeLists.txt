# Copyright (c) 2016, Blue Brain Project
# All rights reserved.

# Redistribution and use in source and binary forms, with or without modification,
# are permitted provided that the following conditions are met:
# 1. Redistributions of source code must retain the above copyright notice,
#    this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright notice,
#    this list of conditions and the following disclaimer in the documentation
#    and/or other materials provided with the distribution.
# 3. Neither the name of the copyright holder nor the names of its contributors
#    may be used to endorse or promote products derived from this software
#    without specific prior written permission.

# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
# LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
# THE POSSIBILITY OF SUCH DAMAGE.


# Ecole Polytechnique Federale de Lausanne
# Brain Mind Institute,
# Blue Brain Project
# (c) 2006-2016. All rights reserved.
#
# Author: Aleksandr Ovcharenko
# Core Neuron


# Initial Setup

CMAKE_MINIMUM_REQUIRED(VERSION 2.8.12)
PROJECT(coreneuron)

set(CORENEURON_DESCRIPTION "BBP CoreNeuron simulator")

set(VERSION_MAJOR "0")
set(VERSION_MINOR "8")
set(VERSION_PATCH "1")
set(VERSION_STRING "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}")

## global options
option(CORENEURON_MAIN "Build the CORE NEURON main" ON)
option(CORENEURON_OPENMP "Build the CORE NEURON with OpenMP implementation" ON)
option(DISABLE_NRN_TIMEOUT "Disable nrn_timeout implementation" OFF)
option(ENABLE_REPORTINGLIB "Enable use of ReportingLib for soma reports" OFF)

## mech / mod options
set(ADDITIONAL_MECHPATH "" CACHE PATH "Search path for optional additional mechanism MOD files")
set(ADDITIONAL_MECHS "" CACHE FILEPATH "File containing list of additional mechanism MOD files")

# test compilations
option(UNIT_TESTS "Enable unit tests compilation and execution" ON)
option(FUNCTIONAL_TESTS "Enable functional tests compilation and execution" ON)


list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/CMake
      ${PROJECT_SOURCE_DIR}/CMake/portability
      ${PROJECT_SOURCE_DIR}/CMake/packages
      ${PROJECT_SOURCE_DIR}/CMake/config)

set(Boost_NO_BOOST_CMAKE TRUE)

include(BlueGenePortability)
include(DefineInstallationPaths)
include(ReleaseDebugAutoFlags)

include(FindPkgConfig)
include(CTest)

#Find all dependencies
find_package(MPI REQUIRED)
find_package(mod2c 1.1  REQUIRED)
find_package(Boost 1.41.0 QUIET COMPONENTS filesystem system atomic unit_test_framework)

include(BlueGenePortability)

# Threading
if(CORENEURON_OPENMP)
    find_package(OpenMP)
    if(OPENMP_FOUND)
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS} ${ADDITIONAL_THREADSAFE_FLAGS}")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS} ${ADDITIONAL_THREADSAFE_FLAGS}")
    endif(OPENMP_FOUND)
endif(CORENEURON_OPENMP)

# ReportingLib
if(ENABLE_REPORTINGLIB)
    find_package(reportinglib REQUIRED)
    if(reportinglib_FOUND)
        include_directories(${reportinglib_INCLUDE_DIR})
        add_definitions("-DENABLE_REPORTING")
    else(reportinglib_FOUND)
        message(FATAL_ERROR "Enabled use of ReportingLib but ReportingLib installion not found!")
    endif(reportinglib_FOUND)
endif(ENABLE_REPORTINGLIB)

include(CheckIncludeFiles)
CHECK_INCLUDE_FILES (malloc.h have_malloc_h)
if(have_malloc_h)
  add_definitions("-DHAVE_MALLOC_H")
endif()

CHECK_INCLUDE_FILES("spi/include/kernel/memory.h" have_memory_h)
if(have_memory_h)
  add_definitions("-DHAVE_MEMORY_H")
endif()

option (ENABLE_SELECTIVE_GPU_PROFILING "Enable GPU profiling only for Solver" ON)

option (ENABLE_OPENACC "Enable use of OpenACC" OFF)

if(ENABLE_OPENACC)
    if(${CMAKE_C_COMPILER_ID} STREQUAL "PGI")
        add_definitions( -DPG_ACC_BUGS)
    endif()

    if(ENABLE_SELECTIVE_GPU_PROFILING)
        find_package(CUDA 5.0)
        if(CUDA_FOUND)
            add_definitions( -DCUDA_PROFILING)
        else(CUDA_FOUND)
            message( FATAL_ERROR "Error : Load CUDA module or disable selective profiling using -DENABLE_SELECTIVE_GPU_PROFILING=OFF !" )
        endif(CUDA_FOUND)

        add_definitions( -DENABLE_SELECTIVE_PROFILING)
    endif(ENABLE_SELECTIVE_GPU_PROFILING)
ELSE(ENABLE_OPENACC)
    SET (ENABLE_SELECTIVE_GPU_PROFILING OFF)
endif(ENABLE_OPENACC)

# Some mechanisms use NEURON specific code which is skipped
# using this macro for CoreNeuron build
add_definitions(-DCORENEURON_BUILD)

if(DISABLE_NRN_TIMEOUT)
    add_definitions("-DDISABLE_TIMEOUT")
endif(DISABLE_NRN_TIMEOUT)

# Core library
add_subdirectory(coreneuron)

# Main
if(CORENEURON_MAIN)
    add_subdirectory(apps)
endif(CORENEURON_MAIN)

# Tests
if(Boost_FOUND)
    add_subdirectory(tests)
endif(Boost_FOUND)


# Documentation
#include(DoxygenRule)
