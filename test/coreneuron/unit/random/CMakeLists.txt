# =============================================================================
# Copyright (c) 2016 - 2024 Blue Brain Project/EPFL
#
# See top-level LICENSE file for details.
# =============================================================================
if(${CMAKE_C_COMPILER_VERSION} VERSION_GREATER_EQUAL 21.11)
  # Random123 does not play nicely with NVHPC 21.11+'s detection of ABM features, see:
  # https://github.com/BlueBrain/CoreNeuron/issues/724 and
  # https://github.com/DEShawResearch/random123/issues/6.
  list(APPEND NRN_R123_COMPILE_DEFS R123_USE_INTRIN_H=0)
endif()
add_executable(random_test_bin test_random.cpp)
target_link_libraries(random_test_bin coreneuron-unit-test Catch2::Catch2WithMain)
target_compile_definitions(random_test_bin PRIVATE ${NRN_R123_COMPILE_DEFS})
add_test(NAME random_test COMMAND $<TARGET_FILE:random_test_bin>)
cpp_cc_configure_sanitizers(TARGET random_test_bin TEST random_test)
