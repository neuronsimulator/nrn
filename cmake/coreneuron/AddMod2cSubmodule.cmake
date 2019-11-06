# =============================================================================
# Copyright (C) 2016-2019 Blue Brain Project
#
# See top-level LICENSE file for details.
# =============================================================================

include(ExternalProject)

find_package(FindPkgConfig QUIET)
find_path(MOD2C_PROJ NAMES CMakeLists.txt PATHS "${CORENEURON_PROJECT_SOURCE_DIR}/external/mod2c")
find_package_handle_standard_args(MOD2C REQUIRED_VARS MOD2C_PROJ)

if(NOT MOD2C_FOUND)
  find_package(Git 1.8.3 QUIET)
  if(NOT ${GIT_FOUND})
    message(FATAL_ERROR "git not found, clone repository with --recursive")
  endif()
  message(STATUS "Sub-module mod2c missing : running git submodule update --init --recursive")
  execute_process(COMMAND ${GIT_EXECUTABLE} submodule update --init --recursive --
                          ${CORENEURON_PROJECT_SOURCE_DIR}/external/mod2c
                  WORKING_DIRECTORY ${CORENEURON_PROJECT_SOURCE_DIR})
else()
  message(STATUS "Using mod2c submodule from ${MOD2C_PROJ}")
endif()

set(ExternalProjectCMakeArgs
    -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
    -DCMAKE_INSTALL_PREFIX=${CMAKE_BINARY_DIR}
    -DCMAKE_C_COMPILER=${CORENRN_FRONTEND_C_COMPILER}
    -DCMAKE_CXX_COMPILER=${CORENRN_FRONTEND_CXX_COMPILER})

externalproject_add(mod2c
                    BUILD_ALWAYS
                    1
                    SOURCE_DIR
                    ${CORENEURON_PROJECT_SOURCE_DIR}/external/mod2c
                    GIT_SUBMODULES
                    CMAKE_ARGS
                    ${ExternalProjectCMakeArgs})
