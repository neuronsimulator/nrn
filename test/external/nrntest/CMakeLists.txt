list(APPEND TEST_ENV
     PATH=${PROJECT_BINARY_DIR}/bin:${PROJECT_SOURCE_DIR}/external/nrntest:$ENV{PATH})
add_test(
  external_nrntest
  ${CMAKE_COMMAND}
  -E
  env
  ${TEST_ENV}
  MPIEXEC_OVERSUBSCRIBE=${MPIEXEC_OVERSUBSCRIBE}
  ${PROJECT_SOURCE_DIR}/external/tests/nrntest/runtests)
