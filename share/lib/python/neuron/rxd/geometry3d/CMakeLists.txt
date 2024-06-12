add_compile_options(${NRN_COMPILE_FLAGS})
add_compile_definitions(${NRN_COMPILE_DEFS})
add_link_options(${NRN_LINK_FLAGS})

set(SOURCES
    graphicsPrimitives
    ctng
    surfaces)

foreach(source ${SOURCES})
add_custom_command(
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${source}.cpp
  COMMAND ${PYTHON_EXECUTABLE} -m cython
  "${CMAKE_CURRENT_SOURCE_DIR}/${source}.pyx" --output-file ${CMAKE_CURRENT_BINARY_DIR}/${source}.cpp
  DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${source}.pyx
  VERBATIM)
endforeach()

add_library(
    surfaces
    SHARED
    ${CMAKE_CURRENT_BINARY_DIR}/surfaces.cpp
    ${PROJECT_SOURCE_DIR}/src/nrnpython/rxd_marching_cubes.cpp
    ${PROJECT_SOURCE_DIR}/src/nrnpython/rxd_llgramarea.cpp
)

target_include_directories(surfaces PUBLIC "${NRN_PYTHON_INCLUDES}" "${NRN_PYTHON_NUMPY_INCLUDES}")
target_link_libraries(
    surfaces
    nrniv_lib
    rxdmath)

if(NRN_ENABLE_PYTHON_DYNAMIC)
  foreach(val RANGE ${NRN_PYTHON_ITERATION_LIMIT})
    target_link_libraries(surfaces nrnpython${pyver})
  endforeach()
else()
  target_link_libraries(surfaces ${NRN_DEFAULT_PYTHON_LIBRARIES})
endif()



install(TARGETS surfaces DESTINATION ${NRN_INSTALL_SHARE_LIB_DIR})
