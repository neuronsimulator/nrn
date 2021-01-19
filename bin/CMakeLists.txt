# =============================================================================
# Copyright (C) 2016-2021 Blue Brain Project
#
# See top-level LICENSE file for details.
# =============================================================================

# =============================================================================
# Copy first into build directory as it will be used for special-core
# =============================================================================
configure_file(nrnivmodl_core_makefile.in
               ${CMAKE_BINARY_DIR}/share/coreneuron/nrnivmodl_core_makefile @ONLY)
configure_file(nrnivmodl-core.in ${CMAKE_BINARY_DIR}/bin/nrnivmodl-core @ONLY)

# =============================================================================
# Install for end users
# =============================================================================
install(FILES ${CMAKE_BINARY_DIR}/share/coreneuron/nrnivmodl_core_makefile
        DESTINATION share/coreneuron)
install(PROGRAMS ${CMAKE_BINARY_DIR}/bin/nrnivmodl-core DESTINATION bin)
