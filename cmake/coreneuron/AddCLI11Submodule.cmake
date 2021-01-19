# =============================================================================
# Copyright (C) 2016-2021 Blue Brain Project
#
# See top-level LICENSE file for details.
# =============================================================================

include(FindPackageHandleStandardArgs)
find_package(FindPkgConfig QUIET)

find_path(
  CLI11_PROJ
  NAMES CMakeLists.txt
  PATHS "${CORENEURON_PROJECT_SOURCE_DIR}/external/CLI11")

find_package_handle_standard_args(CLI11 REQUIRED_VARS CLI11_PROJ)

if(NOT CLI11_FOUND)
  find_package(Git 1.8.3 QUIET)
  if(NOT ${GIT_FOUND})
    message(FATAL_ERROR "git not found, download full repository or install git")
  endif()
  message(STATUS "Sub-module CLI11 missing: running git submodule update --init")
  execute_process(
    COMMAND
      ${GIT_EXECUTABLE} submodule update --init --
      ${CORENEURON_PROJECT_SOURCE_DIR}/external/CLI11
    WORKING_DIRECTORY ${CORENEURON_PROJECT_SOURCE_DIR})
endif()
