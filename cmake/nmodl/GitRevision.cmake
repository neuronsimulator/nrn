# For now use simple approach to get version information as git is always avaialble on the machine
# where we are building

find_package(Git)

if(GIT_FOUND)

  # get last commit sha1
  execute_process(
    COMMAND ${GIT_EXECUTABLE} log -1 --format=%h
    WORKING_DIRECTORY ${NMODL_PROJECT_SOURCE_DIR}
    OUTPUT_VARIABLE NMODL_GIT_REVISION_SHA1
    RESULT_VARIABLE NMODL_GIT_STATUS
    ERROR_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE)
  if(NOT NMODL_GIT_STATUS EQUAL 0)
    set(NMODL_GIT_REVISION_SHA1 "git-error")
  endif()

  # get last commit date
  execute_process(
    COMMAND ${GIT_EXECUTABLE} show -s --format=%ci
    WORKING_DIRECTORY ${NMODL_PROJECT_SOURCE_DIR}
    OUTPUT_VARIABLE NMODL_GIT_REVISION_DATE
    RESULT_VARIABLE NMODL_GIT_STATUS
    ERROR_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE)
  if(NOT NMODL_GIT_STATUS EQUAL 0)
    set(NMODL_GIT_REVISION_SHA1 "git-error")
  endif()

  # remove extra double quotes
  string(REGEX REPLACE "\"" "" NMODL_GIT_REVISION_DATE "${NMODL_GIT_REVISION_DATE}")
  set(NMODL_GIT_REVISION "${NMODL_GIT_REVISION_SHA1} ${NMODL_GIT_REVISION_DATE}")

  # get the last version tag from git
  execute_process(
    COMMAND ${GIT_EXECUTABLE} describe --abbrev=0 --tags
    WORKING_DIRECTORY ${NMODL_PROJECT_SOURCE_DIR}
    OUTPUT_VARIABLE NMODL_GIT_LAST_TAG
    RESULT_VARIABLE NMODL_GIT_STATUS
    ERROR_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE)
  if(NOT NMODL_GIT_STATUS EQUAL 0)
    # Must be a valid version from CMake's perspective.
    set(NMODL_GIT_LAST_TAG "0.0")
  endif()
else()
  set(NMODL_GIT_REVISION "unknown")
  # Must be a valid version from CMake's perspective.
  set(NMODL_GIT_LAST_TAG "0.0")
endif()
