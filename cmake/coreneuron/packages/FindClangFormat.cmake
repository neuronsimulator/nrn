# Copyright (c) 2016, Blue Brain Project
# All rights reserved.

# Redistribution and use in source and binary forms, with or without modification,
# are permitted provided that the following conditions are met:
# 1. Redistributions of source code must retain the above copyright notice,
#    this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright notice,
#    this list of conditions and the following disclaimer in the documentation
#    and/or other materials provided with the distribution.
# 3. Neither the name of the copyright holder nor the names of its contributors
#    may be used to endorse or promote products derived from this software
#    without specific prior written permission.

# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
# LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
# THE POSSIBILITY OF SUCH DAMAGE.

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
