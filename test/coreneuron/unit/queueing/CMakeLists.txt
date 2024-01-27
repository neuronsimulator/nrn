# =============================================================================
# Copyright (c) 2016 - 2022 Blue Brain Project/EPFL
#
# See top-level LICENSE file for details.
# =============================================================================
add_executable(queuing_test_bin test_queueing.cpp)
target_link_libraries(queuing_test_bin coreneuron-unit-test Catch2::Catch2)
add_test(NAME queuing_test COMMAND $<TARGET_FILE:queuing_test_bin>)
cpp_cc_configure_sanitizers(TARGET queuing_test_bin TEST queuing_test)
