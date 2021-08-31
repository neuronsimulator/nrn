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

FetchContent_MakeAvailable(ringtest testcorenrn)

add_subdirectory(ringtest)
add_subdirectory(testcorenrn)
