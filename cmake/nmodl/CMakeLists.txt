# =============================================================================
# Copyright (C) 2018-2019 Blue Brain Project
#
# This file is part of NMODL distributed under the terms of the GNU Lesser General Public License.
# See top-level LICENSE file for details.
# =============================================================================

cmake_minimum_required(VERSION 3.3.0 FATAL_ERROR)
project(NMODL CXX)

# =============================================================================
# CMake common project settings
# =============================================================================
set(PROJECT_VERSION_MAJOR 0)
set(PROJECT_VERSION_MINOR 2)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)

# =============================================================================
# Settings to enable project as submodule
# =============================================================================
set(NMODL_PROJECT_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR})
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
# HPC Coding Conventions
# =============================================================================
set(NMODL_ClangFormat_EXCLUDES_RE
    ".*/ext/.*$$"
    CACHE STRING "list of regular expressions to exclude C/C++ files from formatting" FORCE)
set(NMODL_CMakeFormat_EXCLUDES_RE
    ".*/ext/.*$$"
    CACHE STRING "list of regular expressions to exclude CMake files from formatting" FORCE)
set(NMODL_ClangFormat_DEPENDENCIES
    pyastgen parser-gen
    CACHE STRING "list of CMake targets to build before formatting C++ code" FORCE)
add_subdirectory(cmake/hpc-coding-conventions/cpp)

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
  --keep-output
  "${CMAKE_SOURCE_DIR}/docs/notebooks/*.ipynb")

# =============================================================================
# Include cmake modules
# =============================================================================
list(APPEND CMAKE_MODULE_PATH ${NMODL_PROJECT_SOURCE_DIR}/cmake)
include(Catch)
include(ClangTidyHelper)
include(CompilerHelper)
include(FindClangFormat)
include(FindPythonModule)
include(FlexHelper)
include(GitRevision)
include(PythonLinkHelper)
include(RpathHelper)

# =============================================================================
# Find required python packages
# =============================================================================
message(STATUS "CHECKING FOR PYTHON")
find_package(PythonInterp 3.5 REQUIRED)
find_python_module(jinja2 2.9.3 REQUIRED)
find_python_module(pytest 3.3.0 REQUIRED)
find_python_module(sympy 1.2 REQUIRED)
find_python_module(textwrap 0.9 REQUIRED)
find_python_module(yaml 3.12 REQUIRED)

# =============================================================================
# Compiler specific flags for external submodules
# =============================================================================
if(NMODL_PGI_COMPILER)
  # PGI with llvm code generation doesn't have necessary assembly intrinsic headers
  add_compile_definitions(EIGEN_DONT_VECTORIZE=1)
  # nlohmann/json doesn't check for PGI compiler
  add_compile_definitions(JSON_SKIP_UNSUPPORTED_COMPILER_CHECK=1)
endif()

include_directories(
  ${NMODL_PROJECT_SOURCE_DIR} ${NMODL_PROJECT_SOURCE_DIR}/src ${PROJECT_BINARY_DIR}/src
  ${NMODL_PROJECT_SOURCE_DIR}/ext ${NMODL_PROJECT_SOURCE_DIR}/ext/fmt/include
  ${NMODL_PROJECT_SOURCE_DIR}/ext/spdlog/include)

# =============================================================================
# Include pybind11
# =============================================================================
message(STATUS "INCLUDING PYBIND11")
add_subdirectory(ext/pybind11)

# =============================================================================
# Include fmt library
# =============================================================================
message(STATUS "INCLUDING FMT")
add_subdirectory(${NMODL_PROJECT_SOURCE_DIR}/ext/fmt EXCLUDE_FROM_ALL)
set_property(TARGET fmt PROPERTY POSITION_INDEPENDENT_CODE ON)

# =============================================================================
# Include path from external libraries
# =============================================================================
include_directories(${NMODL_PROJECT_SOURCE_DIR}/ext/cli11/include)

# =============================================================================
# Project version from git and project directories
# =============================================================================
set(PROJECT_VERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR})

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
  add_subdirectory(test)
endif()

# =============================================================================
# Check newer python for generating AST classes
# =============================================================================
# ~~~
# AST classes are generated by python scripts which require Python >= 3.6
# In case of older python, try to find newer python interpreter
# ~~~
if(PYTHON_VERSION_MINOR VERSION_LESS 6)
  unset(PYTHONINTERP_FOUND CACHE)
  unset(PYTHON_EXECUTABLE CACHE)
  find_package(PythonInterp 3.6 REQUIRED)
endif()

# =============================================================================
# list of autogenerated files
# =============================================================================
set(AUTO_GENERATED_FILES
    ${PROJECT_BINARY_DIR}/src/ast/ast.hpp
    ${PROJECT_BINARY_DIR}/src/ast/ast_decl.hpp
    ${PROJECT_BINARY_DIR}/src/ast/ast.cpp
    ${PROJECT_BINARY_DIR}/src/pybind/pyast.hpp
    ${PROJECT_BINARY_DIR}/src/pybind/pyast.cpp
    ${PROJECT_BINARY_DIR}/src/pybind/pysymtab.cpp
    ${PROJECT_BINARY_DIR}/src/pybind/pyvisitor.hpp
    ${PROJECT_BINARY_DIR}/src/pybind/pyvisitor.cpp
    ${PROJECT_BINARY_DIR}/src/visitors/visitor.hpp
    ${PROJECT_BINARY_DIR}/src/visitors/ast_visitor.hpp
    ${PROJECT_BINARY_DIR}/src/visitors/ast_visitor.cpp
    ${PROJECT_BINARY_DIR}/src/visitors/json_visitor.hpp
    ${PROJECT_BINARY_DIR}/src/visitors/json_visitor.cpp
    ${PROJECT_BINARY_DIR}/src/visitors/lookup_visitor.hpp
    ${PROJECT_BINARY_DIR}/src/visitors/lookup_visitor.cpp
    ${PROJECT_BINARY_DIR}/src/visitors/symtab_visitor.hpp
    ${PROJECT_BINARY_DIR}/src/visitors/symtab_visitor.cpp
    ${PROJECT_BINARY_DIR}/src/visitors/nmodl_visitor.hpp
    ${PROJECT_BINARY_DIR}/src/visitors/nmodl_visitor.cpp
    ${PROJECT_BINARY_DIR}/src/visitors/checkparent_visitor.cpp
    ${PROJECT_BINARY_DIR}/src/visitors/checkparent_visitor.hpp)

add_subdirectory(src/codegen)
add_subdirectory(src/language)
add_subdirectory(src/lexer)
add_subdirectory(src/nmodl)
add_subdirectory(src/parser)
add_subdirectory(src/printer)
add_subdirectory(src/symtab)
add_subdirectory(src/utils)
add_subdirectory(src/visitors)
add_subdirectory(src/pybind)
add_subdirectory(src/solver)

# =============================================================================
# Install unit database, examples and utility scripts from share
# =============================================================================
install(DIRECTORY share/ DESTINATION share)
