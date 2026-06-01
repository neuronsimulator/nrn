# ==============================================================
# CMake wrapper for generating mechanisms for (core)NEURON
# ==============================================================

cmake_minimum_required(VERSION @CMAKE_MINIMUM_REQUIRED_VERSION@)
project(nrnivmodl)

find_package(neuron REQUIRED)

set(NRNIVMODL_MOD_FILES
    ""
    CACHE STRING "List of mod files to convert to mechanisms")
set(NRNIVMODL_NEURON
    ON
    CACHE BOOL "Whether to generate mechanisms for NEURON")
set(NRNIVMODL_CORENEURON
    OFF
    CACHE BOOL "Whether to generate mechanisms for coreNEURON")
set(NRNIVMODL_SPECIAL
    ON
    CACHE BOOL "Whether to generate the `special` executable")

set(NRNIVMODL_ARGS)
if(NRNIVMODL_NEURON)
  list(APPEND NRNIVMODL_ARGS "NEURON")
endif()
if(NRNIVMODL_CORENEURON)
  list(APPEND NRNIVMODL_ARGS "CORENEURON")
endif()
if(NRNIVMODL_SPECIAL)
  list(APPEND NRNIVMODL_ARGS "SPECIAL")
endif()

message(STATUS "Received mod files: ${NRNIVMODL_MOD_FILES}")

create_nrnmech(
  ${NRNIVMODL_ARGS}
  MOD_FILES
  ${NRNIVMODL_MOD_FILES}
  LIBRARY_OUTPUT_DIR
  "${CMAKE_CURRENT_BINARY_DIR}"
  EXECUTABLE_OUTPUT_DIR
  "${CMAKE_CURRENT_BINARY_DIR}")
