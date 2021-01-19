# =============================================================================
# Copyright (C) 2016-2021 Blue Brain Project
#
# See top-level LICENSE file for details.
# =============================================================================

if(IS_DIRECTORY "/opt/cray")
  set(CRAY_SYSTEM TRUE)
endif()

if(CRAY_SYSTEM)
  # default build type is static for cray
  if(NOT DEFINED COMPILE_LIBRARY_TYPE)
    set(COMPILE_LIBRARY_TYPE "STATIC")
  endif()

  # Cray wrapper take care of everything!
  set(MPI_LIBRARIES "")
  set(MPI_C_LIBRARIES "")
  set(MPI_CXX_LIBRARIES "")

  # ~~~
  # instead of -rdynamic, cray wrapper needs either -dynamic or -static(default)
  # also cray compiler needs fPIC flag
  # ~~~
  if(COMPILE_LIBRARY_TYPE STREQUAL "SHARED")
    set(CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS "-dynamic")
    # TODO: add Cray compiler flag configurations in CompilerFlagsHelpers.cmake
    if(CMAKE_C_COMPILER_IS_CRAY)
      set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC")
      set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")
    endif()

  else()
    set(CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS "")
  endif()
else()
  # default is shared library
  if(NOT DEFINED COMPILE_LIBRARY_TYPE)
    set(COMPILE_LIBRARY_TYPE "SHARED")
  endif()
endif()
