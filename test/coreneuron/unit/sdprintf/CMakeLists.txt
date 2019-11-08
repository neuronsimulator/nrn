# =============================================================================
# Copyright (C) 2016-2019 Blue Brain Project
#
# See top-level LICENSE file for details.
# =============================================================================

include_directories(${CMAKE_SOURCE_DIR}/coreneuron ${Boost_INCLUDE_DIRS})
file(GLOB sdprintf_test_src "*.cpp")

add_executable(sdprintf_test_bin ${sdprintf_test_src})
target_link_libraries(sdprintf_test_bin
                      ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY}
                      coreneuron
                      ${reportinglib_LIBRARY}
                      ${sonatareport_LIBRARY})

add_test(NAME sdprintf_test
         COMMAND ${TEST_EXEC_PREFIX} ${CMAKE_CURRENT_BINARY_DIR}/sdprintf_test_bin)
