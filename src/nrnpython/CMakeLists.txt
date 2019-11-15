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
set(NRN_LIBDIR ${nrn_dest_sharedlibs})

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

# use compiler from CMake
set(CC ${CMAKE_C_COMPILER})
set(CXX ${CMAKE_CXX_COMPILER})

# =============================================================================
# Options that get expanded to comments
# =============================================================================
set(BUILD_NRNMPI_DYNAMIC_FALSE "#")
set(BUILD_NRNPYTHON_DYNAMIC_TRUE "")
set(BUILD_NRNPYTHON_DYNAMIC_FALSE "#")

if(NRN_ENABLE_PYTHON_DYNAMIC)
  if("${NRN_PYTHON_DYNAMIC}" STREQUAL "")
    set(NRN_PYTHON_EXE_LIST ${PYTHON_EXECUTABLE} CACHE INTERNAL "" FORCE)
  else()
    set(NRN_PYTHON_EXE_LIST ${NRN_PYTHON_DYNAMIC} CACHE INTERNAL "" FORCE)
  endif()
else()
  set(BUILD_NRNPYTHON_DYNAMIC_TRUE "#")
  set(BUILD_NRNPYTHON_DYNAMIC_FALSE "")
  set(NRN_PYTHON_EXE_LIST ${PYTHON_EXECUTABLE} CACHE INTERNAL "" FORCE)
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
  set(BUILD_MINGW_TRUE "")
  set(BUILD_MINGW_FALSE "#")
else()
  set(BUILD_MINGW_TRUE "#")
  set(BUILD_MINGW_FALSE "")
endif()

if(NRN_ENABLE_MEMACS)
  set(BUILD_MEMACS_TRUE "")
else()
  set(BUILD_MEMACS_TRUE "#")
endif()

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
# Build rxdmath library
# =============================================================================
add_library(rxdmath SHARED ${CMAKE_CURRENT_SOURCE_DIR}/rxdmath.c)
install(TARGETS rxdmath DESTINATION ${nrn_dest_sharedlibs})

# =============================================================================
# If NEURON python module installation is enabled
# =============================================================================
if(NRN_ENABLE_MODULE_INSTALL)

  # check arguments for setup.py install : if user has just provided YES, then use default
  if("${NRN_MODULE_INSTALL_OPTIONS}" STREQUAL "")
    set(install_path ${PROJECT_BINARY_DIR})
    # replace windows path of the form C:/msys64 to C:\msys64
    if(MINGW)
      string(REPLACE ":/" ":\\\\" install_path "${install_path}")
    endif()
    set(NRN_MODULE_INSTALL_OPTIONS "--home" "${install_path}" CACHE INTERNAL "" FORCE)
  endif()

  # to tickle setup.py into actually rebuilding if dependencies change,
  # need to copy inithoc.cpp in from source to binary dir if any module dependent
  # changes and make a custom target as well.
  set(bname ${CMAKE_CURRENT_BINARY_DIR}/inithoc.cpp)
  set(sname ${CMAKE_CURRENT_SOURCE_DIR}/inithoc.cpp)
  set(inithoc_hdeps
    ${CMAKE_CURRENT_SOURCE_DIR}/../oc/nrnmpi.h
    ${CMAKE_CURRENT_BINARY_DIR}/../oc/nrnmpiuse.h
    ${CMAKE_CURRENT_BINARY_DIR}/../oc/nrnpthread.h
    ${CMAKE_CURRENT_BINARY_DIR}/nrnpython_config.h
  )

  add_custom_command(OUTPUT ${bname}
    COMMAND cp ${sname} ${bname}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    DEPENDS ${sname} ${CMAKE_CURRENT_BINARY_DIR}/setup.py ${inithoc_hdeps}
  )

  add_custom_target(hoc_module ALL
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    DEPENDS ${bname})

  # For DYNAMIC python, inithoc module name convention is different for
  # autotools version and cmake. So nrnpy_hoc.cpp and inithoc.cpp need
  # to know whether they are compiled via the cmake or autotools build system.
  # (note: see also for inithoc.cpp via setup.py)
  set_property(SOURCE ${PROJECT_SOURCE_DIR}/src/nrnpython/nrnpy_hoc.cpp
    APPEND
    PROPERTY COMPILE_DEFINITIONS NRNCMAKE)

  # =============================================================================
  # Copy necessary files to build directory
  # =============================================================================
  add_custom_command(TARGET hoc_module PRE_BUILD
                   COMMAND ${CMAKE_COMMAND} -E copy_directory
                    ${PROJECT_SOURCE_DIR}/share/lib ${PROJECT_BINARY_DIR}/share/nrn/lib
                    COMMAND ${CMAKE_COMMAND} -E copy_directory
                    ${PROJECT_SOURCE_DIR}/share/demo ${PROJECT_BINARY_DIR}/share/nrn/demo
                    )

  # for each python detected / provided by user, install module at install time
  foreach(pyexe ${NRN_PYTHON_EXE_LIST})
      add_custom_command(TARGET hoc_module
                  POST_BUILD
                  COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_CURRENT_SOURCE_DIR}/inithoc.cpp ${CMAKE_CURRENT_BINARY_DIR}/inithoc.cpp
                  COMMAND ${pyexe} setup.py install ${NRN_MODULE_INSTALL_OPTIONS}
                  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
                  COMMENT "Building python module with: ${pyexe}"
                  )
  endforeach(pyexe)
  add_dependencies(hoc_module nrniv_lib)

  # user has selected dynamic python support (could be multiple versions)
  if(NRN_ENABLE_PYTHON_DYNAMIC)
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

    if (LINK_AGAINST_PYTHON)
      list(LENGTH NRN_PYTHON_EXE_LIST _num_pythons)
      math(EXPR num_pythons "${_num_pythons} - 1")
      foreach (val RANGE ${num_pythons})
        list(GET NRN_PYTHON_VER_LIST ${val} pyver)
        list(GET NRN_PYTHON_INCLUDE_LIST ${val} pyinc)
        list(GET NRN_PYTHON_LIB_LIST ${val} pylib)
        add_library(nrnpython${pyver} SHARED ${NRN_NRNPYTHON_SRC_FILES})
        target_include_directories(nrnpython${pyver} BEFORE PUBLIC ${pyinc} ${INCLUDE_DIRS})
        target_link_libraries(nrnpython${pyver} nrniv_lib ${pylib} ${Readline_LIBRARY})
        add_dependencies(nrnpython${pyver} nrniv_lib)
        add_dependencies(hoc_module nrnpython${pyver})
        install(TARGETS nrnpython${pyver} DESTINATION ${nrn_dest_sharedlibs})
      endforeach()
    else()
      # build python2 library and install it
      if(NRNPYTHON_INCLUDE2)
        add_library(nrnpython2 SHARED ${NRN_NRNPYTHON_SRC_FILES})
        target_include_directories(nrnpython2 BEFORE PUBLIC ${NRNPYTHON_INCLUDE2} ${INCLUDE_DIRS})
        add_dependencies(nrnpython2 nrniv_lib)
        add_dependencies(hoc_module nrnpython2)
        install(TARGETS nrnpython2 DESTINATION ${nrn_dest_sharedlibs})
      endif()

      # build python3 library and install it
      if(NRNPYTHON_INCLUDE3)
        add_library(nrnpython3 SHARED ${NRN_NRNPYTHON_SRC_FILES})
        add_dependencies(nrnpython3 nrniv_lib)
        add_dependencies(hoc_module nrnpython3)
        target_include_directories(nrnpython3 BEFORE PUBLIC ${NRNPYTHON_INCLUDE3} ${INCLUDE_DIRS})
        install(TARGETS nrnpython3 DESTINATION ${nrn_dest_sharedlibs})
      endif()
    endif()
  endif()

  # module is already installed in lib, just copy folder
  install(DIRECTORY ${PROJECT_BINARY_DIR}/lib/ DESTINATION ${NRN_LIBDIR})
endif()
