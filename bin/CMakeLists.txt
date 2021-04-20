# =============================================================================
# Copyright (c) 2016 - 2021 Blue Brain Project/EPFL
#
# See top-level LICENSE file for details.
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
