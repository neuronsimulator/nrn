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
  if(${CMAKE_C_COMPILER_ID} STREQUAL "PGI")
    add_definitions(-DPG_ACC_BUGS)
    set(ACC_FLAGS "-acc -Minline=size:200,levels:10")
    set(CMAKE_C_FLAGS "${ACC_FLAGS} ${CMAKE_C_FLAGS}")
    set(CMAKE_CXX_FLAGS "${ACC_FLAGS} ${CMAKE_CXX_FLAGS}")
  else()
    message(WARNING "Non-PGI compiler : make sure to add required compiler flags to enable OpenACC")
  endif()

  find_package(CUDA 5.0 REQUIRED)
  set(CUDA_SEPARABLE_COMPILATION ON)
  add_definitions(-DCUDA_PROFILING)
else(CORENRN_ENABLE_GPU)
  # OpenACC pragmas are not guarded, disable all unknown pragm warnings
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${IGNORE_UNKNOWN_PRAGMA_FLAGS}")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${IGNORE_UNKNOWN_PRAGMA_FLAGS}")
endif(CORENRN_ENABLE_GPU)
