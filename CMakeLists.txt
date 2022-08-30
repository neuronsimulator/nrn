# =============================================================================
# Copyright (C) 2018-2022 Blue Brain Project
#
# This file is part of NMODL distributed under the terms of the GNU Lesser General Public License.
# See top-level LICENSE file for details.
# =============================================================================

cmake_minimum_required(VERSION 3.15 FATAL_ERROR)

project(NMODL LANGUAGES CXX)

# =============================================================================
# CMake common project settings
# =============================================================================
set(CMAKE_CXX_STANDARD 17)
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
set(NMODL_EXTRA_CXX_FLAGS
    ""
    CACHE STRING "Add extra compile flags for NMODL sources")
separate_arguments(NMODL_EXTRA_CXX_FLAGS)

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
include(cmake/ClangTidyHelper.cmake)
include(cmake/CompilerHelper.cmake)
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
# HPC Coding Conventions
# =============================================================================
# initialize submodule of coding conventions under cmake
if(NOT EXISTS "${PROJECT_SOURCE_DIR}/cmake/hpc-coding-conventions/cpp/CMakeLists.txt")
  initialize_submodule("${PROJECT_SOURCE_DIR}/cmake/hpc-coding-conventions")
endif()
set(CODING_CONV_PREFIX NMODL)
add_subdirectory(cmake/hpc-coding-conventions/cpp)

# =============================================================================
# Enable sanitizer support if the NMODL_SANITIZERS variable is set
# =============================================================================
include(cmake/hpc-coding-conventions/cpp/cmake/sanitizers.cmake)
list(APPEND NMODL_EXTRA_CXX_FLAGS ${NMODL_SANITIZER_COMPILER_FLAGS})

# =============================================================================
# Initialize external libraries as submodules
# =============================================================================
set(NMODL_3RDPARTY_DIR ext)
include(cmake/hpc-coding-conventions/cpp/cmake/3rdparty.cmake)
cpp_cc_git_submodule(catch2 BUILD PACKAGE Catch2 REQUIRED)
if(NMODL_3RDPARTY_USE_CATCH2)
  # If we're using the submodule then make sure the Catch.cmake helper can be found. In newer
  # versions of Catch2, and with hpc-coding-conventions#130, this should just work...
  list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/ext/catch2/contrib")
endif()
include(Catch)
# If we're being built as a submodule of CoreNEURON then CoreNEURON may have already found/loaded a
# CLI11 submodule.
if(NOT TARGET CLI11::CLI11)
  cpp_cc_git_submodule(cli11 BUILD PACKAGE CLI11 REQUIRED)
endif()
# For the moment do not try and use an external Eigen.
cpp_cc_git_submodule(eigen)
cpp_cc_git_submodule(fmt BUILD PACKAGE fmt REQUIRED)
# If we're building from the submodule, make sure we pass -fPIC so that we can link the code into a
# shared library later.
if(NMODL_3RDPARTY_USE_FMT)
  set_property(TARGET fmt PROPERTY POSITION_INDEPENDENT_CODE ON)
endif()
if(NOT NMODL_3RDPARTY_USE_FMT
   AND ((NMODL_PGI_COMPILER AND CMAKE_CXX_COMPILER_VERSION LESS_EQUAL 22.3.0)
        OR CMAKE_CXX_COMPILER_ID STREQUAL "Intel"))
  message(
    WARNING
      "fmt might generate issues with NVHPC <=22.3 and Intel compiler when installed with C++11 or later standard enabled"
  )
endif()
cpp_cc_git_submodule(json BUILD PACKAGE nlohmann_json REQUIRED)
cpp_cc_git_submodule(pybind11 BUILD PACKAGE pybind11 REQUIRED)
# Tell spdlog not to use its bundled fmt, it should either use the fmt submodule or a truly external
# installation for consistency. This line should be harmless if we use an external spdlog.
set(SPDLOG_FMT_EXTERNAL ON)
cpp_cc_git_submodule(spdlog BUILD PACKAGE spdlog REQUIRED)
if(NMODL_3RDPARTY_USE_SPDLOG)
  # See above, same logic as fmt
  set_property(TARGET spdlog PROPERTY POSITION_INDEPENDENT_CODE ON)
endif()

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
find_package(PythonInterp 3.7 REQUIRED)
include(cmake/hpc-coding-conventions/cpp/cmake/bbp-find-python-module.cmake)
cpp_cc_find_python_module(jinja2 2.9.3 REQUIRED)
cpp_cc_find_python_module(pytest 3.3.0 REQUIRED)
cpp_cc_find_python_module(sympy 1.3 REQUIRED)
cpp_cc_find_python_module(textwrap 0.9 REQUIRED)
cpp_cc_find_python_module(yaml 3.12 REQUIRED)

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
string(JOIN " " COMPILER_FLAGS "${COMPILER_FLAGS}" ${NMODL_EXTRA_CXX_FLAGS})

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
message(STATUS "--------------+--------------------------------------------------------------")
message(STATUS " See documentation : https://github.com/BlueBrain/nmodl/")
message(STATUS "--------------+--------------------------------------------------------------")
message(STATUS "")
