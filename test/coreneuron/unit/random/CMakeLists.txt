# =============================================================================
# Copyright (c) 2016 - 2024 Blue Brain Project/EPFL
#
# See top-level LICENSE file for details.
# =============================================================================
list(APPEND CORENRN_COMPILE_DEFS R123_USE_INTRIN_H=0)
add_executable(random_test_bin test_random.cpp)
target_link_libraries(random_test_bin coreneuron-unit-test Catch2::Catch2)
add_test(NAME random_test COMMAND $<TARGET_FILE:random_test_bin>)
cpp_cc_configure_sanitizers(TARGET random_test_bin TEST random_test)
