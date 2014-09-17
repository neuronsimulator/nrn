# configures an external git repository
# Usage:
#  * Automatically reads, parses and updates a .gitexternals file if it only
#    contains lines in the form "# <directory> <giturl> <gittag>".
#    This function parses the file for this pattern and then calls
#    git_external on each found entry. Additionally it provides an
#    update target to bump the tag to the master revision by
#    recreating .gitexternals.
#  * Provides function
#    git_external(<directory> <giturl> <gittag> [RESET <files>])
#  git_external_manage(<file>)

find_package(Git)
if(NOT GIT_EXECUTABLE)
  return()
endif()

include(CMakeParseArguments)

function(GIT_EXTERNAL DIR REPO TAG)
  cmake_parse_arguments(GIT_EXTERNAL "" "" "RESET" ${ARGN})
  get_filename_component(DIR "${DIR}" ABSOLUTE)
  get_filename_component(GIT_EXTERNAL_DIR "${DIR}/.." ABSOLUTE)

  if(NOT EXISTS "${DIR}")
    message(STATUS "git clone ${REPO} ${DIR}")
    execute_process(
      COMMAND "${GIT_EXECUTABLE}" clone "${REPO}" "${DIR}"
      RESULT_VARIABLE nok ERROR_VARIABLE error
      WORKING_DIRECTORY "${GIT_EXTERNAL_DIR}")
    if(nok)
      message(FATAL_ERROR "${DIR} git clone failed: ${error}\n")
    endif()
  endif()

  if(IS_DIRECTORY "${DIR}/.git")
    execute_process(COMMAND ${GIT_EXECUTABLE} rev-parse --short HEAD
      OUTPUT_VARIABLE currentref OUTPUT_STRIP_TRAILING_WHITESPACE
      WORKING_DIRECTORY ${DIR})
    if(currentref STREQUAL TAG) # nothing to do
      return()
    endif()

    # reset generated files
    foreach(GIT_EXTERNAL_RESET_FILE ${GIT_EXTERNAL_RESET})
      execute_process(
        COMMAND "${GIT_EXECUTABLE}" reset -q "${GIT_EXTERNAL_RESET_FILE}"
        RESULT_VARIABLE nok ERROR_VARIABLE error
        WORKING_DIRECTORY "${DIR}")
      execute_process(
        COMMAND "${GIT_EXECUTABLE}" checkout -q -- "${GIT_EXTERNAL_RESET_FILE}"
        RESULT_VARIABLE nok ERROR_VARIABLE error
        WORKING_DIRECTORY "${DIR}")
    endforeach()

    # fetch latest update
    execute_process(COMMAND "${GIT_EXECUTABLE}" fetch --all -q
      RESULT_VARIABLE nok ERROR_VARIABLE error
      WORKING_DIRECTORY "${DIR}")
    if(nok)
      message(STATUS "Update of ${DIR} failed:\n   ${error}")
    endif()

    # checkout requested tag
    execute_process(
      COMMAND "${GIT_EXECUTABLE}" checkout -q "${TAG}"
      RESULT_VARIABLE nok ERROR_VARIABLE error
      WORKING_DIRECTORY "${DIR}"
      )
    if(nok)
      message(STATUS "${DIR} git checkout ${TAG} failed: ${error}\n")
    endif()
  else()
    message(STATUS "Can't update git external ${DIR}: Not a git repository")
  endif()
endfunction()

set(GIT_EXTERNALS ${GIT_EXTERNALS_FILE})
if(NOT GIT_EXTERNALS)
  set(GIT_EXTERNALS "${CMAKE_CURRENT_SOURCE_DIR}/.gitexternals")
endif()

if(EXISTS ${GIT_EXTERNALS})
  file(READ ${GIT_EXTERNALS} GIT_EXTERNAL_FILE)
  string(REGEX REPLACE "\n" ";" GIT_EXTERNAL_FILE "${GIT_EXTERNAL_FILE}")
  foreach(LINE ${GIT_EXTERNAL_FILE})
    if(NOT LINE MATCHES "^#.*$")
      message(FATAL_ERROR "${GIT_EXTERNALS} contains non-comment line: ${LINE}")
    endif()
    string(REGEX REPLACE "^#[ ]*(.+[ ]+.+[ ]+.+)$" "\\1" DATA ${LINE})
    if(NOT LINE STREQUAL DATA)
      string(REGEX REPLACE "[ ]+" ";" DATA "${DATA}")
      list(LENGTH DATA DATA_LENGTH)
      if(DATA_LENGTH EQUAL 3)
        list(GET DATA 0 DIR)
        list(GET DATA 1 REPO)
        list(GET DATA 2 TAG)

        # Create a unique, flat name
        string(REPLACE "/" "_" GIT_EXTERNAL_NAME ${DIR})

        if(NOT TARGET update_git_external_${GIT_EXTERNAL_NAME}) # not done
          # pull in identified external
          git_external(${DIR} ${REPO} ${TAG})

          # Create update script and target to bump external spec
          if(NOT TARGET update)
            add_custom_target(update)
          endif()
          if(NOT TARGET update_git_external)
            add_custom_target(update_git_external)
            add_dependencies(update update_git_external)
          endif()

          # Create a unique, flat name
          file(RELATIVE_PATH GIT_EXTERNALS_BASE ${CMAKE_SOURCE_DIR}
            ${GIT_EXTERNALS})
          string(REPLACE "/" "_" GIT_EXTERNAL_TARGET ${GIT_EXTERNALS_BASE})

          set(GIT_EXTERNAL_TARGET update_git_external_${GIT_EXTERNAL_TARGET})
          if(NOT TARGET ${GIT_EXTERNAL_TARGET})
            set(GIT_EXTERNAL_SCRIPT
              "${CMAKE_CURRENT_BINARY_DIR}/${GIT_EXTERNAL_TARGET}.cmake")
            file(WRITE "${GIT_EXTERNAL_SCRIPT}"
              "file(WRITE ${GIT_EXTERNALS} \"# -*- mode: cmake -*-\n\")\n")
            add_custom_target(${GIT_EXTERNAL_TARGET}
              COMMAND ${CMAKE_COMMAND} -P ${GIT_EXTERNAL_SCRIPT}
              COMMENT "Recreate ${GIT_EXTERNALS_BASE}"
              WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
          endif()

          set(GIT_EXTERNAL_SCRIPT
            "${CMAKE_CURRENT_BINARY_DIR}/gitupdate${GIT_EXTERNAL_NAME}.cmake")
          file(WRITE "${GIT_EXTERNAL_SCRIPT}" "
execute_process(COMMAND ${GIT_EXECUTABLE} fetch --all -q
  WORKING_DIRECTORY ${DIR})
execute_process(
  COMMAND ${GIT_EXECUTABLE} show-ref --hash=7 refs/remotes/origin/master
  OUTPUT_VARIABLE newref WORKING_DIRECTORY ${DIR})
if(newref)
  file(APPEND ${GIT_EXTERNALS} \"# ${DIR} ${REPO} \${newref}\")
else()
  file(APPEND ${GIT_EXTERNALS} \"# ${DIR} ${REPO} ${TAG}\n\")
endif()")
          add_custom_target(update_git_external_${GIT_EXTERNAL_NAME}
            COMMAND ${CMAKE_COMMAND} -P ${GIT_EXTERNAL_SCRIPT}
            COMMENT "Update ${REPO} in ${GIT_EXTERNALS_BASE}"
            DEPENDS ${GIT_EXTERNAL_TARGET}
            WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
          add_dependencies(update_git_external
            update_git_external_${GIT_EXTERNAL_NAME})
        endif()
      endif()
    endif()
  endforeach()
  include(${GIT_EXTERNALS})
endif()
