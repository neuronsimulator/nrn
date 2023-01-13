# =============================================================================
# Set various variables used in template files
# =============================================================================
# TODO: for nrn.defaults but these are repeated in cmake_config/CMakeLists.txt
set(modsubdir ${CMAKE_SYSTEM_PROCESSOR})

set(nrndef_unix "//")
set(nrndef_mac "//")
set(nrndef_mswin "//")
set(MAC_DARWIN_TRUE "\#")
set(MAC_DARWIN_FALSE "")
if(NRN_LINUX_BUILD)
  set(nrndef_unix "")
elseif(NRN_MACOS_BUILD)
  set(nrndef_mac "")
  set(MAC_DARWIN_TRUE "")
  set(MAC_DARWIN_FALSE "\#")
elseif(NRN_WINDOWS_BUILD)
  set(nrndef_mswin "")
endif()

# for nrnmech_makefile
set(CMAKE_INSTALL_BINDIR bin)
set(CMAKE_INSTALL_LIBDIR lib)
set(CMAKE_INSTALL_INCLUDEDIR include)
set(CMAKE_INSTALL_DATADIR share/nrn)

# =============================================================================
# Include nrnivmodl makefile generator
# =============================================================================
include(CMakeListsNrnMech)

# =============================================================================
# nrnmech_makefile (based on coreneuron Configure templates)
# =============================================================================
nrn_configure_file(nrngui bin)
configure_file(sortspike sortspike COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/nrnivmodl_makefile_cmake.in
               ${PROJECT_BINARY_DIR}/bin/nrnmech_makefile @ONLY)

# if running from the build folder (e.g. make test) may need this.
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/nrnpyenv.sh DESTINATION ${PROJECT_BINARY_DIR}/bin)
# Make sure nrnivmodl and neurondemo are executable in the build folder, so we can execute it to
# prepare test files. This can be done more elegantly in newer CMake versions; v3.19+ have
# file(CHMOD ...) and v3.20+ support setting permissions directly in configure_file(...).
set(NRN_CONFIG_EXE_FILES "nrnivmodl" "neurondemo")
foreach(NRN_CONFIG_EXE_FILE ${NRN_CONFIG_EXE_FILES})
  nrn_configure_dest_src(${NRN_CONFIG_EXE_FILE} bin/tmp ${NRN_CONFIG_EXE_FILE} bin)
  file(
    COPY ${PROJECT_BINARY_DIR}/bin/tmp/${NRN_CONFIG_EXE_FILE}
    DESTINATION ${PROJECT_BINARY_DIR}/bin
    FILE_PERMISSIONS
      OWNER_READ
      OWNER_WRITE
      OWNER_EXECUTE
      GROUP_READ
      GROUP_EXECUTE
      WORLD_READ
      WORLD_EXECUTE)
endforeach()
if(NRN_ENABLE_TESTS)
  # Execute neurondemo as part of the build because it lazily calls nrnivmodl. If we don't do this
  # then test_neurondemo.py will run nrnivmodl in a clean build, and report compilation warnings as
  # unexpected stderr.
  set(neurondemo_prefix
      "${PROJECT_BINARY_DIR}/share/nrn/demo/release/${CMAKE_HOST_SYSTEM_PROCESSOR}")
  set(neurondemo_files
      "${neurondemo_prefix}/special"
      "${neurondemo_prefix}/${CMAKE_SHARED_LIBRARY_PREFIX}nrnmech${CMAKE_SHARED_LIBRARY_SUFFIX}")
  add_custom_command(
    OUTPUT ${neurondemo_files}
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${PROJECT_SOURCE_DIR}/share/demo
            ${PROJECT_BINARY_DIR}/share/nrn/demo
    COMMAND
      ${CMAKE_COMMAND} -E env ${NRN_RUN_FROM_BUILD_DIR_ENV} ${NRN_SANITIZER_ENABLE_ENVIRONMENT}
      "${PROJECT_BINARY_DIR}/bin/neurondemo" "-nobanner" "-nogui" "-c" "quit()"
    VERBATIM)
  add_custom_target(generate-neurondemo-mechanism-library ALL DEPENDS ${neurondemo_files} nrniv_lib)
endif()
file(REMOVE_RECURSE ${PROJECT_BINARY_DIR}/bin/tmp)

# =============================================================================
# Install targets
# =============================================================================
install(PROGRAMS ${PROJECT_BINARY_DIR}/bin/nrngui ${PROJECT_BINARY_DIR}/bin/neurondemo
                 ${PROJECT_BINARY_DIR}/bin/nrnivmodl DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)

install(FILES ${PROJECT_BINARY_DIR}/bin/nrnmech_makefile DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)
install(
  PROGRAMS ${CMAKE_CURRENT_SOURCE_DIR}/nrnpyenv.sh ${CMAKE_CURRENT_BINARY_DIR}/sortspike
           ${CMAKE_CURRENT_SOURCE_DIR}/mkthreadsafe ${CMAKE_CURRENT_SOURCE_DIR}/nrnpyenv.sh
           ${CMAKE_CURRENT_SOURCE_DIR}/set_nrnpyenv.sh DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)
