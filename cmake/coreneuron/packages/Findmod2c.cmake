# Findmod2c
# -------------
#
# Find mod2c
#
# Find the mod2c Blue Brain HPC utils library
#
# Using mod2c:
#
# ::
#
#   find_package(mod2c REQUIRED)
#   include_directories(${mod2c_INCLUDE_DIRS})
#   target_link_libraries(foo ${mod2c_LIBRARIES})
#
# This module sets the following variables:
#
# ::
#
#   mod2c_FOUND - set to true if the library is found
#   mod2c_INCLUDE_DIRS - list of required include directories
#   mod2c_LIBRARIES - list of libraries to be linked

#=============================================================================
# Copyright 2015 Adrien Devresse <adrien.devresse@epfl.ch>
#
#=============================================================================
# (To distribute this file outside of CMake, substitute the full
#  License text for the above reference.)


# UNIX paths are standard, no need to write.
find_program(mod2c_BINARY
  NAMES mod2c_core
  PATHS "/usr/bin"
  )


# Checks 'REQUIRED', 'QUIET' and versions.
include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(mod2c
  FOUND_VAR mod2c_FOUND
  REQUIRED_VARS mod2c_BINARY)
  
