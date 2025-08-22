# =============================================================================
# Copyright (c) 2022 Blue Brain Project/EPFL
#
# See top-level LICENSE file for details.
# =============================================================================
add_executable(test-mech-mapping test_mech_mapping.cpp)
target_link_libraries(test-mech-mapping coreneuron-unit-test Catch2::Catch2WithMain)
add_test(NAME test-mech-mapping COMMAND $<TARGET_FILE:test-mech-mapping>)
cpp_cc_configure_sanitizers(TARGET test-mech-mapping TEST test-mech-mapping)
