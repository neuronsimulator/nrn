# =============================================================================
# Copyright (c) 2016 - 2021 Blue Brain Project/EPFL
#
# See top-level LICENSE file for details.
# =============================================================================

include_directories(${CMAKE_SOURCE_DIR}/coreneuron ${Boost_INCLUDE_DIRS})
file(GLOB alignment_test_src "*.cpp")

add_executable(alignment_test_bin ${alignment_test_src})
target_link_libraries(alignment_test_bin ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY})

if(CORENRN_ENABLE_GPU)
  target_link_libraries(alignment_test_bin ${link_cudacoreneuron} ${CUDA_LIBRARIES})
endif()

add_test(NAME alignment_test COMMAND ${TEST_EXEC_PREFIX}
                                     ${CMAKE_CURRENT_BINARY_DIR}/alignment_test_bin)
