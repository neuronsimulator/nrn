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


# Findreportinglib
# -------------
#
# Find reportinglib
#
# Find the reportinglib Blue Brain HPC utils library
#
# Using reportinglib:
#
# ::
#
#   find_package(reportinglib REQUIRED)
#   include_directories(${reportinglib_INCLUDE_DIRS})
#   target_link_libraries(foo ${reportinglib_LIBRARIES})
#
# This module sets the following variables:
#
# ::
#
#   reportinglib_FOUND - set to true if the library is found
#   reportinglib_INCLUDE_DIRS - list of required include directories
#   reportinglib_LIBRARIES - list of libraries to be linked

#=============================================================================
# Copyright 2015 Adrien Devresse <adrien.devresse@epfl.ch>
#
#=============================================================================
# (To distribute this file outside of CMake, substitute the full
#  License text for the above reference.)


# UNIX paths are standard, no need to write.
find_path(reportinglib_INCLUDE_DIR reportinglib/Report.h)

find_library(reportinglib_LIBRARY reportinglib)
get_filename_component(reportinglib_LIB_DIR ${reportinglib_LIBRARY} DIRECTORY)

# Checks 'REQUIRED', 'QUIET' and versions.
include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(reportinglib
  FOUND_VAR reportinglib_FOUND
  REQUIRED_VARS reportinglib_INCLUDE_DIR reportinglib_LIBRARY reportinglib_LIB_DIR)
  
