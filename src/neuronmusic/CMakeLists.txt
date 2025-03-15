include(${PROJECT_SOURCE_DIR}/cmake/PythonCompileHelper.cmake)

set(source neuronmusic)

cythonize(
  "${CMAKE_CURRENT_SOURCE_DIR}/${source}.pyx"
  OUTPUT
  ${CMAKE_CURRENT_BINARY_DIR}/${source}.cpp
  LANGUAGE
  "C"
  PYTHON_EXECUTABLE
  ${NRN_DEFAULT_PYTHON_EXECUTABLE})

if(NRN_ENABLE_PYTHON_DYNAMIC)
  foreach(val RANGE ${NRN_PYTHON_ITERATION_LIMIT})
    list(GET NRN_PYTHON_VERSIONS ${val} pyver)
    list(GET NRN_PYTHON_INCLUDES ${val} pyinc)
    list(GET NRN_PYTHON_LIBRARIES ${val} pylib)

    add_nrn_python_library(
      ${source}
      TARGET
      ${source}${pyver}
      PYTHON_VERSION
      ${pyver}
      LANGUAGE
      "CXX"
      SOURCES
      ${CMAKE_CURRENT_BINARY_DIR}/${source}.cpp
      INCLUDES
      "${pyinc}"
      "${MUSIC_ROOT}/include"
      "${MPI_C_INCLUDE_DIRS}"
      ${PROJECT_SOURCE_DIR}/src/nrnpython
      ${CMAKE_CURRENT_SOURCE_DIR}
      LIBRARIES
      nrnpython${pyver}
      REL_RPATH
      ${rel_rpath}
      OUTPUT_DIR
      ${PROJECT_BINARY_DIR}/lib/python)

  endforeach()
else()
  add_nrn_python_library(
    ${source}
    TARGET
    ${source}
    PYTHON_VERSION
    ${NRN_DEFAULT_PYTHON_VERSION}
    LANGUAGE
    "CXX"
    SOURCES
    ${CMAKE_CURRENT_BINARY_DIR}/${source}.cpp
    INCLUDES
    "${NRN_DEFAULT_PYTHON_INCLUDES}"
    "${MUSIC_ROOT}/include"
    "${MPI_C_INCLUDE_DIRS}"
    ${PROJECT_SOURCE_DIR}/src/nrnpython
    ${CMAKE_CURRENT_SOURCE_DIR}
    OUTPUT_DIR
    ${PROJECT_BINARY_DIR}/lib/python)

endif()
