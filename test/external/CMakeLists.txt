#
# Add tests based on external repositories
#

include(FetchContent)

FetchContent_Declare(
    ringtest
    GIT_REPOSITORY https://github.com/neuronsimulator/ringtest
    GIT_TAG 756f63e82f4f17b5bbaeabf317c21c3e5228a2b2
    SOURCE_DIR ${PROJECT_SOURCE_DIR}/external/tests/ringtest
)

FetchContent_Declare(
    testcorenrn
    GIT_REPOSITORY https://github.com/neuronsimulator/testcorenrn
    GIT_TAG aee682
    SOURCE_DIR ${PROJECT_SOURCE_DIR}/external/tests/testcorenrn
)

FetchContent_MakeAvailable(ringtest testcorenrn)

add_subdirectory(testcorenrn)
