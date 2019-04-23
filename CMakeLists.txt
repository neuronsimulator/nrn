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
set(PROJECT_VERSION_MINOR 1)
set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)

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
set(NMODL_ClangFormat_EXCLUDES_RE ".*/ext/.*$$"
    CACHE STRING "list of regular expressions to exclude C/C++ files from formatting"
    FORCE)
set(NMODL_CMakeFormat_EXCLUDES_RE ".*/ext/.*$$"
    CACHE STRING "list of regular expressions to exclude CMake files from formatting"
    FORCE)
set(NMODL_ClangFormat_DEPENDENCIES pyastgen parser-gen
    CACHE STRING "list of CMake targets to build before formatting C++ code"
    FORCE)
add_subdirectory(cmake/hpc-coding-conventions/cpp)

# =============================================================================
# Format & execute ipynb notebooks in place (pip install nbconvert clean-ipynb)
# =============================================================================
add_custom_target(nb-format
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
list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)
include(ClangTidyHelper)
include(CompilerHelper)
include(FindClangFormat)
include(FindPythonModule)
include(FlexHelper)
include(GitRevision)
include(RpathHelper)

# =============================================================================
# Find required python packages
# =============================================================================
message(STATUS "CHECKING FOR PYTHON")
find_package(PythonInterp 3.6 REQUIRED)
find_python_module(jinja2 2.9.3 REQUIRED)
find_python_module(pytest 3.3.0 REQUIRED)
find_python_module(sympy 1.2 REQUIRED)
find_python_module(textwrap 0.9 REQUIRED)
find_python_module(yaml 3.12 REQUIRED)

include_directories(${PROJECT_SOURCE_DIR}
                    ${PROJECT_SOURCE_DIR}/src
                    ${PROJECT_BINARY_DIR}/src
                    ${PROJECT_SOURCE_DIR}/ext)

# =============================================================================
# Include pybind11
# =============================================================================
message(STATUS "INCLUDING PYBIND11")
add_subdirectory(ext/pybind11)

# =============================================================================
# Include path from external libraries
# =============================================================================
include_directories(${PROJECT_SOURCE_DIR}/ext/cli11/include)

# =============================================================================
# Project version from git and project directories
# =============================================================================
set(PROJECT_VERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR})

# generate file with version number from git and nrnunits.lib file path
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/src/config/config.cpp.in
               ${CMAKE_CURRENT_BINARY_DIR}/config.cpp @ONLY)

# generate Doxyfile with correct source paths
configure_file(${PROJECT_SOURCE_DIR}/docs/Doxyfile.in ${PROJECT_SOURCE_DIR}/docs/Doxyfile)

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
    ${PROJECT_BINARY_DIR}/src/visitors/nmodl_visitor.cpp)

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
# Memory checker options and add tests
# =============================================================================
find_program(MEMORYCHECK_COMMAND valgrind)
set(MEMORYCHECK_COMMAND_OPTIONS "--trace-children=yes \
                                 --leak-check=full \
                                 --track-origins=yes \
                                 --show-possibly-lost=no")
include(CTest)
add_subdirectory(test)
