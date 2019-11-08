# =============================================================================
# Copyright (C) 2016-2019 Blue Brain Project
#
# See top-level LICENSE file for details.
# =============================================================================

# coreneuron executable
include_directories(${CORENEURON_PROJECT_SOURCE_DIR})
add_executable(nrniv-core "${CMAKE_CURRENT_SOURCE_DIR}/coreneuron.cpp")
target_link_libraries(nrniv-core
                      corenrnmech
                      ${MPI_C_LIBRARIES}
                      ${MPI_CXX_LIBRARIES}
                      ${reportinglib_LIBRARY}
                      ${sonatareport_LIBRARY})
set_target_properties(nrniv-core PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# main program required for building special-core
configure_file(coreneuron.cpp ${CMAKE_BINARY_DIR}/share/coreneuron/coreneuron.cpp COPYONLY)

# install targes
install(TARGETS nrniv-core DESTINATION bin)
install(FILES coreneuron.cpp DESTINATION share/coreneuron)
