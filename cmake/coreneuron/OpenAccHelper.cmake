# =============================================================================
# Copyright (C) 2016-2020 Blue Brain Project
#
# See top-level LICENSE file for details.
# =============================================================================

# =============================================================================
# Prepare compiler flags for GPU target
# =============================================================================
if(CORENRN_ENABLE_GPU)

  # cuda unified memory support
  if(CORENRN_ENABLE_CUDA_UNIFIED_MEMORY)
    set(UNIFIED_MEMORY_DEF -DUNIFIED_MEMORY)
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
    set(PGI_ACC_FLAGS "-acc")
    # disable very verbose diagnosis messages and obvious warnings for mod2c
    set(PGI_DIAG_FLAGS "--diag_suppress 161,177,550")
    # inlining of large functions for OpenACC
    set(PGI_INLINE_FLAGS "-Minline=size:200,levels:10")

    # avoid PGI adding standard compliant "-A" flags
    set(CMAKE_CXX11_STANDARD_COMPILE_OPTION --c++11)
    set(CMAKE_CXX14_STANDARD_COMPILE_OPTION --c++14)

  else()
    message(FATAL_ERROR "GPU support is available via OpenACC using PGI/NVIDIA compilers."
                        " Use NVIDIA HPC SDK with -DCMAKE_C_COMPILER=nvc -DCMAKE_CXX_COMPILER=nvc++")
  endif()

  # find_cuda produce verbose messages : use new behavior to use _ROOT variables
  if(POLICY CMP0074)
    cmake_policy(SET CMP0074 NEW)
  endif()
  find_package(CUDA 9.0 REQUIRED)
  set(CUDA_SEPARABLE_COMPILATION ON)
  set(CUDA_PROPAGATE_HOST_FLAGS OFF)
  set(CUDA_PROFILING_DEF -DCUDA_PROFILING)

  set(CORENRN_ACC_GPU_DEFS "${UNIFIED_MEMORY_DEF} ${CUDA_PROFILING_DEF}")
  set(CORENRN_ACC_GPU_FLAGS "${PGI_ACC_FLAGS} ${PGI_DIAG_FLAGS} ${PGI_INLINE_FLAGS}")

  add_definitions(${CORENRN_ACC_GPU_DEFS})
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CORENRN_ACC_GPU_FLAGS}")
endif()

# =============================================================================
# Set global property that will be used by NEURON to link with CoreNEURON
# =============================================================================
if(CORENRN_ENABLE_GPU)
  set_property(
    GLOBAL
    PROPERTY
      CORENEURON_LIB_LINK_FLAGS
      "${PGI_ACC_FLAGS} -rdynamic -lrt -Wl,--whole-archive -L${CMAKE_HOST_SYSTEM_PROCESSOR} -lcorenrnmech -L${CMAKE_INSTALL_PREFIX}/lib -lcoreneuron -lcudacoreneuron -Wl,--no-whole-archive ${CUDA_cudart_static_LIBRARY}"
  )
else()
  set_property(GLOBAL PROPERTY CORENEURON_LIB_LINK_FLAGS
                               "-L${CMAKE_HOST_SYSTEM_PROCESSOR} -lcorenrnmech")
endif(CORENRN_ENABLE_GPU)
