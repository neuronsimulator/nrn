# =============================================================================
# Copyright (C) 2016-2021 Blue Brain Project
#
# See top-level LICENSE file for details.
# =============================================================================

# Helper to parse X.Y[.{anything] into X.Y
function(cnrn_parse_version FULL_VERSION)
  cmake_parse_arguments(PARSE_ARGV 1 CNRN_PARSE_VERSION "" "OUTPUT_MAJOR_MINOR" "")
  if(NOT "${CNRN_PARSE_VERSION_UNPARSED_ARGUMENTS}" STREQUAL "")
    message(
      FATAL_ERROR
        "cnrn_parse_version got unexpected arguments: ${CNRN_PARSE_VERSION_UNPARSED_ARGUMENTS}")
  endif()
  string(FIND ${FULL_VERSION} . first_dot)
  math(EXPR first_dot_plus_one "${first_dot}+1")
  string(SUBSTRING ${FULL_VERSION} ${first_dot_plus_one} -1 minor_and_later)
  string(FIND ${minor_and_later} . second_dot_relative)
  if(${first_dot} EQUAL -1 OR ${second_dot_relative} EQUAL -1)
    message(FATAL_ERROR "Failed to parse major.minor from ${FULL_VERSION}")
  endif()
  math(EXPR second_dot_plus_one "${first_dot}+${second_dot_relative}+1")
  string(SUBSTRING ${FULL_VERSION} 0 ${second_dot_plus_one} major_minor)
  set(${CNRN_PARSE_VERSION_OUTPUT_MAJOR_MINOR}
      ${major_minor}
      PARENT_SCOPE)
endfunction()

# =============================================================================
# Prepare compiler flags for GPU target
# =============================================================================
if(CORENRN_ENABLE_GPU)
  # Enable cudaProfiler{Start,Stop}() behind the Instrumentor::phase... APIs
  list(APPEND CORENRN_COMPILE_DEFS CORENEURON_CUDA_PROFILING CORENEURON_ENABLE_GPU)
  # Plain C++ code in CoreNEURON may need to use CUDA runtime APIs for, for example, starting and
  # stopping profiling. This makes sure those headers can be found.
  include_directories(${CMAKE_CUDA_TOOLKIT_INCLUDE_DIRECTORIES})
  # cuda unified memory support
  if(CORENRN_ENABLE_CUDA_UNIFIED_MEMORY)
    list(APPEND CORENRN_COMPILE_DEFS CORENEURON_UNIFIED_MEMORY)
  endif()
  if(${CMAKE_VERSION} VERSION_LESS 3.17)
    # Hopefully we can drop this soon. Parse ${CMAKE_CUDA_COMPILER_VERSION} into a shorter X.Y
    # version without any patch version.
    if(NOT ${CMAKE_CUDA_COMPILER_ID} STREQUAL "NVIDIA")
      message(FATAL_ERROR "Unsupported CUDA compiler ${CMAKE_CUDA_COMPILER_ID}")
    endif()
    cnrn_parse_version(${CMAKE_CUDA_COMPILER_VERSION} OUTPUT_MAJOR_MINOR CORENRN_CUDA_VERSION_SHORT)
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
  # -cuda links CUDA libraries and also seems to be important to make the NVHPC do the device code
  # linking. Without this, we had problems with linking between the explicit CUDA (.cu) device code
  # and offloaded OpenACC/OpenMP code. Using -cuda when compiling seems to improve error messages in
  # some cases, and to be recommended by NVIDIA. We pass -gpu=cudaX.Y to ensure that OpenACC/OpenMP
  # code is compiled with the same CUDA version as the explicit CUDA code.
  set(NVHPC_ACC_COMP_FLAGS "-cuda -gpu=cuda${CORENRN_CUDA_VERSION_SHORT}")
  # Combining -gpu=lineinfo with -O0 -g gives a warning: Conflicting options --device-debug and
  # --generate-line-info specified, ignoring --generate-line-info option
  if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    string(APPEND NVHPC_ACC_COMP_FLAGS ",debug")
  else()
    string(APPEND NVHPC_ACC_COMP_FLAGS ",lineinfo")
  endif()
  # Make sure that OpenACC code is generated for the same compute capabilities as the explicit CUDA
  # code. Otherwise there may be confusing linker errors. We cannot rely on nvcc and nvc++ using the
  # same default compute capabilities as each other, particularly on GPU-less build machines.
  foreach(compute_capability ${CMAKE_CUDA_ARCHITECTURES})
    string(APPEND NVHPC_ACC_COMP_FLAGS ",cc${compute_capability}")
  endforeach()
  if(CORENRN_ACCELERATOR_OFFLOAD STREQUAL "OpenMP")
    # Enable OpenMP target offload to GPU and if both OpenACC and OpenMP directives are available
    # for a region then prefer OpenMP.
    list(APPEND CORENRN_COMPILE_DEFS CORENEURON_PREFER_OPENMP_OFFLOAD)
    string(APPEND NVHPC_ACC_COMP_FLAGS " -mp=gpu")
  elseif(CORENRN_ACCELERATOR_OFFLOAD STREQUAL "OpenACC")
    # Only enable OpenACC offload for GPU
    string(APPEND NVHPC_ACC_COMP_FLAGS " -acc")
  else()
    message(FATAL_ERROR "${CORENRN_ACCELERATOR_OFFLOAD} not supported with NVHPC compilers")
  endif()
  string(APPEND CMAKE_EXE_LINKER_FLAGS " ${NVHPC_ACC_COMP_FLAGS}")
  # Use `-Mautoinline` option to compile .cpp files generated from .mod files only. This is
  # especially needed when we compile with -O0 or -O1 optimisation level where we get link errors.
  # Use of `-Mautoinline` ensure that the necessary functions like `net_receive_kernel` are inlined
  # for OpenACC code generation.
  set(NVHPC_CXX_INLINE_FLAGS "-Mautoinline")
endif()

# =============================================================================
# Initialise global properties that will be used by NEURON to link with CoreNEURON
# =============================================================================
if(CORENRN_ENABLE_GPU)
  # CORENRN_LIB_LINK_FLAGS is the full set of flags needed to link against libcorenrnmech.so:
  # something like `-acc -lcorenrnmech ...`. CORENRN_NEURON_LINK_FLAGS only contains flags that need
  # to be used when linking the NEURON Python module to make sure it is able to dynamically load
  # libcorenrnmech.so.
  set_property(GLOBAL PROPERTY CORENRN_LIB_LINK_FLAGS "${NVHPC_ACC_COMP_FLAGS}")
  if(CORENRN_ENABLE_SHARED)
    # Because of
    # https://forums.developer.nvidia.com/t/dynamically-loading-an-openacc-enabled-shared-library-from-an-executable-compiled-with-nvc-does-not-work/210968
    # we have to tell NEURON to pass OpenACC flags when linking special, otherwise we end up with an
    # `nrniv` binary that cannot dynamically load CoreNEURON in shared-library builds.
    set_property(GLOBAL PROPERTY CORENRN_NEURON_LINK_FLAGS "${NVHPC_ACC_COMP_FLAGS}")
  endif()
endif()

# NEURON needs to have access to this when CoreNEURON is built as a submodule. If CoreNEURON is
# installed externally then this is set via coreneuron-config.cmake
set_property(GLOBAL PROPERTY CORENRN_ENABLE_SHARED ${CORENRN_ENABLE_SHARED})

if(CORENRN_HAVE_NVHPC_COMPILER)
  if(${CMAKE_CXX_COMPILER_VERSION} VERSION_GREATER_EQUAL 20.7)
    # https://forums.developer.nvidia.com/t/many-all-diagnostic-numbers-increased-by-1-from-previous-values/146268/3
    # changed the numbering scheme in newer versions. The following list is from a clean start 13
    # August 2021. It would clearly be nicer to apply these suppressions only to relevant files.
    # Examples of the suppressed warnings are given below.
    # ~~~
    # "include/Random123/array.h", warning #111-D: statement is unreachable
    # "include/Random123/features/sse.h", warning #550-D: variable "edx" was set but never used
    # ~~~
    set(CORENEURON_CXX_WARNING_SUPPRESSIONS --diag_suppress=111,550)
    # Extra suppressions for .cpp files translated from .mod files.
    # ~~~
    # "x86_64/corenrn/mod2c/pattern.cpp", warning #161-D: unrecognized #pragma
    # "x86_64/corenrn/mod2c/svclmp.cpp", warning #177-D: variable "..." was declared but never referenced
    # ~~~
    string(JOIN " " CORENEURON_TRANSLATED_CODE_COMPILE_FLAGS ${CORENEURON_CXX_WARNING_SUPPRESSIONS}
           --diag_suppress=161,177)
  endif()
endif()
