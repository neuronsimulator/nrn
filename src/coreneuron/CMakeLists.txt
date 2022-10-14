# =============================================================================
# Copyright (c) 2016 - 2022 Blue Brain Project/EPFL
#
# See top-level LICENSE file for details.
# =============================================================================

# Add compiler flags that should apply to all CoreNEURON targets, but which should not leak into
# other included projects.
add_compile_definitions(${CORENRN_COMPILE_DEFS})
add_compile_options(${CORENRN_EXTRA_CXX_FLAGS})
add_link_options(${CORENRN_EXTRA_LINK_FLAGS})

# put libraries (e.g. dll) in bin directory
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

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
if(CORENRN_ENABLE_MPI)
  # Building these requires -ldl, which is only added if MPI is enabled.
  list(APPEND CORENEURON_CODE_FILES "mpi/core/resolve.cpp" "mpi/core/nrnmpidec.cpp")
endif()
file(COPY ${CORENEURON_PROJECT_SOURCE_DIR}/external/Random123/include/Random123
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
# Set up build rules that copy builtin mod files from
# {source}/coreneuron/mechanism/mech/modfile/*.mod to {build_dir}/share/modfile/
file(GLOB builtin_modfiles
     "${CORENEURON_PROJECT_SOURCE_DIR}/coreneuron/mechanism/mech/modfile/*.mod")
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
  # ~~~
  # artificial cells and some other cpp files (using Random123) should be compiled
  # without OpenACC to avoid use of GPU Random123 streams
  # OL210813: this shouldn't be needed anymore, but it may have a small performance benefit
  # ~~~
  set(OPENACC_EXCLUDED_FILES
      ${CMAKE_CURRENT_BINARY_DIR}/netstim.cpp
      ${CMAKE_CURRENT_BINARY_DIR}/netstim_inhpoisson.cpp
      ${CMAKE_CURRENT_BINARY_DIR}/pattern.cpp
      ${CMAKE_CURRENT_SOURCE_DIR}/io/nrn_setup.cpp
      ${CMAKE_CURRENT_SOURCE_DIR}/io/setup_fornetcon.cpp
      ${CMAKE_CURRENT_SOURCE_DIR}/io/corenrn_data_return.cpp
      ${CMAKE_CURRENT_SOURCE_DIR}/io/global_vars.cpp)

  set_source_files_properties(${OPENACC_EXCLUDED_FILES} PROPERTIES COMPILE_FLAGS
                                                                   "-DDISABLE_OPENACC")
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
  if(CORENRN_ENABLE_NMODL AND EXISTS ${CORENRN_MOD2CPP_INCLUDE}/partial_piv_lu/partial_piv_lu.cpp)
    list(APPEND CORENEURON_CODE_FILES ${CORENRN_MOD2CPP_INCLUDE}/partial_piv_lu/partial_piv_lu.cpp)
    if(CORENRN_ENABLE_GPU
       AND CORENRN_HAVE_NVHPC_COMPILER
       AND CMAKE_BUILD_TYPE STREQUAL "Debug")
      # In this case OpenAccHelper.cmake passes -gpu=debug, which makes these Eigen functions
      # extremely slow. Downgrade that to -gpu=lineinfo for this file.
      set_source_files_properties(${CORENRN_MOD2CPP_INCLUDE}/partial_piv_lu/partial_piv_lu.cpp
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
if(CORENRN_ENABLE_MPI AND NOT CORENRN_ENABLE_MPI_DYNAMIC)
  add_library(${CORENRN_MPI_LIB_NAME} OBJECT ${MPI_LIB_FILES})
  target_include_directories(
    ${CORENRN_MPI_LIB_NAME} PRIVATE ${MPI_INCLUDE_PATH} ${CORENEURON_PROJECT_SOURCE_DIR}
                                    ${CORENEURON_PROJECT_BINARY_DIR}/generated)
  target_link_libraries(${CORENRN_MPI_LIB_NAME} ${CORENRN_CALIPER_LIB})
  set_property(TARGET ${CORENRN_MPI_LIB_NAME} PROPERTY POSITION_INDEPENDENT_CODE ON)
  set(CORENRN_MPI_OBJ $<TARGET_OBJECTS:${CORENRN_MPI_LIB_NAME}>)
endif()

# Library containing the bulk of the non-mechanism CoreNEURON code. This is always created and
# installed as a static library, and then the nrnivmodl-core workflow extracts the object files from
# it and does one of the following:
#
# * shared build: creates libcorenrnmech.so from these objects plus those from the translated MOD
#   files
# * static build: creates a (temporary, does not get installed) libcorenrnmech.a from these objects
#   plus those from the translated MOD files, then statically links that into special-core
#   (nrniv-core)
#
# This scheme means that both core and mechanism .o files are linked in a single step, which is
# important for GPU linking. It does, however, mean that the core code is installed twice, once in
# libcoreneuron-core.a and once in libcorenrnmech.so (shared) or nrniv-core (static). In a GPU
# build, libcoreneuron-cuda.{a,so} is also linked to provide the CUDA implementation of the Hines
# solver. This cannot be included in coreneuron-core because of this issue:
# https://forums.developer.nvidia.com/t/cannot-dynamically-load-a-shared-library-containing-both-openacc-and-cuda-code/210972
add_library(coreneuron-core STATIC ${CORENEURON_CODE_FILES} ${CORENRN_MPI_OBJ})
if(CORENRN_ENABLE_GPU)
  set(coreneuron_cuda_target coreneuron-cuda)
  add_library(coreneuron-cuda ${COMPILE_LIBRARY_TYPE} ${CORENEURON_CUDA_FILES})
  target_link_libraries(coreneuron-core PUBLIC coreneuron-cuda)
endif()

foreach(target coreneuron-core ${coreneuron_cuda_target})
  target_include_directories(${target} PRIVATE ${CORENEURON_PROJECT_SOURCE_DIR}
                                               ${CORENEURON_PROJECT_BINARY_DIR}/generated)
endforeach()

# we can link to MPI libraries in non-dynamic-mpi build
if(CORENRN_ENABLE_MPI AND NOT CORENRN_ENABLE_MPI_DYNAMIC)
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
if(CORENRN_ENABLE_MPI AND CORENRN_ENABLE_MPI_DYNAMIC)
  # store mpi library targets that will be built
  list(APPEND corenrn_mpi_targets "")

  # ~~~
  # if coreneuron is built as a submodule of neuron then check if NEURON has created
  # list of libraries that needs to be built. We use neuron cmake variables here because
  # we don't need to duplicate CMake code into coreneuron (we want to have unified cmake
  # project soon). In the absense of neuron just build a single library libcorenrn_mpi.
  # This is mostly used for the testing.
  # ~~~
  if(NOT CORENEURON_AS_SUBPROJECT)
    add_library(${CORENRN_MPI_LIB_NAME} SHARED ${MPI_LIB_FILES})
    target_link_libraries(${CORENRN_MPI_LIB_NAME} ${MPI_CXX_LIBRARIES})
    target_include_directories(
      ${CORENRN_MPI_LIB_NAME} PRIVATE ${MPI_INCLUDE_PATH} ${CORENEURON_PROJECT_SOURCE_DIR}
                                      ${CORENEURON_PROJECT_BINARY_DIR}/generated)
    set_property(TARGET ${CORENRN_MPI_LIB_NAME} PROPERTY POSITION_INDEPENDENT_CODE ON)
    list(APPEND corenrn_mpi_targets ${CORENRN_MPI_LIB_NAME})
  else()
    # ~~~
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
      target_link_libraries(core${libname}_lib ${CORENRN_CALIPER_LIB})
      target_include_directories(
        core${libname}_lib
        PUBLIC ${include}
        PRIVATE ${CORENEURON_PROJECT_SOURCE_DIR} ${CORENEURON_PROJECT_BINARY_DIR}/generated)

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
  endif()

  set_target_properties(
    ${corenrn_mpi_targets}
    PROPERTIES ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
               LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
               POSITION_INDEPENDENT_CODE ON)
  install(TARGETS ${corenrn_mpi_targets} DESTINATION lib)
endif()

# Suppress some compiler warnings.
target_compile_options(coreneuron-core PRIVATE ${CORENEURON_CXX_WARNING_SUPPRESSIONS})
target_link_libraries(coreneuron-core PUBLIC ${reportinglib_LIBRARY} ${sonatareport_LIBRARY}
                                             ${CORENRN_CALIPER_LIB} ${likwid_LIBRARIES})

# TODO: fix adding a dependency of coreneuron-core on CLI11::CLI11 when CLI11 is a submodule. Right
# now this doesn't work because the CLI11 targets are not exported/installed but coreneuron-core is.
get_target_property(CLI11_HEADER_DIRECTORY CLI11::CLI11 INTERFACE_INCLUDE_DIRECTORIES)
target_include_directories(
  coreneuron-core SYSTEM PRIVATE ${CLI11_HEADER_DIRECTORY}
                                 ${CORENEURON_PROJECT_SOURCE_DIR}/external/Random123/include)

# See: https://en.cppreference.com/w/cpp/filesystem#Notes
if(CMAKE_CXX_COMPILER_IS_GCC AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 9.1)
  target_link_libraries(coreneuron-core PUBLIC stdc++fs)
endif()

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
# create special-core with halfgap.mod for tests
# =============================================================================
set(modfile_directory "${CORENEURON_PROJECT_SOURCE_DIR}/tests/integration/ring_gap/mod files")
file(GLOB modfiles "${modfile_directory}/*.mod")

# We have to link things like unit tests against this because some "core" .cpp files refer to
# symbols in the translated versions of default .mod files
set(nrniv_core_prefix "${CMAKE_BINARY_DIR}/bin/${CMAKE_SYSTEM_PROCESSOR}")
set(corenrn_mech_library
    "${nrniv_core_prefix}/${CMAKE_${COMPILE_LIBRARY_TYPE}_LIBRARY_PREFIX}corenrnmech${CMAKE_${COMPILE_LIBRARY_TYPE}_LIBRARY_SUFFIX}"
)
set(output_binaries "${nrniv_core_prefix}/special-core" "${corenrn_mech_library}")

add_custom_command(
  OUTPUT ${output_binaries}
  DEPENDS coreneuron-core ${NMODL_TARGET_TO_DEPEND} ${modfiles} ${CORENEURON_BUILTIN_MODFILES}
  COMMAND ${CMAKE_BINARY_DIR}/bin/nrnivmodl-core -b ${COMPILE_LIBRARY_TYPE} -m
          ${CORENRN_MOD2CPP_BINARY} -p 4 "${modfile_directory}"
  WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/bin
  COMMENT "Running nrnivmodl-core with halfgap.mod")
add_custom_target(nrniv-core ALL DEPENDS ${output_binaries})

# Build a target representing the libcorenrnmech.so that is produced under bin/x86_64, which
# executables such as the unit tests must link against
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
# ${corenrn_mech_library} is libcorenrnmech.{a,so}, which contains both the compiled default
# mechanisms and the content of libcoreneuron-core.a.
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

configure_file("${CORENEURON_PROJECT_BINARY_DIR}/generated/coreneuron/config/neuron_version.hpp"
               "${CMAKE_BINARY_DIR}/include/coreneuron/config/neuron_version.hpp" COPYONLY)
foreach(header ${main_headers})
  configure_file("${header}" "${CMAKE_BINARY_DIR}/include/coreneuron/${header}" COPYONLY)
endforeach()

configure_file("utils/profile/profiler_interface.h"
               ${CMAKE_BINARY_DIR}/include/coreneuron/nrniv/profiler_interface.h COPYONLY)

# main program required for building special-core
file(COPY apps/coreneuron.cpp DESTINATION ${CMAKE_BINARY_DIR}/share/coreneuron)

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

# copy mod2c/nmodl for nrnivmodl-core
install(PROGRAMS ${CORENRN_MOD2CPP_BINARY} DESTINATION bin)

if(NOT CORENRN_ENABLE_NMODL)
  install(FILES ${NMODL_UNITS_FILE} DESTINATION share/mod2c)
endif()

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
