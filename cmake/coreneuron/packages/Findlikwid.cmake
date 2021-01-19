# =============================================================================
# Copyright (C) 2016-2021 Blue Brain Project
#
# See top-level LICENSE file for details.
# =============================================================================

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
#   set(LIKWID_DIR "" CACHE PATH "Path likwid performance monitoring and benchmarking suite")
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

find_path(likwid_INCLUDE_DIRS "likwid.h" HINTS "${LIKWID_DIR}/include")
find_library(likwid_LIBRARIES likwid HINTS "${LIKWID_DIR}/lib")

# Checks 'REQUIRED', 'QUIET' and versions.
include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(likwid
    REQUIRED_VARS likwid_INCLUDE_DIRS likwid_LIBRARIES)
