set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/test)

include_directories(${PROJECT_SOURCE_DIR}/test ${PROJECT_SOURCE_DIR}/src/nrnoc ${PROJECT_SOURCE_DIR}/src/nrniv ${PROJECT_SOURCE_DIR}/src/oc)

# =============================================================================
# Test executables
# =============================================================================
add_executable(testneuron unit_test.cpp)
target_compile_features(testneuron PUBLIC cxx_std_11)
target_link_libraries(testneuron Catch2::Catch2 nrniv_lib)

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
add_test(NAME ringtest COMMAND ${CMAKE_BINARY_DIR}/bin/nrniv -python ${RINGTEST_DIR}/ringtest.py -tstop 100 WORKING_DIRECTORY ${RINGTEST_DIR})

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
endif()
