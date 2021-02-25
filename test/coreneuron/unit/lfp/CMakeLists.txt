# =============================================================================
# Copyright (C) 2016-2020 Blue Brain Project
#
# See top-level LICENSE file for details.
# =============================================================================

include_directories(${CMAKE_SOURCE_DIR}/coreneuron ${Boost_INCLUDE_DIRS})
file(GLOB lfp_test_src "*.cpp")

add_executable(lfp_test_bin ${lfp_test_src})
target_link_libraries(
  lfp_test_bin
  ${MPI_CXX_LIBRARIES}
  ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY}
  coreneuron
  ${corenrn_mech_lib}
  ${reportinglib_LIBRARY}
  ${sonatareport_LIBRARY})

if(CORENRN_ENABLE_GPU)
  target_link_libraries(lfp_test_bin ${link_cudacoreneuron} ${CUDA_LIBRARIES})
endif()

add_dependencies(lfp_test_bin nrniv-core)

add_test(NAME lfp_test COMMAND ${TEST_EXEC_PREFIX} $<TARGET_FILE:lfp_test_bin>)
