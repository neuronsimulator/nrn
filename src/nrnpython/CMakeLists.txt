# Add directory-level default compiler flags -- these should be added to all NEURON targets, but not
# targets from included projects like CoreNEURON and NMODL
add_compile_options(${NRN_COMPILE_FLAGS})
add_compile_definitions(${NRN_COMPILE_DEFS})
add_link_options(${NRN_LINK_FLAGS})

include(${PROJECT_SOURCE_DIR}/cmake/PythonCompileHelper.cmake)

# =============================================================================
# rxdmath libraries (always build)
# =============================================================================
add_library(rxdmath SHARED ${CMAKE_CURRENT_SOURCE_DIR}/rxdmath.cpp)
install(TARGETS rxdmath DESTINATION ${NRN_INSTALL_SHARE_LIB_DIR})

# =============================================================================
# nrnpython libraries (one lib per python)
# =============================================================================
set(nrnpython_lib_list)

set(NRNPYTHON_FILES_LIST
    nrnpython.cpp
    nrnpy_hoc.cpp
    nrn_metaclass.cpp
    nrnpy_nrn.cpp
    nrnpy_p2h.cpp
    nrnpy_utils.cpp
    grids.cpp
    rxd.cpp
    rxd_extracellular.cpp
    rxd_intracellular.cpp
    rxd_vol.cpp
    rxd_marching_cubes.cpp
    rxd_llgramarea.cpp)

set(INCLUDE_DIRS
    ..
    ../oc
    ../nrnoc
    ../ivoc
    ../nrniv
    ../nrnmpi
    ${CMAKE_CURRENT_BINARY_DIR}
    ${PROJECT_BINARY_DIR}/src/oc)
if(NRN_ENABLE_INTERVIEWS)
  list(APPEND INCLUDE_DIRS ${IV_INCLUDE_DIR})
else()
  list(APPEND INCLUDE_DIRS ../ivos)
endif()

# user has selected dynamic python support (could be multiple versions)
if(NRN_ENABLE_PYTHON_DYNAMIC)
  foreach(val RANGE ${NRN_PYTHON_ITERATION_LIMIT})

    # the NEURON Python library (with nanobind)
    list(GET NRN_PYTHON_VERSIONS ${val} pyver)
    list(GET NRN_PYTHON_INCLUDES ${val} pyinc)
    list(GET NRN_PYTHON_LIBRARIES ${val} pylib)
    set(nanobind_target "nanobind_py${pyver}")
    make_nanobind_target(${nanobind_target} ${pyinc})
    add_library(nrnpython${pyver} SHARED ${NRNPYTHON_FILES_LIST})
    target_include_directories(nrnpython${pyver} BEFORE PUBLIC ${pyinc} ${INCLUDE_DIRS})
    target_link_libraries(nrnpython${pyver} PUBLIC nrniv_lib)
    target_link_libraries(nrnpython${pyver} PRIVATE ${Readline_LIBRARY} ${nanobind_target})
    if(NRN_LINK_AGAINST_PYTHON)
      target_link_libraries(nrnpython${pyver} PUBLIC ${pylib})
    endif()
    add_dependencies(nrnpython${pyver} nrniv_lib)
    list(APPEND nrnpython_lib_list nrnpython${pyver})
    install(TARGETS nrnpython${pyver} DESTINATION ${NRN_INSTALL_SHARE_LIB_DIR})

    # the hoc Python library

    # module install = installation using CMake
    if(NOT NRN_ENABLE_MODULE_INSTALL)
      # taken from setup.py
      set(libraries_rpath "/.data/lib/")
    else()
      # taken from setup.py
      set(libraries_rpath "/../../")
    endif()

    set(hoc_module_name hoc${pyver})

    add_nrn_python_library(
      hoc
      TARGET
      ${hoc_module_name}
      PYTHON_VERSION
      ${pyver}
      LANGUAGE
      "CXX"
      SOURCES
      ${CMAKE_CURRENT_SOURCE_DIR}/inithoc.cpp
      INCLUDES
      ${PROJECT_SOURCE_DIR}/src
      ${PROJECT_SOURCE_DIR}/src/oc
      ${PROJECT_BINARY_DIR}/src/oc
      ${PROJECT_BINARY_DIR}/include
      ${CMAKE_CURRENT_SOURCE_DIR}
      # for nrnconf.h
      ${CMAKE_CURRENT_BINARY_DIR}
      ${PROJECT_BINARY_DIR}/nrnmpi
      ${pyinc}
      LIBRARIES
      nrniv_lib
      rxdmath
      nrnpython${pyver}
      REL_RPATH
      ${libraries_rpath}
      OUTPUT_DIR
      ${PROJECT_BINARY_DIR}/lib/python/neuron)

    if(NRN_LINK_AGAINST_PYTHON)
      target_link_libraries(${hoc_module_name} PUBLIC ${pylib})
    endif()

    add_dependencies(${hoc_module_name} nrniv_lib rxdmath)

  endforeach(val)
else()
  # libnrnpython
  add_library(nrnpython OBJECT ${NRNPYTHON_FILES_LIST})
  add_dependencies(nrnpython generated_source_files)
  set_property(TARGET nrnpython PROPERTY POSITION_INDEPENDENT_CODE ON)
  target_include_directories(nrnpython PUBLIC ${INCLUDE_DIRS})
  target_include_directories(nrnpython PUBLIC "${NRN_DEFAULT_PYTHON_INCLUDES}")
  # see nrn/issues/2358
  if(NOT NRN_WITH_MACOS_CONDA_PYTHON)
    target_link_libraries(nrnpython ${NRN_DEFAULT_PYTHON_LIBRARIES})
  endif()
  target_link_libraries(nrnpython fmt::fmt)
  target_include_directories(nrnpython SYSTEM
                             PUBLIC ${PROJECT_SOURCE_DIR}/${NRN_3RDPARTY_DIR}/eigen)
  target_include_directories(nrnpython PUBLIC ${PROJECT_BINARY_DIR}/src/nrniv/oc_generated)
  make_nanobind_target(nanobind ${NRN_DEFAULT_PYTHON_INCLUDES})
  target_link_libraries(nrnpython nanobind)

  # module install = installation using CMake
  if(NOT NRN_ENABLE_MODULE_INSTALL)
    # taken from setup.py
    set(libraries_rpath "/.data/lib/")
  else()
    # taken from setup.py
    set(libraries_rpath "/../../")
  endif()

  # hoc*.so
  set(hoc_module_name "hoc_module")
  add_nrn_python_library(
    hoc
    TARGET
    ${hoc_module_name}
    PYTHON_VERSION
    ${NRN_DEFAULT_PYTHON_VERSION}
    LANGUAGE
    "CXX"
    SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/inithoc.cpp
    INCLUDES
    ${PROJECT_SOURCE_DIR}/src
    ${PROJECT_SOURCE_DIR}/src/oc
    ${PROJECT_BINARY_DIR}/src/oc
    ${PROJECT_BINARY_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}
    # for nrnconf.h
    ${CMAKE_CURRENT_BINARY_DIR}
    ${PROJECT_BINARY_DIR}/nrnmpi
    ${pyinc}
    LIBRARIES
    nrniv_lib
    rxdmath
    REL_RPATH
    ${libraries_rpath}
    OUTPUT_DIR
    ${PROJECT_BINARY_DIR}/lib/python/neuron)

  add_dependencies(${hoc_module_name} nrniv_lib rxdmath)

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

  install(DIRECTORY ${NRN_PYTHON_BUILD_LIB} DESTINATION lib)

else()
  # Make sure this is included in the wheels
  install(FILES "${PROJECT_BINARY_DIR}/lib/python/neuron/_config_params.py"
          DESTINATION lib/python/neuron)
  install(TARGETS ${hoc_module_name} DESTINATION lib/python/neuron)
endif()
