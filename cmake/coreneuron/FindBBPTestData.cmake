include(UpdateFile)

# /bgscratch mount available
if(IS_DIRECTORY "/bgscratch" AND NOT BBP_BGSCRATCH_MOUNT)
  set(BBP_BGSCRATCH_MOUNT "/bgscratch")
endif()
if(IS_DIRECTORY "${BBP_BGSCRATCH_MOUNT}/bbp")
  message(STATUS "Blue Gene test dataset mounted at ${BBP_BGSCRATCH_MOUNT}")
  add_definitions(-DBBP_BGSCRATCH_MOUNT=\"${BBP_BGSCRATCH_MOUNT}\")
else()
  set(BBP_BGSCRATCH_MOUNT)
endif()

# local test dataset
find_path(BBP_TESTDATA
  local/simulations/may17_2011/Control/BlueConfig.in
  ${CMAKE_SOURCE_DIR}/BBPTestData
  ${CMAKE_SOURCE_DIR}/../BBPTestData
  ${CMAKE_SOURCE_DIR}/../../../BBPTestData
  ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/BBPTestData
  ${CMAKE_SOURCE_DIR}/testdata
  ${CMAKE_SOURCE_DIR}/../testdata
  ${CMAKE_SOURCE_DIR}/../../../testdata
  ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/testdata
  ENV BBP_TESTDATA_ROOT BBP_SDK_LOCAL_TEST_SIMULATIONS)

if(BBP_TESTDATA)
  message(STATUS "Local test dataset in ${BBP_TESTDATA}")
  set(BBP_TESTDATA_FOUND ON)
  set(BBP_TESTDATA_INCLUDE_DIRS "${CMAKE_BINARY_DIR}/include/")

  # test dataset - derive and substitute absolute paths in example blueconfig
  set(BBP_LOCAL_BLUECONFIG "${CMAKE_BINARY_DIR}/simulations/may17_2011/Control/BlueConfig")

  # temporary, change name to BBP_TESTDATA in BlueConfig.in once SDK is migrated
  set(BBP_SDK_LOCAL_TEST_SIMULATIONS "${BBP_TESTDATA}")

  add_definitions(-DBBP_LOCAL_BLUECONFIG=\"${BBP_LOCAL_BLUECONFIG}\")

  update_file(
    "${BBP_TESTDATA}/local/simulations/may17_2011/Control/BlueConfig.in"
    "${BBP_LOCAL_BLUECONFIG}")

  update_file( "${BBP_TESTDATA}/include/TestDatasets.in.h"
    "${CMAKE_BINARY_DIR}/include/BBP/TestDatasets.h")
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(BBP_TESTDATA DEFAULT_MSG
  BBP_TESTDATA_INCLUDE_DIRS)
