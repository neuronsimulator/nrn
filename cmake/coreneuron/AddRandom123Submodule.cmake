# =============================================================================
# Copyright (C) 2016-2020 Blue Brain Project
#
# See top-level LICENSE file for details.
# =============================================================================

include(FindPackageHandleStandardArgs)
find_package(FindPkgConfig QUIET)

find_path(
  Random123_PROJ
  NAMES LICENSE
  PATHS "${CORENEURON_PROJECT_SOURCE_DIR}/external/Random123"
  NO_SYSTEM_ENVIRONMENT_PATH)

find_package_handle_standard_args(Random123 REQUIRED_VARS Random123_PROJ)

if(NOT Random123_FOUND)
  find_package(Git 1.8.3 QUIET)
  if(NOT ${GIT_FOUND})
    message(FATAL_ERROR "git not found, clone repository with --recursive")
  endif()
  message(STATUS "Sub-module Random123 missing: running git submodule update --init --recursive")
  execute_process(
    COMMAND
      ${GIT_EXECUTABLE} submodule update --init --recursive --
      ${CORENEURON_PROJECT_SOURCE_DIR}/external/Random123
    WORKING_DIRECTORY ${CORENEURON_PROJECT_SOURCE_DIR})
endif()
