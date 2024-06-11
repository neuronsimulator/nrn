set(SOURCES
    graphicsPrimitives
    ctng
    surfaces)

foreach(source ${SOURCES})
add_custom_command(
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${source}.cpp
  COMMAND Python::Interpreter -m cython
  "${CMAKE_CURRENT_SOURCE_DIR}/${source}.pyx" --output-file ${CMAKE_CURRENT_BINARY_DIR}/${source}.pyx
  DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${source}.pyx
  VERBATIM)
endforeach()
