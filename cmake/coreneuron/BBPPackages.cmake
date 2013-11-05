
# Copyright (c) 2013, EPFL/Blue Brain Project
#                     Stefan.Eilemann@epfl.ch

# HOW-TO:
# This file contains the 'bbp-package' rule for any BBP software, currently
# limited to deb (Ubuntu) and rpm (RedHat) packages. This file must be included
# after the CPack configuration inside the CPackConfig.cmake to work correctly.
# Note that the release procedure is handled separately as this needs to be
# executed only on one machine contrary to the packaging. The file containing
# the release procedure rules is BBPRelease.cmake.
#
# Below variables must be set beforehand in order to get the task working.

if(APPLE OR MSVC)
  return()
endif()

if(CMAKE_VERSION VERSION_LESS 2.8.3)
  # WAR bug
  get_filename_component(CMAKE_CURRENT_LIST_DIR ${CMAKE_CURRENT_LIST_FILE} PATH)
  list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/2.8.3)
endif()

# ID/name of the used distribution, e.g. "Ubuntu"
include(${CMAKE_CURRENT_LIST_DIR}/oss/LSBInfo.cmake)
if(NOT LSB_DISTRIBUTOR_ID)
  message(FATAL_ERROR "Need LSB_DISTRIBUTOR_ID")
endif()

# will be set if CPack was configured before
if(NOT CPACK_PACKAGE_FILE_NAME)
  message(FATAL_ERROR "Need CPACK_PACKAGE_FILE_NAME")
endif()

set(PACKAGE_CODE)
set(PACKAGE_NAME)
set(PACKAGE_DIR)

if(LSB_DISTRIBUTOR_ID MATCHES "Ubuntu")
  if(NOT LSB_CODENAME)
    message(FATAL_ERROR "Need LSB_CODENAME")
  endif()
  set(PACKAGE_NAME ${CPACK_PACKAGE_FILE_NAME}.deb)
  set(PACKAGE_DIR ${CMAKE_SOURCE_DIR}/../BBPPackages/ubuntu/${LSB_CODENAME})
  set(PACKAGE_CODE ${LSB_CODENAME})
elseif(LSB_DISTRIBUTOR_ID MATCHES "RedHatEnterpriseServer")
  set(PACKAGE_NAME ${CPACK_PACKAGE_FILE_NAME}.rpm)
  set(PACKAGE_DIR ${CMAKE_SOURCE_DIR}/../BBPPackages/redhat)
  set(PACKAGE_CODE rhel)
else()
  message(STATUS "Unknown OS for package-upload")
endif()

if(PACKAGE_CODE AND NOT TARGET bbp-package-${PACKAGE_CODE})
  add_custom_target(bbp-package-copy-${PACKAGE_CODE}
    COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target package
    COMMAND ${CMAKE_COMMAND} -E make_directory ${PACKAGE_DIR}
    COMMAND ${CMAKE_COMMAND} -E copy ${PACKAGE_NAME} ${PACKAGE_DIR}
    COMMENT "Build and copy package to ${PACKAGE_DIR}"
    VERBATIM)

  add_custom_target(bbp-package-${PACKAGE_CODE}
    COMMAND ${CMAKE_COMMAND} -DCMAKE_SOURCE_DIR="${CMAKE_SOURCE_DIR}" -DCMAKE_CURRENT_BINARY_DIR="${CMAKE_CURRENT_BINARY_DIR}" -DCMAKE_PROJECT_NAME="${GIT_DOCUMENTATION_REPO}" -P ${CMAKE_CURRENT_LIST_DIR}/BBPPackagesGit.cmake
    COMMENT "Updating BBPPackages"
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/../BBPPackages"
    DEPENDS bbp-package-copy-${PACKAGE_CODE}
    )

  if(TARGET bbp-package)
    add_dependencies(bbp-package bbp-package-${PACKAGE_CODE})
  else()
    add_custom_target(bbp-package DEPENDS bbp-package-${PACKAGE_CODE})
  endif()
endif()
