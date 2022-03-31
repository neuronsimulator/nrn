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
  GIT_TAG 40b66929c
  SOURCE_DIR ${PROJECT_SOURCE_DIR}/external/tests/testcorenrn)

FetchContent_Declare(
  nrntest
  GIT_REPOSITORY https://github.com/neuronsimulator/nrntest
  GIT_TAG dac8cf2c2d97f45a81e06dc0bc5db2c02ee475b9
  SOURCE_DIR ${PROJECT_SOURCE_DIR}/external/tests/nrntest)

FetchContent_Declare(
  reduced_dentate
  GIT_REPOSITORY https://github.com/neuronsimulator/reduced_dentate
  GIT_TAG a1d410e7a2a3e687fcc3e4ee20f4fcf6d52c7cfa
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
    GIT_TAG f83be1
    SOURCE_DIR ${PROJECT_SOURCE_DIR}/external/tests/channel-benchmark)
  FetchContent_MakeAvailable(channel-benchmark)
  add_subdirectory(channel-benchmark)
endif()

if("olfactory" IN_LIST NRN_ENABLE_MODEL_TESTS)
  FetchContent_Declare(
    olfactory-bulb-3d
    GIT_REPOSITORY https://github.com/HumanBrainProject/olfactory-bulb-3d.git
    GIT_TAG 9fb5b17
    SOURCE_DIR ${PROJECT_SOURCE_DIR}/external/tests/olfactory-bulb-3d)
  FetchContent_MakeAvailable(olfactory-bulb-3d)
  add_subdirectory(olfactory-bulb-3d)
endif()
