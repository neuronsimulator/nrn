# =============================================================================
# Copyright (C) 2018-2019 Blue Brain Project
#
# This file is part of NMODL distributed under the terms of the GNU Lesser General Public License.
# See top-level LICENSE file for details.
# =============================================================================

cmake_minimum_required(VERSION 3.15 FATAL_ERROR)

project(NMODL LANGUAGES CXX)

# =============================================================================
# CMake common project settings
# =============================================================================
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)

# =============================================================================
# Build options for NMODL
# =============================================================================
option(NMODL_ENABLE_PYTHON_BINDINGS "Enable pybind11 based python bindings" ON)
option(NMODL_ENABLE_LEGACY_UNITS "Use original faraday, R, etc. instead of 2019 nist constants" OFF)
if(NMODL_ENABLE_LEGACY_UNITS)
  add_definitions(-DUSE_LEGACY_UNITS)
endif()

# =============================================================================
# Settings to enable project as submodule
# =============================================================================
set(NMODL_PROJECT_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(NMODL_PROJECT_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR})
set(NMODL_AS_SUBPROJECT OFF)
if(NOT CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
  set(NMODL_AS_SUBPROJECT ON)
  # output targets into top level build directory
  set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
  set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
endif()

# =============================================================================
# Compile static libraries with hidden visibility
# =============================================================================
set(CMAKE_CXX_VISIBILITY_PRESET hidden)

# =============================================================================
# Find required packages
# =============================================================================
message(STATUS "CHECKING FOR FLEX/BISON")
find_package(FLEX 2.6 REQUIRED)
find_package(BISON 3.0 REQUIRED)

# =============================================================================
# Include cmake modules. Filenames ensure we always pick up NMODL's versions.
# =============================================================================
include(cmake/Catch.cmake)
include(cmake/ClangTidyHelper.cmake)
include(cmake/CompilerHelper.cmake)
include(cmake/FindPythonModule.cmake)
include(cmake/FlexHelper.cmake)
include(cmake/GitRevision.cmake)
include(cmake/PythonLinkHelper.cmake)
include(cmake/RpathHelper.cmake)
include(cmake/ExternalProjectHelper.cmake)

# This should apply to all NMODL targets but should not leak out when NMODL is built as a submodule.
add_compile_options(${NMODL_COMPILER_WARNING_SUPPRESSIONS})

# =============================================================================
# Set the project version now using git
# =============================================================================
project(
  NMODL
  VERSION ${NMODL_GIT_LAST_TAG}
  LANGUAGES CXX)

# =============================================================================
# Initialize external libraries as submodule
# =============================================================================
set(THIRD_PARTY_DIRECTORY "${PROJECT_SOURCE_DIR}/ext")
add_external_project(fmt OFF)
add_external_project(spdlog OFF)
add_external_project(pybind11)
add_external_project(cli11 OFF)
add_external_project(eigen OFF)

add_subdirectory(${THIRD_PARTY_DIRECTORY}/fmt EXCLUDE_FROM_ALL)
set_property(TARGET fmt PROPERTY POSITION_INDEPENDENT_CODE ON)
if(NMODL_PGI_COMPILER)
  # "format.cc", warning #128-D: loop is not reachable
  set_source_files_properties(${CMAKE_CURRENT_SOURCE_DIR}/ext/fmt/src/format.cc
                              PROPERTIES COMPILE_FLAGS "--diag_suppress=128")
endif()
include_directories(
  SYSTEM ${THIRD_PARTY_DIRECTORY} ${THIRD_PARTY_DIRECTORY}/catch/include
  ${THIRD_PARTY_DIRECTORY}/fmt/include ${THIRD_PARTY_DIRECTORY}/spdlog/include
  ${THIRD_PARTY_DIRECTORY}/cli11/include)

# =============================================================================
# HPC Coding Conventions
# =============================================================================
set(NMODL_ClangFormat_EXCLUDES_RE
    ".*/ext/.*$$"
    CACHE STRING "list of regular expressions to exclude C/C++ files from formatting" FORCE)
set(NMODL_CMakeFormat_EXCLUDES_RE
    ".*/ext/.*$$" ".*/src/language/templates/.*$$"
    CACHE STRING "list of regular expressions to exclude CMake files from formatting" FORCE)
set(NMODL_ClangFormat_DEPENDENCIES
    pyastgen parser-gen
    CACHE STRING "list of CMake targets to build before formatting C++ code" FORCE)

# initialize submodule of coding conventions under cmake
set(THIRD_PARTY_DIRECTORY "${PROJECT_SOURCE_DIR}/cmake")
add_external_project(hpc-coding-conventions OFF)
add_subdirectory(cmake/hpc-coding-conventions/cpp)
include(FindClangFormat)

# =============================================================================
# Format & execute ipynb notebooks in place (pip install nbconvert clean-ipynb)
# =============================================================================
add_custom_target(
  nb-format
  jupyter
  nbconvert
  --to
  notebook
  --execute
  --inplace
  --ExecutePreprocessor.timeout=360
  "${CMAKE_SOURCE_DIR}/docs/notebooks/*.ipynb"
  &&
  clean_ipynb
  "${CMAKE_SOURCE_DIR}/docs/notebooks/*.ipynb")

# =============================================================================
# Adjust install prefix for wheel
# =============================================================================
if(NOT LINK_AGAINST_PYTHON)
  set(NMODL_INSTALL_DIR_SUFFIX "nmodl/.data/")
endif()

# =============================================================================
# Find required python packages
# =============================================================================
message(STATUS "CHECKING FOR PYTHON")
find_package(PythonInterp 3.6 REQUIRED)
nmodl_find_python_module(jinja2 2.9.3 REQUIRED)
nmodl_find_python_module(pytest 3.3.0 REQUIRED)
nmodl_find_python_module(sympy 1.2 REQUIRED)
nmodl_find_python_module(textwrap 0.9 REQUIRED)
nmodl_find_python_module(yaml 3.12 REQUIRED)

# =============================================================================
# Compiler specific flags for external submodules
# =============================================================================
if(NMODL_PGI_COMPILER)
  # PGI with llvm code generation doesn't have necessary assembly intrinsic headers
  add_compile_definitions(EIGEN_DONT_VECTORIZE=1)
  # nlohmann/json doesn't check for PGI compiler
  add_compile_definitions(JSON_SKIP_UNSUPPORTED_COMPILER_CHECK=1)
endif()

include_directories(${NMODL_PROJECT_SOURCE_DIR} ${NMODL_PROJECT_SOURCE_DIR}/src
                    ${PROJECT_BINARY_DIR}/src)

# generate file with version number from git and nrnunits.lib file path
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/src/config/config.cpp.in
               ${PROJECT_BINARY_DIR}/src/config/config.cpp @ONLY)

# generate Doxyfile with correct source paths
configure_file(${NMODL_PROJECT_SOURCE_DIR}/docs/Doxyfile.in
               ${NMODL_PROJECT_SOURCE_DIR}/docs/Doxyfile)

# =============================================================================
# Memory checker options and add tests
# =============================================================================
find_program(MEMORYCHECK_COMMAND valgrind)
set(MEMORYCHECK_COMMAND_OPTIONS
    "--trace-children=yes \
                                 --leak-check=full \
                                 --track-origins=yes \
                                 --show-possibly-lost=no")
# do not enable tests if nmodl is used as submodule
if(NOT NMODL_AS_SUBPROJECT)
  include(CTest)
  add_subdirectory(test/unit)
  add_subdirectory(test/integration)
endif()

# =============================================================================
# list of autogenerated files
# =============================================================================
include(${PROJECT_SOURCE_DIR}/src/language/code_generator.cmake)

add_subdirectory(src)

# =============================================================================
# Prepare units database file from nrnunits.lib.in
# =============================================================================
if(NMODL_ENABLE_LEGACY_UNITS)
  set(LegacyY "")
  set(LegacyN "/")
else()
  set(LegacyY "/")
  set(LegacyN "")
endif()
configure_file(share/nrnunits.lib.in ${CMAKE_CURRENT_BINARY_DIR}/share/nmodl/nrnunits.lib @ONLY)

# =============================================================================
# Install unit database to share
# =============================================================================
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/share/nmodl/nrnunits.lib
        DESTINATION ${NMODL_INSTALL_DIR_SUFFIX}share/nmodl)

# to print compiler flags in the build status
if(CMAKE_BUILD_TYPE)
  string(TOUPPER ${CMAKE_BUILD_TYPE} BUILD_TYPE_UPPER)
  set(COMPILER_FLAGS "${CMAKE_CXX_FLAGS} ${CMAKE_CXX_FLAGS_${BUILD_TYPE_UPPER}}")
else()
  set(COMPILER_FLAGS "${CMAKE_CXX_FLAGS}")
endif()

# =============================================================================
# Build status
# =============================================================================
message(STATUS "")
message(STATUS "Configured NMODL ${PROJECT_VERSION} (${NMODL_GIT_REVISION})")
message(STATUS "")
message(STATUS "You can now build NMODL using:")
message(STATUS "  cmake --build . --parallel 8 [--target TARGET]")
message(STATUS "You might want to adjust the number of parallel build jobs for your system.")
message(STATUS "Some non-default targets you might want to build:")
message(STATUS "--------------------+--------------------------------------------------------")
message(STATUS " Target             |   Description")
message(STATUS "--------------------+--------------------------------------------------------")
message(STATUS "test                | Run unit tests")
message(STATUS "install             | Will install NMODL to: ${CMAKE_INSTALL_PREFIX}")
message(STATUS "--------------------+--------------------------------------------------------")
message(STATUS " Build option       | Status")
message(STATUS "--------------------+--------------------------------------------------------")
message(STATUS "CXX COMPILER        | ${CMAKE_CXX_COMPILER}")
message(STATUS "COMPILE FLAGS       | ${COMPILER_FLAGS}")
message(STATUS "Build Type          | ${CMAKE_BUILD_TYPE}")
message(STATUS "Legacy Units        | ${NMODL_ENABLE_LEGACY_UNITS}")
message(STATUS "Python Bindings     | ${NMODL_ENABLE_PYTHON_BINDINGS}")
message(STATUS "Flex                | ${FLEX_EXECUTABLE}")
message(STATUS "Bison               | ${BISON_EXECUTABLE}")
message(STATUS "Python              | ${PYTHON_EXECUTABLE}")
if(NMODL_CLANG_FORMAT)
  message(STATUS "Clang Format        | ${ClangFormat_EXECUTABLE}")
endif()
if(NMODL_CMAKE_FORMAT)
  message(STATUS "Cmake Format        | ${CMakeFormat_EXECUTABLE}")
endif()
message(STATUS "--------------+--------------------------------------------------------------")
message(STATUS " See documentation : https://github.com/BlueBrain/nmodl/")
message(STATUS "--------------+--------------------------------------------------------------")
message(STATUS "")
