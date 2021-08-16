# =============================================================================
# Copyright (C) 2016-2021 Blue Brain Project
#
# See top-level LICENSE file for details.
# =============================================================================

# =============================================================================
# Prepare compiler flags for GPU target
# =============================================================================
if(CORENRN_ENABLE_GPU)
  # Enable cudaProfiler{Start,Stop}() behind the Instrumentor::phase... APIs
  add_compile_definitions(CORENEURON_CUDA_PROFILING CORENEURON_ENABLE_GPU)
  # cuda unified memory support
  if(CORENRN_ENABLE_CUDA_UNIFIED_MEMORY)
    add_compile_definitions(CORENEURON_UNIFIED_MEMORY)
  endif()
  if(${CMAKE_VERSION} VERSION_LESS 3.17)
    # Hopefully we can drop this soon. Parse ${CMAKE_CUDA_COMPILER_VERSION} into a shorter X.Y
    # version without any patch version.
    if(NOT ${CMAKE_CUDA_COMPILER_ID} STREQUAL "NVIDIA")
      message(FATAL_ERROR "Unsupported CUDA compiler ${CMAKE_CUDA_COMPILER_ID}")
    endif()
    # Parse CMAKE_CUDA_COMPILER_VERSION=x.y.z into CUDA_VERSION=x.y
    string(FIND ${CMAKE_CUDA_COMPILER_VERSION} . first_dot)
    math(EXPR first_dot_plus_one "${first_dot}+1")
    string(SUBSTRING ${CMAKE_CUDA_COMPILER_VERSION} ${first_dot_plus_one} -1 minor_and_later)
    string(FIND ${minor_and_later} . second_dot_relative)
    if(${first_dot} EQUAL -1 OR ${second_dot_relative} EQUAL -1)
      message(
        FATAL_ERROR
          "Failed to parse a CUDA_VERSION from CMAKE_CUDA_COMPILER_VERSION=${CMAKE_CUDA_COMPILER_VERSION}"
      )
    endif()
    math(EXPR second_dot_plus_one "${first_dot}+${second_dot_relative}+1")
    string(SUBSTRING ${CMAKE_CUDA_COMPILER_VERSION} 0 ${second_dot_plus_one}
                     CORENRN_CUDA_VERSION_SHORT)
  else()
    # This is a lazy way of getting the major/minor versions separately without parsing
    # ${CMAKE_CUDA_COMPILER_VERSION}
    find_package(CUDAToolkit 9.0 REQUIRED)
    # Be a bit paranoid
    if(NOT ${CMAKE_CUDA_COMPILER_VERSION} STREQUAL ${CUDAToolkit_VERSION})
      message(
        FATAL_ERROR
          "CUDA compiler (${CMAKE_CUDA_COMPILER_VERSION}) and toolkit (${CUDAToolkit_VERSION}) versions are not the same!"
      )
    endif()
    set(CORENRN_CUDA_VERSION_SHORT "${CUDAToolkit_VERSION_MAJOR}.${CUDAToolkit_VERSION_MINOR}")
  endif()
  # -acc enables OpenACC support, -cuda links CUDA libraries and (very importantly!) seems to be
  # required to make the NVHPC compiler do the device code linking. Otherwise the explicit CUDA
  # device code (.cu files in libcoreneuron) has to be linked in a separate, earlier, step, which
  # apparently causes problems with interoperability with OpenACC. Passing -cuda to nvc++ when
  # compiling (as opposed to linking) seems to enable CUDA C++ support, which has other consequences
  # due to e.g. __CUDACC__ being defined. See https://github.com/BlueBrain/CoreNeuron/issues/607 for
  # more information about this. -gpu=cudaX.Y ensures that OpenACC code is compiled with the same
  # CUDA version as is used for the explicit CUDA code.
  set(NVHPC_ACC_COMP_FLAGS "-acc -gpu=cuda${CORENRN_CUDA_VERSION_SHORT}")
  set(NVHPC_ACC_LINK_FLAGS "-cuda")
  # Make sure that OpenACC code is generated for the same compute capabilities as the explicit CUDA
  # code. Otherwise there may be confusing linker errors. We cannot rely on nvcc and nvc++ using the
  # same default compute capabilities as each other, particularly on GPU-less build machines.
  foreach(compute_capability ${CMAKE_CUDA_ARCHITECTURES})
    string(APPEND NVHPC_ACC_COMP_FLAGS ",cc${compute_capability}")
  endforeach()
  # disable very verbose diagnosis messages and obvious warnings for mod2c
  set(PGI_DIAG_FLAGS "--diag_suppress 161,177,550")
  # avoid PGI adding standard compliant "-A" flags
  set(CMAKE_CXX11_STANDARD_COMPILE_OPTION --c++11)
  set(CMAKE_CXX14_STANDARD_COMPILE_OPTION --c++14)
  string(APPEND CMAKE_CXX_FLAGS " ${NVHPC_ACC_COMP_FLAGS} ${PGI_DIAG_FLAGS}")
  string(APPEND CMAKE_EXE_LINKER_FLAGS " ${NVHPC_ACC_LINK_FLAGS}")
endif()

# =============================================================================
# Set global property that will be used by NEURON to link with CoreNEURON
# =============================================================================
if(CORENRN_ENABLE_GPU)
  set_property(
    GLOBAL
    PROPERTY
      CORENEURON_LIB_LINK_FLAGS
      "${NVHPC_ACC_COMP_FLAGS} ${NVHPC_ACC_LINK_FLAGS} -rdynamic -lrt -Wl,--whole-archive -L${CMAKE_HOST_SYSTEM_PROCESSOR} -lcorenrnmech -L${CMAKE_INSTALL_PREFIX}/lib -lcoreneuron -Wl,--no-whole-archive"
  )
else()
  set_property(GLOBAL PROPERTY CORENEURON_LIB_LINK_FLAGS
                               "-L${CMAKE_HOST_SYSTEM_PROCESSOR} -lcorenrnmech")
endif(CORENRN_ENABLE_GPU)
