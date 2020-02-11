# ===================================================================
# MAC after_install and macpkg targets
# ===================================================================

if (NRN_MACOS_BUILD)
  # should only be invoked after a 'make install'. Sets up icons and
  # other housecleaning for an installation ready to to make a
  # distribution package.
  add_custom_target(after-install
    COMMAND sh ${CMAKE_CURRENT_SOURCE_DIR}/after-cmake-install.sh "x86_64" "${CMAKE_INSTALL_PREFIX}" "${PROJECT_SOURCE_DIR}" "${CMAKE_INSTALL_PREFIX}/lib"
    WORKING_DIRECTORY ${PROJECT_BINARY_DIR})

  # should only be invoked after a 'make after install'. Creates signed pkg
  add_custom_target(macpkg
    COMMAND sh ${CMAKE_CURRENT_SOURCE_DIR}/nrn_x11_opt2usr.sh "${CMAKE_INSTALL_PREFIX}" "x86_64"
    COMMAND /usr/local/bin/packagesbuild ${CMAKE_CURRENT_BINARY_DIR}/macdist.pkgproj
    COMMAND sh ${CMAKE_CURRENT_SOURCE_DIR}/nrnproductsign.sh ${CMAKE_CURRENT_BINARY_DIR}/build/NEURON-${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.pkg
    DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/macdist.pkgproj
    WORKING_DIRECTORY ${PROJECT_BINARY_DIR})
endif()
