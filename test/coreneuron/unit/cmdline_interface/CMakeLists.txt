

FILE(GLOB cmd_interface_test_src "*.cpp")

add_executable(cmd_interface_test_bin ${cmd_interface_test_src})
target_link_libraries(cmd_interface_test_bin ${MPI_C_LIBRARIES} ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY} coreneuron)

add_test(NAME cmd_interface_test COMMAND ${TEST_EXEC_PREFIX} ${CMAKE_CURRENT_BINARY_DIR}/cmd_interface_test_bin)

