# =============================================================================
# Copyright (c) 2016 - 2021 Blue Brain Project/EPFL
#
# See top-level LICENSE file for details.
# =============================================================================
add_executable(queuing_test_bin test_queueing.cpp)
target_link_libraries(
  queuing_test_bin
  ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY}
  ${Boost_SYSTEM_LIBRARY}
  coreneuron
  ${corenrn_mech_lib}
  ${reportinglib_LIBRARY}
  ${sonatareport_LIBRARY})
add_dependencies(queuing_test_bin nrniv-core)
# Tell CMake *not* to run an explicit device code linker step (which will produce errors); let the
# NVHPC C++ compiler handle this implicitly.
set_target_properties(queuing_test_bin PROPERTIES CUDA_RESOLVE_DEVICE_SYMBOLS OFF)
add_test(NAME queuing_test COMMAND ${TEST_EXEC_PREFIX} $<TARGET_FILE:queuing_test_bin>)
