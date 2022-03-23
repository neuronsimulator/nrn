add_test(
  external_nrntest
  ${CMAKE_COMMAND}
  -E
  env
  ${NRN_RUN_FROM_BUILD_DIR_ENV}
  MPIEXEC_OVERSUBSCRIBE=${MPIEXEC_OVERSUBSCRIBE}
  ${PROJECT_SOURCE_DIR}/external/tests/nrntest/runtests)
