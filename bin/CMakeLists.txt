# =============================================================================
# Set various variables used in template files
# =============================================================================
# TODO: for nrn.defaults but these are repeated in cmake_config/CMakeLists.txt
include(${CODING_CONV_CMAKE}/build-time-copy.cmake)

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
set(CMAKE_INSTALL_DATADIR_NMODL share/nmodl)

# =============================================================================
# Include nrnivmodl makefile generator
# =============================================================================
include(CMakeListsNrnMech)

# =============================================================================
# nrnmech_makefile (based on coreneuron Configure templates)
# =============================================================================
configure_file("nrngui.in" "nrngui" @ONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/sortspike ${CMAKE_CURRENT_BINARY_DIR}/sortspike COPYONLY)
string(JOIN " " NRN_PYTHON_VERSIONS_STRING ${NRN_PYTHON_VERSIONS})
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/nrnpyenv.sh.in ${PROJECT_BINARY_DIR}/bin/nrnpyenv.sh
               @ONLY)

# Make sure nrnivmodl and neurondemo are executable in the build folder, so we can execute it to
# prepare test files. This can be done more elegantly in newer CMake versions; v3.19+ have
# file(CHMOD ...) and v3.20+ support setting permissions directly in configure_file(...).
set(NRN_CONFIG_EXE_FILES "nrnivmodl" "neurondemo")
set(NRNIVMODL_NEURON ON)
foreach(NRN_CONFIG_EXE_FILE ${NRN_CONFIG_EXE_FILES})
  configure_file("${NRN_CONFIG_EXE_FILE}.in" "tmp/${NRN_CONFIG_EXE_FILE}" @ONLY)
  file(
    COPY "${CMAKE_CURRENT_BINARY_DIR}/tmp/${NRN_CONFIG_EXE_FILE}"
    DESTINATION "${CMAKE_CURRENT_BINARY_DIR}"
    FILE_PERMISSIONS
      OWNER_READ
      OWNER_WRITE
      OWNER_EXECUTE
      GROUP_READ
      GROUP_EXECUTE
      WORLD_READ
      WORLD_EXECUTE)
endforeach()
unset(NRNIVMODL_NEURON)

if(NRN_ENABLE_CORENEURON)
  # nrnivmodl-core builds mechanisms for coreNEURON only
  set(NRNIVMODL_CORENEURON ON)
  configure_file(${CMAKE_CURRENT_SOURCE_DIR}/nrnivmodl.in
                 "${CMAKE_CURRENT_BINARY_DIR}/tmp/nrnivmodl-core" @ONLY)
  file(
    COPY "${CMAKE_CURRENT_BINARY_DIR}/tmp/nrnivmodl-core"
    DESTINATION "${CMAKE_CURRENT_BINARY_DIR}"
    FILE_PERMISSIONS
      OWNER_READ
      OWNER_WRITE
      OWNER_EXECUTE
      GROUP_READ
      GROUP_EXECUTE
      WORLD_READ
      WORLD_EXECUTE)
  unset(NRNIVMODL_CORENEURON)

  # nrnivmodl-all builds mechanisms for both NEURON and coreNEURON
  set(NRNIVMODL_NEURON ON)
  set(NRNIVMODL_CORENEURON ON)
  configure_file(${CMAKE_CURRENT_SOURCE_DIR}/nrnivmodl.in
                 "${CMAKE_CURRENT_BINARY_DIR}/tmp/nrnivmodl-all" @ONLY)
  file(
    COPY "${CMAKE_CURRENT_BINARY_DIR}/tmp/nrnivmodl-all"
    DESTINATION "${CMAKE_CURRENT_BINARY_DIR}"
    FILE_PERMISSIONS
      OWNER_READ
      OWNER_WRITE
      OWNER_EXECUTE
      GROUP_READ
      GROUP_EXECUTE
      WORLD_READ
      WORLD_EXECUTE)
  unset(NRNIVMODL_CORENEURON)
  unset(NRNIVMODL_NEURON)
endif()

# Delete the temporary files
file(REMOVE_RECURSE "${CMAKE_CURRENT_BINARY_DIR}/bin/tmp")

# =============================================================================
# Install targets
# =============================================================================
install(PROGRAMS ${PROJECT_BINARY_DIR}/bin/nrngui ${PROJECT_BINARY_DIR}/bin/neurondemo
                 ${PROJECT_BINARY_DIR}/bin/nrnivmodl DESTINATION ${NRN_INSTALL_DATA_PREFIX}bin)
if(NRN_ENABLE_CORENEURON)
  install(PROGRAMS ${PROJECT_BINARY_DIR}/bin/nrnivmodl-core ${PROJECT_BINARY_DIR}/bin/nrnivmodl-all
          DESTINATION ${NRN_INSTALL_DATA_PREFIX}bin)
endif()
install(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/sortspike ${CMAKE_CURRENT_SOURCE_DIR}/mkthreadsafe
                 ${PROJECT_BINARY_DIR}/bin/nrnpyenv.sh ${CMAKE_CURRENT_SOURCE_DIR}/set_nrnpyenv.sh
        DESTINATION ${NRN_INSTALL_DATA_PREFIX}bin)
