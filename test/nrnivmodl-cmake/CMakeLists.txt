# ~~~
# Test for `create_nrnmech` as if it was already installed
# ~~~
set(TEST_NAMESPACE "nrnivmodl-cmake")
set(TEST_DIR "${CMAKE_CURRENT_BINARY_DIR}/build_cmake")
file(GLOB MOD_FILES "${PROJECT_SOURCE_DIR}/test/coreneuron/mod files/*.mod")

# This is admittedly a bit hacky, but we are launching CMake inside of CMake, and we cannot use any
# of the set variables, either via CMake itself, or through the env. The only thing that must be
# available is the NEURON CMake config file, and NMODL must be in PATH at CMake configure time.
add_test(
  NAME "${TEST_NAMESPACE}::modfiles"
  COMMAND
    sh -c
    "CMAKE_PREFIX_PATH='${PROJECT_BINARY_DIR}/lib/cmake' PATH='${PROJECT_BINARY_DIR}/bin:$ENV{PATH}' \
  ${CMAKE_COMMAND} -S '${CMAKE_CURRENT_SOURCE_DIR}/build_cmake' -B '${TEST_DIR}' -DMOD_FILES='${MOD_FILES}' \
  && \
  ${CMAKE_COMMAND} --build '${TEST_DIR}' -v --parallel \
  && \
  rm -fr '${TEST_DIR}'")
