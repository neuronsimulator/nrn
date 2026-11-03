# Invoked by the test-install custom target. Runs ctest with the configured label set, then always
# prints how to re-run ctest.
#
# Expected -D definitions from the parent project: NRN_FOREIGN_CTEST_COMMAND,
# NRN_FOREIGN_BINARY_DIR, NRN_FOREIGN_TEST_INSTALL_LABELS, NRN_FOREIGN_TEST_INSTALL_JOBS

if(NOT DEFINED NRN_FOREIGN_CTEST_COMMAND)
  set(NRN_FOREIGN_CTEST_COMMAND "ctest")
endif()
if(NOT DEFINED NRN_FOREIGN_BINARY_DIR)
  message(FATAL_ERROR "NRN_FOREIGN_BINARY_DIR not set")
endif()
if(NOT DEFINED NRN_FOREIGN_TEST_INSTALL_LABELS)
  set(NRN_FOREIGN_TEST_INSTALL_LABELS "serial")
endif()
if(NOT DEFINED NRN_FOREIGN_TEST_INSTALL_JOBS)
  set(NRN_FOREIGN_TEST_INSTALL_JOBS "4")
endif()

execute_process(
  COMMAND ${NRN_FOREIGN_CTEST_COMMAND} --test-dir "${NRN_FOREIGN_BINARY_DIR}" --output-on-failure -L
          "${NRN_FOREIGN_TEST_INSTALL_LABELS}" -j "${NRN_FOREIGN_TEST_INSTALL_JOBS}"
  RESULT_VARIABLE _ctest_rc)

message(STATUS "")
message(
  STATUS
    "Install check finished (label: ${NRN_FOREIGN_TEST_INSTALL_LABELS}, ctest rc=${_ctest_rc}).")
message(STATUS "Re-run or filter tests with ctest against this foreign build dir:")
message(STATUS "  ctest --test-dir ${NRN_FOREIGN_BINARY_DIR} --output-on-failure")
message(STATUS "  ctest --test-dir ${NRN_FOREIGN_BINARY_DIR} -L mpi -j2")
message(STATUS "  ctest --test-dir ${NRN_FOREIGN_BINARY_DIR} -L coreneuron -j2")
message(STATUS "  ctest --test-dir ${NRN_FOREIGN_BINARY_DIR} -R \"pytest::\" --rerun-failed")
message(STATUS "Foreign build dir: ${NRN_FOREIGN_BINARY_DIR}")
message(STATUS "")

if(NOT _ctest_rc EQUAL 0)
  message(FATAL_ERROR "test-install: ctest failed with exit code ${_ctest_rc}")
endif()
