
set (modsubdir ${host_cpu})

# for nrn.defaults
set(nrndef_unix "//")
set(nrndef_mac "//")
set(nrndef_mswin "//")
set(MAC_DARWIN_TRUE "\#")
set(MAC_DARWIN_FALSE "")
if (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
  set(nrndef_unix "")
elseif (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  set(nrndef_mac "")
  set(MAC_DARWIN_TRUE "")
  set(MAC_DARWIN_FALSE "\#")
elseif (${CMAKE_SYSTEM_NAME} MATCHES "Windows")
  set(nrndef_mswin "")
endif()

#for nrnmech_makefile
set(CMAKE_INSTALL_BINDIR bin)
set(CMAKE_INSTALL_LIBDIR lib)
set(CMAKE_INSTALL_INCLUDEDIR include)
set(CMAKE_INSTALL_DATADIR share/nrn)
#------modified coreneuron/extra/CMakeLists.txt
include(${CMAKE_CURRENT_SOURCE_DIR}/CMakeListsNrnMech.txt)
#------end of modified coreneuron/extra/CMakeLists.txt

set(NRNMECH_DLL_STYLE_FALSE "\#")
set(NRNMECH_DLL_STYLE_TRUE "")
nrn_configure_file(nrngui bin)
nrn_configure_file(neurondemo bin)
nrn_configure_file(nrnivmodl bin)
#nrnmech_makefile.in is intimately bound up with autoconf (libtool)
#nrn_configure_file(nrnmech_makefile bin)
#so instead use a modified copy of the CoreNEURON version of
#nrnivmodl_core_makefile.in
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/nrnivmodl_makefile.cmk
  ${PROJECT_BINARY_DIR}/bin/nrnmech_makefile
  @ONLY
)

install(PROGRAMS ${PROJECT_BINARY_DIR}/bin/nrngui
  ${PROJECT_BINARY_DIR}/bin/neurondemo
  ${PROJECT_BINARY_DIR}/bin/nrnivmodl
  DESTINATION ${CMAKE_INSTALL_PREFIX}/bin
)
install(FILES ${PROJECT_BINARY_DIR}/bin/nrnmech_makefile
  DESTINATION ${CMAKE_INSTALL_PREFIX}/bin
)


