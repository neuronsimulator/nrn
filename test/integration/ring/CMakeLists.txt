include_directories(${CMAKE_SOURCE_DIR}/src)
link_directories (${CMAKE_BINARY_DIR}/src/simcore ${CMAKE_BINARY_DIR}/src/mech)


set(LIBRARY corebluron coremech)

add_definitions (-DBOOST_TEST_DYN_LINK -Wno-unused-variable)

set(tests ring)

foreach(test ${tests})
#    configure_file(${test}.cpp.in ${CMAKE_SOURCE_DIR}/test/integration/${test}/${test}.cpp @ONLY)
    add_executable(${test}_test ${test}.cpp)
    target_link_libraries(${test}_test ${Boost_LIBRARIES} ${LIBRARY})
    add_test(${test}_test ${test}_test) 
endforeach(test)


