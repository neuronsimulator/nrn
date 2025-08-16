# Read headers, remove everything up to and including "#pragma once", then remove header includes.
file(READ ${CMAKE_CURRENT_SOURCE_DIR}/crout/crout.hpp NMODL_CROUT_HPP_RAW)
string(REGEX REPLACE ".*#pragma once[ \t\r\n]*" "" NMODL_CROUT_HPP "${NMODL_CROUT_HPP_RAW}")
file(READ ${CMAKE_CURRENT_SOURCE_DIR}/newton/newton.hpp NMODL_NEWTON_HPP_RAW)
string(REGEX REPLACE ".*#pragma once[ \t\r\n]*" "" NMODL_NEWTON_HPP_TMP "${NMODL_NEWTON_HPP_RAW}")
string(REGEX REPLACE "#include <crout/crout.hpp>[ \t\r\n]*" "" NMODL_NEWTON_HPP
                     "${NMODL_NEWTON_HPP_TMP}")
file(READ ${CMAKE_CURRENT_SOURCE_DIR}/matexp/MatrixExponential.h NMODL_MATEXP_H)
set_property(
  DIRECTORY
  APPEND
  PROPERTY CMAKE_CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/crout/crout.hpp
           ${CMAKE_CURRENT_SOURCE_DIR}/newton/newton.hpp
           ${CMAKE_CURRENT_SOURCE_DIR}/matexp/MatrixExponential.h)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/solver.hpp.inc ${CMAKE_CURRENT_BINARY_DIR}/solver.hpp)

add_custom_target(nmodl_copy_solver_files ALL DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/solver.hpp")
