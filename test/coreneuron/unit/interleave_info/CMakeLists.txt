# =============================================================================
# Copyright (c) 2016 - 2022 Blue Brain Project/EPFL
#
# See top-level LICENSE file for details.
# =============================================================================
add_executable(interleave_info_bin check_constructors.cpp)
target_link_libraries(interleave_info_bin coreneuron-unit-test Catch2::Catch2)
add_test(NAME interleave_info_constructor_test COMMAND $<TARGET_FILE:interleave_info_bin>)
cpp_cc_configure_sanitizers(TARGET interleave_info_bin TEST interleave_info_constructor_test)
