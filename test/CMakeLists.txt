set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/test)

include_directories(${PROJECT_SOURCE_DIR}/test)

# =============================================================================
# Test executables
# =============================================================================
add_executable(testfactorial unit_test.cpp)
target_compile_features(testfactorial PUBLIC cxx_std_11)
target_link_libraries(testfactorial Catch2::Catch2)

# =============================================================================
# Use catch_discover instead of add_test for granular test report if CMAKE ver is greater than 3.9,
# else use the normal add_test method
# =============================================================================
foreach(test_name
        testfactorial)

  if(${CMAKE_VERSION} VERSION_GREATER "3.10")
    catch_discover_tests(${test_name}
                         TEST_PREFIX
                         "${test_name}/")
  else()
    add_test(NAME ${test_name} COMMAND ${test_name})
  endif()
endforeach()

# =============================================================================
# Add ringtest
# =============================================================================
set(RINGTEST_DIR ${PROJECT_SOURCE_DIR}/test/ringtest)
add_test(NAME ringtest COMMAND ${CMAKE_BINARY_DIR}/bin/nrniv -python ${RINGTEST_DIR}/ringtest.py -tstop 100 WORKING_DIRECTORY ${RINGTEST_DIR})
