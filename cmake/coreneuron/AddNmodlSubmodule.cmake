# =============================================================================
# Copyright (C) 2016-2021 Blue Brain Project
#
# See top-level LICENSE file for details.
# =============================================================================

find_package(FindPkgConfig QUIET)

find_path(
  NMODL_PROJ
  NAMES CMakeLists.txt
  PATHS "${CORENEURON_PROJECT_SOURCE_DIR}/external/nmodl")

find_package_handle_standard_args(NMODL REQUIRED_VARS NMODL_PROJ)

if(NOT NMODL_FOUND)
  find_package(Git 1.8.3 QUIET)
  if(NOT ${GIT_FOUND})
    message(FATAL_ERROR "git not found, clone repository with --recursive")
  endif()
  message(STATUS "Sub-module nmodl missing : running git submodule update --init")
  execute_process(
    COMMAND
      ${GIT_EXECUTABLE} submodule update --init --
      ${CORENEURON_PROJECT_SOURCE_DIR}/external/nmodl
    WORKING_DIRECTORY ${CORENEURON_PROJECT_SOURCE_DIR})
else()
  message(STATUS "Using nmodl submodule from ${NMODL_PROJ}")
endif()

add_subdirectory(${CORENEURON_PROJECT_SOURCE_DIR}/external/nmodl)
