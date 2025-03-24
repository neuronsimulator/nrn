# Add directory-level default compiler flags -- these should be added to all NEURON targets, but not
# targets from included projects like CoreNEURON and NMODL
add_compile_options(${NRN_COMPILE_FLAGS})
add_compile_definitions(${NRN_COMPILE_DEFS})
add_link_options(${NRN_LINK_FLAGS})

# =============================================================================
# List of python executables to build nrnpython against
# =============================================================================
if(NRN_ENABLE_PYTHON_DYNAMIC)
  if("${NRN_PYTHON_DYNAMIC}" STREQUAL "")
    set(NRN_PYTHON_EXE_LIST
        ${PYTHON_EXECUTABLE}
        CACHE INTERNAL "" FORCE)
  else()
    set(NRN_PYTHON_EXE_LIST
        ${NRN_PYTHON_DYNAMIC}
        CACHE INTERNAL "" FORCE)
  endif()
else()
  set(NRN_PYTHON_EXE_LIST
      ${PYTHON_EXECUTABLE}
      CACHE INTERNAL "" FORCE)
endif()

# =============================================================================
# rxdmath libraries (always build)
# =============================================================================
add_library(rxdmath SHARED ${CMAKE_CURRENT_SOURCE_DIR}/rxdmath.cpp)
install(TARGETS rxdmath DESTINATION ${NRN_INSTALL_SHARE_LIB_DIR})

# =============================================================================
# nrnpython libraries (one lib per python)
# =============================================================================

set(nrnpython_lib_list)

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

  if(LINK_AGAINST_PYTHON)
    list(LENGTH NRN_PYTHON_EXE_LIST _num_pythons)
    math(EXPR num_pythons "${_num_pythons} - 1")
    foreach(val RANGE ${num_pythons})
      list(GET NRN_PYTHON_VER_LIST ${val} pyver)
      list(GET NRN_PYTHON_INCLUDE_LIST ${val} pyinc)
      list(GET NRN_PYTHON_LIB_LIST ${val} pylib)
      add_library(nrnpython${pyver} SHARED ${NRN_NRNPYTHON_SRC_FILES})
      target_include_directories(nrnpython${pyver} BEFORE PUBLIC ${pyinc} ${INCLUDE_DIRS})
      target_link_libraries(nrnpython${pyver} nrniv_lib ${pylib} ${Readline_LIBRARY})
      add_dependencies(nrnpython${pyver} nrniv_lib)
      list(APPEND nrnpython_lib_list nrnpython${pyver})
      install(TARGETS nrnpython${pyver} DESTINATION ${NRN_INSTALL_SHARE_LIB_DIR})
    endforeach()
  else()
    # build python3 library and install it
    if(NRNPYTHON_INCLUDE3)
      add_library(nrnpython3 SHARED ${NRN_NRNPYTHON_SRC_FILES})
      add_dependencies(nrnpython3 nrniv_lib)
      target_include_directories(nrnpython3 PRIVATE "${NRN_OC_GENERATED_SOURCES}")
      list(APPEND nrnpython_lib_list nrnpython3)
      target_include_directories(nrnpython3 BEFORE PUBLIC ${NRNPYTHON_INCLUDE3} ${INCLUDE_DIRS})
      install(TARGETS nrnpython3 DESTINATION ${NRN_INSTALL_SHARE_LIB_DIR})
    endif()
  endif()
endif()

configure_file(_config_params.py.in "${PROJECT_BINARY_DIR}/lib/python/neuron/_config_params.py"
               @ONLY)

# Install package files that were created in build (e.g. .py.in)
install(
  DIRECTORY ${PROJECT_BINARY_DIR}/share/lib/python
  DESTINATION lib
  FILES_MATCHING
  PATTERN *.py
  PATTERN *.so
  PATTERN *.dylib
  PATTERN *.dat)

# =============================================================================
# If NEURON python module installation is enabled (CMake install)
# =============================================================================
if(NRN_ENABLE_MODULE_INSTALL)

  # Setup MinGW toolchain for setuptools
  if(MINGW)
    file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/setup.cfg "[build]\ncompiler=mingw32")

    # replace windows path of the form C:/msys64 to C:\msys64
    string(REPLACE ":/" ":\\\\" module_install_opts "${NRN_MODULE_INSTALL_OPTIONS}")
    set(NRN_MODULE_INSTALL_OPTIONS
        "${module_install_opts}"
        CACHE INTERNAL "" FORCE)
  endif()

  # Here and for the neuron/rxd/geometry3d extensions, we build the neuron module in lib/python
  set(NRN_PYTHON_BUILD_LIB
      ${PROJECT_BINARY_DIR}/lib/python
      CACHE INTERNAL "" FORCE)

  # ~~~
  # To tickle setup.py into actually rebuilding if dependencies change,
  # need to copy inithoc.cpp in from source to binary dir if any module
  # dependent changes and make a custom target as well.
  # ~~~

  set(binary_dir_filename ${CMAKE_CURRENT_BINARY_DIR}/inithoc.cpp)
  set(source_dir_filename ${CMAKE_CURRENT_SOURCE_DIR}/inithoc.cpp)
  set(inithoc_hdeps
      ${CMAKE_CURRENT_SOURCE_DIR}/../oc/nrnmpi.h ${CMAKE_CURRENT_BINARY_DIR}/../oc/nrnmpiuse.h
      ${CMAKE_CURRENT_BINARY_DIR}/../oc/nrnpthread.h ${CMAKE_CURRENT_BINARY_DIR}/nrnpython_config.h)

  add_custom_command(
    OUTPUT ${binary_dir_filename}
    COMMAND cp ${source_dir_filename} ${binary_dir_filename}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    DEPENDS ${source_dir_filename} ${inithoc_hdeps})

  add_custom_target(
    hoc_module ALL
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    DEPENDS ${binary_dir_filename})

  # =============================================================================
  # Copy necessary files to build directory
  # =============================================================================

  add_custom_command(
    TARGET hoc_module
    PRE_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${PROJECT_SOURCE_DIR}/share/lib
            ${PROJECT_BINARY_DIR}/share/nrn/lib
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${PROJECT_SOURCE_DIR}/share/demo
            ${PROJECT_BINARY_DIR}/share/nrn/demo)
  # Don't do exactly the same copy twice in a row. Presumably it's sometimes important to do both...
  if(NOT "${NRN_BUILD_SHARE_DIR}/lib" STREQUAL "${PROJECT_BINARY_DIR}/share/nrn/lib")
    add_custom_command(
      TARGET hoc_module
      PRE_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_directory ${PROJECT_SOURCE_DIR}/share/lib
              ${NRN_BUILD_SHARE_DIR}/lib)
  endif()

  # =============================================================================
  # Build python module
  # =============================================================================
  # for each python detected / provided by user, install module at install time

  # setup.py tries to compile C++ code with the C compiler. This works with gcc and intel but fails
  # with at least PGI/NVHPC compiler. Hence, use C++ compiler if PGI/NVHPC compiler is used.
  if(NRN_HAVE_NVHPC_COMPILER)
    list(APPEND extra_env "CC=${CMAKE_CXX_COMPILER}" "CXX=${CMAKE_CXX_COMPILER}")
  else()
    list(APPEND extra_env "CC=${CMAKE_C_COMPILER}" "CXX=${CMAKE_CXX_COMPILER}")
  endif()

  if(NRN_COMPILE_FLAGS)
    list(APPEND extra_env "CFLAGS=${NRN_COMPILE_FLAGS_STRING}")
  endif()
  if(NRN_LINK_FLAGS)
    list(APPEND extra_env "LDFLAGS=${NRN_LINK_FLAGS_STRING}")
  endif()
  # Make sure the same compiler (currently always clang in the sanitizer builds) is used to link as
  # was used to build. By default, Python will try to use the compiler that was used to build
  # Python, which is often gcc.
  list(APPEND extra_env "LDCSHARED=${CMAKE_C_COMPILER} ${CMAKE_SHARED_LIBRARY_CREATE_C_FLAGS}"
       "LDCXXSHARED=${CMAKE_CXX_COMPILER} ${CMAKE_SHARED_LIBRARY_CREATE_CXX_FLAGS}")

  # Prepare build options for setup.py build
  set(NRN_SETUP_PY_BUILD_OPTIONS "--cmake-build-dir")
  list(APPEND NRN_SETUP_PY_BUILD_OPTIONS ${CMAKE_BINARY_DIR})
  if(NOT NRN_ENABLE_RX3D)
    list(APPEND NRN_SETUP_PY_BUILD_OPTIONS "--disable-rx3d")
  else()
    list(APPEND NRN_SETUP_PY_BUILD_OPTIONS "--rx3d-opt-level" ${NRN_RX3D_OPT_LEVEL})
  endif()
  if(MINGW)
    list(APPEND NRN_SETUP_PY_BUILD_OPTIONS "--compiler=mingw32")
  endif()
  if(NOT NRN_ENABLE_PYTHON_DYNAMIC)
    list(APPEND NRN_SETUP_PY_BUILD_OPTIONS "--without-nrnpython")
  endif()
  if(NRN_ENABLE_MUSIC)
    list(APPEND NRN_SETUP_PY_BUILD_OPTIONS "--enable-music")
  endif()
  if(NRN_MAC_PKG)
    list(APPEND NRN_SETUP_PY_BUILD_OPTIONS "--mac_pkg")
  endif()
  list(APPEND NRN_SETUP_PY_BUILD_OPTIONS "--build-lib=${NRN_PYTHON_BUILD_LIB}")

  # Prepare build_ext options for setup.py build
  set(NRN_SETUP_PY_BUILD_EXT_OPTIONS "build_ext")
  if(NRN_ENABLE_MUSIC)
    # NRN_SETUP_PY_INC_DIRS is crafted for the MUSIC build, to be extended to other if needed
    set(NRN_SETUP_PY_INC_DIRS "${MUSIC_INCDIR};${MPI_INCLUDE_PATH}")
    string(REPLACE ";" ":" NRN_SETUP_PY_INC_DIRS "${NRN_SETUP_PY_INC_DIRS}")
    list(APPEND NRN_SETUP_PY_BUILD_EXT_OPTIONS "--include-dirs=${NRN_SETUP_PY_INC_DIRS}")
  endif()
  if(MINGW)
    list(APPEND NRN_SETUP_PY_BUILD_EXT_OPTIONS "--define=MS_WIN64")
  endif()
  # force rebuild of cython generated files for PYTHON_DYNAMIC
  if(NRN_ENABLE_PYTHON_DYNAMIC)
    list(APPEND NRN_SETUP_PY_BUILD_EXT_OPTIONS "--force")
  endif()

  # Build python module
  foreach(pyexe ${NRN_PYTHON_EXE_LIST})
    add_custom_command(
      TARGET hoc_module
      POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_CURRENT_SOURCE_DIR}/inithoc.cpp
              ${CMAKE_CURRENT_BINARY_DIR}/inithoc.cpp
      COMMAND ${CMAKE_COMMAND} -E env ${extra_env} ${pyexe} setup.py build
              ${NRN_SETUP_PY_BUILD_OPTIONS} ${NRN_SETUP_PY_BUILD_EXT_OPTIONS}
      WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
      COMMENT "Building python module with: ${pyexe} setup.py build
      ${NRN_SETUP_PY_BUILD_OPTIONS} ${NRN_SETUP_PY_BUILD_EXT_OPTIONS}")
  endforeach(pyexe)

  add_dependencies(hoc_module nrniv_lib rxdmath ${nrnpython_lib_list})
  install(DIRECTORY ${NRN_PYTHON_BUILD_LIB} DESTINATION lib)
else()
  # Make sure this is included in the wheels
  install(FILES "${PROJECT_BINARY_DIR}/lib/python/neuron/_config_params.py"
          DESTINATION lib/python/neuron)
endif()
