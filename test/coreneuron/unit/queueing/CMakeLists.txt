# =============================================================================
# Copyright (c) 2016 - 2022 Blue Brain Project/EPFL
#
# See top-level LICENSE file for details.
# =============================================================================
add_executable(queuing_test_bin test_queueing.cpp)
target_link_libraries(queuing_test_bin ${Boost_SYSTEM_LIBRARY} coreneuron ${corenrn_mech_library}
                      ${reportinglib_LIBRARY} ${sonatareport_LIBRARY})
add_dependencies(queuing_test_bin nrniv-core)
# Tell CMake *not* to run an explicit device code linker step (which will produce errors); let the
# NVHPC C++ compiler handle this implicitly.
set_target_properties(queuing_test_bin PROPERTIES CUDA_RESOLVE_DEVICE_SYMBOLS OFF)
target_compile_options(queuing_test_bin PRIVATE ${CORENEURON_BOOST_UNIT_TEST_COMPILE_FLAGS})
add_test(NAME queuing_test COMMAND ${TEST_EXEC_PREFIX} $<TARGET_FILE:queuing_test_bin>)
cpp_cc_configure_sanitizers(TARGET queuing_test_bin TEST queuing_test)
