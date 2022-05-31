# =============================================================================
# Various project components and their source files
# =============================================================================
set(BISON_GENERATED_SOURCE_FILES
    ${PROJECT_BINARY_DIR}/src/parser/nmodl/nmodl_parser.cpp
    ${PROJECT_BINARY_DIR}/src/parser/verbatim_parser.cpp
    ${PROJECT_BINARY_DIR}/src/parser/diffeq/diffeq_parser.cpp
    ${PROJECT_BINARY_DIR}/src/parser/c/c11_parser.cpp
    ${PROJECT_BINARY_DIR}/src/parser/unit/unit_parser.cpp)
set(BISON_GENERATED_HEADER_FILES
    ${PROJECT_BINARY_DIR}/src/parser/nmodl/location.hh
    ${PROJECT_BINARY_DIR}/src/parser/nmodl/nmodl_parser.hpp
    ${PROJECT_BINARY_DIR}/src/parser/nmodl/position.hh
    ${PROJECT_BINARY_DIR}/src/parser/verbatim_parser.hpp
    ${PROJECT_BINARY_DIR}/src/parser/diffeq/diffeq_parser.hpp
    ${PROJECT_BINARY_DIR}/src/parser/c/c11_parser.hpp
    ${PROJECT_BINARY_DIR}/src/parser/unit/unit_parser.hpp)

set(UNIT_SOURCE_FILES ${NMODL_PROJECT_SOURCE_DIR}/src/units/units.cpp)

set(NMODL_DRIVER_FILES ${NMODL_PROJECT_SOURCE_DIR}/src/parser/nmodl_driver.cpp)

set(DIFFEQ_DRIVER_FILES ${NMODL_PROJECT_SOURCE_DIR}/src/parser/diffeq_driver.cpp
                        ${NMODL_PROJECT_SOURCE_DIR}/src/parser/diffeq_context.cpp)

set(C_DRIVER_FILES ${NMODL_PROJECT_SOURCE_DIR}/src/parser/c11_driver.cpp)

set_source_files_properties(${AST_GENERATED_SOURCES} PROPERTIES GENERATED TRUE)

set(UNIT_DRIVER_FILES ${NMODL_PROJECT_SOURCE_DIR}/src/parser/unit_driver.cpp)

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

if(NMODL_PGI_COMPILER)
  # "verbatim_lexer.cpp", warning #550-D: variable "..." was set but never used
  set_source_files_properties(${CMAKE_CURRENT_BINARY_DIR}/verbatim_lexer.cpp
                              PROPERTIES COMPILE_FLAGS "--diag_suppress 550")
endif()

# =============================================================================
# Directories for parsers (as they need to be in separate directories)
# =============================================================================
file(MAKE_DIRECTORY ${PROJECT_BINARY_DIR}/src/parser/nmodl ${PROJECT_BINARY_DIR}/src/parser/diffeq
     ${PROJECT_BINARY_DIR}/src/parser/c ${PROJECT_BINARY_DIR}/src/parser/unit)

# =============================================================================
# Lexer & Parser commands
# =============================================================================
# Command to generate nmodl parser Construct a relative path to avoid baking any absolute paths into
# the output .cpp and .hpp files (via __FILE__ and so on), which cause ccache misses when the build
# prefix changes (e.g. GitLab CI).
set(NMODL_PARSER_SOURCE_DIR "${NMODL_PROJECT_SOURCE_DIR}/src/parser")
set(NMODL_PARSER_BINARY_DIR "${NMODL_PROJECT_BINARY_DIR}/src/parser")
file(RELATIVE_PATH NMODL_YY_FROM_PARSER_BINARY_DIR "${NMODL_PARSER_BINARY_DIR}"
     "${NMODL_PARSER_SOURCE_DIR}/nmodl.yy")
add_custom_command(
  OUTPUT "${NMODL_PARSER_BINARY_DIR}/nmodl/location.hh"
         "${NMODL_PARSER_BINARY_DIR}/nmodl/nmodl_parser.cpp"
         "${NMODL_PARSER_BINARY_DIR}/nmodl/nmodl_parser.hpp"
         "${NMODL_PARSER_BINARY_DIR}/nmodl/position.hh"
         "${NMODL_PARSER_BINARY_DIR}/nmodl/stack.hh"
  WORKING_DIRECTORY "${NMODL_PARSER_BINARY_DIR}"
  COMMAND ${BISON_EXECUTABLE} ARGS -d -o nmodl/nmodl_parser.cpp "${NMODL_YY_FROM_PARSER_BINARY_DIR}"
  DEPENDS "${NMODL_PARSER_SOURCE_DIR}/nmodl.yy" pyastgen
  COMMENT "-- NMODL : GENERATING NMODL_CORE PARSER WITH BISON! --")

# Command to generate verbatim parser. See comment above about absolute paths.
file(RELATIVE_PATH VERBATIM_YY_FROM_PARSER_BINARY_DIR "${NMODL_PARSER_BINARY_DIR}"
     "${NMODL_PARSER_SOURCE_DIR}/verbatim.yy")
add_custom_command(
  OUTPUT "${NMODL_PARSER_BINARY_DIR}/verbatim_parser.cpp"
         "${NMODL_PARSER_BINARY_DIR}/verbatim_parser.hpp"
  WORKING_DIRECTORY "${NMODL_PARSER_BINARY_DIR}"
  COMMAND ${BISON_EXECUTABLE} ARGS -d -o verbatim_parser.cpp "${VERBATIM_YY_FROM_PARSER_BINARY_DIR}"
  DEPENDS "${NMODL_PARSER_SOURCE_DIR}/verbatim.yy"
  COMMENT "-- NMODL : GENERATING VERBATIM PARSER WITH BISON! --")

# Command to generate differential equation parser. See comment above about absolute paths.
file(RELATIVE_PATH DIFFEQ_YY_FROM_PARSER_BINARY_DIR "${NMODL_PARSER_BINARY_DIR}"
     "${NMODL_PARSER_SOURCE_DIR}/diffeq.yy")
add_custom_command(
  OUTPUT "${NMODL_PARSER_BINARY_DIR}/diffeq/diffeq_parser.cpp"
         "${NMODL_PARSER_BINARY_DIR}/diffeq/diffeq_parser.hpp"
         "${NMODL_PARSER_BINARY_DIR}/diffeq/stack.hh"
  WORKING_DIRECTORY "${NMODL_PARSER_BINARY_DIR}"
  COMMAND ${BISON_EXECUTABLE} ARGS -d -o diffeq/diffeq_parser.cpp
          "${DIFFEQ_YY_FROM_PARSER_BINARY_DIR}"
  DEPENDS "${NMODL_PARSER_SOURCE_DIR}/diffeq.yy" "${NMODL_PARSER_SOURCE_DIR}/diffeq_context.hpp"
          "${NMODL_PARSER_SOURCE_DIR}/diffeq_context.cpp"
          "${NMODL_PARSER_SOURCE_DIR}/diffeq_helper.hpp"
  COMMENT "-- NMODL : GENERATING DIFFERENTIAL EQUATION PARSER WITH BISON! --")

# Command to generate C (11) parser. See comment above about absolute paths.
file(RELATIVE_PATH C11_YY_FROM_PARSER_BINARY_DIR "${NMODL_PARSER_BINARY_DIR}"
     "${NMODL_PARSER_SOURCE_DIR}/c11.yy")
add_custom_command(
  OUTPUT "${NMODL_PARSER_BINARY_DIR}/c/c11_parser.cpp" "${NMODL_PARSER_BINARY_DIR}/c/c11_parser.hpp"
         "${NMODL_PARSER_BINARY_DIR}/c/stack.hh"
  WORKING_DIRECTORY "${NMODL_PARSER_BINARY_DIR}"
  COMMAND ${BISON_EXECUTABLE} ARGS -d -o c/c11_parser.cpp "${C11_YY_FROM_PARSER_BINARY_DIR}"
  DEPENDS "${NMODL_PARSER_SOURCE_DIR}/c11.yy"
  COMMENT "-- NMODL : GENERATING C (11) PARSER WITH BISON! --")

# Command to generate Units parser. See comment above about absolute paths.
file(RELATIVE_PATH UNIT_YY_FROM_PARSER_BINARY_DIR "${NMODL_PARSER_BINARY_DIR}"
     "${NMODL_PARSER_SOURCE_DIR}/unit.yy")
add_custom_command(
  OUTPUT "${NMODL_PARSER_BINARY_DIR}/unit/unit_parser.cpp"
         "${NMODL_PARSER_BINARY_DIR}/unit/unit_parser.hpp"
         "${NMODL_PARSER_BINARY_DIR}/unit/stack.hh"
  WORKING_DIRECTORY "${NMODL_PARSER_BINARY_DIR}"
  COMMAND ${BISON_EXECUTABLE} ARGS -d -o unit/unit_parser.cpp "${UNIT_YY_FROM_PARSER_BINARY_DIR}"
  DEPENDS "${NMODL_PARSER_SOURCE_DIR}/unit.yy"
  COMMENT "-- NMODL : GENERATING UNIT PARSER WITH BISON! --")

# Command to generate nmodl lexer. See comment above about absolute paths.
file(RELATIVE_PATH NMODL_LL_FROM_BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}"
     "${CMAKE_CURRENT_SOURCE_DIR}/nmodl.ll")
add_custom_command(
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/nmodl_base_lexer.cpp
         ${CMAKE_CURRENT_BINARY_DIR}/nmodl_base_lexer.hpp
  COMMAND ${FLEX_EXECUTABLE} ARGS "${NMODL_LL_FROM_BINARY_DIR}"
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/
  DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/nmodl.ll ${CMAKE_CURRENT_SOURCE_DIR}/nmodl_utils.hpp
  COMMENT "-- NMODL : GENERATING NMODL LEXER WITH FLEX! --")

# Command to generate verbatim lexer. See comment above about absolute paths.
file(RELATIVE_PATH VERBATIM_LL_FROM_BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}"
     "${CMAKE_CURRENT_SOURCE_DIR}/verbatim.l")
add_custom_command(
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/verbatim_lexer.cpp
         ${CMAKE_CURRENT_BINARY_DIR}/verbatim_lexer.hpp
  COMMAND ${FLEX_EXECUTABLE} ARGS "${VERBATIM_LL_FROM_BINARY_DIR}"
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/verbatim.l
  COMMENT "-- NMODL : GENERATING VERBATIM LEXER WITH FLEX! --")

# Command to generate differential equation lexer. See comment above about absolute paths.
file(RELATIVE_PATH DIFFEQ_LL_FROM_BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}"
     "${CMAKE_CURRENT_SOURCE_DIR}/diffeq.ll")
add_custom_command(
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/diffeq_base_lexer.cpp
         ${CMAKE_CURRENT_BINARY_DIR}/diffeq_base_lexer.hpp
  COMMAND ${FLEX_EXECUTABLE} ARGS "${DIFFEQ_LL_FROM_BINARY_DIR}"
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/diffeq.ll
  COMMENT "-- NMODL : GENERATING DIFFERENTIAL EQUATION LEXER WITH FLEX! --")

# Command to generate C (11) lexer. See comment above about absolute paths.
file(RELATIVE_PATH C11_LL_FROM_BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}"
     "${CMAKE_CURRENT_SOURCE_DIR}/c11.ll")
add_custom_command(
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/c11_base_lexer.cpp
         ${CMAKE_CURRENT_BINARY_DIR}/c11_base_lexer.hpp
  COMMAND ${FLEX_EXECUTABLE} ARGS "${C11_LL_FROM_BINARY_DIR}"
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/c11.ll
  COMMENT "-- NMODL : GENERATING C(11) LEXER WITH FLEX! --")

# Command to generate Units lexer. See comment above about absolute paths.
file(RELATIVE_PATH UNIT_LL_FROM_BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}"
     "${CMAKE_CURRENT_SOURCE_DIR}/unit.ll")
add_custom_command(
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/unit_base_lexer.cpp
         ${CMAKE_CURRENT_BINARY_DIR}/unit_base_lexer.hpp
  COMMAND ${FLEX_EXECUTABLE} ARGS "${UNIT_LL_FROM_BINARY_DIR}"
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
