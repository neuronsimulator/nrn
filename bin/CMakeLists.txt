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


# =============================================================================
# CoreNEURON related configuration
# =============================================================================

# =============================================================================
# Copy first into build directory as it will be used for special-core
# =============================================================================
configure_file(nrnivmodl_core_makefile.in
               ${CMAKE_BINARY_DIR}/share/coreneuron/nrnivmodl_core_makefile @ONLY)
configure_file(nrnivmodl-core.in ${CMAKE_BINARY_DIR}/bin/nrnivmodl-core @ONLY)
# nrnivmodl-core depends on the building of NMODL_TARGET_TO_DEPEND and the configuration of the
# nrnivmodl-core and nrnivmodl_core_makefile this doesn't imply that whenever there is a change in
# one of those files then the prebuilt mod files are going to be rebuilt
add_custom_target(
  nrnivmodl-core ALL
  DEPENDS ${CMAKE_BINARY_DIR}/bin/nrnivmodl-core
          ${CMAKE_BINARY_DIR}/share/coreneuron/nrnivmodl_core_makefile ${NMODL_TARGET_TO_DEPEND})

# =============================================================================
# Install for end users
# =============================================================================
install(FILES ${CMAKE_BINARY_DIR}/share/coreneuron/nrnivmodl_core_makefile
        DESTINATION share/coreneuron)
install(PROGRAMS ${CMAKE_BINARY_DIR}/bin/nrnivmodl-core DESTINATION bin)
