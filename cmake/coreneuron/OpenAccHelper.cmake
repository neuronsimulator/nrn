# =============================================================================
# Copyright (C) 2016-2020 Blue Brain Project
#
# See top-level LICENSE file for details.
# =============================================================================

if(CORENRN_ENABLE_GPU)
  # cuda unified memory support
  if(CORENRN_ENABLE_CUDA_UNIFIED_MEMORY)
    add_definitions(-DUNIFIED_MEMORY)
  endif()

  # if user don't specify host compiler, use gcc from $PATH
  if(NOT CUDA_HOST_COMPILER)
    find_program(GCC_BIN gcc)
    set(CUDA_HOST_COMPILER
        ${GCC_BIN}
        CACHE FILEPATH "" FORCE)
  endif()

  # various flags for PGI compiler with GPU build
  if(${CMAKE_C_COMPILER_ID} STREQUAL "PGI")
    # workaround for old PGI version
    add_definitions(-DPG_ACC_BUGS)
    set(ACC_FLAGS "-acc")
    # disable very verbose diagnosis messages and obvious warnings for mod2c
    set(PGI_DIAG_FLAGS "--diag_suppress 161,177,550")
    # some of the mod files can have too many functions, increase inline level
    set(PGI_INLINE_FLAGS "-Minline=size:200,levels:10")
    # C/C++ compiler flags
    set(CMAKE_C_FLAGS "${ACC_FLAGS} ${CMAKE_C_FLAGS}")
    set(CMAKE_CXX_FLAGS "${ACC_FLAGS} ${CMAKE_CXX_FLAGS} ${PGI_DIAG_FLAGS}")
    # avoid PGI adding standard compliant "-A" flags
    set(CMAKE_CXX11_STANDARD_COMPILE_OPTION --c++11)
    set(CMAKE_CXX14_STANDARD_COMPILE_OPTION --c++14)
  else()
    message(FATAL_ERROR "GPU support is available via OpenACC using PGI/NVIDIA compilers."
                        " Use NVIDIA HPC SDK with -DCMAKE_C_COMPILER=nvc -DCMAKE_CXX_COMPILER=nvc++")
  endif()

  # set property for neuron to link with coreneuron libraries
  set_property(
    GLOBAL
    PROPERTY
      CORENEURON_LIB_LINK_FLAGS
      "-acc -rdynamic -lrt -Wl,--whole-archive -L${CMAKE_HOST_SYSTEM_PROCESSOR} -lcorenrnmech -L${CMAKE_INSTALL_PREFIX}/lib -lcoreneuron -lcudacoreneuron -Wl,--no-whole-archive ${CUDA_cudart_static_LIBRARY}"
  )

  # find_cuda produce verbose messages : use new behavior to use _ROOT variables
  if(POLICY CMP0074)
    cmake_policy(SET CMP0074 NEW)
  endif()
  find_package(CUDA 9.0 REQUIRED)
  set(CUDA_SEPARABLE_COMPILATION ON)
  set(CUDA_PROPAGATE_HOST_FLAGS OFF)
  add_definitions(-DCUDA_PROFILING)
else(CORENRN_ENABLE_GPU)
  # OpenACC pragmas are not guarded, disable all unknown pragm warnings
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${IGNORE_UNKNOWN_PRAGMA_FLAGS}")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${IGNORE_UNKNOWN_PRAGMA_FLAGS}")
  set_property(GLOBAL PROPERTY CORENEURON_LIB_LINK_FLAGS
                               "-L${CMAKE_HOST_SYSTEM_PROCESSOR} -lcorenrnmech")
endif(CORENRN_ENABLE_GPU)
