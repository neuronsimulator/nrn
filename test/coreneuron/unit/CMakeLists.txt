# =============================================================================
# Copyright (C) 2016-2019 Blue Brain Project
#
# See top-level LICENSE file for details.
# =============================================================================

include(TestHelpers)

include_directories(${CORENEURON_PROJECT_SOURCE_DIR} ${Boost_INCLUDE_DIRS})

if(NOT Boost_USE_STATIC_LIBS)
  add_definitions(-DBOOST_TEST_DYN_LINK=TRUE)
endif()

set(Boost_NO_BOOST_CMAKE TRUE)
find_package(Boost 1.41.0 QUIET
             COMPONENTS filesystem
                        system
                        atomic
                        unit_test_framework)

if(Boost_FOUND)
  if(CORENRN_ENABLE_UNIT_TESTS)
    include_directories(${Boost_INCLUDE_DIRS})
    add_subdirectory(unit/cmdline_interface)
    add_subdirectory(unit/interleave_info)
    add_subdirectory(unit/alignment)
    add_subdirectory(unit/queueing)
  endif()
else()
  message(STATUS "Boost not found, unit tests disabled")
endif()

add_subdirectory(integration)
