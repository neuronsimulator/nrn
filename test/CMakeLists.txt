set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/test)

include_directories(${PROJECT_SOURCE_DIR}/test ${PROJECT_SOURCE_DIR}/src/nrnoc ${PROJECT_SOURCE_DIR}/src/nrniv ${PROJECT_SOURCE_DIR}/src/oc)

# =============================================================================
# Test executables
# =============================================================================
set(TEST_SOURCES oc/hoc_interpreter.cpp)
add_executable(testneuron unit_test.cpp ${TEST_SOURCES})
target_compile_features(testneuron PUBLIC cxx_std_11)
target_link_libraries(testneuron Catch2::Catch2 nrniv_lib)

set(TEST_ENV PYTHONPATH=${PROJECT_BINARY_DIR}/lib/python:${PROJECT_SOURCE_DIR}/test/rxd:$ENV{PYTHONPATH}
        LD_LIBRARY_PATH=${PROJECT_BINARY_DIR}/lib:$ENV{LD_LIBRARY_PATH}
        DYLD_LIBRARY_PATH=${PROJECT_BINARY_DIR}/lib:$ENV{DYLD_LIBRARY_PATH})

# =============================================================================
# Use catch_discover instead of add_test for granular test report if CMAKE ver is greater than 3.9,
# else use the normal add_test method
# =============================================================================
foreach(test_name
        testneuron)

  if(${CMAKE_VERSION} VERSION_GREATER "3.10")
    catch_discover_tests(${test_name}
                         TEST_PREFIX
                         "${test_name}/")
  else()
    add_test(NAME ${test_name} COMMAND ${test_name})
  endif()
endforeach()


# =============================================================================
# Add ringtest (to run python test make install must be run first)
# =============================================================================
set(RINGTEST_DIR ${PROJECT_SOURCE_DIR}/test/ringtest)
add_test(NAME ringtest COMMAND ${CMAKE_BINARY_DIR}/bin/nrniv ${RINGTEST_DIR}/ring.hoc
         WORKING_DIRECTORY ${RINGTEST_DIR})
set_tests_properties(ringtest
        PROPERTIES ENVIRONMENT "${TEST_ENV}")


# =============================================================================
# Add small hoc test
# =============================================================================
set(HOCTEST_DIR ${PROJECT_SOURCE_DIR}/test/simple_hoc_test)
add_test(NAME testconnectdend COMMAND ${CMAKE_BINARY_DIR}/bin/nrniv ${HOCTEST_DIR}/connect_dend.hoc WORKING_DIRECTORY ${HOCTEST_DIR})


# =============================================================================
# Add pytest
# =============================================================================
if(PYTEST_FOUND)
  add_test(NAME Basic_tests
          COMMAND ${PYTHON_EXECUTABLE} -m pytest ${PROJECT_SOURCE_DIR}/test/pynrn)
  set_tests_properties(Basic_tests
          PROPERTIES ENVIRONMENT "${TEST_ENV}")
  add_test(NAME build_rxd_tests_mech
          COMMAND $ENV{SHELL} ${CMAKE_BINARY_DIR}/bin/nrnivmodl ./ ./ecs WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/test/rxd)
  add_test(NAME rxd_tests
          COMMAND ${PYTHON_EXECUTABLE} -m pytest ${PROJECT_SOURCE_DIR}/test/rxd)
  set_tests_properties(rxd_tests
          PROPERTIES ENVIRONMENT "${TEST_ENV}")
  if(NRN_ENABLE_MPI)
    add_test(NAME rxd_mpi_tests
            COMMAND ${MPIEXEC} -np 1 ${PYTHON_EXECUTABLE} -m pytest ${PROJECT_SOURCE_DIR}/test/rxd --mpi)
    set_tests_properties(rxd_mpi_tests
            PROPERTIES ENVIRONMENT "${TEST_ENV}")
  endif()

endif()
