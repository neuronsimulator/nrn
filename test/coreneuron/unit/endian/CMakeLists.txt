# Need to test the endian code against aggressive compiler optimization
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CMAKE_${COMPILER_LANGUAGE}_OPT_AGGRESSIVE}")

FILE(GLOB endian_test_src "*.cpp")

foreach(test_src_file ${endian_test_src})
get_filename_component(test_name ${test_src_file} NAME)

add_executable(${test_name}_test_bin ${test_src_file})
target_link_libraries(${test_name}_test_bin ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY})

add_test(NAME ${test_name}_test COMMAND ${TEST_EXEC_PREFIX} ${CMAKE_CURRENT_BINARY_DIR}/${test_name}_test_bin)

endforeach()
