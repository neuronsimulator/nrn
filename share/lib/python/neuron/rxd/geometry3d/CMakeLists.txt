include(${PROJECT_SOURCE_DIR}/cmake/PythonCompileHelper.cmake)

# Convert pyx files to C files using Cython.
set(rxd_sources graphicsPrimitives ctng)
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

if(NOT SKBUILD)
  set(rel_rpath "/../../")
else()
  set(rel_rpath "/../../.data/lib")
endif()

# find numpy includes of the default Python install
execute_process(
  COMMAND ${NRN_DEFAULT_PYTHON_EXECUTABLE} -c "import numpy;print(numpy.get_include())"
  OUTPUT_STRIP_TRAILING_WHITESPACE
  RESULT_VARIABLE STATUS_CODE
  OUTPUT_VARIABLE NUMPY_INCLUDE_DIR)

if(NOT "${STATUS_CODE}" STREQUAL "0")
  message(FATAL_ERROR "Could not find NumPy headers for ${NRN_DEFAULT_PYTHON_EXECUTABLE}")
endif()

# case of dynamic Python
if(NRN_ENABLE_PYTHON_DYNAMIC)
  foreach(val RANGE ${NRN_PYTHON_ITERATION_LIMIT})
    list(GET NRN_PYTHON_VERSIONS ${val} pyver)
    list(GET NRN_PYTHON_INCLUDES ${val} pyinc)
    list(GET NRN_PYTHON_LIBRARIES ${val} pylib)
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
        # on Linux, if numpy is installed via the system package manager, CMake would use those
        # instead, so we place the numpy headers before the Python ones
        "${NUMPY_INCLUDE_DIR}"
        "${pyinc}"
        LIBRARIES
        nrniv_lib
        rxdmath
        nrnpython${pyver}
        INSTALL_REL_RPATH
        ${rel_rpath}
        OUTPUT_DIR
        ${PROJECT_BINARY_DIR}/lib/python/neuron/rxd/geometry3d)
      target_compile_options(${source}${pyver} PRIVATE "-O${NRN_RX3D_OPT_LEVEL}")
      target_compile_definitions(${source}${pyver}
                                 PRIVATE NPY_NO_DEPRECATED_API=NPY_1_7_API_VERSION)
      if(APPLE)
        target_link_options(${source}${pyver} PRIVATE "-headerpad_max_install_names")
      endif()
    endforeach()

    # install in the correct location
    install(TARGETS graphicsPrimitives${pyver} ctng${pyver}
            DESTINATION ${NRN_INSTALL_PYTHON_PREFIX}/rxd/geometry3d)

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
      # on Linux, if numpy is installed via the system package manager, CMake would use those
      # instead, so we place the numpy headers before the Python ones
      "${NUMPY_INCLUDE_DIR}"
      "${NRN_DEFAULT_PYTHON_INCLUDES}"
      LIBRARIES
      nrniv_lib
      rxdmath
      INSTALL_REL_RPATH
      ${rel_rpath}
      OUTPUT_DIR
      ${PROJECT_BINARY_DIR}/lib/python/neuron/rxd/geometry3d)
    target_compile_options(${source} PRIVATE "-O${NRN_RX3D_OPT_LEVEL}")
    target_compile_definitions(${source} PRIVATE NPY_NO_DEPRECATED_API=NPY_1_7_API_VERSION)
    if(APPLE)
      target_link_options(${source} PRIVATE "-headerpad_max_install_names")
    endif()

  endforeach()
  install(TARGETS graphicsPrimitives ctng DESTINATION ${NRN_INSTALL_PYTHON_PREFIX}/rxd/geometry3d)


endif(NRN_ENABLE_PYTHON_DYNAMIC)
