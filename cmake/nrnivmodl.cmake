cmake_minimum_required(VERSION @CMAKE_MINIMUM_REQUIRED_VERSION@)
project(nrnivmodl)

find_package(neuron REQUIRED)

message(STATUS "Received mod files: ${MOD_FILES}")

create_nrnmech(
  NEURON
  ${CORENEURON}
  SPECIAL
  MOD_FILES
  ${MOD_FILES}
  LIBRARY_OUTPUT_DIR
  "${CMAKE_CURRENT_BINARY_DIR}/.libs"
  EXECUTABLE_OUTPUT_DIR
  "${CMAKE_CURRENT_BINARY_DIR}")
