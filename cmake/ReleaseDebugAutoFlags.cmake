# =============================================================================
# Helper for different build types
# =============================================================================

# default configuration
if(NOT CMAKE_BUILD_TYPE AND (NOT CMAKE_CONFIGURATION_TYPES))
  set(CMAKE_BUILD_TYPE
      ${CMAKE_BUILD_TYPE_DEFAULT}
      CACHE STRING "Empty or one of ${allowableBuildTypes}." FORCE)
  message(STATUS "Setting build type to '${CMAKE_BUILD_TYPE}' as none was specified.")
elseif(NOT CMAKE_BUILD_TYPE IN_LIST allowableBuildTypes)
  message(
    FATAL_ERROR "Invalid build type: ${CMAKE_BUILD_TYPE} : Must be one of ${allowableBuildTypes}")
endif()

# ~~~
# Different configuration types:
#
# Custom  : User specify flags with CMAKE_C_FLAGS and CMAKE_CXX_FLAGS
# Debug   : Optimized for debugging, include debug symbols
# Release : Release mode, no debuginfo
# RelWithDebInfo : Distribution mode, basic optimizations for portable code with debuginfos
# Fast : Maximum level of optimization. Target native architecture, not portable code
# ~~~

include(CompilerFlagsHelpers)

# ~~~
set(CMAKE_C_FLAGS_DEBUG
    "${CMAKE_C_DEBUGINFO_FLAGS}  ${CMAKE_C_OPT_NONE} ${CMAKE_C_STACK_PROTECTION} ${CMAKE_C_IGNORE_WARNINGS}")
set(CMAKE_CXX_FLAGS_DEBUG
    "${CMAKE_CXX_DEBUGINFO_FLAGS}  ${CMAKE_CXX_OPT_NONE} ${CMAKE_CXX_STACK_PROTECTION} ${CMAKE_CXX_IGNORE_WARNINGS}")

set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_OPT_NORMAL} ${CMAKE_C_IGNORE_WARNINGS}")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_OPT_NORMAL} ${CMAKE_CXX_IGNORE_WARNINGS}")

set(CMAKE_C_FLAGS_RELWITHDEBINFO "${CMAKE_C_DEBUGINFO_FLAGS}  ${CMAKE_C_OPT_NORMAL} ${CMAKE_C_IGNORE_WARNINGS}")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_DEBUGINFO_FLAGS}  ${CMAKE_CXX_OPT_NORMAL} ${CMAKE_CXX_IGNORE_WARNINGS}")

set(CMAKE_C_FLAGS_FAST "${CMAKE_C_OPT_FAST} ${CMAKE_C_LINK_TIME_OPT} ${CMAKE_C_GEN_NATIVE} ${CMAKE_C_IGNORE_WARNINGS}")
set(CMAKE_CXX_FLAGS_FAST "${CMAKE_CXX_OPT_FAST} ${CMAKE_CXX_LINK_TIME_OPT} ${CMAKE_CXX_GEN_NATIVE} ${CMAKE_CXX_IGNORE_WARNINGS}")
# ~~~
