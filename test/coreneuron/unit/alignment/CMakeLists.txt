
include_directories(${CMAKE_SOURCE_DIR}/coreneuron ${Boost_INCLUDE_DIRS})
FILE(GLOB alignment_test_src "*.cpp")

add_executable(alignment_test_bin ${alignment_test_src})
target_link_libraries(alignment_test_bin ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY})

add_test(NAME alignment_test COMMAND ${TEST_EXEC_PREFIX} ${CMAKE_CURRENT_BINARY_DIR}/alignment_test_bin)


