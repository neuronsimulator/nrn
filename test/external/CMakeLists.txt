#
# Add tests based on external repositories
#

include(FetchContent)

FetchContent_Declare(
  ringtest
  GIT_REPOSITORY https://github.com/neuronsimulator/ringtest
  GIT_TAG ee24c8
  SOURCE_DIR ${PROJECT_SOURCE_DIR}/external/tests/ringtest)

FetchContent_Declare(
  testcorenrn
  GIT_REPOSITORY https://github.com/neuronsimulator/testcorenrn
  GIT_TAG e56028063c83a40a2cecc3bd3868ee0570c4a553
  SOURCE_DIR ${PROJECT_SOURCE_DIR}/external/tests/testcorenrn)

FetchContent_Declare(
  nrntest
  GIT_REPOSITORY https://github.com/neuronsimulator/nrntest
  GIT_TAG 0752af1fd35ab34f538d6f212fb3d10bcce6a2d3
  SOURCE_DIR ${PROJECT_SOURCE_DIR}/external/tests/nrntest)

FetchContent_Declare(
  reduced_dentate
  GIT_REPOSITORY https://github.com/neuronsimulator/reduced_dentate
  GIT_TAG 2aa03cfbfe8a620b7e9ae29922513be7842983de
  SOURCE_DIR ${PROJECT_SOURCE_DIR}/external/tests/reduced_dentate)

FetchContent_Declare(
  tqperf
  GIT_REPOSITORY https://github.com/neuronsimulator/tqperf.git
  GIT_TAG aa1344a5764f61bd097b9e2cf2e59d06c577ae85
  SOURCE_DIR ${PROJECT_SOURCE_DIR}/external/tests/tqperf)

FetchContent_MakeAvailable(nrntest reduced_dentate ringtest testcorenrn tqperf)

add_subdirectory(nrntest)
add_subdirectory(reduced_dentate)
add_subdirectory(ringtest)
add_subdirectory(testcorenrn)

# tqperf requires libcrypto library
find_package(OpenSSL)

if(OPENSSL_FOUND)
  add_subdirectory(tqperf)
else()
  message(STATUS "Skipping tqperf test as OpenSSL package not found")
endif()

if("channel-benchmark" IN_LIST NRN_ENABLE_MODEL_TESTS)
  FetchContent_Declare(
    channel-benchmark
    GIT_REPOSITORY git@bbpgitlab.epfl.ch:hpc/channel-benchmark.git
    GIT_TAG e412e6
    SOURCE_DIR ${PROJECT_SOURCE_DIR}/external/tests/channel-benchmark)
  FetchContent_MakeAvailable(channel-benchmark)
  add_subdirectory(channel-benchmark)
endif()

if("olfactory" IN_LIST NRN_ENABLE_MODEL_TESTS)
  FetchContent_Declare(
    olfactory-bulb-3d
    GIT_REPOSITORY https://github.com/HumanBrainProject/olfactory-bulb-3d.git
    GIT_TAG dde6f77e0477e5e8c931ca414f15807c326a2089
    SOURCE_DIR ${PROJECT_SOURCE_DIR}/external/tests/olfactory-bulb-3d)
  FetchContent_MakeAvailable(olfactory-bulb-3d)
  add_subdirectory(olfactory-bulb-3d)
endif()
