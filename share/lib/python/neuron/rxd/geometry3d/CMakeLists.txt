include(${PROJECT_SOURCE_DIR}/cmake/PythonCompileHelper.cmake)

# convert pyx files to C files using Cython we only need to use the default Python executable since
# the output of the transpilation (from pyx to C) does not depend on which Python version we are
# using
set(rxd_sources graphicsPrimitives ctng surfaces)
foreach(source ${rxd_sources})
  cythonize(
    "${CMAKE_CURRENT_SOURCE_DIR}/${source}.pyx"
    OUTPUT
    ${CMAKE_CURRENT_BINARY_DIR}/${source}.cpp
    LANGUAGE
    "C"
    PYTHON_EXECUTABLE
    ${NRN_DEFAULT_PYTHON_EXECUTABLE})
endforeach(source)

set(graphicsPrimitives_sources ${CMAKE_CURRENT_BINARY_DIR}/graphicsPrimitives.cpp)
set(ctng_sources ${CMAKE_CURRENT_BINARY_DIR}/ctng.cpp)
set(surfaces_sources
    ${CMAKE_CURRENT_BINARY_DIR}/surfaces.cpp
    ${PROJECT_SOURCE_DIR}/src/nrnpython/rxd_marching_cubes.cpp
    ${PROJECT_SOURCE_DIR}/src/nrnpython/rxd_llgramarea.cpp)

if(NRN_ENABLE_MODULE_INSTALL)
  set(rel_rpath "/../../")
else()
  set(rel_rpath "/../../.data/lib")
endif()

# case of dynamic Python
if(NRN_ENABLE_PYTHON_DYNAMIC)
  foreach(val RANGE ${NRN_PYTHON_ITERATION_LIMIT})
    list(GET NRN_PYTHON_VERSIONS ${val} pyver)
    list(GET NRN_PYTHON_INCLUDES ${val} pyinc)
    list(GET NRN_PYTHON_LIBRARIES ${val} pylib)
    list(GET NRN_PYTHON_NUMPY_INCLUDES ${val} npinc)

    foreach(source ${rxd_sources})
      add_nrn_python_library(
        ${source}
        TARGET
        ${source}${pyver}
        PYTHON_VERSION
        ${pyver}
        LANGUAGE
        "C"
        SOURCES
        ${${source}_sources}
        INCLUDES
        "${pyinc}"
        "${npinc}"
        LIBRARIES
        nrnpython${pyver}
        REL_RPATH
        ${rel_rpath}
        OUTPUT_DIR
        ${PROJECT_BINARY_DIR}/lib/python/neuron/rxd/geometry3d)
      target_compile_options(${source}${pyver} PRIVATE "-O${NRN_RX3D_OPT_LEVEL}")
      if(APPLE)
        target_link_options(${source}${pyver} PRIVATE "-headerpad_max_install_names")
      endif()
    endforeach()

    # install in the correct location
    install(TARGETS graphicsPrimitives${pyver} ctng${pyver} surfaces${pyver}
            DESTINATION lib/python/neuron/rxd/geometry3d)

  endforeach(val)
else(NRN_ENABLE_PYTHON_DYNAMIC)
  foreach(source ${rxd_sources})
    add_nrn_python_library(
      ${source}
      TARGET
      ${source}
      PYTHON_VERSION
      ${NRN_DEFAULT_PYTHON_VERSION}
      LANGUAGE
      "C"
      SOURCES
      ${${source}_sources}
      INCLUDES
      "${NRN_DEFAULT_PYTHON_INCLUDES}"
      "${NRN_DEFAULT_PYTHON_NUMPY_INCLUDES}"
      LIBRARIES
      nrnpython${NRN_DEFAULT_PYTHON_VERSION}
      REL_RPATH
      ${rel_rpath}
      OUTPUT_DIR
      ${PROJECT_BINARY_DIR}/lib/python/neuron/rxd/geometry3d)
    target_compile_options(${source} PRIVATE "-O${NRN_RX3D_OPT_LEVEL}")
    if(APPLE)
      target_link_options(${source} PRIVATE "-headerpad_max_install_names")
    endif()

  endforeach()
  install(TARGETS graphicsPrimitives ctng surfaces DESTINATION lib/python/neuron/rxd/geometry3d)

endif(NRN_ENABLE_PYTHON_DYNAMIC)
