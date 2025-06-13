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
# FastDebug: Similar to Debug with a bit higher level optimisations (-O1) and other compiler
#            flags so that it's faster than -O0 but still produces consistent results for
#            testing and debugging purposes.
# ~~~

include(CompilerFlagsHelpers)

# ~~~
set(CMAKE_C_FLAGS_DEBUG
    "${CMAKE_C_DEBUGINFO_FLAGS}  ${CMAKE_C_OPT_NONE} ${CMAKE_C_STACK_PROTECTION} ${CMAKE_C_IGNORE_WARNINGS}"
)
set(CMAKE_CXX_FLAGS_DEBUG
    "${CMAKE_CXX_DEBUGINFO_FLAGS}  ${CMAKE_CXX_OPT_NONE} ${CMAKE_CXX_STACK_PROTECTION} ${CMAKE_CXX_IGNORE_WARNINGS}"
)

set(C_UNSAFE_MATH_FLAGS "")
set(CXX_UNSAFE_MATH_FLAGS "")
if(NRN_ENABLE_MATH_OPT)
  set(C_UNSAFE_MATH_FLAGS ${CMAKE_C_UNSAFE_MATH})
  set(CXX_UNSAFE_MATH_FLAGS ${CMAKE_CXX_UNSAFE_MATH})
endif()

set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_OPT_NORMAL} ${CMAKE_C_IGNORE_WARNINGS} ${C_UNSAFE_MATH_FLAGS}")
set(CMAKE_CXX_FLAGS_RELEASE
    "${CMAKE_CXX_OPT_NORMAL} ${CMAKE_CXX_IGNORE_WARNINGS} ${CXX_UNSAFE_MATH_FLAGS}")

set(CMAKE_C_FLAGS_RELWITHDEBINFO
    "${CMAKE_C_DEBUGINFO_FLAGS}  ${CMAKE_C_OPT_NORMAL} ${CMAKE_C_IGNORE_WARNINGS} ${C_UNSAFE_MATH_FLAGS}"
)
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO
    "${CMAKE_CXX_DEBUGINFO_FLAGS}  ${CMAKE_CXX_OPT_NORMAL} ${CMAKE_CXX_IGNORE_WARNINGS} ${CXX_UNSAFE_MATH_FLAGS}"
)

set(CMAKE_C_FLAGS_FAST
    "${CMAKE_C_OPT_FAST} ${CMAKE_C_LINK_TIME_OPT} ${CMAKE_C_GEN_NATIVE} ${CMAKE_C_IGNORE_WARNINGS}")
set(CMAKE_CXX_FLAGS_FAST
    "${CMAKE_CXX_OPT_FAST} ${CMAKE_CXX_LINK_TIME_OPT} ${CMAKE_CXX_GEN_NATIVE} ${CMAKE_CXX_IGNORE_WARNINGS}"
)

set(CMAKE_C_FLAGS_FASTDEBUG "${CMAKE_C_OPT_FASTDEBUG} ${CMAKE_C_IGNORE_WARNINGS}")
set(CMAKE_CXX_FLAGS_FASTDEBUG "${CMAKE_CXX_OPT_FASTDEBUG} ${CMAKE_CXX_IGNORE_WARNINGS}")
# ~~~

# for binary distributions, avoid addition of OpenMP specific flag as compiler on end-user machine
# may not support it.
if(NOT DEFINED NRN_BINARY_DIST_BUILD OR NOT NRN_BINARY_DIST_BUILD)
  include(CheckCXXCompilerFlag)
  # Check support for OpenMP SIMD constructs
  set(SIMD_FLAGS "")

  if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Intel")
    set(SIMD_FLAGS "-qopenmp-simd")
  elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
    set(SIMD_FLAGS "-openmp:experimental")
  elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    set(SIMD_FLAGS "-openmp-simd")
  else() # not ICC, MSVC, or Clang => GCC and others
    set(SIMD_FLAGS "-fopenmp-simd")
  endif()

  check_cxx_compiler_flag("${SIMD_FLAGS}" COMPILER_SUPPORT_SIMD)
  if(COMPILER_SUPPORT_SIMD)
    set(CMAKE_C_FLAGS "${CMAKE_CXX_FLAGS} ${SIMD_FLAGS}")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${SIMD_FLAGS}")
  else()
    message(STATUS "The compiler ${CMAKE_CXX_COMPILER} has no support for OpenMP SIMD construct")
  endif()
endif()
