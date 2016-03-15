FILE(GLOB queuing_test_src "*.cpp")

add_executable(queuing_test_bin ${queuing_test_src})
target_link_libraries(queuing_test_bin ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY} ${Boost_SYSTEM_LIBRARY} coreneuron)

add_test(NAME queuing_test COMMAND ${TEST_EXEC_PREFIX} ${CMAKE_CURRENT_BINARY_DIR}/queuing_test_bin)





