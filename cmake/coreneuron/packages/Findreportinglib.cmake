# =============================================================================
# Copyright (C) 2016-2021 Blue Brain Project
#
# See top-level LICENSE file for details.
# =============================================================================

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

# UNIX paths are standard, no need to write.
find_path(reportinglib_INCLUDE_DIR reportinglib/Report.h)
find_library(reportinglib_LIBRARY reportinglib)
get_filename_component(reportinglib_LIB_DIR ${reportinglib_LIBRARY} DIRECTORY)
find_program (reportinglib_somaDump somaDump ${reportinglib_LIB_DIR}/../bin)

# Checks 'REQUIRED', 'QUIET' and versions.
include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(reportinglib
  FOUND_VAR reportinglib_FOUND
  REQUIRED_VARS reportinglib_INCLUDE_DIR reportinglib_LIBRARY reportinglib_LIB_DIR)

