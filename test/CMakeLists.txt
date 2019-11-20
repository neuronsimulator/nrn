set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/test)

include_directories(${PROJECT_SOURCE_DIR}/test
                    ${PROJECT_SOURCE_DIR}/src/nrnoc
                    ${PROJECT_SOURCE_DIR}/src/nrniv
                    ${PROJECT_SOURCE_DIR}/src/oc)

# =============================================================================
# Test executables
# =============================================================================
set(TEST_SOURCES unit_tests/oc/hoc_interpreter.cpp)
add_executable(testneuron unit_tests/unit_test.cpp ${TEST_SOURCES})
target_compile_features(testneuron PUBLIC cxx_std_11)
target_link_libraries(testneuron Catch2::Catch2 nrniv_lib)

set(TEST_ENV
    PYTHONPATH=${PROJECT_BINARY_DIR}/lib/python:${PROJECT_SOURCE_DIR}/test/rxd:$ENV{PYTHONPATH}
    NEURONHOME=${PROJECT_BINARY_DIR}/share/nrn
    LD_LIBRARY_PATH=${PROJECT_BINARY_DIR}/lib:$ENV{LD_LIBRARY_PATH}
    DYLD_LIBRARY_PATH=${PROJECT_BINARY_DIR}/lib:$ENV{DYLD_LIBRARY_PATH})

# =============================================================================
# Use catch_discover instead of add_test for granular test report if CMAKE ver is greater than 3.9,
# else use the normal add_test method
# =============================================================================
foreach(test_name testneuron)

  if(${CMAKE_VERSION} VERSION_GREATER "3.10")
    catch_discover_tests(${test_name} TEST_PREFIX "${test_name}/")
  else()
    add_test(NAME ${test_name} COMMAND ${test_name})
  endif()
endforeach()

# =============================================================================
# Add ringtest
# =============================================================================
set(RINGTEST_DIR ${PROJECT_SOURCE_DIR}/test/ringtest)
add_test(ringtest
         ${CMAKE_COMMAND}
         -Dexecutable=${CMAKE_BINARY_DIR}/bin/nrniv
         -Dexec_arg=ring.hoc
         -Dout_file=out.dat
         -Dref_file=out.dat.ref
         -Dwork_dir=${RINGTEST_DIR}
         -P
         ${PROJECT_SOURCE_DIR}/cmake/RunHOCTest.cmake)

# =============================================================================
# Add small hoc test
# =============================================================================
set(HOCTEST_DIR ${PROJECT_SOURCE_DIR}/test/hoc_tests/connect_dend)
add_test(connect_dend
         ${CMAKE_COMMAND}
         -Dexecutable=${CMAKE_BINARY_DIR}/bin/nrniv
         -Dexec_arg=connect_dend.hoc
         -Dout_file=cell3soma.dat
         -Dref_file=cell3soma.dat.ref
         -Dwork_dir=${HOCTEST_DIR}
         -P
         ${PROJECT_SOURCE_DIR}/cmake/RunHOCTest.cmake)

# =============================================================================
# Add pytest
# =============================================================================
if(PYTEST_FOUND)
  add_test(NAME py_tests
           COMMAND ${PYTHON_EXECUTABLE} -m pytest ${PROJECT_SOURCE_DIR}/test/python_tests)
  set_tests_properties(py_tests PROPERTIES ENVIRONMENT "${TEST_ENV}")
  add_test(NAME rxd_tests
           COMMAND ${PYTHON_EXECUTABLE} -m pytest ${PROJECT_SOURCE_DIR}/test/rxd)
  set_tests_properties(rxd_tests PROPERTIES ENVIRONMENT "${TEST_ENV}")
endif()
