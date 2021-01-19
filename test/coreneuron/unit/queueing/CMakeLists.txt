# =============================================================================
# Copyright (C) 2016-2021 Blue Brain Project
#
# See top-level LICENSE file for details.
# =============================================================================

file(GLOB queuing_test_src "*.cpp")

add_executable(queuing_test_bin ${queuing_test_src})
target_link_libraries(queuing_test_bin ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY} ${Boost_SYSTEM_LIBRARY}
                      coreneuron ${corenrn_mech_lib} ${reportinglib_LIBRARY} ${sonatareport_LIBRARY})
add_dependencies(queuing_test_bin nrniv-core)

add_test(NAME queuing_test COMMAND ${TEST_EXEC_PREFIX} ${CMAKE_CURRENT_BINARY_DIR}/queuing_test_bin)
