# ===================================================================
# MAC after_install and macpkg targets
# ===================================================================

if (NRN_MACOS_BUILD)
  # Sets up icons, apps, and activates them
  install(CODE
    "execute_process(COMMAND sh ${CMAKE_CURRENT_SOURCE_DIR}/activate-apps-cmake.sh \"x86_64\" \"${CMAKE_INSTALL_PREFIX}\" \"${PROJECT_SOURCE_DIR}\" \"${CMAKE_INSTALL_PREFIX}/lib\")")

  set(MACPKG_INSTALL_PATH "/Applications/NEURON-${PROJECT_VERSION}")
  if (${CMAKE_INSTALL_PREFIX} STREQUAL ${MACPKG_INSTALL_PATH})
    # should only be invoked after a 'make install'. Creates signed pkg.
    add_custom_target(macpkg
      COMMAND sh ${CMAKE_CURRENT_SOURCE_DIR}/clean-for-pkg-cmake.sh "${CMAKE_INSTALL_PREFIX}"
      COMMAND sh ${CMAKE_CURRENT_SOURCE_DIR}/nrn_x11_opt2usr.sh "${CMAKE_INSTALL_PREFIX}" "x86_64"
      COMMAND /usr/local/bin/packagesbuild ${CMAKE_CURRENT_BINARY_DIR}/macdist.pkgproj
      COMMAND sh ${CMAKE_CURRENT_SOURCE_DIR}/nrnproductsign.sh ${CMAKE_CURRENT_BINARY_DIR}/build/NEURON-${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.pkg
      DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/macdist.pkgproj
      WORKING_DIRECTORY ${PROJECT_BINARY_DIR})
  else()
    add_custom_target(macpkg
      COMMAND echo "CMAKE_INSTALL_PREFIX must be \"${MACPKG_INSTALL_PATH}\""
      COMMAND exit 1
    )
  endif()
endif()
