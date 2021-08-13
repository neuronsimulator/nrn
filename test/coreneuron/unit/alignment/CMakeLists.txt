# =============================================================================
# Copyright (c) 2016 - 2021 Blue Brain Project/EPFL
#
# See top-level LICENSE file for details.
# =============================================================================
include_directories(${CMAKE_SOURCE_DIR}/coreneuron ${Boost_INCLUDE_DIRS})

add_executable(alignment_test_bin alignment.cpp)
target_link_libraries(alignment_test_bin ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY})
add_test(NAME alignment_test COMMAND ${TEST_EXEC_PREFIX} $<TARGET_FILE:alignment_test_bin>)
