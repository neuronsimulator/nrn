# =============================================================================
# Copyright (c) 2016 - 2022 Blue Brain Project/EPFL
#
# See top-level LICENSE file for details.
# =============================================================================
add_executable(lfp_test_bin lfp.cpp)
target_link_libraries(lfp_test_bin coreneuron-unit-test Catch2::Catch2)
add_test(NAME lfp_test COMMAND $<TARGET_FILE:lfp_test_bin>)
cpp_cc_configure_sanitizers(TARGET lfp_test_bin TEST lfp_test)
set_property(
  TEST lfp_test
  APPEND
  PROPERTY ENVIRONMENT OMP_NUM_THREADS=1)
