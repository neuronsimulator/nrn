FILE(GLOB omp_test_src "*.cpp")

add_executable(omp_test_bin ${omp_test_src})
target_link_libraries(omp_test_bin coreneuron ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY})

add_test(NAME omp_test COMMAND ${TEST_EXEC_PREFIX} ${CMAKE_CURRENT_BINARY_DIR}/omp_test_bin)

