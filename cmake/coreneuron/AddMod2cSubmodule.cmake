# =============================================================================
# Copyright (C) 2016-2021 Blue Brain Project
#
# See top-level LICENSE file for details.
# =============================================================================

find_package(FindPkgConfig QUIET)

find_path(
  MOD2C_PROJ
  NAMES CMakeLists.txt
  PATHS "${CORENEURON_PROJECT_SOURCE_DIR}/external/mod2c")

find_package_handle_standard_args(MOD2C REQUIRED_VARS MOD2C_PROJ)

if(NOT MOD2C_FOUND)
  find_package(Git 1.8.3 QUIET)
  if(NOT ${GIT_FOUND})
    message(FATAL_ERROR "git not found, clone repository with --recursive")
  endif()
  message(STATUS "Sub-module mod2c missing : running git submodule update --init --recursive")
  execute_process(
    COMMAND
      ${GIT_EXECUTABLE} submodule update --init --recursive --
      ${CORENEURON_PROJECT_SOURCE_DIR}/external/mod2c
    WORKING_DIRECTORY ${CORENEURON_PROJECT_SOURCE_DIR})
else()
  message(STATUS "Using mod2c submodule from ${MOD2C_PROJ}")
endif()

add_subdirectory(${CORENEURON_PROJECT_SOURCE_DIR}/external/mod2c)
