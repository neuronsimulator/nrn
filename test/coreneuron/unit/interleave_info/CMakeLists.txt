# =============================================================================
# Copyright (C) 2016-2021 Blue Brain Project
#
# See top-level LICENSE file for details.
# =============================================================================

file(GLOB interleave_info_src "*.cpp")

add_executable(interleave_info_bin ${interleave_info_src})
target_link_libraries(interleave_info_bin ${MPI_CXX_LIBRARIES} ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY}
                      coreneuron ${corenrn_mech_lib} ${reportinglib_LIBRARY} ${sonatareport_LIBRARY})
add_dependencies(interleave_info_bin nrniv-core)

add_test(NAME interleave_info_constructor_test
         COMMAND ${TEST_EXEC_PREFIX} ${CMAKE_CURRENT_BINARY_DIR}/interleave_info_bin)
