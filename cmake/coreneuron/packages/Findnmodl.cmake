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


# Findnmodl
# -------------
#
# Find nmodl
#
# Find the nmodl Blue Brain HPC utils library
#
# Using nmodl:
#
# ::
#   set(NMODL_ROOT "" CACHE PATH "Path nmodl source-to-source compiler root")
#   find_package(nmodl REQUIRED)
#   include_directories(${nmodl_INCLUDE_DIRS})
#   target_link_libraries(foo ${nmodl_LIBRARIES})
#
# This module sets the following variables:
#
# ::
#
#   nmodl_FOUND   - set to true if the library is found
#   nmodl_INCLUDE - list of required include directories
#   nmodl_BINARY  - the nmodl binary


# UNIX paths are standard, no need to write.
find_program(nmodl_BINARY NAMES nmodl
        HINTS "${NMODL_ROOT}/bin")

find_path(nmodl_INCLUDE "nmodl/fast_math.ispc" HINTS "${NMODL_ROOT}/include")
find_path(nmodl_PYTHONPATH "nmodl/__init__.py" HINTS "${NMODL_ROOT}/lib/python")

# Checks 'REQUIRED', 'QUIET' and versions.
include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(nmodl
  FOUND_VAR nmodl_FOUND
  REQUIRED_VARS nmodl_BINARY nmodl_INCLUDE nmodl_PYTHONPATH)
  
