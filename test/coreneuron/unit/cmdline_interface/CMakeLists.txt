# =============================================================================
# Copyright (c) 2016 - 2022 Blue Brain Project/EPFL
#
# See top-level LICENSE file for details.
# =============================================================================
add_executable(cmd_interface_test_bin test_cmdline_interface.cpp)
target_link_libraries(cmd_interface_test_bin coreneuron-unit-test Catch2::Catch2)
add_test(NAME cmd_interface_test COMMAND $<TARGET_FILE:cmd_interface_test_bin>)
cpp_cc_configure_sanitizers(TARGET cmd_interface_test_bin TEST cmd_interface_test)
