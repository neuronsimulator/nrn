# Add directory-level default compiler flags -- these should be added to all NEURON targets, but not
# targets from included projects like CoreNEURON and NMODL
add_compile_options(${NRN_COMPILE_FLAGS})
add_compile_definitions(${NRN_COMPILE_DEFS})
add_link_options(${NRN_LINK_FLAGS})

include(${PROJECT_SOURCE_DIR}/cmake/PythonCompileHelper.cmake)

# Some modules should be placed in the `neuron` Python directory (as they are directly importable),
# while others should be placed in `neuron/.data/lib` (as they are not directly importable).
if(NOT SKBUILD)
  file(RELATIVE_PATH rel_path "${CMAKE_INSTALL_PREFIX}/${NRN_INSTALL_PYTHON_PREFIX}"
       "${CMAKE_INSTALL_PREFIX}/${NRN_INSTALL_DATA_PREFIX}/lib")
  set(libraries_rpath "/${rel_path}")
else()
  set(libraries_rpath "/.data/lib/")
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
    target_link_libraries(nrnpython${pyver} PRIVATE fmt::fmt ${Readline_LIBRARY} ${nanobind_target})
    if(NRN_LINK_AGAINST_PYTHON)
      target_link_libraries(nrnpython${pyver} PUBLIC ${pylib})
    endif()
    add_dependencies(nrnpython${pyver} nrniv_lib)
    list(APPEND nrnpython_lib_list nrnpython${pyver})
    install(TARGETS nrnpython${pyver} DESTINATION ${NRN_INSTALL_SHARE_LIB_DIR})

    # the hoc Python library
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
      INSTALL_REL_RPATH
      ${libraries_rpath}
      OUTPUT_DIR
      ${PROJECT_BINARY_DIR}/lib/python/neuron)

    if(NRN_LINK_AGAINST_PYTHON)
      target_link_libraries(${hoc_module_name} PUBLIC ${pylib})
    endif()

    add_dependencies(${hoc_module_name} nrniv_lib rxdmath)

    install(TARGETS ${hoc_module_name} DESTINATION ${NRN_INSTALL_PYTHON_PREFIX})

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

  # the hoc Python library
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
    ${NRN_DEFAULT_PYTHON_INCLUDES}
    LIBRARIES
    nrniv_lib
    rxdmath
    INSTALL_REL_RPATH
    ${libraries_rpath}
    OUTPUT_DIR
    ${PROJECT_BINARY_DIR}/lib/python/neuron)

  add_dependencies(${hoc_module_name} nrniv_lib rxdmath)

  install(TARGETS ${hoc_module_name} DESTINATION ${NRN_INSTALL_PYTHON_PREFIX})

endif()

configure_file(_config_params.py.in "${PROJECT_BINARY_DIR}/lib/python/neuron/_config_params.py"
               @ONLY)

install(FILES "${PROJECT_BINARY_DIR}/lib/python/neuron/_config_params.py"
        DESTINATION ${NRN_INSTALL_PYTHON_PREFIX})
