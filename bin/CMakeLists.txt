# =============================================================================
# Set various variables used in template files
# =============================================================================
# TODO: for nrn.defaults but these are repeated in cmake_config/CMakeLists.txt
set(modsubdir ${CMAKE_SYSTEM_PROCESSOR})
set(NRNMECH_DLL_STYLE_FALSE "\#")
set(NRNMECH_DLL_STYLE_TRUE "")

set(nrndef_unix "//")
set(nrndef_mac "//")
set(nrndef_mswin "//")
set(MAC_DARWIN_TRUE "\#")
set(MAC_DARWIN_FALSE "")
if (NRN_LINUX_BUILD)
  set(nrndef_unix "")
elseif (NRN_MACOS_BUILD)
  set(nrndef_mac "")
  set(MAC_DARWIN_TRUE "")
  set(MAC_DARWIN_FALSE "\#")
elseif (NRN_WINDOWS_BUILD)
  set(nrndef_mswin "")
endif()

# for nrnmech_makefile
set(CMAKE_INSTALL_DATADIR share/nrn)

# =============================================================================
# Inclode nrnivmodl Makefile generator, adapted from CoreNEURON
# =============================================================================
include(CMakeListsNrnMech)

# =============================================================================
# nrnmech_makefile.in is bound up with autoconf, used based on coreneuron Configure templates
# =============================================================================
nrn_configure_file(nrngui bin)
nrn_configure_file(neurondemo bin)
nrn_configure_file(nrnivmodl bin)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/nrnivmodl_makefile_cmake.in
               ${PROJECT_BINARY_DIR}/bin/nrnmech_makefile @ONLY)

# =============================================================================
# Install targets
# =============================================================================
install(PROGRAMS ${PROJECT_BINARY_DIR}/bin/nrngui ${PROJECT_BINARY_DIR}/bin/neurondemo
                 ${PROJECT_BINARY_DIR}/bin/nrnivmodl
        DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)

install(FILES ${PROJECT_BINARY_DIR}/bin/nrnmech_makefile DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/nrnpyenv.sh
              ${CMAKE_CURRENT_SOURCE_DIR}/sortspike
              ${CMAKE_CURRENT_SOURCE_DIR}/mkthreadsafe
              ${CMAKE_CURRENT_SOURCE_DIR}/nrnpyenv.sh
        DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)
