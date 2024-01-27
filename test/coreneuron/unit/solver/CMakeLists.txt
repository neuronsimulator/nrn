# =============================================================================
# Copyright (c) 2022 Blue Brain Project/EPFL
#
# See top-level LICENSE file for details.
# =============================================================================
add_executable(test-solver test_solver.cpp)
target_link_libraries(test-solver coreneuron-unit-test Catch2::Catch2)
add_test(NAME test-solver COMMAND $<TARGET_FILE:test-solver>)
cpp_cc_configure_sanitizers(TARGET test-solver TEST test-solver)
