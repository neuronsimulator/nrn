# ~~~
# Test for `create_nrnmech` as if it was already installed
# ~~~
set(TEST_NAMESPACE "nrnivmodl_cmake")
file(GLOB MOD_FILES "${PROJECT_SOURCE_DIR}/test/coreneuron/mod files/*.mod")

# This is admittedly a bit hacky, but we are launching CMake inside of CMake, and we cannot use any
# of the set variables, either via CMake itself, or through the env. The only thing that must be
# available is the NEURON CMake config file, and NMODL must be in PATH at CMake configure time. Note
# that we do not try to use Python abilities of `special`, since those require setting env
# variables, which the user may not be aware of.
set(TEST_DIR "${CMAKE_CURRENT_BINARY_DIR}/build_cmake/build")
if(NRN_ENABLE_CORENEURON)
  set(CORENEURON CORENEURON)
endif()
# we need to enable CUDA if using NVHPC, otherwise the flags are not propagated properly
if(CMAKE_CUDA_COMPILER)
    set(CUDA_STRING "CUDA")
endif()
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/build_cmake/CMakeLists.txt" "${CMAKE_CURRENT_BINARY_DIR}/build_cmake/CMakeLists.txt" @ONLY)
set(test_command
"CMAKE_PREFIX_PATH='${PROJECT_BINARY_DIR}/lib/cmake' \
    PATH='${PROJECT_BINARY_DIR}/bin:$ENV{PATH}' \
    MODLUNIT='${PROJECT_BINARY_DIR}/share/nrn/lib/nrnunits.lib' \
    ${CMAKE_COMMAND} \
    -S '${CMAKE_CURRENT_BINARY_DIR}/build_cmake' -B '${TEST_DIR}' \
    -DMOD_FILES='${MOD_FILES}' \
    -DCMAKE_BUILD_TYPE=Debug \
    -DCORENEURON=${CORENEURON} \
    -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER} \
    -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}")
if(CMAKE_LANGUAGES MATCHES "CUDA")
    set(test_command "${test_command} -DCMAKE_CUDA_COMPILER=${CMAKE_CUDA_COMPILER}")
endif()
add_test(
  NAME "${TEST_NAMESPACE}::modfiles"
  COMMAND
    sh -c "${test_command} \
  && \
  ${CMAKE_COMMAND} --build '${TEST_DIR}' -v --parallel \
  && \
  '${TEST_DIR}/special' -nopython -nobanner -nogui -c 'quit()' \
  rm -fr '${TEST_DIR}'")
