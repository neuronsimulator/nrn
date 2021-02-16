# =============================================================================
# Copyright (c) 2016 - 2021 Blue Brain Project/EPFL
#
# See top-level LICENSE file for details.
# =============================================================================
add_executable(interleave_info_bin check_constructors.cpp)
target_link_libraries(
  interleave_info_bin
  ${MPI_CXX_LIBRARIES}
  ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY}
  coreneuron
  ${corenrn_mech_lib}
  ${reportinglib_LIBRARY}
  ${sonatareport_LIBRARY})
add_dependencies(interleave_info_bin nrniv-core)

add_test(NAME interleave_info_constructor_test COMMAND ${TEST_EXEC_PREFIX}
                                                       $<TARGET_FILE:interleave_info_bin>)
