find_package(Boost COMPONENTS unit_test_framework)


if(Boost_FOUND)
    set(TEST_LIBRARIES ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY})
    set(TEST_LABEL unit)
    make_mpi_exeprefix(1 TEST_PREFIX)
    include_directories(${CMAKE_SOURCE_DIR}/corebluron ${Boost_INCLUDE_DIRS})

    # Need to test the endian code against aggressive compiler optimization:
    # -O4 is xlc highest level without link-level optimization.
    # clang may well complain.
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O4")

    include(CommonCPPCTest)
endif()

add_test_class(unit)

