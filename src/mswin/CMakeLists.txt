# =============================================================================
# Build a Windows setup.exe distribution.
# =============================================================================

# ~~~
#  The prerequisite for 'make setup_exe' is 'make install'.
#  -DCMAKE_INSTALL_PREFIX=/C/marshalnrn/nrn is recommended.
#  ... because nrnmingwenv.sh checks for the optional existence of
#  ${CMAKE_INSTALL_PREFIX}/../nrnhtml
#
#  A setup.exe is generally built using the configuration
#  /mingw64/bin/cmake .. -G "Unix Makefiles" \
#    -DCMAKE_INSTALL_PREFIX=/C/marshalnrn/nrn \
#    -DIV_DIR=/home/hines/neuron/ivcmake/build/install \
#    -DPYTHON_EXECUTABLE=/e/python38/python \
#    -DNRN_ENABLE_PYTHON_DYNAMIC=ON \
#    -DNRN_PYTHON_DYNAMIC="e:/python38/python;e:/python39/python;e:/python310/python;e:/python311/python" \
#    -DNRN_ENABLE_MPI_DYNAMIC=ON \
#    -DCMAKE_PREFIX_PATH="/c/ms-mpi" \
#    -DNRN_ENABLE_CORENEURON=Off
#  make -j install
#  make setup_exe
# ~~~

if(MINGW)
  set(mswinsrc ${CMAKE_CURRENT_SOURCE_DIR})
  file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/nrnicon.rc "id ICON \"${mswinsrc}/nrniv10.ico\"\n")
  add_custom_command(
    OUTPUT nrnicon.res
    COMMAND windres nrnicon.rc -O coff -o nrnicon.res
    DEPENDS ${mswinsrc}/nrniv10.ico)
  set_source_files_properties(${mswinsrc}/extra/neuron.cpp ${mswinsrc}/extra/mos2nrn.cpp
                              ${mswinsrc}/extra/nrnbinstr.cpp PROPERTIES COMPILE_DEFINITIONS MINGW)
  add_executable(neuron ${mswinsrc}/extra/neuron.cpp nrnicon.res)
  add_executable(mos2nrn ${mswinsrc}/extra/mos2nrn.cpp)
  add_executable(nrnbinstr ${mswinsrc}/extra/nrnbinstr.cpp)
  set(INCDIRS ${PROJECT_SOURCE_DIR}/src/oc)
  target_include_directories(mos2nrn PUBLIC ${INCDIRS})

  # ~~~
  # Presumes previous 'make install'
  # Invoke after 'make install' by 'make setup_exe'.
  # ~~~
  set(MAKENSIS_EXE "c:/Program Files (x86)/NSIS/makensis")

  # ~~~
  # Target used for building installer
  # nrnmingwenv.sh:
  #    - copy enough mingw tool chain for mknrndll to work
  # pre_setup_exe.sh:
  #    - Copy mpi. Reduce size by removing inadvertently installed files,
  #      some huge test data files, and strip binary executables and libraries.
  #    - Create the nrnmech.dll for neurondemo.
  # nrnsetupmingw.nsi:
  #    - make the setup.exe
  # ~~~
  add_custom_target(
    setup_exe
    COMMAND bash ${PROJECT_SOURCE_DIR}/mingw_files/nrnmingwenv.sh ${CMAKE_INSTALL_PREFIX}
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMAND bash ${PROJECT_BINARY_DIR}/src/mswin/pre_setup_exe.sh
    COMMAND ${MAKENSIS_EXE} ${PROJECT_BINARY_DIR}/src/mswin/nrnsetupmingw.nsi
    DEPENDS ${PROJECT_SOURCE_DIR}/mingw_files/nrnmingwenv.sh
            ${PROJECT_BINARY_DIR}/src/mswin/pre_setup_exe.sh
            ${PROJECT_BINARY_DIR}/src/mswin/nrnsetupmingw.nsi
    COMMENT "Copy mknrndll tool chain to ${CMAKE_INSTALL_PREFIX} and build setup.exe ")

  # install mswin specfic. See cmake/NeuronFileLists.cmake for MSWIN... lists.
  install(FILES ${MSWIN_FILES} DESTINATION ${CMAKE_INSTALL_PREFIX})
  install(FILES ${MSWIN_BIN_FILES} DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)
  install(FILES ${MSWIN_LIB_FILES} DESTINATION ${CMAKE_INSTALL_PREFIX}/lib)
  install(TARGETS neuron mos2nrn nrnbinstr DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)
endif()
