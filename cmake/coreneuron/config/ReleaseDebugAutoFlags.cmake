# =============================================================================
# Copyright (C) 2016-2021 Blue Brain Project
#
# See top-level LICENSE file for details.
# =============================================================================

# ~~~
# ReleaseDebugAutoFlags.cmake
# Release / Debug configuration helper
# ~~~

# default configuration
if(NOT CMAKE_BUILD_TYPE AND (NOT CMAKE_CONFIGURATION_TYPES))
  set(CMAKE_BUILD_TYPE
      RelWithDebInfo
      CACHE STRING "Choose the type of build." FORCE)
  message(STATUS "Setting build type to '${CMAKE_BUILD_TYPE}' as none was specified.")
endif()

# =============================================================================
# Different build types
# =============================================================================
# ~~~
# Debug : Optimized for debugging, include debug symbols
# Release : Release mode, no debuginfo
# RelWithDebInfo : Distribution mode, basic optimizations for potable code with debuginfos
# Fast : Maximum level of optimization. Target native architecture, not portable code
# ~~~

include(CompilerFlagsHelpers)

# ~~~
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_OPT_NORMAL}")
set(CMAKE_C_FLAGS_DEBUG
    "${CMAKE_C_DEBUGINFO_FLAGS}  ${CMAKE_C_OPT_NONE} ${CMAKE_C_STACK_PROTECTION}")
set(CMAKE_C_FLAGS_RELWITHDEBINFO "${CMAKE_C_DEBUGINFO_FLAGS}  ${CMAKE_C_OPT_NORMAL}")
set(CMAKE_C_FLAGS_FAST " ${CMAKE_C_OPT_FASTEST} ${CMAKE_C_LINK_TIME_OPT} ${CMAKE_C_GEN_NATIVE}")

set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_OPT_NORMAL}")
set(CMAKE_CXX_FLAGS_DEBUG
    "${CMAKE_CXX_DEBUGINFO_FLAGS}  ${CMAKE_CXX_OPT_NONE} ${CMAKE_CXX_STACK_PROTECTION}")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_DEBUGINFO_FLAGS}  ${CMAKE_CXX_OPT_NORMAL}")
set(CMAKE_CXX_FLAGS_FAST
    " ${CMAKE_CXX_OPT_FASTEST} ${CMAKE_CXX_LINK_TIME_OPT} ${CMAKE_CXX_GEN_NATIVE}")
# ~~~
