find_package(Boost COMPONENTS unit_test_framework)


if(Boost_FOUND)
    set(TEST_LIBRARIES ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY} coreneuron)
    set(TEST_LABEL unit)
    make_mpi_exeprefix(1 TEST_PREFIX)
    include_directories(${CMAKE_SOURCE_DIR}/coreneuron ${Boost_INCLUDE_DIRS})

    include(CommonCPPCTest)
endif()

add_test_class(unit)

