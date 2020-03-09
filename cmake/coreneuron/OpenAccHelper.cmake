# =============================================================================
# Copyright (C) 2016-2019 Blue Brain Project
#
# See top-level LICENSE file for details.
# =============================================================================

if(CORENRN_ENABLE_GPU)

  set(COMPILE_LIBRARY_TYPE "STATIC")

  if(CORENRN_ENABLE_CUDA_UNIFIED_MEMORY)
    add_definitions(-DUNIFIED_MEMORY)
  endif()

  if(NOT CUDA_HOST_COMPILER)
    find_program(GCC_BIN gcc)
    set(CUDA_HOST_COMPILER ${GCC_BIN} CACHE FILEPATH "" FORCE)
  endif()

  if(${CMAKE_C_COMPILER_ID} STREQUAL "PGI")
    add_definitions(-DPG_ACC_BUGS)
    set(ACC_FLAGS "-acc")
    set(PGI_DIAG_FLAGS "--diag_suppress 177")
    set(PGI_INLINE_FLAGS "-Minline=size:200,levels:10")
    set(CMAKE_C_FLAGS "${ACC_FLAGS} ${CMAKE_C_FLAGS}")
    set(CMAKE_CXX_FLAGS "${ACC_FLAGS} ${CMAKE_CXX_FLAGS} ${PGI_DIAG_FLAGS}")
    set(CMAKE_CXX11_STANDARD_COMPILE_OPTION  --c++11)
    set(CMAKE_CXX14_STANDARD_COMPILE_OPTION  --c++14)
  else()
    message(WARNING "Non-PGI compiler : make sure to add required compiler flags to enable OpenACC")
  endif()

  if(POLICY CMP0074)
    cmake_policy(SET CMP0074 NEW)
  endif()
  find_package(CUDA 5.0 REQUIRED)
  set(CUDA_SEPARABLE_COMPILATION ON)
  set(CUDA_PROPAGATE_HOST_FLAGS OFF)

  add_definitions(-DCUDA_PROFILING)
else(CORENRN_ENABLE_GPU)
  # OpenACC pragmas are not guarded, disable all unknown pragm warnings
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${IGNORE_UNKNOWN_PRAGMA_FLAGS}")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${IGNORE_UNKNOWN_PRAGMA_FLAGS}")
endif(CORENRN_ENABLE_GPU)
