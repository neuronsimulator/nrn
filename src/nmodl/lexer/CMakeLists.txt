# =============================================================================
# Various project components and their source files
# =============================================================================
set(BISON_GENERATED_SOURCE_FILES
    ${CMAKE_CURRENT_BINARY_DIR}/../parser/nmodl/nmodl_parser.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/../parser/verbatim_parser.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/../parser/diffeq/diffeq_parser.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/../parser/c/c11_parser.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/../parser/unit/unit_parser.cpp)
set(BISON_GENERATED_HEADER_FILES
    ${CMAKE_CURRENT_BINARY_DIR}/../parser/nmodl/location.hh
    ${CMAKE_CURRENT_BINARY_DIR}/../parser/nmodl/nmodl_parser.hpp
    ${CMAKE_CURRENT_BINARY_DIR}/../parser/nmodl/position.hh
    ${CMAKE_CURRENT_BINARY_DIR}/../parser/verbatim_parser.hpp
    ${CMAKE_CURRENT_BINARY_DIR}/../parser/diffeq/diffeq_parser.hpp
    ${CMAKE_CURRENT_BINARY_DIR}/../parser/c/c11_parser.hpp
    ${CMAKE_CURRENT_BINARY_DIR}/../parser/unit/unit_parser.hpp)

set(UNIT_SOURCE_FILES ${CMAKE_CURRENT_SOURCE_DIR}/../units/units.cpp)

set(NMODL_DRIVER_FILES ${CMAKE_CURRENT_SOURCE_DIR}/../parser/nmodl_driver.cpp)

set(DIFFEQ_DRIVER_FILES ${CMAKE_CURRENT_SOURCE_DIR}/../parser/diffeq_driver.cpp
                        ${CMAKE_CURRENT_SOURCE_DIR}/../parser/diffeq_context.cpp)

set(C_DRIVER_FILES ${CMAKE_CURRENT_SOURCE_DIR}/../parser/c11_driver.cpp)

set_source_files_properties(${AST_GENERATED_SOURCES} PROPERTIES GENERATED TRUE)

set(UNIT_DRIVER_FILES ${CMAKE_CURRENT_SOURCE_DIR}/../parser/unit_driver.cpp)

set(LEXER_SOURCE_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/token_mapping.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/nmodl_utils.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/modtoken.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/nmodl_base_lexer.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/verbatim_lexer.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/diffeq_base_lexer.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/c11_base_lexer.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/unit_base_lexer.cpp
    ${NMODL_DRIVER_FILES}
    ${DIFFEQ_DRIVER_FILES}
    ${C_DRIVER_FILES}
    ${UNIT_DRIVER_FILES})

# ~~~
# modify invocation of bison (use -M flag if version 3.7 or above)
# ~~~
function(compose_bison_command OUT_VAR)
  cmake_parse_arguments(ARG "" "" "PATHS" ${ARGN})
  set(BISON_CMD ${BISON_EXECUTABLE})
  if(BISON_VERSION VERSION_GREATER_EQUAL "3.7")
    foreach(path IN LISTS ARG_PATHS)
      list(APPEND BISON_CMD -M "${path}")
    endforeach()
  endif()
  set(${OUT_VAR}
      "${BISON_CMD}"
      PARENT_SCOPE)
endfunction()

if(NMODL_PGI_COMPILER)
  # "verbatim_lexer.cpp", warning #550-D: variable "..." was set but never used
  set_source_files_properties(${CMAKE_CURRENT_BINARY_DIR}/verbatim_lexer.cpp
                              PROPERTIES COMPILE_FLAGS "--diag_suppress 550")
endif()

set(NMODL_PARSER_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../parser")
set(NMODL_PARSER_BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}/../parser")

# =============================================================================
# Directories for parsers (as they need to be in separate directories)
# =============================================================================
file(MAKE_DIRECTORY ${NMODL_PARSER_BINARY_DIR}/nmodl ${NMODL_PARSER_BINARY_DIR}/diffeq
     ${NMODL_PARSER_BINARY_DIR}/c ${NMODL_PARSER_BINARY_DIR}/unit)

# =============================================================================
# Lexer & Parser commands
# =============================================================================
# Command to generate nmodl parser Construct a relative path to avoid baking any absolute paths into
# the output .cpp and .hpp files (via __FILE__ and so on), which cause ccache misses when the build
# prefix changes (e.g. GitLab CI).
file(RELATIVE_PATH NMODL_YY_FROM_PARSER_BINARY_DIR "${NMODL_PARSER_BINARY_DIR}"
     "${NMODL_PARSER_SOURCE_DIR}/nmodl.yy")
compose_bison_command(
  BISON_CMD PATHS "${NMODL_YY_FROM_PARSER_BINARY_DIR}=../${NMODL_YY_FROM_PARSER_BINARY_DIR}"
  "nmodl/location.hh=src/nmodl/parser/nmodl/location.hh"
  "nmodl/nmodl_parser.hpp=src/nmodl/parser/nmodl/nmodl_parser.hpp")
add_custom_command(
  OUTPUT "${NMODL_PARSER_BINARY_DIR}/nmodl/location.hh"
         "${NMODL_PARSER_BINARY_DIR}/nmodl/nmodl_parser.cpp"
         "${NMODL_PARSER_BINARY_DIR}/nmodl/nmodl_parser.hpp"
         "${NMODL_PARSER_BINARY_DIR}/nmodl/position.hh"
         "${NMODL_PARSER_BINARY_DIR}/nmodl/stack.hh"
  WORKING_DIRECTORY "${NMODL_PARSER_BINARY_DIR}"
  COMMAND ${BISON_CMD} -d -o nmodl/nmodl_parser.cpp "${NMODL_YY_FROM_PARSER_BINARY_DIR}"
  DEPENDS "${NMODL_PARSER_SOURCE_DIR}/nmodl.yy" pyastgen
  COMMENT "-- NMODL : GENERATING NMODL_CORE PARSER WITH BISON! --")

# Command to generate verbatim parser. See comment above about absolute paths.
file(RELATIVE_PATH VERBATIM_YY_FROM_PARSER_BINARY_DIR "${NMODL_PARSER_BINARY_DIR}"
     "${NMODL_PARSER_SOURCE_DIR}/verbatim.yy")
add_custom_command(
  OUTPUT "${NMODL_PARSER_BINARY_DIR}/verbatim_parser.cpp"
         "${NMODL_PARSER_BINARY_DIR}/verbatim_parser.hpp"
  WORKING_DIRECTORY "${NMODL_PARSER_BINARY_DIR}"
  COMMAND ${BISON_EXECUTABLE} -d -o verbatim_parser.cpp "${VERBATIM_YY_FROM_PARSER_BINARY_DIR}"
  DEPENDS "${NMODL_PARSER_SOURCE_DIR}/verbatim.yy"
  COMMENT "-- NMODL : GENERATING VERBATIM PARSER WITH BISON! --")

# Command to generate differential equation parser. See comment above about absolute paths.
file(RELATIVE_PATH DIFFEQ_YY_FROM_PARSER_BINARY_DIR "${NMODL_PARSER_BINARY_DIR}"
     "${NMODL_PARSER_SOURCE_DIR}/diffeq.yy")
compose_bison_command(BISON_CMD PATHS
                      "${DIFFEQ_YY_FROM_PARSER_BINARY_DIR}=../${DIFFEQ_YY_FROM_PARSER_BINARY_DIR}")
add_custom_command(
  OUTPUT "${NMODL_PARSER_BINARY_DIR}/diffeq/diffeq_parser.cpp"
         "${NMODL_PARSER_BINARY_DIR}/diffeq/diffeq_parser.hpp"
         "${NMODL_PARSER_BINARY_DIR}/diffeq/stack.hh"
  WORKING_DIRECTORY "${NMODL_PARSER_BINARY_DIR}"
  COMMAND ${BISON_CMD} -d -o diffeq/diffeq_parser.cpp "${DIFFEQ_YY_FROM_PARSER_BINARY_DIR}"
  DEPENDS "${NMODL_PARSER_SOURCE_DIR}/diffeq.yy" "${NMODL_PARSER_SOURCE_DIR}/diffeq_context.hpp"
          "${NMODL_PARSER_SOURCE_DIR}/diffeq_context.cpp"
          "${NMODL_PARSER_SOURCE_DIR}/diffeq_helper.hpp"
  COMMENT "-- NMODL : GENERATING DIFFERENTIAL EQUATION PARSER WITH BISON! --")

# Command to generate C (11) parser. See comment above about absolute paths.
file(RELATIVE_PATH C11_YY_FROM_PARSER_BINARY_DIR "${NMODL_PARSER_BINARY_DIR}"
     "${NMODL_PARSER_SOURCE_DIR}/c11.yy")
compose_bison_command(BISON_CMD PATHS
                      "${C11_YY_FROM_PARSER_BINARY_DIR}=../${C11_YY_FROM_PARSER_BINARY_DIR}")
add_custom_command(
  OUTPUT "${NMODL_PARSER_BINARY_DIR}/c/c11_parser.cpp" "${NMODL_PARSER_BINARY_DIR}/c/c11_parser.hpp"
         "${NMODL_PARSER_BINARY_DIR}/c/stack.hh"
  WORKING_DIRECTORY "${NMODL_PARSER_BINARY_DIR}"
  COMMAND ${BISON_CMD} -d -o c/c11_parser.cpp "${C11_YY_FROM_PARSER_BINARY_DIR}"
  DEPENDS "${NMODL_PARSER_SOURCE_DIR}/c11.yy"
  COMMENT "-- NMODL : GENERATING C (11) PARSER WITH BISON! --")

# Command to generate Units parser. See comment above about absolute paths.
file(RELATIVE_PATH UNIT_YY_FROM_PARSER_BINARY_DIR "${NMODL_PARSER_BINARY_DIR}"
     "${NMODL_PARSER_SOURCE_DIR}/unit.yy")
compose_bison_command(
  BISON_CMD PATHS "${UNIT_YY_FROM_PARSER_BINARY_DIR}=../${UNIT_YY_FROM_PARSER_BINARY_DIR}"
  "unit/unit_parser.hpp=src/nmodl/parser/unit/unit_parser.hpp")
add_custom_command(
  OUTPUT "${NMODL_PARSER_BINARY_DIR}/unit/unit_parser.cpp"
         "${NMODL_PARSER_BINARY_DIR}/unit/unit_parser.hpp"
         "${NMODL_PARSER_BINARY_DIR}/unit/stack.hh"
  WORKING_DIRECTORY "${NMODL_PARSER_BINARY_DIR}"
  COMMAND ${BISON_CMD} -d -o unit/unit_parser.cpp "${UNIT_YY_FROM_PARSER_BINARY_DIR}"
  DEPENDS "${NMODL_PARSER_SOURCE_DIR}/unit.yy"
  COMMENT "-- NMODL : GENERATING UNIT PARSER WITH BISON! --")

# Command to generate nmodl lexer. See comment above about absolute paths.
file(RELATIVE_PATH NMODL_LL_FROM_BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}"
     "${CMAKE_CURRENT_SOURCE_DIR}/nmodl.ll")
add_custom_command(
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/nmodl_base_lexer.cpp
         ${CMAKE_CURRENT_BINARY_DIR}/nmodl_base_lexer.hpp
  COMMAND ${FLEX_EXECUTABLE} "${NMODL_LL_FROM_BINARY_DIR}"
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/
  DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/nmodl.ll ${CMAKE_CURRENT_SOURCE_DIR}/nmodl_utils.hpp
  COMMENT "-- NMODL : GENERATING NMODL LEXER WITH FLEX! --")

# Command to generate verbatim lexer. See comment above about absolute paths.
file(RELATIVE_PATH VERBATIM_LL_FROM_BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}"
     "${CMAKE_CURRENT_SOURCE_DIR}/verbatim.l")
add_custom_command(
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/verbatim_lexer.cpp
         ${CMAKE_CURRENT_BINARY_DIR}/verbatim_lexer.hpp
  COMMAND ${FLEX_EXECUTABLE} "${VERBATIM_LL_FROM_BINARY_DIR}"
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/verbatim.l
  COMMENT "-- NMODL : GENERATING VERBATIM LEXER WITH FLEX! --")

# Command to generate differential equation lexer. See comment above about absolute paths.
file(RELATIVE_PATH DIFFEQ_LL_FROM_BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}"
     "${CMAKE_CURRENT_SOURCE_DIR}/diffeq.ll")
add_custom_command(
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/diffeq_base_lexer.cpp
         ${CMAKE_CURRENT_BINARY_DIR}/diffeq_base_lexer.hpp
  COMMAND ${FLEX_EXECUTABLE} "${DIFFEQ_LL_FROM_BINARY_DIR}"
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/diffeq.ll
  COMMENT "-- NMODL : GENERATING DIFFERENTIAL EQUATION LEXER WITH FLEX! --")

# Command to generate C (11) lexer. See comment above about absolute paths.
file(RELATIVE_PATH C11_LL_FROM_BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}"
     "${CMAKE_CURRENT_SOURCE_DIR}/c11.ll")
add_custom_command(
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/c11_base_lexer.cpp
         ${CMAKE_CURRENT_BINARY_DIR}/c11_base_lexer.hpp
  COMMAND ${FLEX_EXECUTABLE} "${C11_LL_FROM_BINARY_DIR}"
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/c11.ll
  COMMENT "-- NMODL : GENERATING C(11) LEXER WITH FLEX! --")

# Command to generate Units lexer. See comment above about absolute paths.
file(RELATIVE_PATH UNIT_LL_FROM_BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}"
     "${CMAKE_CURRENT_SOURCE_DIR}/unit.ll")
add_custom_command(
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/unit_base_lexer.cpp
         ${CMAKE_CURRENT_BINARY_DIR}/unit_base_lexer.hpp
  COMMAND ${FLEX_EXECUTABLE} "${UNIT_LL_FROM_BINARY_DIR}"
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/unit.ll
  COMMENT "-- NMODL : GENERATING UNIT LEXER WITH FLEX! --")
# =============================================================================
# Libraries & executables
# =============================================================================

add_library(lexer STATIC ${LEXER_SOURCE_FILES} ${BISON_GENERATED_SOURCE_FILES}
                         ${AST_GENERATED_SOURCES} ${UNIT_SOURCE_FILES})
set_property(TARGET lexer PROPERTY POSITION_INDEPENDENT_CODE ON)
target_link_libraries(lexer PRIVATE util)

if(NOT NMODL_AS_SUBPROJECT)
  add_executable(nmodl_lexer main_nmodl.cpp)
  add_executable(c_lexer main_c.cpp)
  add_executable(units_lexer main_units.cpp)

  target_link_libraries(nmodl_lexer CLI11::CLI11 lexer util)
  target_link_libraries(c_lexer CLI11::CLI11 lexer util)
  target_link_libraries(units_lexer CLI11::CLI11 lexer util)
endif()

# =============================================================================
# Install executable
# =============================================================================
if(NOT NMODL_AS_SUBPROJECT)
  install(
    TARGETS nmodl_lexer
    DESTINATION ${NMODL_INSTALL_DIR_SUFFIX}bin/lexer
    CONFIGURATIONS Debug)
  install(
    TARGETS c_lexer
    DESTINATION ${NMODL_INSTALL_DIR_SUFFIX}bin/lexer
    CONFIGURATIONS Debug)
  install(
    TARGETS units_lexer
    DESTINATION ${NMODL_INSTALL_DIR_SUFFIX}bin/lexer
    CONFIGURATIONS Debug)
endif()

add_custom_target(parser-gen DEPENDS ${BISON_GENERATED_SOURCE_FILES}
                                     ${BISON_GENERATED_HEADER_FILES})
