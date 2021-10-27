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
  GIT_TAG 1e2e54
  SOURCE_DIR ${PROJECT_SOURCE_DIR}/external/tests/testcorenrn)

FetchContent_Declare(
  nrntest
  GIT_REPOSITORY https://github.com/neuronsimulator/nrntest
  GIT_TAG 2fbb3c8303c94d76113cc74f13ef9df0014fdf71
  SOURCE_DIR ${PROJECT_SOURCE_DIR}/external/tests/nrntest)

FetchContent_Declare(
  reduced_dentate
  GIT_REPOSITORY https://github.com/pramodk/reduced_dentate
  GIT_TAG 70f363d7f82dfe6037fbddd8ed2e58cb20df6343
  SOURCE_DIR ${PROJECT_SOURCE_DIR}/external/tests/reduced_dentate)

FetchContent_MakeAvailable(nrntest reduced_dentate ringtest testcorenrn)

add_subdirectory(nrntest)
add_subdirectory(reduced_dentate)
add_subdirectory(ringtest)
add_subdirectory(testcorenrn)

if(NRN_ENABLE_TESTS_BBP)
  FetchContent_Declare(
    channel-benchmark
    GIT_REPOSITORY git@bbpgitlab.epfl.ch:hpc/channel-benchmark.git
    GIT_TAG f83be1
    SOURCE_DIR ${PROJECT_SOURCE_DIR}/external/tests/channel-benchmark)
  FetchContent_MakeAvailable(channel-benchmark)
  add_subdirectory(channel-benchmark)
endif()
