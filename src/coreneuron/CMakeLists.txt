# =============================================================================
# Copyright (c) 2016 - 2021 Blue Brain Project/EPFL
#
# See top-level LICENSE file for details.
# =============================================================================

# commonly included directories
include_directories(
  utils/randoms ${MPI_INCLUDE_PATH} ${PROJECT_SOURCE_DIR} ${PROJECT_BINARY_DIR}/coreneuron
  ${CMAKE_BINARY_DIR}/coreneuron ${CMAKE_BINARY_DIR}/include)

# put libraries (e.g. dll) in bin directory
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# =============================================================================
# gather various source files
# =============================================================================
file(GLOB_RECURSE CORENEURON_HEADER_FILES "*.h*")
file(GLOB_RECURSE CORENEURON_TEMPLATE_FILES "*.ipp")
file(
  GLOB
  CORENEURON_CODE_FILES
  "apps/main1.cpp"
  "apps/corenrn_parameters.cpp"
  "gpu/*.cpp"
  "io/*.cpp"
  "io/reports/*.cpp"
  "mechanism/*.cpp"
  "mpi/*.cpp"
  "network/*.cpp"
  "permute/*.cpp"
  "sim/*.cpp"
  "utils/*.cpp"
  "utils/*/*.c"
  "utils/*/*.cpp")
file(GLOB_RECURSE CORENEURON_CUDA_FILES "*.cu")
file(GLOB SCOPMATH_CODE_FILES "sim/scopmath/*.cpp")
file(COPY ${CORENEURON_PROJECT_SOURCE_DIR}/external/Random123/include/Random123
     DESTINATION ${CMAKE_BINARY_DIR}/include)
list(APPEND CORENEURON_CODE_FILES ${PROJECT_BINARY_DIR}/coreneuron/config/config.cpp)

set(DIMPLIC_CODE_FILE "mechanism/mech/dimplic.cpp")
set(ENGINEMECH_CODE_FILE "mechanism/mech/enginemech.cpp")

# for external mod files we need to generate modl_ref function in mod_func.c
set(MODFUNC_PERL_SCRIPT "mechanism/mech/mod_func.c.pl")

# ~~~
# as openacc limits passing function pointers as arguments, mod2c translated files
# depend on a _kinderiv.h file that is constructed by kinderiv.py
# ~~~
set(KINDERIV_PYTHON_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/kinderiv.py")
set(KINDERIV_HEADER_FILE "${CMAKE_CURRENT_BINARY_DIR}/_kinderiv.h")

set(NMODL_UNITS_FILE "${CMAKE_BINARY_DIR}/share/mod2c/nrnunits.lib")

# =============================================================================
# Copy files that are required by nrnivmodl-core to the build tree at build time.
# =============================================================================
cpp_cc_build_time_copy(
  INPUT "${CMAKE_CURRENT_SOURCE_DIR}/${MODFUNC_PERL_SCRIPT}"
  OUTPUT "${CMAKE_BINARY_DIR}/share/coreneuron/mod_func.c.pl"
  NO_TARGET)
cpp_cc_build_time_copy(
  INPUT "${KINDERIV_PYTHON_SCRIPT}"
  OUTPUT "${CMAKE_BINARY_DIR}/share/coreneuron/kinderiv.py"
  NO_TARGET)
cpp_cc_build_time_copy(
  INPUT "${CMAKE_CURRENT_SOURCE_DIR}/${DIMPLIC_CODE_FILE}"
  OUTPUT "${CMAKE_BINARY_DIR}/share/coreneuron/dimplic.cpp"
  NO_TARGET)
cpp_cc_build_time_copy(
  INPUT "${CMAKE_CURRENT_SOURCE_DIR}/${ENGINEMECH_CODE_FILE}"
  OUTPUT "${CMAKE_BINARY_DIR}/share/coreneuron/enginemech.cpp"
  NO_TARGET)
set(nrnivmodl_core_dependencies
    "${CMAKE_BINARY_DIR}/share/coreneuron/mod_func.c.pl"
    "${CMAKE_BINARY_DIR}/share/coreneuron/kinderiv.py"
    "${CMAKE_BINARY_DIR}/share/coreneuron/dimplic.cpp"
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
      ${CMAKE_CURRENT_SOURCE_DIR}/utils/randoms/nrnran123.cpp
      ${CMAKE_CURRENT_SOURCE_DIR}/io/nrn_setup.cpp
      ${CMAKE_CURRENT_SOURCE_DIR}/io/setup_fornetcon.cpp
      ${CMAKE_CURRENT_SOURCE_DIR}/io/corenrn_data_return.cpp
      ${CMAKE_CURRENT_SOURCE_DIR}/io/global_vars.cpp)

  set_source_files_properties(${OPENACC_EXCLUDED_FILES} PROPERTIES COMPILE_FLAGS
                                                                   "-DDISABLE_OPENACC")
  # nrnran123.cpp is a symlink to nrnran123.cu, in GPU builds we compile this as CUDA code (so we
  # want to remove the .cpp here), while in non-GPU builds we compile it as plain C++. Unfortunately
  # CMake <v3.20 does not pass explicit -x <lang> options based on the LANGUAGE property
  # (https://cmake.org/cmake/help/latest/policy/CMP0119.html), so using a single .cu file and
  # setting LANGUAGE=CXX in non-GPU builds does not work.
  list(REMOVE_ITEM CORENEURON_CODE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/utils/randoms/nrnran123.cpp")
  list(APPEND CORENEURON_CODE_FILES ${CORENEURON_CUDA_FILES})
endif()

# =============================================================================
# eion.cpp depends on CORENRN_USE_LEGACY_UNITS
# =============================================================================
set(LegacyFR_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/mechanism/eion.cpp ${CMAKE_CURRENT_SOURCE_DIR}/apps/main1.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/io/global_vars.cpp)

set_property(
  SOURCE ${LegacyFR_FILES}
  APPEND
  PROPERTY COMPILE_DEFINITIONS "CORENRN_USE_LEGACY_UNITS=${CORENRN_USE_LEGACY_UNITS}")

# =============================================================================
# run KINDERIV_PYTHON_SCRIPT to generate _kinderiv.h
# =============================================================================
add_custom_command(
  OUTPUT "${KINDERIV_HEADER_FILE}"
  DEPENDS ${NMODL_INBUILT_MOD_OUTPUTS} "${KINDERIV_PYTHON_SCRIPT}"
  COMMAND ${PYTHON_EXECUTABLE} ${KINDERIV_PYTHON_SCRIPT} ${CMAKE_CURRENT_BINARY_DIR}
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMENT "Generating kinderiv.h by inspecting MOD files")
add_custom_target(kin_deriv_header DEPENDS "${KINDERIV_HEADER_FILE}")

# =============================================================================
# create libraries
# =============================================================================

# main coreneuron library
add_library(
  coreneuron ${COMPILE_LIBRARY_TYPE} ${CORENEURON_HEADER_FILES} ${CORENEURON_TEMPLATE_FILES}
             ${CORENEURON_CODE_FILES} ${cudacorenrn_objs} ${NMODL_INBUILT_MOD_OUTPUTS})
# Prevent CMake from running a device code link step when assembling libcoreneuron.a in GPU builds.
# The device code linking needs to be deferred to the final step, where it is done by `nvc++ -cuda`.
set_target_properties(coreneuron PROPERTIES CUDA_SEPARABLE_COMPILATION ON)

# need to have _kinderiv.h for mod2c generated files and nrnivmodl-core and nmodl building
add_dependencies(coreneuron kin_deriv_header nrnivmodl-core)

# scopmath is created separately for nrnivmodl-core workflow
add_library(scopmath STATIC ${CORENEURON_HEADER_FILES} ${SCOPMATH_CODE_FILES})

target_link_libraries(coreneuron ${reportinglib_LIBRARY} ${sonatareport_LIBRARY} ${CALIPER_LIB}
                      ${likwid_LIBRARIES} ${MPI_C_LIBRARIES})
target_include_directories(coreneuron SYSTEM
                           PRIVATE ${CORENEURON_PROJECT_SOURCE_DIR}/external/Random123/include)
target_include_directories(coreneuron SYSTEM
                           PRIVATE ${CORENEURON_PROJECT_SOURCE_DIR}/external/CLI11/include)

set_target_properties(
  coreneuron scopmath
  PROPERTIES ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
             LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
             POSITION_INDEPENDENT_CODE ON)

# =============================================================================
# create special-core with halfgap.mod for tests
# =============================================================================
set(modfile_directory "${CORENEURON_PROJECT_SOURCE_DIR}/tests/integration/ring_gap/mod")
file(GLOB modfiles "${modfile_directory}/*.mod")
set(output_binaries "${CMAKE_BINARY_DIR}/bin/${CMAKE_SYSTEM_PROCESSOR}/special-core"
                    "${CMAKE_BINARY_DIR}/bin/${CMAKE_SYSTEM_PROCESSOR}/libcorenrnmech.a")
add_custom_command(
  OUTPUT ${output_binaries}
  DEPENDS scopmath coreneuron ${NMODL_TARGET_TO_DEPEND} ${modfiles} ${CORENEURON_BUILTIN_MODFILES}
  COMMAND ${CMAKE_BINARY_DIR}/bin/nrnivmodl-core -b STATIC -m ${CORENRN_MOD2CPP_BINARY} -p 1
          "${modfile_directory}"
  WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/bin
  COMMENT "Running nrnivmodl-core with halfgap.mod")
add_custom_target(nrniv-core ALL DEPENDS ${output_binaries})

include_directories(${CORENEURON_PROJECT_SOURCE_DIR})

# =============================================================================
# Extract link definitions to be used with nrnivmodl-core
# =============================================================================
get_target_property(CORENRN_LINK_LIBS coreneuron LINK_LIBRARIES)
if(NOT CORENRN_LINK_LIBS)
  set(CORENRN_LINK_LIBS "")
endif()
set(CORENRN_LINK_LIBS
    "${CORENRN_LINK_LIBS}"
    PARENT_SCOPE)

# Make headers avail to build tree
configure_file(engine.h.in ${CMAKE_BINARY_DIR}/include/coreneuron/engine.h @ONLY)

file(
  GLOB_RECURSE main_headers
  RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
  *.h *.hpp *.ispc)

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
  TARGETS coreneuron
  EXPORT coreneuron
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
  INCLUDES
  DESTINATION $<INSTALL_INTERFACE:include>)

# scopemath into share for nrnivmodl-core
install(
  TARGETS scopmath
  EXPORT coreneuron
  DESTINATION lib)

# headers and some standalone code files for nrnivmodl-core
install(
  DIRECTORY ${CMAKE_BINARY_DIR}/include/coreneuron
  DESTINATION include/
  FILES_MATCHING
  PATTERN "*.h*"
  PATTERN "*.ipp"
  PATTERN "*.ispc")
install(FILES ${KINDERIV_PYTHON_SCRIPT} ${MODFUNC_PERL_SCRIPT} ${DIMPLIC_CODE_FILE}
              ${ENGINEMECH_CODE_FILE} DESTINATION share/coreneuron)

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

# install random123 and nmodl headers
install(DIRECTORY ${CMAKE_BINARY_DIR}/include/ DESTINATION include)

# install mod files
install(DIRECTORY ${CMAKE_BINARY_DIR}/share/modfile DESTINATION share)
