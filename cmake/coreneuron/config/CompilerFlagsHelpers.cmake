# =============================================================================
# Copyright (C) 2016-2021 Blue Brain Project
#
# See top-level LICENSE file for details.
# =============================================================================

# ~~~
# CompilerFlagsHelpers.cmake
# set of Convenience functions for portable compiler flags
# ~~~

set(SUPPORTED_COMPILER_LANGUAGE_LIST "C;CXX")

# detect compiler
foreach(COMPILER_LANGUAGE ${SUPPORTED_COMPILER_LANGUAGE_LIST})
  if(CMAKE_${COMPILER_LANGUAGE}_COMPILER_ID STREQUAL "XL")
    set(CMAKE_${COMPILER_LANGUAGE}_COMPILER_IS_XLC ON)
  elseif(CMAKE_${COMPILER_LANGUAGE}_COMPILER_ID STREQUAL "Intel")
    set(CMAKE_${COMPILER_LANGUAGE}_COMPILER_IS_ICC ON)
  elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
    set(CMAKE_${COMPILER_LANGUAGE}_COMPILER_IS_MSVC)
  elseif(${CMAKE_${COMPILER_LANGUAGE}_COMPILER_ID} STREQUAL "Clang")
    set(CMAKE_${COMPILER_LANGUAGE}_COMPILER_IS_CLANG ON)
  elseif(CMAKE_${COMPILER_LANGUAGE}_COMPILER_ID STREQUAL "GNU")
    set(CMAKE_${COMPILER_LANGUAGE}_COMPILER_IS_GCC ON)
  elseif(CMAKE_${COMPILER_LANGUAGE}_COMPILER_ID STREQUAL "Cray")
    set(CMAKE_${COMPILER_LANGUAGE}_COMPILER_IS_CRAY ON)
  endif()
endforeach()

foreach(COMPILER_LANGUAGE ${SUPPORTED_COMPILER_LANGUAGE_LIST})
  # XLC compiler
  if(CMAKE_${COMPILER_LANGUAGE}_COMPILER_IS_XLC)
    # ~~~
    # XLC -qinfo=all is awfully verbose on any platforms that use the GNU STL
    # Enable by default only the relevant one
    # ~~~
    set(CMAKE_${COMPILER_LANGUAGE}_WARNING_ALL "-qformat=all -qinfo=lan:trx:ret:zea:cmp:ret")

    set(CMAKE_${COMPILER_LANGUAGE}_DEBUGINFO_FLAGS "-g")

    set(CMAKE_${COMPILER_LANGUAGE}_OPT_NONE "-O0")
    set(CMAKE_${COMPILER_LANGUAGE}_OPT_NORMAL "-O2")
    set(CMAKE_${COMPILER_LANGUAGE}_OPT_AGGRESSIVE "-O3")
    set(CMAKE_${COMPILER_LANGUAGE}_OPT_FASTEST "-O5")

    set(CMAKE_${COMPILER_LANGUAGE}_STACK_PROTECTION "-qstackprotect")

    set(CMAKE_${COMPILER_LANGUAGE}_POSITION_INDEPENDENT "-qpic=small")

    set(CMAKE_${COMPILER_LANGUAGE}_VECTORIZE "-qhot")
    set(ADDITIONAL_THREADSAFE_FLAGS "-qthreaded")
    set(IGNORE_UNKNOWN_PRAGMA_FLAGS "-qsuppress=1506-224")

    # Microsoft compiler
  elseif(CMAKE_${COMPILER_LANGUAGE}_COMPILER_IS_MSVC)

    set(CMAKE_${COMPILER_LANGUAGE}_DEBUGINFO_FLAGS "-Zi")

    set(CMAKE_${COMPILER_LANGUAGE}_OPT_NONE "")
    set(CMAKE_${COMPILER_LANGUAGE}_OPT_NORMAL "-O2")
    set(CMAKE_${COMPILER_LANGUAGE}_OPT_AGGRESSIVE "-O2")
    set(CMAKE_${COMPILER_LANGUAGE}_OPT_FASTEST "-O2")

    set(CMAKE_${COMPILER_LANGUAGE}_STACK_PROTECTION "-GS")

    # enable by default on MSVC
    set(CMAKE_${COMPILER_LANGUAGE}_POSITION_INDEPENDENT "")

    # GCC
  elseif(CMAKE_${COMPILER_LANGUAGE}_COMPILER_IS_GCC)

    set(CMAKE_${COMPILER_LANGUAGE}_WARNING_ALL "-Wall")
    set(CMAKE_${COMPILER_LANGUAGE}_DEBUGINFO_FLAGS "-g")

    set(CMAKE_${COMPILER_LANGUAGE}_OPT_NONE "-O0")
    set(CMAKE_${COMPILER_LANGUAGE}_OPT_NORMAL "-O2")
    set(CMAKE_${COMPILER_LANGUAGE}_OPT_AGGRESSIVE "-O3")
    set(CMAKE_${COMPILER_LANGUAGE}_OPT_FASTEST "-Ofast -march=native")

    set(CMAKE_${COMPILER_LANGUAGE}_STACK_PROTECTION "-fstack-protector")

    set(CMAKE_${COMPILER_LANGUAGE}_POSITION_INDEPENDENT "-fPIC")

    set(CMAKE_${COMPILER_LANGUAGE}_VECTORIZE "-ftree-vectorize")
    set(IGNORE_UNKNOWN_PRAGMA_FLAGS "-Wno-unknown-pragmas")

    if(CMAKE_${COMPILER_LANGUAGE}_COMPILER_VERSION VERSION_GREATER "4.7.0")
      set(CMAKE_${COMPILER_LANGUAGE}_LINK_TIME_OPT "-flto")
    endif()

    if((CMAKE_HOST_SYSTEM_PROCESSOR MATCHES "^ppc") OR (CMAKE_HOST_SYSTEM_PROCESSOR MATCHES "^power"
                                                       ))
      # ppc arch do not support -march= syntax
      set(CMAKE_${COMPILER_LANGUAGE}_GEN_NATIVE "-mcpu=native")
    else()
      set(CMAKE_${COMPILER_LANGUAGE}_GEN_NATIVE "-march=native")
    endif()

    # CLANG
  elseif(CMAKE_${COMPILER_LANGUAGE}_COMPILER_IS_CLANG)
    # Force same ld behavior as when called from gcc --as-needed forces the linker to check whether
    # a dynamic library mentioned in the command line is actually needed by the objects being
    # linked. Symbols needed in shared objects are already linked when building that library.
    set(CMAKE_EXE_LINKER_FLAGS "-Wl,--as-needed")
    set(CMAKE_SHARED_LINKER_FLAGS "-Wl,--as-needed")

    # rest of the world
  else()
    set(CMAKE_${COMPILER_LANGUAGE}_WARNING_ALL "-Wall")
    set(CMAKE_${COMPILER_LANGUAGE}_DEBUGINFO_FLAGS "-g")

    set(CMAKE_${COMPILER_LANGUAGE}_OPT_NONE "-O0")
    set(CMAKE_${COMPILER_LANGUAGE}_OPT_NORMAL "-O2")
    set(CMAKE_${COMPILER_LANGUAGE}_OPT_AGGRESSIVE "-O3")
    set(CMAKE_${COMPILER_LANGUAGE}_OPT_FASTEST "-O3")

    set(CMAKE_${COMPILER_LANGUAGE}_STACK_PROTECTION "")
    set(CMAKE_${COMPILER_LANGUAGE}_POSITION_INDEPENDENT "-fPIC")
    set(CMAKE_${COMPILER_LANGUAGE}_VECTORIZE "")

    if(CMAKE_${COMPILER_LANGUAGE}_COMPILER_IS_ICC)
      # unknown compiler flags produce error on Cray and hence just set this for intel now
      set(IGNORE_UNKNOWN_PRAGMA_FLAGS "-Wno-unknown-pragmas")
      # Intel O3 is extreme
      set(CMAKE_${COMPILER_LANGUAGE}_OPT_AGGRESSIVE "-O2")
    endif()

    if(CMAKE_${COMPILER_LANGUAGE}_COMPILER_ID STREQUAL "PGI")
      set(CMAKE_${COMPILER_LANGUAGE}_WARNING_ALL "")
    endif()
  endif()

endforeach()

# ===============================================================================
# Allow undefined reference in shared library as mod files will be linked later
# ===============================================================================
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR ${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  set(UNDEFINED_SYMBOLS_IGNORE_FLAG "-undefined dynamic_lookup")
  string(APPEND CMAKE_SHARED_LIBRARY_CREATE_CXX_FLAGS " ${UNDEFINED_SYMBOLS_IGNORE_FLAG}")
  string(APPEND CMAKE_SHARED_LIBRARY_CREATE_C_FLAGS " ${UNDEFINED_SYMBOLS_IGNORE_FLAG}")
endif()
