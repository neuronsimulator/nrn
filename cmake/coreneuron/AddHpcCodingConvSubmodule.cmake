# =============================================================================
# Copyright (C) 2016-2021 Blue Brain Project
#
# See top-level LICENSE file for details.
# =============================================================================

include(FindPackageHandleStandardArgs)
find_package(FindPkgConfig QUIET)

find_path(
    HpcCodingConv_PROJ
  NAMES setup.cfg
  PATHS "${CORENEURON_PROJECT_SOURCE_DIR}/CMake/hpc-coding-conventions/")

find_package_handle_standard_args(HpcCodingConv REQUIRED_VARS HpcCodingConv_PROJ)

if(NOT HpcCodingConv_FOUND)
  find_package(Git 1.8.3 QUIET)
  if(NOT ${GIT_FOUND})
    message(FATAL_ERROR "git not found, clone repository with --recursive")
  endif()
  message(STATUS "Sub-module CMake/hpc-coding-conventions missing: running git submodule update --init")
  execute_process(
    COMMAND
      ${GIT_EXECUTABLE} submodule update --init --
      ${CORENEURON_PROJECT_SOURCE_DIR}/CMake/hpc-coding-conventions
    WORKING_DIRECTORY ${CORENEURON_PROJECT_SOURCE_DIR})
endif()
