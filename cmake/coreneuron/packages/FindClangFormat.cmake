# =============================================================================
# Copyright (C) 2016-2019 Blue Brain Project
#
# See top-level LICENSE file for details.
# =============================================================================

string(REPLACE ":" ";" _PATH $ENV{PATH})
set(CLANG_FORMAT_FOUND OFF)
foreach(p ${_PATH})
    file(GLOB cand ${p}/clang-format ${p}/clang-format-[0-9].[0-9])
    foreach(candidate ${cand})
        set(CLANG_FORMAT_EXECUTABLE ${candidate})
        execute_process(COMMAND ${CLANG_FORMAT_EXECUTABLE} -version OUTPUT_VARIABLE clang_out )
        string(REGEX MATCH "version \([0-9]\\.[0-9]\)" VERSION_OUTPUT ${clang_out})
        set(CLANG_FORMAT_VERSION ${CMAKE_MATCH_1})
        if ((${CLANG_FORMAT_VERSION} VERSION_GREATER ${CLANG_FORMAT_MIN_VERSION}) OR (${CLANG_FORMAT_VERSION} VERSION_EQUAL ${CLANG_FORMAT_MIN_VERSION}))
            set (FIT_MIN_REQUIRED true)
        else ()
            set (FIT_MIN_REQUIRED false)
        endif()
        if ((${CLANG_FORMAT_VERSION} VERSION_LESS ${CLANG_FORMAT_MAX_VERSION}) OR (${CLANG_FORMAT_VERSION} VERSION_EQUAL ${CLANG_FORMAT_MAX_VERSION}))
            set (FIT_MAX_REQUIRED true)
        else ()
            set (FIT_MAX_REQUIRED false)
        endif()
        if ( ${FIT_MIN_REQUIRED} AND ${FIT_MAX_REQUIRED})
            set(CLANG_FORMAT_FOUND ON)
            break()
        endif()
    endforeach()
    if (${CLANG_FORMAT_FOUND})
        break()
    endif()
endforeach()
