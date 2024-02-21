# ~~~
# It is a bad idea having floating point versions, since macros cant handle them
# We therefore, have version as an int, which is pretty much standard
#
# CORENEURON_VERSION_COMBINED is used to defined CORENEURON_VERSION in engine.h
# and this macros is used by some BBP mod file.
# ~~~
math(EXPR CORENEURON_VERSION_COMBINED "${NEURON_VERSION_MAJOR} * 100 + ${NEURON_VERSION_MINOR}")

# Regardless of whether we are being built as a submodule of NEURON, NRN_VERSION_{MAJOR,MINOR,PATCH}
# are now set to the version that we should claim compatibility with when compiling translated MOD
# files. Generate a header under a special `generated` prefix in the build directory, so that
# -I/path/to/src -I/path/to/build/generated is safe (headers from the source prefix are copied
# elsewhere under the build prefix, so there is scope for confusion) This is currently used when MOD
# files are translated on coreneuron side
configure_file(${PROJECT_SOURCE_DIR}/src/coreneuron/config/neuron_version.hpp.in
               ${PROJECT_BINARY_DIR}/generated/coreneuron/config/neuron_version.hpp)

# =============================================================================
# Include common cmake modules
# =============================================================================
include(CheckIncludeFiles)
include(CrayPortability)
include(SetRpath)
include(CTest)
include(GitRevision)
include(${CODING_CONV_CMAKE}/build-time-copy.cmake)

# =============================================================================
# Build options
# =============================================================================
option(CORENRN_ENABLE_OPENMP "Build the CORE NEURON with OpenMP implementation" ON)
option(CORENRN_ENABLE_OPENMP_OFFLOAD "Prefer OpenMP target offload to OpenACC" ON)
option(CORENRN_ENABLE_TIMEOUT "Enable nrn_timeout implementation" ON)
option(CORENRN_ENABLE_REPORTING "Enable use of libsonata for soma reports" OFF)
option(CORENRN_ENABLE_HOC_EXP "Enable wrapping exp with hoc_exp()" OFF)
option(CORENRN_ENABLE_SPLAYTREE_QUEUING "Enable use of Splay tree for spike queuing" ON)
option(CORENRN_ENABLE_NET_RECEIVE_BUFFER "Enable event buffering in net_receive function" ON)
option(CORENRN_ENABLE_CALIPER_PROFILING "Enable Caliper instrumentation" OFF)
option(CORENRN_ENABLE_LIKWID_PROFILING "Enable LIKWID instrumentation" OFF)
option(CORENRN_ENABLE_CUDA_UNIFIED_MEMORY "Enable CUDA unified memory support" OFF)
option(CORENRN_ENABLE_UNIT_TESTS "Enable unit tests execution" ON)
option(CORENRN_ENABLE_GPU "Enable GPU support using OpenACC or OpenMP" OFF)
option(CORENRN_ENABLE_SHARED "Enable shared library build" ON)
option(CORENRN_ENABLE_PRCELLSTATE "Enable NRN_PRCELLSTATE debug feature" OFF)

set(CORENRN_NMODL_DIR
    ""
    CACHE PATH "Path to nmodl source-to-source compiler installation")
set(LIKWID_DIR
    ""
    CACHE PATH "Path to likwid performance analysis suite")

# Older CMake versions label NVHPC as PGI, newer ones label it as NVHPC.
if(${CMAKE_CXX_COMPILER_ID} STREQUAL "PGI" OR ${CMAKE_CXX_COMPILER_ID} STREQUAL "NVHPC")
  set(CORENRN_HAVE_NVHPC_COMPILER ON)
else()
  set(CORENRN_HAVE_NVHPC_COMPILER OFF)
endif()

set(CORENRN_ACCELERATOR_OFFLOAD "Disabled")
if(CORENRN_ENABLE_GPU)
  # Older CMake versions than 3.15 have not been tested for GPU/CUDA/OpenACC support after
  # https://github.com/BlueBrain/CoreNeuron/pull/609.

  # Fail hard and early if we don't have the PGI/NVHPC compiler.
  if(NOT CORENRN_HAVE_NVHPC_COMPILER)
    message(
      FATAL_ERROR
        "GPU support is available via OpenACC using PGI/NVIDIA compilers."
        " Use NVIDIA HPC SDK with -DCMAKE_C_COMPILER=nvc -DCMAKE_CUDA_COMPILER=nvcc -DCMAKE_CXX_COMPILER=nvc++"
    )
  endif()

  # Set some sensible default CUDA architectures.
  if(NOT DEFINED CMAKE_CUDA_ARCHITECTURES)
    set(CMAKE_CUDA_ARCHITECTURES 70 80)
    message(STATUS "Setting default CUDA architectures to ${CMAKE_CUDA_ARCHITECTURES}")
  endif()

  # See https://gitlab.kitware.com/cmake/cmake/-/issues/23081, this should not be needed according
  # to the CMake documentation, but it is not clear that any version behaves as documented.
  if(DEFINED CMAKE_CUDA_HOST_COMPILER)
    unset(ENV{CUDAHOSTCXX})
  endif()

  # Enable CUDA language support.
  enable_language(CUDA)

  # Prefer shared libcudart.so
  if(${CMAKE_VERSION} VERSION_LESS 3.17)
    # Ugly workaround from https://gitlab.kitware.com/cmake/cmake/-/issues/17559, remove when
    # possible
    if(CMAKE_CUDA_HOST_IMPLICIT_LINK_LIBRARIES)
      list(REMOVE_ITEM CMAKE_CUDA_HOST_IMPLICIT_LINK_LIBRARIES "cudart_static")
      list(REMOVE_ITEM CMAKE_CUDA_HOST_IMPLICIT_LINK_LIBRARIES "cudadevrt")
      list(APPEND CMAKE_CUDA_HOST_IMPLICIT_LINK_LIBRARIES "cudart")
    endif()
    if(CMAKE_CUDA_IMPLICIT_LINK_LIBRARIES)
      list(REMOVE_ITEM CMAKE_CUDA_IMPLICIT_LINK_LIBRARIES "cudart_static")
      list(REMOVE_ITEM CMAKE_CUDA_IMPLICIT_LINK_LIBRARIES "cudadevrt")
      list(APPEND CMAKE_CUDA_IMPLICIT_LINK_LIBRARIES "cudart")
    endif()
  else()
    # nvc++ -cuda implicitly links dynamically to libcudart.so. Setting this makes sure that CMake
    # does not add -lcudart_static and trigger errors due to mixed dynamic/static linkage.
    set(CMAKE_CUDA_RUNTIME_LIBRARY Shared)
  endif()

  # Patch CUDA_ARCHITECTURES support into older CMake versions
  if(${CMAKE_VERSION} VERSION_LESS 3.18)
    foreach(cuda_arch ${CMAKE_CUDA_ARCHITECTURES})
      string(
        APPEND CMAKE_CUDA_FLAGS
        " --generate-code=arch=compute_${cuda_arch},code=[compute_${cuda_arch},sm_${cuda_arch}]")
    endforeach()
  endif()

  # ~~~
  # Needed for the Eigen GPU support Warning suppression (Eigen GPU-related):
  # 3057 : Warning on ignoring __host__ annotation in some functions
  # 3085 : Warning on redeclaring a __host__ function as __host__ __device__
  # ~~~
  set(CMAKE_CUDA_FLAGS
      "${CMAKE_CUDA_FLAGS} --expt-relaxed-constexpr -Xcudafe --diag_suppress=3057,--diag_suppress=3085"
  )

  # NMODL supports both OpenACC and OpenMP target offload
  if(CORENRN_ENABLE_OPENMP AND CORENRN_ENABLE_OPENMP_OFFLOAD)
    set(CORENRN_ACCELERATOR_OFFLOAD "OpenMP")
  else()
    set(CORENRN_ACCELERATOR_OFFLOAD "OpenACC")
  endif()
endif()

# =============================================================================
# Project version from git and project directories
# =============================================================================
set(CN_PROJECT_VERSION ${PROJECT_VERSION})

# generate file with version number from git and nrnunits.lib file path
configure_file(${PROJECT_SOURCE_DIR}/src/coreneuron/config/config.cpp.in
               ${PROJECT_BINARY_DIR}/coreneuron/config/config.cpp @ONLY)

# =============================================================================
# Include cmake modules after cmake options
# =============================================================================
include(OpenAccHelper)

# =============================================================================
# Common dependencies
# =============================================================================
find_package(Perl REQUIRED)

# =============================================================================
# Common build options
# =============================================================================
# build mod files for coreneuron
list(APPEND CORENRN_COMPILE_DEFS CORENEURON_BUILD)
set(CMAKE_REQUIRED_QUIET TRUE)
check_include_files(malloc.h have_malloc_h)
if(have_malloc_h)
  list(APPEND CORENRN_COMPILE_DEFS HAVE_MALLOC_H)
endif()

# =============================================================================
# Build option specific compiler flags
# =============================================================================
# We use Eigen for "small" matrices with thread-level parallelism handled at a higher level; tell
# Eigen not to try to multithread internally
list(APPEND CORENRN_COMPILE_DEFS EIGEN_DONT_PARALLELIZE)
if(CORENRN_HAVE_NVHPC_COMPILER)
  # PGI with llvm code generation doesn't have necessary assembly intrinsic headers
  list(APPEND CORENRN_COMPILE_DEFS EIGEN_DONT_VECTORIZE=1)
  if(NOT CORENRN_ENABLE_GPU AND ${CMAKE_CXX_COMPILER_VERSION} VERSION_GREATER_EQUAL 21.11)
    # Random123 does not play nicely with NVHPC 21.11+'s detection of ABM features if it detects the
    # compiler to be PGI or NVHPC, see: https://github.com/BlueBrain/CoreNeuron/issues/724 and
    # https://github.com/DEShawResearch/random123/issues/6. In fact in GPU builds Random123
    # (mis)detects nvc++ as nvcc because we pass the -cuda option and we therefore avoid the
    # problem. If GPU support is disabled, we define R123_USE_INTRIN_H=0 to avoid the problem.
    list(APPEND CORENRN_COMPILE_DEFS R123_USE_INTRIN_H=0)
  endif()
endif()

if(CORENRN_ENABLE_SHARED)
  set(COMPILE_LIBRARY_TYPE "SHARED")
else()
  set(COMPILE_LIBRARY_TYPE "STATIC")
endif()

if(NRN_ENABLE_MPI)
  list(APPEND CORENRN_COMPILE_DEFS NRNMPI=1)
else()
  list(APPEND CORENRN_COMPILE_DEFS NRNMPI=0)
  list(APPEND CORENRN_COMPILE_DEFS NRN_MULTISEND=0)
endif()
if(NRN_ENABLE_MPI_DYNAMIC)
  list(APPEND CORENRN_COMPILE_DEFS NRNMPI_DYNAMICLOAD=1)
endif()

if(CORENRN_ENABLE_OPENMP)
  find_package(OpenMP QUIET)
  if(OPENMP_FOUND)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS} ${ADDITIONAL_THREADSAFE_FLAGS}")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS} ${ADDITIONAL_THREADSAFE_FLAGS}")
  endif()
endif()

list(APPEND CORENRN_COMPILE_DEFS LAYOUT=0)

if(NOT CORENRN_ENABLE_HOC_EXP)
  list(APPEND CORENRN_COMPILE_DEFS DISABLE_HOC_EXP)
endif()

# splay tree required for net_move
if(CORENRN_ENABLE_SPLAYTREE_QUEUING)
  list(APPEND CORENRN_COMPILE_DEFS ENABLE_SPLAYTREE_QUEUING)
endif()

if(NOT CORENRN_ENABLE_NET_RECEIVE_BUFFER)
  list(APPEND CORENRN_COMPILE_DEFS NET_RECEIVE_BUFFERING=0)
endif()

if(NOT CORENRN_ENABLE_TIMEOUT)
  list(APPEND CORENRN_COMPILE_DEFS DISABLE_TIMEOUT)
endif()

if(CORENRN_ENABLE_REPORTING)
  find_package(sonata REQUIRED)

  if(TARGET sonata::sonata_report)
    list(APPEND CORENRN_COMPILE_DEFS ENABLE_SONATA_REPORTS)
    set(ENABLE_SONATA_REPORTS_TESTS ON)
  else()
    message(SEND_ERROR "SONATA library was found but without reporting support")
  endif()

  include_directories(${sonatareport_INCLUDE_DIR})
endif()

if(CORENRN_ENABLE_PRCELLSTATE)
  set(CORENRN_NRN_PRCELLSTATE 1)
else()
  set(CORENRN_NRN_PRCELLSTATE 0)
endif()
if(MINGW)
  list(APPEND CORENRN_COMPILE_DEFS MINGW)
endif()

# =============================================================================
# NMODL specific options
# =============================================================================
find_package(nmodl)
if(NOT "${CORENRN_NMODL_DIR}" STREQUAL "" AND NOT nmodl_FOUND)
  message(FATAL_ERROR "Cannot find NMODL in ${CORENRN_NMODL_DIR}")
endif()
if(nmodl_FOUND)
  set(CORENRN_NMODL_BINARY ${nmodl_BINARY})
  set(CORENRN_NMODL_INCLUDE ${nmodl_INCLUDE})
  # path to python interface
  set(ENV{PYTHONPATH} "${nmodl_PYTHONPATH}:$ENV{PYTHONPATH}")
  set(CORENRN_NMODL_PYTHONPATH $ENV{PYTHONPATH})
else()
  set(NMODL_ENABLE_PYTHON_BINDINGS
      ON
      CACHE BOOL "Disable NMODL python bindings")
  nrn_add_external_project(nmodl OFF)
  add_subdirectory(${PROJECT_SOURCE_DIR}/external/nmodl ${CMAKE_BINARY_DIR}/external/nmodl)
  set(CORENRN_NMODL_BINARY ${CMAKE_BINARY_DIR}/bin/nmodl${CMAKE_EXECUTABLE_SUFFIX})
  set(CORENRN_NMODL_INCLUDE ${CMAKE_BINARY_DIR}/include)
  set(ENV{PYTHONPATH} "$ENV{PYTHONPATH}")
  set(nmodl_PYTHONPATH "${CMAKE_BINARY_DIR}/lib")
  set(CORENRN_NMODL_PYTHONPATH "${nmodl_PYTHONPATH}:$ENV{PYTHONPATH}")
  set(NMODL_TARGET_TO_DEPEND nmodl)
  set(NMODL_PROJECT_BINARY_DIR ${CMAKE_BINARY_DIR}/external/nmodl)
  # install nrnunits.lib and libpywrapper.so from external/nmodl
  install(
    FILES ${NMODL_PROJECT_BINARY_DIR}/lib/libpywrapper${CMAKE_SHARED_LIBRARY_SUFFIX}
    DESTINATION lib
    COMPONENT pywrapper
    OPTIONAL)
  install(
    FILES ${NMODL_PROJECT_BINARY_DIR}/share/nmodl/nrnunits.lib
    DESTINATION share/nmodl
    COMPONENT nrnunits)
endif()
set_property(GLOBAL PROPERTY CORENRN_NMODL_BINARY "${CORENRN_NMODL_BINARY}")

# set correct arguments for nmodl for cpu/gpu target
set(CORENRN_NMODL_FLAGS
    ""
    CACHE STRING "Extra NMODL options such as passes")

# =============================================================================
# Profiler/Instrumentation Options
# =============================================================================
if(CORENRN_ENABLE_CALIPER_PROFILING)
  find_package(caliper REQUIRED)
  list(APPEND CORENRN_COMPILE_DEFS NRN_CALIPER)
  set(CORENRN_CALIPER_LIB caliper)
endif()

if(CORENRN_ENABLE_LIKWID_PROFILING)
  include(GetLIKWID)

  list(APPEND CORENRN_COMPILE_DEFS LIKWID_PERFMON)
  set(CORENRN_LIKWID_LIB nrn_likwid)
endif()

# enable debugging code with extra logs to stdout
if(CORENRN_ENABLE_DEBUG_CODE)
  list(APPEND CORENRN_COMPILE_DEFS CORENRN_DEBUG CHKPNTDEBUG CORENRN_DEBUG_QUEUE INTERLEAVE_DEBUG)
endif()

# =============================================================================
# Common CXX flags : ignore unknown pragma warnings
# =============================================================================
# Do not set this when building wheels. The nrnivmodl workflow means that we do not know what
# compiler will be invoked with these flags, so we have to use flags that are as generic as
# possible.
if(NOT DEFINED NRN_BINARY_DIST_BUILD OR NOT NRN_BINARY_DIST_BUILD)
  list(APPEND CORENRN_EXTRA_CXX_FLAGS "${IGNORE_UNKNOWN_PRAGMA_FLAGS}")
endif()

# Add compiler flags that should apply to all CoreNEURON targets, but which should not leak into
# other included projects.
add_compile_definitions(${CORENRN_COMPILE_DEFS})
add_compile_options(${CORENRN_EXTRA_CXX_FLAGS})
add_link_options(${CORENRN_EXTRA_LINK_FLAGS})

# =============================================================================
# gather various source files
# =============================================================================
file(
  GLOB
  CORENEURON_CODE_FILES
  "apps/main1.cpp"
  "apps/corenrn_parameters.cpp"
  "gpu/nrn_acc_manager.cpp"
  "io/*.cpp"
  "io/reports/*.cpp"
  "mechanism/*.cpp"
  "mpi/core/nrnmpi_def_cinc.cpp"
  "network/*.cpp"
  "permute/*.cpp"
  "sim/*.cpp"
  "sim/scopmath/abort.cpp"
  "sim/scopmath/newton_thread.cpp"
  "utils/*.cpp"
  "utils/*/*.c"
  "utils/*/*.cpp")

set(MPI_LIB_FILES "mpi/lib/mpispike.cpp" "mpi/lib/nrnmpi.cpp")
if(NRN_ENABLE_MPI)
  # Building these requires -ldl, which is only added if MPI is enabled.
  list(APPEND CORENEURON_CODE_FILES "mpi/core/resolve.cpp")
endif()

file(COPY ${PROJECT_SOURCE_DIR}/external/Random123/include/Random123
     DESTINATION ${CMAKE_BINARY_DIR}/include)
list(APPEND CORENEURON_CODE_FILES ${PROJECT_BINARY_DIR}/coreneuron/config/config.cpp)

set(ENGINEMECH_CODE_FILE "mechanism/mech/enginemech.cpp")

# for external mod files we need to generate modl_ref function in mod_func.c
set(MODFUNC_PERL_SCRIPT "mechanism/mech/mod_func.c.pl")

set(NMODL_UNITS_FILE "${CMAKE_BINARY_DIR}/share/mod2c/nrnunits.lib")

# =============================================================================
# Copy files that are required by nrnivmodl-core to the build tree at build time.
# =============================================================================
cpp_cc_build_time_copy(
  INPUT "${CMAKE_CURRENT_SOURCE_DIR}/${MODFUNC_PERL_SCRIPT}"
  OUTPUT "${CMAKE_BINARY_DIR}/share/coreneuron/mod_func.c.pl"
  NO_TARGET)
cpp_cc_build_time_copy(
  INPUT "${CMAKE_CURRENT_SOURCE_DIR}/${ENGINEMECH_CODE_FILE}"
  OUTPUT "${CMAKE_BINARY_DIR}/share/coreneuron/enginemech.cpp"
  NO_TARGET)
set(nrnivmodl_core_dependencies "${CMAKE_BINARY_DIR}/share/coreneuron/mod_func.c.pl"
                                "${CMAKE_BINARY_DIR}/share/coreneuron/enginemech.cpp")
# Set up build rules that copy builtin mod files from src/coreneuron/mechanism/mech/modfile/*.mod to
# {build_dir}/share/modfile/
file(GLOB builtin_modfiles "${PROJECT_SOURCE_DIR}/src/coreneuron/mechanism/mech/modfile/*.mod")
foreach(builtin_modfile ${builtin_modfiles})
  # Construct the path in the build directory.
  get_filename_component(builtin_modfile_name "${builtin_modfile}" NAME)
  set(modfile_build_path "${CMAKE_BINARY_DIR}/share/modfile/${builtin_modfile_name}")
  # Create a build rule to copy the modfile there.
  cpp_cc_build_time_copy(
    INPUT "${builtin_modfile}"
    OUTPUT "${modfile_build_path}"
    NO_TARGET)
  list(APPEND nrnivmodl_core_dependencies "${modfile_build_path}")
endforeach()
add_custom_target(coreneuron-copy-nrnivmodl-core-dependencies ALL
                  DEPENDS ${nrnivmodl_core_dependencies})
# Store the build-tree modfile paths in a cache variable; these are an implicit dependency of
# nrnivmodl-core.
set(CORENEURON_BUILTIN_MODFILES
    "${nrnivmodl_core_dependencies}"
    CACHE STRING "List of builtin modfiles that nrnivmodl-core implicitly depends on" FORCE)

# =============================================================================
# coreneuron GPU library
# =============================================================================
if(CORENRN_ENABLE_GPU)
  # Only compile the explicit CUDA implementation of the Hines solver in GPU builds. Because of
  # https://forums.developer.nvidia.com/t/cannot-dynamically-load-a-shared-library-containing-both-openacc-and-cuda-code/210972
  # this cannot be included in the same shared library as the rest of the OpenACC code.
  set(CORENEURON_CUDA_FILES ${CMAKE_CURRENT_SOURCE_DIR}/permute/cellorder.cu)

  # Eigen functions cannot be called directly from OpenACC regions, but Eigen is sort-of compatible
  # with being compiled as CUDA code. Because of
  # https://forums.developer.nvidia.com/t/cannot-dynamically-load-a-shared-library-containing-both-openacc-and-cuda-code/210972
  # this has to mean `nvc++ -cuda` rather than `nvcc`. We explicitly instantiate Eigen functions for
  # different matrix sizes in partial_piv_lu.cpp (with CUDA attributes but without OpenACC or OpenMP
  # annotations) and dispatch to these from a wrapper in partial_piv_lu.h that does have
  # OpenACC/OpenMP annotations.
  if(EXISTS ${CORENRN_NMODL_INCLUDE}/partial_piv_lu/partial_piv_lu.cpp)
    list(APPEND CORENEURON_CODE_FILES ${CORENRN_NMODL_INCLUDE}/partial_piv_lu/partial_piv_lu.cpp)
    if(CORENRN_ENABLE_GPU
       AND CORENRN_HAVE_NVHPC_COMPILER
       AND CMAKE_BUILD_TYPE STREQUAL "Debug")
      # In this case OpenAccHelper.cmake passes -gpu=debug, which makes these Eigen functions
      # extremely slow. Downgrade that to -gpu=lineinfo for this file.
      set_source_files_properties(${CORENRN_NMODL_INCLUDE}/partial_piv_lu/partial_piv_lu.cpp
                                  PROPERTIES COMPILE_FLAGS "-gpu=lineinfo,nodebug -O1")
    endif()
  endif()
endif()

# =============================================================================
# create libraries
# =============================================================================

# name of coreneuron mpi objects or dynamic library
set(CORENRN_MPI_LIB_NAME
    "corenrn_mpi"
    CACHE INTERNAL "")

# for non-dynamic mpi mode just build object files
if(NRN_ENABLE_MPI AND NOT NRN_ENABLE_MPI_DYNAMIC)
  add_library(${CORENRN_MPI_LIB_NAME} OBJECT ${MPI_LIB_FILES})
  target_include_directories(
    ${CORENRN_MPI_LIB_NAME} PRIVATE ${MPI_C_INCLUDE_DIRS} ${PROJECT_SOURCE_DIR}/src
                                    ${CMAKE_BINARY_DIR}/generated)
  target_link_libraries(${CORENRN_MPI_LIB_NAME} ${CORENRN_CALIPER_LIB} ${CORENRN_LIKWID_LIB})
  set_property(TARGET ${CORENRN_MPI_LIB_NAME} PROPERTY POSITION_INDEPENDENT_CODE ON)
  set(CORENRN_MPI_OBJ $<TARGET_OBJECTS:${CORENRN_MPI_LIB_NAME}>)
endif()

# Library containing the bulk of the non-mechanism CoreNEURON code. This is always created and
# installed as a static library, and then the nrnivmodl-core workflow extracts the object files from
# it and does one of the following:
#
# * shared build: creates libcorenrnmech_internal.so from these objects plus those from the
#   translated MOD files
# * static build: creates a (temporary, does not get installed) libcorenrnmech_internal.a from these
#   objects plus those from the translated MOD files, then statically links that into special-core
#   (nrniv-core)
#
# This scheme means that both core and mechanism .o files are linked in a single step, which is
# important for GPU linking. It does, however, mean that the core code is installed twice, once in
# libcoreneuron-core.a and once in libcorenrnmech_internal.so (shared) or nrniv-core (static). In a
# GPU build, libcoreneuron-cuda.{a,so} is also linked to provide the CUDA implementation of the
# Hines solver. This cannot be included in coreneuron-core because of this issue:
# https://forums.developer.nvidia.com/t/cannot-dynamically-load-a-shared-library-containing-both-openacc-and-cuda-code/210972
add_library(coreneuron-core STATIC ${CORENEURON_CODE_FILES} ${CORENRN_MPI_OBJ})
if(CORENRN_ENABLE_GPU)
  set(coreneuron_cuda_target coreneuron-cuda)
  add_library(coreneuron-cuda ${COMPILE_LIBRARY_TYPE} ${CORENEURON_CUDA_FILES})
  target_link_libraries(coreneuron-core PUBLIC coreneuron-cuda)
endif()

foreach(target coreneuron-core ${coreneuron_cuda_target})
  target_include_directories(${target} PRIVATE ${PROJECT_SOURCE_DIR}/src
                                               ${CMAKE_BINARY_DIR}/generated)
endforeach()

# we can link to MPI libraries in non-dynamic-mpi build
if(NRN_ENABLE_MPI AND NOT NRN_ENABLE_MPI_DYNAMIC)
  target_link_libraries(coreneuron-core PUBLIC ${MPI_CXX_LIBRARIES})
endif()

# ~~~
# main coreneuron library needs to be linked to libdl.so
# only in case of dynamic mpi build. But on old system
# like centos7, we saw mpich library require explici
# link to libdl.so. See
#   https://github.com/neuronsimulator/nrn-build-ci/pull/51
# ~~~
target_link_libraries(coreneuron-core PUBLIC ${CMAKE_DL_LIBS})

# this is where we handle dynamic mpi library build
if(NRN_ENABLE_MPI AND NRN_ENABLE_MPI_DYNAMIC)
  # store mpi library targets that will be built
  list(APPEND corenrn_mpi_targets "")

  # ~~~
  # if coreneuron is build as part of neuron then check if NEURON has created
  # list of libraries that needs to be built. We use neuron cmake variables here because
  # we don't need to duplicate CMake code into coreneuron (we want to have unified cmake
  # project soon). In the absense of neuron just build a single library libcorenrn_mpi.
  # This is mostly used for the testing.
  #
  # from neuron we know how many different libraries needs to be built, their names
  # include paths to be used for building shared libraries. Iterate through those
  # and build separate library for each MPI distribution. For example, following
  # libraries are created:
  # - libcorenrn_mpich.so
  # - libcorenrn_ompi.so
  # - libcorenrn_mpt.so
  # ~~~
  list(LENGTH NRN_MPI_LIBNAME_LIST _num_mpi)
  math(EXPR num_mpi "${_num_mpi} - 1")
  foreach(val RANGE ${num_mpi})
    list(GET NRN_MPI_INCLUDE_LIST ${val} include)
    list(GET NRN_MPI_LIBNAME_LIST ${val} libname)

    add_library(core${libname}_lib SHARED ${MPI_LIB_FILES})
    target_link_libraries(core${libname}_lib ${CORENRN_CALIPER_LIB} ${CORENRN_LIKWID_LIB})
    target_include_directories(
      core${libname}_lib
      PUBLIC ${include}
      PRIVATE ${PROJECT_SOURCE_DIR}/src ${CMAKE_BINARY_DIR}/generated)

    # ~~~
    # TODO: somehow mingw requires explicit linking. This needs to be verified
    # when we will test coreneuron on windows.
    # ~~~
    if(MINGW) # type msmpi only
      add_dependencies(core${libname}_lib coreneuron-core)
      target_link_libraries(core${libname}_lib ${MPI_C_LIBRARIES} coreneuron-core)
    endif()
    set_property(TARGET core${libname}_lib PROPERTY OUTPUT_NAME core${libname})
    list(APPEND corenrn_mpi_targets "core${libname}_lib")
  endforeach(val)

  set_target_properties(
    ${corenrn_mpi_targets}
    PROPERTIES ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
               LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
               POSITION_INDEPENDENT_CODE ON)
  install(TARGETS ${corenrn_mpi_targets} DESTINATION lib)
endif()

# Suppress some compiler warnings.
target_compile_options(coreneuron-core PRIVATE ${CORENEURON_CXX_WARNING_SUPPRESSIONS})
target_link_libraries(coreneuron-core PUBLIC ${sonatareport_LIBRARY} ${CORENRN_CALIPER_LIB}
                                             ${CORENRN_LIKWID_LIB})

# TODO: fix adding a dependency of coreneuron-core on CLI11::CLI11 when CLI11 is a submodule. Right
# now this doesn't work because the CLI11 targets are not exported/installed but coreneuron-core is.
get_target_property(CLI11_HEADER_DIRECTORY CLI11::CLI11 INTERFACE_INCLUDE_DIRECTORIES)
target_include_directories(
  coreneuron-core SYSTEM PRIVATE ${CLI11_HEADER_DIRECTORY}
                                 ${PROJECT_SOURCE_DIR}/external/Random123/include)

if(CORENRN_ENABLE_GPU)
  # nrnran123.cpp uses Boost.Pool in GPU builds if it's available.
  find_package(Boost QUIET)
  if(Boost_FOUND)
    message(STATUS "Boost found, enabling use of memory pools for Random123...")
    target_include_directories(coreneuron-core SYSTEM PRIVATE ${Boost_INCLUDE_DIRS})
    target_compile_definitions(coreneuron-core PRIVATE CORENEURON_USE_BOOST_POOL)
  endif()
endif()

set_target_properties(
  coreneuron-core ${coreneuron_cuda_target}
  PROPERTIES ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
             LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
             POSITION_INDEPENDENT_CODE ${CORENRN_ENABLE_SHARED})
cpp_cc_configure_sanitizers(TARGET coreneuron-core ${coreneuron_cuda_target} ${corenrn_mpi_targets})

# =============================================================================
# create special-core with inbuilt mode files
# =============================================================================
# We have to link things like unit tests against this because some "core" .cpp files refer to
# symbols in the translated versions of default .mod files
set(nrniv_core_prefix "${CMAKE_BINARY_DIR}/bin/${CMAKE_SYSTEM_PROCESSOR}")
set(corenrn_mech_library
    "${nrniv_core_prefix}/${CMAKE_${COMPILE_LIBRARY_TYPE}_LIBRARY_PREFIX}corenrnmech_internal${CMAKE_${COMPILE_LIBRARY_TYPE}_LIBRARY_SUFFIX}"
)
set(output_binaries "${nrniv_core_prefix}/special-core" "${corenrn_mech_library}")

add_custom_command(
  OUTPUT ${output_binaries}
  DEPENDS coreneuron-core ${NMODL_TARGET_TO_DEPEND} ${CORENEURON_BUILTIN_MODFILES}
  COMMAND
    ${CMAKE_COMMAND} -E env NMODLHOME=${NMODL_PROJECT_BINARY_DIR}
    ${CMAKE_BINARY_DIR}/bin/nrnivmodl-core -b ${COMPILE_LIBRARY_TYPE} -m ${CORENRN_NMODL_BINARY} -n
    "internal" -p 4 "."
  WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/bin
  COMMENT "Running nrnivmodl-core with internal mod files")
add_custom_target(nrniv-core ALL DEPENDS ${output_binaries})

# Build a target representing the libcorenrnmech_internal.so that is produced under bin/x86_64,
# which executables such as the unit tests must link against
add_library(builtin-libcorenrnmech SHARED IMPORTED)
add_dependencies(builtin-libcorenrnmech nrniv-core)
set_target_properties(builtin-libcorenrnmech PROPERTIES IMPORTED_LOCATION "${corenrn_mech_library}")

if(CORENRN_ENABLE_GPU)
  separate_arguments(CORENRN_ACC_FLAGS UNIX_COMMAND "${NVHPC_ACC_COMP_FLAGS}")
  target_compile_options(coreneuron-core PRIVATE ${CORENRN_ACC_FLAGS})
endif()

# Create an extra target for use by NEURON when CoreNEURON is being built as a submodule. NEURON
# tests will depend on this, so it must in turn depend on everything that is needed to run nrnivmodl
# -coreneuron.
add_custom_target(coreneuron-for-tests)
add_dependencies(coreneuron-for-tests coreneuron-core ${NMODL_TARGET_TO_DEPEND})
# Create an extra target for internal use that unit tests and so on can depend on.
# ${corenrn_mech_library} is libcorenrnmech_internal.{a,so}, which contains both the compiled
# default mechanisms and the content of libcoreneuron-core.a.
add_library(coreneuron-all INTERFACE)
target_link_libraries(coreneuron-all INTERFACE builtin-libcorenrnmech)
# Also copy the dependencies of libcoreneuron-core as interface dependencies of this new target
# (example: ${corenrn_mech_library} will probably depend on MPI, so when the unit tests link against
# ${corenrn_mech_library} they need to know to link against MPI too).
get_target_property(coreneuron_core_deps coreneuron-core LINK_LIBRARIES)
if(coreneuron_core_deps)
  foreach(dep ${coreneuron_core_deps})
    target_link_libraries(coreneuron-all INTERFACE ${dep})
  endforeach()
endif()

# Make headers avail to build tree
configure_file(engine.h.in ${CMAKE_BINARY_DIR}/include/coreneuron/engine.h @ONLY)

file(
  GLOB_RECURSE main_headers
  RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
  *.h *.hpp)

configure_file(${CMAKE_BINARY_DIR}/generated/coreneuron/config/neuron_version.hpp
               ${CMAKE_BINARY_DIR}/include/coreneuron/config/neuron_version.hpp COPYONLY)
foreach(header ${main_headers})
  configure_file(${CMAKE_CURRENT_SOURCE_DIR}/${header}
                 ${CMAKE_BINARY_DIR}/include/coreneuron/${header} COPYONLY)
endforeach()

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/utils/profile/profiler_interface.h
               ${CMAKE_BINARY_DIR}/include/coreneuron/nrniv/profiler_interface.h COPYONLY)

# main program required for building special-core
cpp_cc_build_time_copy(INPUT ${CMAKE_CURRENT_SOURCE_DIR}/apps/coreneuron.cpp
                       OUTPUT ${CMAKE_BINARY_DIR}/share/coreneuron/coreneuron.cpp)

# Extract the various compiler option strings to use inside nrnivmodl-core. Sets the global property
# CORENRN_LIB_LINK_FLAGS, which contains the arguments that must be added to the link line for
# `special` to link against `libcorenrnmech_internal.{a,so}`
include(MakefileBuildOptions)

# =============================================================================
# CoreNEURON related configuration
# =============================================================================

# =============================================================================
# Copy first into build directory as it will be used for special-core
# =============================================================================
configure_file(${PROJECT_SOURCE_DIR}/bin/nrnivmodl_core_makefile.in
               ${CMAKE_BINARY_DIR}/share/coreneuron/nrnivmodl_core_makefile @ONLY)
configure_file(${PROJECT_SOURCE_DIR}/bin/nrnivmodl-core.in ${CMAKE_BINARY_DIR}/bin/nrnivmodl-core
               @ONLY)
# nrnivmodl-core depends on the building of NMODL_TARGET_TO_DEPEND and the configuration of the
# nrnivmodl-core and nrnivmodl_core_makefile this doesn't imply that whenever there is a change in
# one of those files then the prebuilt mod files are going to be rebuilt
add_custom_target(
  nrnivmodl-core ALL
  DEPENDS ${CMAKE_BINARY_DIR}/bin/nrnivmodl-core
          ${CMAKE_BINARY_DIR}/share/coreneuron/nrnivmodl_core_makefile ${NMODL_TARGET_TO_DEPEND})

if(CORENRN_ENABLE_UNIT_TESTS)
  add_subdirectory(${PROJECT_SOURCE_DIR}/test/coreneuron/unit
                   ${CMAKE_BINARY_DIR}/test/coreneuron/unit)
endif()

# =============================================================================
# Install main targets
# =============================================================================

# coreneuron main libraries
install(
  TARGETS coreneuron-core ${coreneuron_cuda_target}
  EXPORT coreneuron
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
  INCLUDES
  DESTINATION $<INSTALL_INTERFACE:include>)

# headers and some standalone code files for nrnivmodl-core
install(
  DIRECTORY ${CMAKE_BINARY_DIR}/include/coreneuron
  DESTINATION include/
  FILES_MATCHING
  PATTERN "*.h*"
  PATTERN "*.ipp")
install(FILES ${MODFUNC_PERL_SCRIPT} ${ENGINEMECH_CODE_FILE} DESTINATION share/coreneuron)

# copy nmodl for nrnivmodl-core
install(PROGRAMS ${CORENRN_NMODL_BINARY} DESTINATION bin)

# install nrniv-core app
install(
  PROGRAMS ${CMAKE_BINARY_DIR}/bin/${CMAKE_HOST_SYSTEM_PROCESSOR}/special-core
  DESTINATION bin
  RENAME nrniv-core)
install(FILES apps/coreneuron.cpp DESTINATION share/coreneuron)

# install mechanism library in shared library builds, if we're linking statically then there is no
# need
if(CORENRN_ENABLE_SHARED)
  install(FILES ${corenrn_mech_library} DESTINATION lib)
endif()

# install random123 and nmodl headers
install(DIRECTORY ${CMAKE_BINARY_DIR}/include/ DESTINATION include)

# install mod files
install(DIRECTORY ${CMAKE_BINARY_DIR}/share/modfile DESTINATION share)

# =============================================================================
# Install for end users
# =============================================================================
install(FILES ${CMAKE_BINARY_DIR}/share/coreneuron/nrnivmodl_core_makefile
        DESTINATION share/coreneuron)
install(PROGRAMS ${CMAKE_BINARY_DIR}/bin/nrnivmodl-core DESTINATION bin)

# =============================================================================
# Set flag for NEURON linking
# =============================================================================
get_property(CORENRN_NEURON_LINK_FLAGS GLOBAL PROPERTY CORENRN_NEURON_LINK_FLAGS)

# =============================================================================
# Build status
# =============================================================================
message(STATUS "")
message(STATUS "CoreNEURON is enabled with following build configuration:")
message(STATUS "--------------------+--------------------------------------------------------")
message(STATUS " Build option       | Status")
message(STATUS "--------------------+--------------------------------------------------------")
message(STATUS "CXX COMPILER        | ${CMAKE_CXX_COMPILER}")
message(STATUS "COMPILE FLAGS       | ${CORENRN_CXX_FLAGS}")
message(STATUS "Build Type          | ${COMPILE_LIBRARY_TYPE}")
message(STATUS "MPI                 | ${NRN_ENABLE_MPI}")
if(NRN_ENABLE_MPI)
  message(STATUS "  DYNAMIC           | ${NRN_ENABLE_MPI_DYNAMIC}")
  if(NRN_ENABLE_MPI_DYNAMIC AND NRN_MPI_LIBNAME_LIST)
    # ~~~
    # for dynamic mpi, rely on neuron for list of libraries to build
    # this is to avoid cmake code duplication on the coreneuron side
    # ~~~
    list(LENGTH NRN_MPI_LIBNAME_LIST _num_mpi)
    math(EXPR num_mpi "${_num_mpi} - 1")
    foreach(val RANGE ${num_mpi})
      list(GET NRN_MPI_LIBNAME_LIST ${val} libname)
      list(GET NRN_MPI_INCLUDE_LIST ${val} include)
      message(STATUS "    LIBNAME         | core${libname}")
      message(STATUS "    INC             | ${include}")
    endforeach(val)
  else()
    message(STATUS "  INC               | ${MPI_CXX_INCLUDE_DIRS}")
  endif()
endif()
message(STATUS "OpenMP              | ${CORENRN_ENABLE_OPENMP}")
message(STATUS "NMODL PATH          | ${CORENRN_NMODL_BINARY}")
message(STATUS "NMODL FLAGS         | ${CORENRN_NMODL_FLAGS}")
message(STATUS "GPU Support         | ${CORENRN_ENABLE_GPU}")
if(CORENRN_ENABLE_GPU)
  message(STATUS "  CUDA              | ${CUDAToolkit_LIBRARY_DIR}")
  message(STATUS "  Offload           | ${CORENRN_ACCELERATOR_OFFLOAD}")
  message(STATUS "  Unified Memory    | ${CORENRN_ENABLE_CUDA_UNIFIED_MEMORY}")
endif()
message(STATUS "Auto Timeout        | ${CORENRN_ENABLE_TIMEOUT}")
message(STATUS "Wrap exp()          | ${CORENRN_ENABLE_HOC_EXP}")
message(STATUS "SplayTree Queue     | ${CORENRN_ENABLE_SPLAYTREE_QUEUING}")
message(STATUS "NetReceive Buffer   | ${CORENRN_ENABLE_NET_RECEIVE_BUFFER}")
message(STATUS "Caliper             | ${CORENRN_ENABLE_CALIPER_PROFILING}")
message(STATUS "Likwid              | ${CORENRN_ENABLE_LIKWID_PROFILING}")
message(STATUS "Unit Tests          | ${CORENRN_ENABLE_UNIT_TESTS}")
message(STATUS "Reporting           | ${CORENRN_ENABLE_REPORTING}")
if(CORENRN_ENABLE_REPORTING)
  message(STATUS "  sonatareport_INC  | ${sonatareport_INCLUDE_DIR}")
  message(STATUS "  sonatareport_LIB  | ${sonatareport_LIBRARY}")
endif()
message(STATUS "--------------------+--------------------------------------------------------")
message(STATUS "")
