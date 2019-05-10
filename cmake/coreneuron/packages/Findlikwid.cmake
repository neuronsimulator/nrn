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


# Findlikwid
# -------------
#
# Find likwid
#
# Find the likwid RRZE Performance Monitoring and Benchmarking Suite
#
# Using likwid:
#
# ::
#   set(LIKWID_ROOT "" CACHE PATH "Path likwid performance monitoring and benchmarking suite")
#   find_package(likwid REQUIRED)
#   include_directories(${likwid_INCLUDE_DIRS})
#   target_link_libraries(foo ${likwid_LIBRARIES})
#
# This module sets the following variables:
#
# ::
#
#   likwid_FOUND     - set to true if the library is found
#   likwid_INCLUDE   - list of required include directories
#   likwid_LIBRARIES - list of required library directories

find_path(likwid_INCLUDE_DIRS "likwid.h" HINTS "${LIKWID_ROOT}/include")
find_library(likwid_LIBRARIES likwid HINTS "${LIKWID_ROOT}/lib")

# Checks 'REQUIRED', 'QUIET' and versions.
include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(likwid
    REQUIRED_VARS likwid_INCLUDE_DIRS likwid_LIBRARIES)
