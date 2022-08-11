# =============================================================================
# Copyright (c) 2016 - 2022 Blue Brain Project/EPFL
#
# See top-level LICENSE file for details.
# =============================================================================
include_directories(${CMAKE_SOURCE_DIR}/coreneuron ${Boost_INCLUDE_DIRS})

add_executable(alignment_test_bin alignment.cpp)
target_compile_options(alignment_test_bin PRIVATE ${CORENEURON_BOOST_UNIT_TEST_COMPILE_FLAGS})
add_test(NAME alignment_test COMMAND ${TEST_EXEC_PREFIX} $<TARGET_FILE:alignment_test_bin>)
cpp_cc_configure_sanitizers(TARGET alignment_test_bin TEST alignment_test)
