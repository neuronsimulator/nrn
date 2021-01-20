# =============================================================================
# Copyright (C) 2016-2021 Blue Brain Project
#
# See top-level LICENSE file for details.
# =============================================================================

file(GLOB cmd_interface_test_src "*.cpp")

add_executable(cmd_interface_test_bin ${cmd_interface_test_src})
target_link_libraries(
  cmd_interface_test_bin
  ${MPI_CXX_LIBRARIES}
  ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY}
  coreneuron
  ${corenrn_mech_lib}
  ${reportinglib_LIBRARY}
  ${sonatareport_LIBRARY})
target_include_directories(cmd_interface_test_bin SYSTEM
                           PRIVATE ${CORENEURON_PROJECT_SOURCE_DIR}/external/CLI11/include)
add_dependencies(cmd_interface_test_bin nrniv-core)

add_test(NAME cmd_interface_test COMMAND ${TEST_EXEC_PREFIX}
                                         ${CMAKE_CURRENT_BINARY_DIR}/cmd_interface_test_bin)
