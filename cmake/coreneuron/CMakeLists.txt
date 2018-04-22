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

CMAKE_MINIMUM_REQUIRED(VERSION 3.0.0)
PROJECT(coreneuron)
set(CORENEURON_DESCRIPTION "BBP CoreNeuron simulator")

set(VERSION_MAJOR "1")
set(VERSION_MINOR "0")
set(VERSION_PATCH "1")
set(VERSION_STRING "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}")

## global options
option(CORENEURON_MAIN "Build the CORE NEURON main" ON)
option(CORENEURON_OPENMP "Build the CORE NEURON with OpenMP implementation" ON)
option(DISABLE_NRN_TIMEOUT "Disable nrn_timeout implementation" OFF)
option(ENABLE_REPORTINGLIB "Enable use of ReportingLib for soma reports" OFF)
option(EXPORT_MECHS_FUNCTIONS "Enable exporting of mod init, jacob, state, cur and BeforeAfter functions of mod-based mechanisms" OFF)
option(ENABLE_MPI "Enable MPI-based execution" ON)
option(ENABLE_SOA "Enable SoA Memory Layout" ON)
option(DISABLE_HOC_EXP "Disable wrapping exp with hoc_exp()" ON)
option(ENABLE_OPENACC_INFO "Enable OpenACC compiler information messages" OFF)
option(ENABLE_SPLAYTREE_QUEUING "Enable use of Splay tree for spike queuing" OFF)
option(ENABLE_CUDA_MODULES "Enable modules like Random123 written in CUDA" ON)
option(ENABLE_NET_RECEIVE_BUFFERING "Enable event buffering in net_receive function" ON)
option(ENABLE_OMP_RUNTIME_SCHEDULE "Use runtime schedule for OpenMP" OFF)


set(FRONTEND_C_COMPILER gcc CACHE FILEPATH "C compiler for building mod2c [frontend executable]")
set(FRONTEND_CXX_COMPILER g++ CACHE FILEPATH "C++ compiler for building mod2c [frontend executable]")
## mech / mod options
set(ADDITIONAL_MECHPATH "" CACHE PATH "Search path for optional additional mechanism MOD files")
set(ADDITIONAL_MECHS "" CACHE FILEPATH "File containing list of additional mechanism MOD files")

# test compilations
option(UNIT_TESTS "Enable unit tests compilation and execution" ON)
option(FUNCTIONAL_TESTS "Enable functional tests compilation and execution" ON)

# For external development, allow one to install all headers and mod files
# todo: need to refactor to install specific set of headers only
option(ENABLE_DEV_FILES_INSTALLATION "Allow to install hedaers/mod files for development" OFF)

list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/CMake
      ${PROJECT_SOURCE_DIR}/CMake/portability
      ${PROJECT_SOURCE_DIR}/CMake/packages
      ${PROJECT_SOURCE_DIR}/CMake/config)

set(Boost_NO_BOOST_CMAKE TRUE)

include(CheckIncludeFiles)
include(ReleaseDebugAutoFlags)
include(CrayPortability)
include(BlueGenePortability)
include(DefineInstallationPaths)
include(SetRpath)
include(DeclareMod2CSubModule)
include(FindPkgConfig)
include(CTest)

#Find all dependencies
if (ENABLE_MPI)
    find_package(MPI REQUIRED)
    add_definitions("-DNRNMPI=1")
else()
    message(STATUS "Use of MPI disabled by user-provided option")
    add_definitions("-DNRNMPI=0")
    add_definitions("-DNRN_MULTISEND=0")
endif()

#Memory layout transformation: 0 for SoA and 1 for AoS
if (ENABLE_SOA)
    add_definitions("-DLAYOUT=0")
else()
    add_definitions("-DLAYOUT=1")
endif()

#OpenMP runtime schedule
if (ENABLE_OMP_RUNTIME_SCHEDULE)
    add_definitions("-DENABLE_OMP_RUNTIME_SCHEDULE")
endif()

#Disable hoc_exp wrapper for exp
if (DISABLE_HOC_EXP)
    add_definitions("-DDISABLE_HOC_EXP")
endif()

#Enable use of splay tree for queuing (for models using net_move)
if (ENABLE_SPLAYTREE_QUEUING)
    add_definitions("-DENABLE_SPLAYTREE_QUEUING")
endif()

#Turn off event buffering if asked
if (NOT ENABLE_NET_RECEIVE_BUFFERING)
    add_definitions("-DNET_RECEIVE_BUFFERING=0")
endif()

find_package(Boost 1.41.0 QUIET COMPONENTS filesystem system atomic unit_test_framework)

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

CHECK_INCLUDE_FILES (malloc.h have_malloc_h)
if(have_malloc_h)
  add_definitions("-DHAVE_MALLOC_H")
endif()

# Some mechanisms use NEURON specific code which is skipped
# using this macro for CoreNeuron build
add_definitions(-DCORENEURON_BUILD)

option (ENABLE_SELECTIVE_GPU_PROFILING "Enable GPU profiling only for Solver" ON)
option (ENABLE_OPENACC "Enable use of OpenACC" OFF)

set(CLANG_FORMAT_MIN_VERSION "4.0")
set(CLANG_FORMAT_MAX_VERSION "4.9")
find_package(ClangFormat)

if(CLANG_FORMAT_FOUND)
    message("clang-format : ${CLANG_FORMAT_EXECUTABLE} : ${CLANG_FORMAT_VERSION}")
else()
  message("clang-format executable with appropriate version (min: ${CLANG_FORMAT_MIN_VERSION}, max: ${CLANG_FORMAT_MAX_VERSION}) not found")
endif()

if(CLANG_FORMAT_FOUND)
    file(COPY ${PROJECT_SOURCE_DIR}/.clang-format DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

    #to indent files using clang-format
    file(GLOB_RECURSE SRC_FILES_FOR_CLANG_FORMAT
        ${PROJECT_SOURCE_DIR}/coreneuron/*.c
        ${PROJECT_SOURCE_DIR}/coreneuron/*.cpp
        ${PROJECT_SOURCE_DIR}/coreneuron/*.h*
        ${PROJECT_SOURCE_DIR}/coreneuron/*.ipp*)

    # exclude ezoption header file
    list(REMOVE_ITEM SRC_FILES_FOR_CLANG_FORMAT
        "${PROJECT_SOURCE_DIR}/coreneuron/utils/ezoption/ezOptionParser.hpp")

    add_custom_target(formatsource COMMAND ${CMAKE_COMMAND}
        -DSOURCE_FILES:STRING="${SRC_FILES_FOR_CLANG_FORMAT}"
        -DCLANG_FORMAT_EXECUTABLE=${CLANG_FORMAT_EXECUTABLE}
        -P "${PROJECT_SOURCE_DIR}/CMake/ClangFormatUtils.cmake"
        )
endif()

#Turn off inline assembly for PGI
if(${CMAKE_C_COMPILER_ID} STREQUAL "PGI")
    add_definitions(-DSWAP_ENDIAN_DISABLE_ASM)
endif()

if(ENABLE_OPENACC)
    if(${CMAKE_C_COMPILER_ID} STREQUAL "PGI")
        add_definitions( -DPG_ACC_BUGS)
        set(ACC_FLAGS "-acc -Minline=size:200,levels:10")
        if (ENABLE_OPENACC_INFO)
            set(ACC_FLAGS "${ACC_FLAGS} -Minfo=acc")
        endif()
        set(CMAKE_C_FLAGS "${ACC_FLAGS} ${CMAKE_C_FLAGS}")
        set(CMAKE_CXX_FLAGS "${ACC_FLAGS} ${CMAKE_CXX_FLAGS}")
    else()
        message(WARNING "OpenACC implementation is only supported and tested using only PGI")
        message(WARNING "Add required compiler flags to enabled OpenACC")
    endif()

    if(ENABLE_CUDA_MODULES)
        find_package(CUDA 5.0)
        SET(CUDA_SEPARABLE_COMPILATION ON)

        if(CUDA_FOUND)
            if(ENABLE_SELECTIVE_GPU_PROFILING)
                add_definitions( -DCUDA_PROFILING)
                add_definitions( -DENABLE_SELECTIVE_PROFILING)
            endif(ENABLE_SELECTIVE_GPU_PROFILING)
        else(CUDA_FOUND)
            message( FATAL_ERROR "Error : Can't file CUDA, load module or set path!" )
        endif(CUDA_FOUND)
    else(ENABLE_CUDA_MODULES)
        message(INFO "Support for CUDA modules (e.g. Random123) is disabled!")
    endif(ENABLE_CUDA_MODULES)

ELSE(ENABLE_OPENACC)
    SET (ENABLE_SELECTIVE_GPU_PROFILING OFF)
    SET (ENABLE_CUDA_MODULES OFF)
    #OpenACC pragmas are not guarded, disable all unknown pragm warnings
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${IGNORE_UNKNOWN_PRAGMA_FLAGS}")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${IGNORE_UNKNOWN_PRAGMA_FLAGS}")
endif(ENABLE_OPENACC)

if(DISABLE_NRN_TIMEOUT)
    add_definitions("-DDISABLE_TIMEOUT")
endif()

if(EXPORT_MECHS_FUNCTIONS)
    add_definitions("-DEXPORT_MECHS_FUNCTIONS")
endif()

# Core library
add_subdirectory(coreneuron)

# Main
if(CORENEURON_MAIN)
    add_subdirectory(apps)
endif(CORENEURON_MAIN)

# Tests
if(Boost_FOUND)
    add_subdirectory(tests)
else()
    message(STATUS "Boost not found, disabling tests")
endif(Boost_FOUND)


# Documentation
#include(DoxygenRule)
