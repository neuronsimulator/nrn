# Add directory-level default compiler flags -- these should be added to all NEURON targets, but not
# targets from included projects like CoreNEURON and NMODL
add_compile_options(${NRN_COMPILE_FLAGS})
add_compile_definitions(${NRN_COMPILE_DEFS})
add_link_options(${NRN_LINK_FLAGS})

# =============================================================================
# Definitions used in setup.py
# =============================================================================
set(NRN_SRCDIR ${PROJECT_SOURCE_DIR})
set(NRNPYTHON_DEFINES "")

# ~~~
# If NRN_PYTHON_DYNAMIC then the following three will be determined from the
# actual pyexe that runs setup.py
# ~~~
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
    set(NRN_PYTHON_EXE_LIST
        ${PYTHON_EXECUTABLE}
        CACHE INTERNAL "" FORCE)
  else()
    set(NRN_PYTHON_EXE_LIST
        ${NRN_PYTHON_DYNAMIC}
        CACHE INTERNAL "" FORCE)
  endif()
else()
  set(BUILD_NRNPYTHON_DYNAMIC_TRUE "#")
  set(BUILD_NRNPYTHON_DYNAMIC_FALSE "")
  set(NRN_PYTHON_EXE_LIST
      ${PYTHON_EXECUTABLE}
      CACHE INTERNAL "" FORCE)
endif()

if(NRN_MACOS_BUILD)
  set(MAC_DARWIN_TRUE "")
  set(MAC_DARWIN_FALSE "#")
else()
  set(MAC_DARWIN_TRUE "#")
  set(MAC_DARWIN_FALSE "")
endif()

if(NRN_WINDOWS_BUILD)
  set(BUILD_MINGW_TRUE "")
  set(BUILD_MINGW_FALSE "#")
else()
  set(BUILD_MINGW_TRUE "#")
  set(BUILD_MINGW_FALSE "")
endif()

# rxdmath libraries (always build)
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
      ..
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

# Install package files that were created in build (e.g. .py.in)
install(
  DIRECTORY ${PROJECT_BINARY_DIR}/share/lib/python
  DESTINATION ${NRN_LIBDIR}
  FILES_MATCHING
  PATTERN *.py
  PATTERN *.so
  PATTERN *.dylib
  PATTERN *.dat)

# =============================================================================
# If NEURON python module installation is enabled
# =============================================================================
if(NRN_ENABLE_MODULE_INSTALL)

  # All variables are set, prepare setup.py for python module install
  nrn_configure_file(setup.py src/nrnpython)

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
      ${CMAKE_CURRENT_BINARY_DIR}/nrnpython_config.h)

  add_custom_command(
    OUTPUT ${binary_dir_filename}
    COMMAND cp ${source_dir_filename} ${binary_dir_filename}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    DEPENDS ${source_dir_filename} ${CMAKE_CURRENT_BINARY_DIR}/setup.py ${inithoc_hdeps})

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

  # Workaround for: https://github.com/neuronsimulator/nrn/issues/1605 See:
  # https://setuptools.pypa.io/en/latest/deprecated/distutils-legacy.html
  # https://setuptools.pypa.io/en/latest/history.html#id228
  set(extra_env ${CMAKE_COMMAND} -E env "SETUPTOOLS_USE_DISTUTILS=stdlib")

  if(NRN_SANITIZERS)
    # Make sure the same compiler (currently always clang in the sanitizer builds) is used to link
    # as was used to build. By default, Python will try to use the compiler that was used to build
    # Python, which is often gcc.
    list(APPEND extra_env "LDCSHARED=${CMAKE_C_COMPILER} ${CMAKE_SHARED_LIBRARY_CREATE_C_FLAGS}"
         "LDCXXSHARED=${CMAKE_CXX_COMPILER} ${CMAKE_SHARED_LIBRARY_CREATE_CXX_FLAGS}")
  endif()

  foreach(pyexe ${NRN_PYTHON_EXE_LIST})
    add_custom_command(
      TARGET hoc_module
      POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_CURRENT_SOURCE_DIR}/inithoc.cpp
              ${CMAKE_CURRENT_BINARY_DIR}/inithoc.cpp
      COMMAND
        ${CMAKE_COMMAND} -E copy_if_different
        ${PROJECT_SOURCE_DIR}/share/lib/python/neuron/help_data.dat
        ${CMAKE_CURRENT_BINARY_DIR}/lib/python/neuron/help_data.dat
      COMMAND ${extra_env} ${pyexe} setup.py --quiet build --build-lib=${NRN_PYTHON_BUILD_LIB}
      WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
      COMMENT "Building python module with: ${pyexe}")
  endforeach(pyexe)

  add_dependencies(hoc_module nrniv_lib ${nrnpython_lib_list})
  if(NRN_ENABLE_RX3D)
    add_dependencies(hoc_module rxd_cython_generated)
  endif()

  # ~~~
  # neuron module (possibly with multiple extension versions) was built
  # in NRN_PYTHON_BUILD_LIB. Not a problem if install overwrites multiple
  # times to same install folder or if each install ends up in different
  # place.
  # ~~~
  file(
    WRITE ${CMAKE_CURRENT_BINARY_DIR}/neuron_module_install.sh
    "\
#!bash\n\
echo 'Installing python module using:'\n\
set -ex\n\
cd ${CMAKE_CURRENT_BINARY_DIR}\n\
export SETUPTOOLS_USE_DISTUTILS=stdlib
$1 setup.py --quiet build --build-lib=${NRN_PYTHON_BUILD_LIB} install ${NRN_MODULE_INSTALL_OPTIONS}\n\
")
  foreach(pyexe ${NRN_PYTHON_EXE_LIST})
    # install(CODE ...) only takes a single CMake code expression, so we can't easily roll our own
    # check on the return code. Modern CMake versions support the COMMAND_ERROR_IS_FATAL option,
    # which will cause the installation to abort if the shell script returns an error code.
    if(${CMAKE_VERSION} VERSION_LESS "3.19")
      install(
        CODE "execute_process(COMMAND bash ${CMAKE_CURRENT_BINARY_DIR}/neuron_module_install.sh ${pyexe})"
      )
    else()
      install(
        CODE "execute_process(COMMAND bash ${CMAKE_CURRENT_BINARY_DIR}/neuron_module_install.sh ${pyexe} COMMAND_ERROR_IS_FATAL LAST)"
      )
    endif()
  endforeach(pyexe)
endif()
