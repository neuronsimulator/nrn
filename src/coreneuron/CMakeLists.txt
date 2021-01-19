# =============================================================================
# Copyright (C) 2016-2021 Blue Brain Project
#
# See top-level LICENSE file for details.
# =============================================================================

# commonly included directories
include_directories(utils/randoms ${MPI_INCLUDE_PATH} ${PROJECT_BINARY_DIR}/coreneuron
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
  "sim/*.cpp")
file(GLOB_RECURSE CORENEURON_UTILS_FILES "utils/*.c*")
list(APPEND CORENEURON_CODE_FILES ${CORENEURON_UTILS_FILES})
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

# copy inbuilt mod files to share
file(COPY ${CORENEURON_PROJECT_SOURCE_DIR}/coreneuron/mechanism/mech/modfile
     DESTINATION ${CMAKE_BINARY_DIR}/share)

# =============================================================================
# coreneuron GPU library
# =============================================================================
if(CORENRN_ENABLE_GPU)
  # ~~~
  # artificial cells and some other cpp files (using Random123) should be compiled
  # without OpenACC to avoid use of GPU Random123 streams
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

  set_source_files_properties(${DIMPLIC_CODE_FILE} ${NMODL_INBUILT_MOD_OUTPUTS} PROPERTIES
                              COMPILE_FLAGS "")

  set_source_files_properties(${OPENACC_EXCLUDED_FILES} PROPERTIES COMPILE_FLAGS
                              "-DDISABLE_OPENACC")

  # compile cuda files for multiple architecture
  cuda_add_library(
    "cudacoreneuron" ${CORENEURON_CUDA_FILES}
    OPTIONS -arch=sm_60
            -gencode=arch=compute_60,code=sm_60
            -gencode=arch=compute_72,code=sm_72
            -gencode=arch=compute_52,code=compute_52
            -Xcompiler
            -fPIC)
  set(link_cudacoreneuron cudacoreneuron)
else()
  set(link_cudacoreneuron "")
endif()

# =============================================================================
# eion.cpp depends on CORENRN_USE_LEGACY_UNITS
# =============================================================================
set(LegacyFR_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/mechanism/eion.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/apps/main1.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/io/global_vars.cpp)

set_property(SOURCE ${LegacyFR_FILES} APPEND PROPERTY COMPILE_DEFINITIONS
                              "CORENRN_USE_LEGACY_UNITS=${CORENRN_USE_LEGACY_UNITS}")

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

# need to have _kinderiv.h for mod2c generated files
add_dependencies(coreneuron kin_deriv_header)

# scopmath is created separately for nrnivmodl-core workflow
add_library(scopmath STATIC ${CORENEURON_HEADER_FILES} ${SCOPMATH_CODE_FILES})

target_link_libraries(
  coreneuron
  ${reportinglib_LIBRARY}
  ${sonatareport_LIBRARY}
  ${link_cudacoreneuron}
  ${CUDA_LIBRARIES}
  ${CALIPER_LIB}
  ${CALIPER_MPI_LIB}
  ${likwid_LIBRARIES}
  ${MPI_C_LIBRARIES})
target_include_directories(coreneuron SYSTEM
                           PRIVATE ${CORENEURON_PROJECT_SOURCE_DIR}/external/Random123/include)
target_include_directories(coreneuron SYSTEM
                           PRIVATE ${CORENEURON_PROJECT_SOURCE_DIR}/external/CLI11/include)

set_target_properties(
  coreneuron scopmath ${link_cudacoreneuron}
  PROPERTIES ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib LIBRARY_OUTPUT_DIRECTORY
             ${CMAKE_BINARY_DIR}/lib POSITION_INDEPENDENT_CODE ON)

# =============================================================================
# create special-core with halfgap.mod for tests
# =============================================================================
add_custom_target(
  nrniv-core ALL
  COMMAND ${CMAKE_BINARY_DIR}/bin/nrnivmodl-core -b STATIC
          -m ${CORENRN_MOD2CPP_BINARY}
          -p 1
          ${CORENEURON_PROJECT_SOURCE_DIR}/tests/integration/ring_gap/mod
  WORKING_DIRECTORY
    ${CMAKE_BINARY_DIR}/bin
    BYPRODUCTS
    ${CMAKE_BINARY_DIR}/bin/${CMAKE_SYSTEM_PROCESSOR}/special-core
  COMMENT "Running nrnivmodl-core with halfgap.mod")
add_dependencies(nrniv-core scopmath coreneuron ${NMODL_TARGET_TO_DEPEND})

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

# =============================================================================
# Copy files for nrnivmodl-core workflow during build time
# =============================================================================

configure_file(${MODFUNC_PERL_SCRIPT} ${CMAKE_BINARY_DIR}/share/coreneuron/mod_func.c.pl COPYONLY)
configure_file(${KINDERIV_PYTHON_SCRIPT} ${CMAKE_BINARY_DIR}/share/coreneuron/kinderiv.py COPYONLY)
configure_file(${DIMPLIC_CODE_FILE} ${CMAKE_BINARY_DIR}/share/coreneuron/dimplic.cpp COPYONLY)
configure_file(${ENGINEMECH_CODE_FILE} ${CMAKE_BINARY_DIR}/share/coreneuron/enginemech.cpp COPYONLY)

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
  TARGETS coreneuron ${link_cudacoreneuron}
  EXPORT coreneuron
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
  INCLUDES DESTINATION $<INSTALL_INTERFACE:include>)

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
