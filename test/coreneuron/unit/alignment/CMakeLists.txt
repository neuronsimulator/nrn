# =============================================================================
# Copyright (c) 2016 - 2022 Blue Brain Project/EPFL
#
# See top-level LICENSE file for details.
# =============================================================================
add_executable(alignment_test_bin alignment.cpp)
target_link_libraries(alignment_test_bin coreneuron-unit-test Catch2::Catch2)
add_test(NAME alignment_test COMMAND $<TARGET_FILE:alignment_test_bin>)
cpp_cc_configure_sanitizers(TARGET alignment_test_bin TEST alignment_test)
