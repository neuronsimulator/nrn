# =============================================================================
# Copyright (C) 2022 Blue Brain Project
#
# See top-level LICENSE file for details.
# =============================================================================

include_directories(${CMAKE_SOURCE_DIR}/coreneuron ${Boost_INCLUDE_DIRS})
add_executable(test-solver test_solver.cpp)
target_link_libraries(test-solver ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY} coreneuron
                      ${corenrn_mech_library})
target_include_directories(test-solver SYSTEM
                           PRIVATE ${CORENEURON_PROJECT_SOURCE_DIR}/external/CLI11/include)

# Tell CMake *not* to run an explicit device code linker step (which will produce errors); let the
# NVHPC C++ compiler handle this implicitly.
set_target_properties(test-solver PROPERTIES CUDA_RESOLVE_DEVICE_SYMBOLS OFF)
target_compile_options(test-solver PRIVATE ${CORENEURON_BOOST_UNIT_TEST_COMPILE_FLAGS})
add_dependencies(test-solver nrniv-core)
add_test(NAME test-solver COMMAND $<TARGET_FILE:test-solver>)
cpp_cc_configure_sanitizers(TARGET test-solver TEST test-solver)
