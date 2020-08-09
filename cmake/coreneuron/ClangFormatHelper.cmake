# =============================================================================
# Copyright (C) 2016-2019 Blue Brain Project
#
# See top-level LICENSE file for details.
# =============================================================================

set(CLANG_FORMAT_MIN_VERSION "4.0")
set(CLANG_FORMAT_MAX_VERSION "4.9")

find_package(ClangFormat)

if(CLANG_FORMAT_FOUND)
  message(INFO "clang-format ${CLANG_FORMAT_EXECUTABLE} (${CLANG_FORMAT_VERSION}) found")
endif()

if(CLANG_FORMAT_FOUND)
  file(COPY ${CORENEURON_PROJECT_SOURCE_DIR}/.clang-format DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

  # all source files under coreneuron directory
  file(GLOB_RECURSE SRC_FILES_FOR_CLANG_FORMAT
                    ${CORENEURON_PROJECT_SOURCE_DIR}/coreneuron/*.c
                    ${CORENEURON_PROJECT_SOURCE_DIR}/coreneuron/*.cpp
                    ${CORENEURON_PROJECT_SOURCE_DIR}/coreneuron/*.h*
                    ${CORENEURON_PROJECT_SOURCE_DIR}/coreneuron/*.ipp*)

  # exclude random123
  file(GLOB_RECURSE RANDOM123_FILES
                    ${CORENEURON_PROJECT_SOURCE_DIR}/coreneuron/utils/randoms/Random123/*.cpp
                    ${CORENEURON_PROJECT_SOURCE_DIR}/coreneuron/utils/randoms/Random123/*.h)

  foreach(R123_PATH ${RANDOM123_FILES})
    list(REMOVE_ITEM SRC_FILES_FOR_CLANG_FORMAT ${R123_PATH})
  endforeach(R123_PATH)

  add_custom_target(clang-format
                    COMMAND ${CMAKE_COMMAND} -DSOURCE_FILES:STRING="${SRC_FILES_FOR_CLANG_FORMAT}"
                            -DCLANG_FORMAT_EXECUTABLE=${CLANG_FORMAT_EXECUTABLE} -P
                            "${CORENEURON_PROJECT_SOURCE_DIR}/CMake/ClangFormatUtils.cmake")
endif()
