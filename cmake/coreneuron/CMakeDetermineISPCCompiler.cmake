# =============================================================================
# Copyright (C) 2016-2021 Blue Brain Project
#
# See top-level LICENSE file for details.
# =============================================================================

# Find the ISPC compiler
find_program(
  CMAKE_ISPC_COMPILER
  NAMES $ENV{ISPC} ispc
  PATHS ENV PATH
  DOC "ISPC compiler")
mark_as_advanced(CMAKE_ISPC_COMPILER)

set(CMAKE_ISPC_SOURCE_FILE_EXTENSIONS ispc)
set(CMAKE_ISPC_OUTPUT_EXTENSION .obj)
set(CMAKE_ISPC_COMPILER_ENV_VAR "ISPC")

# Configure variables set in this file for fast reload later on
configure_file(${CMAKE_CURRENT_LIST_DIR}/CMakeISPCCompiler.cmake.in
               ${CMAKE_PLATFORM_INFO_DIR}/CMakeISPCCompiler.cmake)
