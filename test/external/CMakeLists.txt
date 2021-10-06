#
# Add tests based on external repositories
#

include(FetchContent)

FetchContent_Declare(
    ringtest
    GIT_REPOSITORY https://github.com/neuronsimulator/ringtest
    GIT_TAG ee24c8
    SOURCE_DIR ${PROJECT_SOURCE_DIR}/external/tests/ringtest
)

FetchContent_Declare(
    testcorenrn
    GIT_REPOSITORY https://github.com/neuronsimulator/testcorenrn
    GIT_TAG aee682
    SOURCE_DIR ${PROJECT_SOURCE_DIR}/external/tests/testcorenrn
)

FetchContent_Declare(
    nrntest
    GIT_REPOSITORY https://github.com/neuronsimulator/nrntest
    GIT_TAG 12df7acc69a354d5d395680a79096b16c3f0d56d
    SOURCE_DIR ${PROJECT_SOURCE_DIR}/external/tests/nrntest
)

FetchContent_MakeAvailable(ringtest testcorenrn nrntest)

add_subdirectory(ringtest)
add_subdirectory(testcorenrn)
add_subdirectory(nrntest)

if(NRN_ENABLE_TESTS_BBP)
    FetchContent_Declare(
        channel-benchmark
        GIT_REPOSITORY git@bbpgitlab.epfl.ch:hpc/channel-benchmark.git
        GIT_TAG f83be1
        SOURCE_DIR ${PROJECT_SOURCE_DIR}/external/tests/channel-benchmark
    )
    FetchContent_MakeAvailable(channel-benchmark)
    add_subdirectory(channel-benchmark)
endif()
