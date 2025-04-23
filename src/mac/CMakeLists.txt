# ===================================================================
# MAC after_install and macpkg targets
# ===================================================================

if(NRN_MACOS_BUILD)
  # Sets up icons, apps, and activates them If universal build, definitely needs to run after (or at
  # end of) install, so we create a macro and run at end of top level CMakeLists.txt
  macro(nrn_macos_after_install)
    install(
      CODE "execute_process(COMMAND sh ${PROJECT_SOURCE_DIR}/src/mac/activate-apps-cmake.sh \"CMAKE_SYSTEM_PROCESSOR\" \"${CMAKE_INSTALL_PREFIX}\" \"${PROJECT_SOURCE_DIR}\" \"${CMAKE_INSTALL_PREFIX}/lib\" \"${NRN_UNIVERSAL2_BUILD}\")"
    )
  endmacro()

  set(MACPKG_INSTALL_PATH "/Applications/NEURON")
  if(${CMAKE_INSTALL_PREFIX} STREQUAL ${MACPKG_INSTALL_PATH})
    # should only be invoked after a 'make install'. Creates signed pkg.
    set(PKG_NAME "NEURON")
    set(PKG_PATH "${CMAKE_CURRENT_BINARY_DIR}/build/${PKG_NAME}.pkg")
    add_custom_target(
      macpkg
      COMMAND sh ${CMAKE_CURRENT_SOURCE_DIR}/clean-for-pkg-cmake.sh "${CMAKE_INSTALL_PREFIX}"
      COMMAND sh ${CMAKE_CURRENT_SOURCE_DIR}/nrn_x11_opt2usr.sh "${CMAKE_INSTALL_PREFIX}" "x86_64"
      COMMAND sh ${CMAKE_CURRENT_SOURCE_DIR}/nrn_codesign.sh "${CMAKE_INSTALL_PREFIX}"
              "${CMAKE_CURRENT_SOURCE_DIR}"
      COMMAND /usr/local/bin/packagesbuild ${CMAKE_CURRENT_BINARY_DIR}/macdist.pkgproj
      COMMAND sh ${CMAKE_CURRENT_SOURCE_DIR}/nrn_productsign.sh "${PKG_PATH}"
      COMMAND sh ${CMAKE_CURRENT_SOURCE_DIR}/nrn_notarize.sh "${PKG_PATH}" "${PKG_NAME}"
      DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/macdist.pkgproj
      WORKING_DIRECTORY ${PROJECT_BINARY_DIR})
  else()
    add_custom_target(
      macpkg
      COMMAND echo "CMAKE_INSTALL_PREFIX must be \"${MACPKG_INSTALL_PATH}\""
      COMMAND exit 1)
  endif()
endif()
