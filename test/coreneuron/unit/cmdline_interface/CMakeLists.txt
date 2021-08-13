# =============================================================================
# Copyright (c) 2016 - 2021 Blue Brain Project/EPFL
#
# See top-level LICENSE file for details.
# =============================================================================
add_executable(cmd_interface_test_bin test_cmdline_interface.cpp)
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
# Tell CMake *not* to run an explicit device code linker step (which will produce errors); let the
# NVHPC C++ compiler handle this implicitly.
set_target_properties(cmd_interface_test_bin PROPERTIES CUDA_RESOLVE_DEVICE_SYMBOLS OFF)
add_test(NAME cmd_interface_test COMMAND ${TEST_EXEC_PREFIX} $<TARGET_FILE:cmd_interface_test_bin>)
