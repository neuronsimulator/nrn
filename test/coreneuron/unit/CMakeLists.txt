include(FindSLURM)
include(TestHelpers)

include_directories(${CMAKE_SOURCE_DIR}/coreneuron ${CMAKE_SOURCE_DIR} ${Boost_INCLUDE_DIRS})

if(NOT Boost_USE_STATIC_LIBS)
    add_definitions(-DBOOST_TEST_DYN_LINK=TRUE)
endif()


if(UNIT_TESTS)

add_subdirectory(unit/endian)
add_subdirectory(unit/sdprintf)
add_subdirectory(unit/mechbuild)
add_subdirectory(unit/omp)
add_subdirectory(unit/cmdline_interface)
add_subdirectory(unit/alignment)

endif()

if(FUNCTIONAL_TESTS)

add_subdirectory(integration)

endif()

