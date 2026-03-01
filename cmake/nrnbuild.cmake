# ==============================================================
# CMake wrapper for generating mechanisms for (core)NEURON
#
# At configuration-time this file is renamed to CMakeLists.txt.
# The at run-time this file calls create_nrnmech which configures cmake to build
# the mechanism files. This file provides a command line interface for create_nrnmech.
#
# ==============================================================

cmake_minimum_required(VERSION @CMAKE_MINIMUM_REQUIRED_VERSION@)
project(nrnbuild)

find_package(neuron REQUIRED)

set(NRNBUILD_MOD_FILES
  ""
  CACHE STRING "List of mod files to convert to mechanisms")
set(NRNBUILD_NEURON
  ON
  CACHE BOOL "Whether to generate mechanisms for NEURON")
set(NRNBUILD_CORENEURON
  OFF
  CACHE BOOL "Whether to generate mechanisms for coreNEURON")
set(NRNBUILD_SPECIAL
  ON
  CACHE BOOL "Whether to generate the `special` executable")
set(NRNBUILD_NMODL
  ON
  CACHE BOOL "Whether to use NMODL with NEURON")
set(NRNBUILD_LIBRARY_TYPE
  SHARED
  CACHE STRING "Either STATIC or SHARED")
set(NRNBUILD_NMODL_ARGS
  ""
  CACHE STRING "extra arguments to pass to NMODL for NEURON codegen")
set(NRNBUILD_CORENEURON_ARGS
  ""
  CACHE STRING "extra arguments to pass to NMODL for CoreNEURON codegen")
set(NRNBUILD_ENV
  ""
  CACHE STRING "extra environment variables for NMODL")

set(NRNBUILD_ARGS)
if(NRNBUILD_NEURON)
  list(APPEND NRNBUILD_ARGS "NEURON")
endif()
if(NRNBUILD_CORENEURON)
  list(APPEND NRNBUILD_ARGS "CORENEURON")
endif()
if(NRNBUILD_SPECIAL)
  list(APPEND NRNBUILD_ARGS "SPECIAL")
endif()
if(NRNBUILD_NMODL)
  list(APPEND NRNBUILD_ARGS "NMODL_NEURON_CODEGEN")
endif()

separate_arguments(NRNBUILD_NMODL_ARGS_LIST NATIVE_COMMAND ${NRNBUILD_NMODL_ARGS})
separate_arguments(NRNBUILD_CORENEURON_ARGS_LIST NATIVE_COMMAND ${NRNBUILD_CORENEURON_ARGS})
separate_arguments(NRNBUILD_ENV_LIST NATIVE_COMMAND ${NRNBUILD_ENV})

message(STATUS "Received mod files: ${NRNBUILD_MOD_FILES}")

create_nrnmech(
  ${NRNBUILD_ARGS}
  LIBRARY_TYPE
  ${NRNBUILD_LIBRARY_TYPE}
  MOD_FILES
  ${NRNBUILD_MOD_FILES}
  ARTIFACTS_OUTPUT_DIR
  "${CMAKE_CURRENT_BINARY_DIR}"
  LIBRARY_OUTPUT_DIR
  "${CMAKE_CURRENT_BINARY_DIR}"
  EXECUTABLE_OUTPUT_DIR
  "${CMAKE_CURRENT_BINARY_DIR}"
  NMODL_NEURON_EXTRA_ARGS
  ${NRNBUILD_NMODL_ARGS_LIST}
  NMODL_CORENEURON_EXTRA_ARGS
  ${NRNBUILD_CORENEURON_ARGS_LIST}
  EXTRA_ENV
  ${NRNBUILD_ENV_LIST})
