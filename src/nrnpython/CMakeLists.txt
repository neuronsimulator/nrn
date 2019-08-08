# =============================================================================
# Definitions used in setup.py
# =============================================================================
set(NRN_SRCDIR ${PROJECT_SOURCE_DIR})
set(NRNPYTHON_DEFINES "")

# if NRN_PYTHON_DYNAMIC then the following three will be determined from the actual pyexe that runs
# setup.py
set(NRNPYTHON_EXEC ${PYTHON_EXECUTABLE})
set(NRNPYTHON_PYVER ${PYTHON_VERSION_MAJOR}.${PYTHON_VERSION_MINOR})
set(npy_pyver10 "")

# reset since no " allowed
set(PACKAGE_VERSION ${PROJECT_VERSION})
set(NRN_LIBDIR ${CMAKE_INSTALL_PREFIX}/lib)

# set by IV's cmake module
set(IV_LIBDIR "${IV_INCLUDE_DIR}/../lib")

# no suffix for readline
set(READLINE_SOSUFFIX "")

if(NRN_ENABLE_RX3D)
  set(BUILD_RX3D "1")
else()
  set(BUILD_RX3D "0")
endif()

if(NRN_ENABLE_INTERVIEWS)
  set(IVHINES "interviews")
else()
  set(IVHINES "")
endif()

# use compiler from CMaek
set(CC ${CMAKE_C_COMPILER})
set(CXX ${CMAKE_CXX_COMPILER})

# =============================================================================
# Options that get expanded to comments
# =============================================================================
set(BUILD_NRNMPI_DYNAMIC_FALSE "#")
set(BUILD_NRNPYTHON_DYNAMIC_TRUE "")
set(BUILD_NRNPYTHON_DYNAMIC_FALSE "#")

if("${NRN_ENABLE_PYTHON_DYNAMIC}" MATCHES "NO")
  set(BUILD_NRNPYTHON_DYNAMIC_TRUE "#")
  set(BUILD_NRNPYTHON_DYNAMIC_FALSE "")
  set(NRN_PYTHON_EXE_LIST ${PYTHON_EXECUTABLE})
elseif("${NRN_ENABLE_PYTHON_DYNAMIC}" MATCHES "YES")
  set(NRN_PYTHON_EXE_LIST ${PYTHON_EXECUTABLE})
else()
  set(NRN_PYTHON_EXE_LIST ${NRN_ENABLE_PYTHON_DYNAMIC})
endif()

if(NRN_MACOS_BUILD)
  set(MAC_DARWIN_TRUE "")
  set(MAC_DARWIN_FALSE "#")
  set(setup_extra_link_args "#extra_link_args")
else()
  set(MAC_DARWIN_TRUE "#")
  set(MAC_DARWIN_FALSE "")
  set(setup_extra_link_args "extra_link_args")
endif()

if(NRN_WINDOWS_BUILD)
  set(WIN_MINGW_TRUE "")
  set(WIN_MINGW_FALSE "#")
else()
  set(WIN_MINGW_TRUE "#")
  set(WIN_MINGW_FALSE "")
endif()

if(NRN_ENABLE_MEMACS)
  set(BUILD_MEMACS_TRUE "")
else()
  set(BUILD_MEMACS_TRUE "#")
endif()

# =============================================================================
# Todo: add description of why this is copied
# =============================================================================
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/inithoc.cpp DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

# =============================================================================
# All variables are set, prepare setup.py for python module install
# =============================================================================
nrn_configure_file(setup.py src/nrnpython)

# =============================================================================
# Setup MinGW toolchain for setuptools
# =============================================================================
if(MINGW)
  file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/setup.cfg "[build]\ncompiler=mingw32")
endif()

# =============================================================================
# If NEURON python module installation is enabled
# =============================================================================
if(NOT "${NRN_ENABLE_MODULE_INSTALL}" MATCHES "NO")

  # check arguments for setup.py install : if user has just provided YES, then use default
  set(pyinst "${NRN_ENABLE_MODULE_INSTALL}")
  if("${pyinst}" MATCHES "YES")
    set(install_path ${CMAKE_INSTALL_PREFIX})
    # replace windows path of the form C:/msys64 to C:\msys64
    if(MINGW)
      string(REPLACE ":/" ":\\" install_path "${install_path}")
    endif()
    set(pyinst "--home ${install_path}")
  endif()
  # show commands to user
  foreach(pyexe ${NRN_PYTHON_EXE_LIST})
    message(
      STATUS
        "Install neuron module with: ${pyexe} setup.py install ${pyinst}"
      )
  endforeach(pyexe)

  # user has selected dynamic python support (could be multiple versions)
  if(NOT "${NRN_ENABLE_PYTHON_DYNAMIC}" MATCHES "NO")
    set(INCLUDE_DIRS
        .
        ../oc
        ../nrnoc
        ../ivoc
        ../nrniv
        ../ivos
        ../gnu
        ../mesch
        ../nrnmpi
        ${PROJECT_BINARY_DIR}/src/nrnpython
        ${PROJECT_BINARY_DIR}/src/ivos
        ${PROJECT_BINARY_DIR}/src/oc
        ${IV_INCLUDE_DIR})

    # build python2 library and install it
    if(NRNPYTHON_INCLUDE2)
      add_library(nrnpython2 SHARED ${NRN_NRNPYTHON_SRC_FILES})
      target_include_directories(nrnpython2 BEFORE PUBLIC ${NRNPYTHON_INCLUDE2} ${INCLUDE_DIRS})
      add_dependencies(nrnpython2 nrniv_lib)
      install(TARGETS nrnpython2 DESTINATION lib)
    endif()

    # build python3 library and install it
    if(NRNPYTHON_INCLUDE3)
      add_library(nrnpython3 SHARED ${NRN_NRNPYTHON_SRC_FILES})
      add_dependencies(nrnpython3 nrniv_lib)
      target_include_directories(nrnpython3 BEFORE PUBLIC ${NRNPYTHON_INCLUDE3} ${INCLUDE_DIRS})
      install(TARGETS nrnpython3 DESTINATION lib)
    endif()
  endif()

  # for each python detected / provided by user, install module at install time
  foreach(pyexe ${NRN_PYTHON_EXE_LIST})
    install(CODE "
      message(STATUS \"Installing neuron module with: ${pyexe} setup.py install ${pyinst}\")
      execute_process(COMMAND ${pyexe} setup.py install ${pyinst}
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
      )
    ")
  endforeach(pyexe)
endif()
