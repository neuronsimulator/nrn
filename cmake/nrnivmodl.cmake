cmake_minimum_required(VERSION @CMAKE_MINIMUM_REQUIRED_VERSION@)
project(nrnivmodl)

find_package(neuron REQUIRED)

message(STATUS "Received mod files: ${MOD_FILES}")

create_nrnmech(${CORENEURON} MOD_FILES ${MOD_FILES})
