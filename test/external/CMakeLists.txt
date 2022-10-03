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
  GIT_TAG 014aab9bee60a2bdd44b28a784c88092d2f6b92b
  SOURCE_DIR ${PROJECT_SOURCE_DIR}/external/tests/testcorenrn)

FetchContent_Declare(
  nrntest
  GIT_REPOSITORY https://github.com/neuronsimulator/nrntest
  GIT_TAG aa899cec3109a17d7484ed6edb6cc66712662075
  SOURCE_DIR ${PROJECT_SOURCE_DIR}/external/tests/nrntest)

FetchContent_Declare(
  reduced_dentate
  GIT_REPOSITORY https://github.com/neuronsimulator/reduced_dentate
  GIT_TAG 58ad3890536ba09e05fd3ee7a178a18270ea8faa
  SOURCE_DIR ${PROJECT_SOURCE_DIR}/external/tests/reduced_dentate)

FetchContent_Declare(
  tqperf
  GIT_REPOSITORY https://github.com/neuronsimulator/tqperf.git
  GIT_TAG add46f2899f1cde55032aa63cda54f3941e81155
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
    GIT_TAG 85e282834ec04c48adbd4f38d04bbbce6fb99afc
    SOURCE_DIR ${PROJECT_SOURCE_DIR}/external/tests/channel-benchmark)
  FetchContent_MakeAvailable(channel-benchmark)
  add_subdirectory(channel-benchmark)
endif()

if("olfactory" IN_LIST NRN_ENABLE_MODEL_TESTS)
  FetchContent_Declare(
    olfactory-bulb-3d
    GIT_REPOSITORY https://github.com/HumanBrainProject/olfactory-bulb-3d.git
    GIT_TAG b07b76dc08173fa4d6dce7ef5d05d559b54769ed
    SOURCE_DIR ${PROJECT_SOURCE_DIR}/external/tests/olfactory-bulb-3d)
  FetchContent_MakeAvailable(olfactory-bulb-3d)
  add_subdirectory(olfactory-bulb-3d)
endif()
