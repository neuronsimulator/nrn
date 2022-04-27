find_package(Git QUIET)
if(GIT_FOUND)
  # --abbrev=0 just gives the parent tag without a suffix representing the number of commits since
  # the tag and the current commit hash
  execute_process(
    COMMAND ${GIT_EXECUTABLE} describe --abbrev=0
    WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
    RESULT_VARIABLE GIT_EXIT_CODE
    OUTPUT_VARIABLE PARENT_GIT_TAG
    ERROR_VARIABLE GIT_STDERR
    OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_STRIP_TRAILING_WHITESPACE)
  if(${GIT_EXIT_CODE} EQUAL 0)
    if(NOT GIT_STDERR STREQUAL "")
      message(FATAL_ERROR "git describe succeeded but had stderr: '${GIT_STDERR}'")
    endif()
    # ${PARENT_GIT_TAG} contains the name of the parent tag of the current commit. In a tagged
    # release, this should be exactly the version number in semver-compatible MAJOR.MINOR.PATCH
    # format. On a development or feature branch, this should be a tag that has been created just
    # after the last release from that branch. For example, the master branch (and feature branches
    # based on it) after the 8.1 release have a parent tag called 8.2.dev, so that git describe
    # produces output compatible with the guidelines in the NEURON documentation:
    # https://nrn.readthedocs.io/en/latest/scm/guide/SCMGuide.html#a-versioning-scheme-for-neuron.
    # In a branch whose parent tag is 8.2.dev, the project(...) call should say that the version is
    # 8.2.0. If and when an incompatible change is merged to the master branch then a 9.0.dev tag
    # should be created and the project(...) version should be updated to 9.0.0. If changes are
    # cherry-picked to a branch such as release/8.1 then a 8.1.1.dev tag should be created there and
    # the project(...) version in that branch updated to 8.1.1. The three cases handled are:
    #
    # * if the parent tag is a MAJOR.MINOR.PATCH version then it should exactly match project(...)
    # * if the parent tag is MAJOR.MINOR(a|b|rcN) then project(...) should contain MAJOR.MINOR.0
    # * if the parent tag has a .dev suffix, the part before .dev is suffixed with .0 if needed to
    #   make a full MAJOR.MINOR.PATCH version, which should exactly match project(...)
    if(PARENT_GIT_TAG MATCHES "^([0-9]+\\.[0-9]+)\\.dev$")
      # X.Y.dev CMAKE_MATCH_1 holds X.Y
      set(expected_version "${CMAKE_MATCH_1}.0")
    elseif(PARENT_GIT_TAG MATCHES "^([0-9]+\\.[0-9]+\\.[0-9]+)\\.dev$")
      # X.Y.Z.dev, CMAKE_MATCH_1 holds X.Y.Z
      set(expected_version "${CMAKE_MATCH_1}")
    elseif(PARENT_GIT_TAG MATCHES "^([0-9]+)\\.([0-9]+)\\.([0-9]+)$")
      # Release tag
      set(expected_version "${PARENT_GIT_TAG}")
    elseif(PARENT_GIT_TAG MATCHES "^([0-9]+\\.[0-9]+)(a|b|rc[0-9]+)$")
      # Alpha, beta or release candidate (what about alpha/beta/rc of a patch release, i.e. 8.1.1a?
      # This is not foreseen in the docs.)
      set(expected_version "${CMAKE_MATCH_1}.0")
    else()
      message(FATAL_ERROR "Failed to parse Git tag: '${GIT_STDOUT}'")
    endif()
    if(NOT expected_version STREQUAL
       "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}")
      message(WARNING "Parent git tag is ${PARENT_GIT_TAG} but the CMake version is "
                      "${PROJECT_VERSION}; you should probably update CMakeLists.txt "
                      "to contain project(NEURON VERSION ${expected_version} ...)!")
    endif()
  else()
    message(
      STATUS "git describe failed "
             "(stdout: '${PARENT_GIT_TAG}', stderr: '${GIT_STDERR}', code: ${GIT_EXIT_CODE}), "
             "skipping the version check")
  endif()
endif()
