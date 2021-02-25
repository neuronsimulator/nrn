# =============================================================================
# Various project components and their source files
# =============================================================================
set(BISON_GENERATED_SOURCE_FILES
    ${PROJECT_BINARY_DIR}/src/parser/nmodl/location.hh
    ${PROJECT_BINARY_DIR}/src/parser/nmodl/nmodl_parser.cpp
    ${PROJECT_BINARY_DIR}/src/parser/nmodl/nmodl_parser.hpp
    ${PROJECT_BINARY_DIR}/src/parser/nmodl/position.hh
    ${PROJECT_BINARY_DIR}/src/parser/verbatim_parser.cpp
    ${PROJECT_BINARY_DIR}/src/parser/verbatim_parser.hpp
    ${PROJECT_BINARY_DIR}/src/parser/diffeq/diffeq_parser.cpp
    ${PROJECT_BINARY_DIR}/src/parser/diffeq/diffeq_parser.hpp
    ${PROJECT_BINARY_DIR}/src/parser/c/c11_parser.cpp
    ${PROJECT_BINARY_DIR}/src/parser/c/c11_parser.hpp
    ${PROJECT_BINARY_DIR}/src/parser/unit/unit_parser.cpp
    ${PROJECT_BINARY_DIR}/src/parser/unit/unit_parser.hpp)

set(UNIT_SOURCE_FILES ${NMODL_PROJECT_SOURCE_DIR}/src/units/units.hpp
                      ${NMODL_PROJECT_SOURCE_DIR}/src/units/units.cpp)

set(NMODL_DRIVER_FILES ${NMODL_PROJECT_SOURCE_DIR}/src/parser/nmodl_driver.hpp
                       ${NMODL_PROJECT_SOURCE_DIR}/src/parser/nmodl_driver.cpp)

set(DIFFEQ_DRIVER_FILES
    ${NMODL_PROJECT_SOURCE_DIR}/src/parser/diffeq_driver.hpp
    ${NMODL_PROJECT_SOURCE_DIR}/src/parser/diffeq_driver.cpp
    ${NMODL_PROJECT_SOURCE_DIR}/src/parser/diffeq_context.hpp
    ${NMODL_PROJECT_SOURCE_DIR}/src/parser/diffeq_context.cpp
    ${NMODL_PROJECT_SOURCE_DIR}/src/parser/diffeq_helper.hpp)

set(C_DRIVER_FILES ${NMODL_PROJECT_SOURCE_DIR}/src/parser/c11_driver.hpp
                   ${NMODL_PROJECT_SOURCE_DIR}/src/parser/c11_driver.cpp)

set_source_files_properties(${AST_GENERATED_SOURCES} PROPERTIES GENERATED TRUE)

set(UNIT_DRIVER_FILES ${NMODL_PROJECT_SOURCE_DIR}/src/parser/unit_driver.hpp
                      ${NMODL_PROJECT_SOURCE_DIR}/src/parser/unit_driver.cpp)

set(LEXER_SOURCE_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/modl.h
    ${CMAKE_CURRENT_SOURCE_DIR}/token_mapping.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/token_mapping.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/nmodl_utils.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/nmodl_utils.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/modtoken.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/modtoken.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/nmodl_base_lexer.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/nmodl_base_lexer.hpp
    ${CMAKE_CURRENT_BINARY_DIR}/verbatim_lexer.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/verbatim_lexer.hpp
    ${CMAKE_CURRENT_BINARY_DIR}/diffeq_base_lexer.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/diffeq_base_lexer.hpp
    ${CMAKE_CURRENT_BINARY_DIR}/c11_base_lexer.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/c11_base_lexer.hpp
    ${CMAKE_CURRENT_BINARY_DIR}/unit_base_lexer.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/unit_base_lexer.hpp
    ${NMODL_DRIVER_FILES}
    ${DIFFEQ_DRIVER_FILES}
    ${C_DRIVER_FILES}
    ${UNIT_DRIVER_FILES})

# =============================================================================
# Directories for parsers (as they need to be in separate directories)
# =============================================================================
file(MAKE_DIRECTORY ${PROJECT_BINARY_DIR}/src/parser/nmodl ${PROJECT_BINARY_DIR}/src/parser/diffeq
     ${PROJECT_BINARY_DIR}/src/parser/c ${PROJECT_BINARY_DIR}/src/parser/unit)

# =============================================================================
# Lexer & Parser commands
# =============================================================================
# command to generate nmodl parser
add_custom_command(
  OUTPUT ${PROJECT_BINARY_DIR}/src/parser/nmodl/location.hh
         ${PROJECT_BINARY_DIR}/src/parser/nmodl/nmodl_parser.cpp
         ${PROJECT_BINARY_DIR}/src/parser/nmodl/nmodl_parser.hpp
         ${PROJECT_BINARY_DIR}/src/parser/nmodl/position.hh
         ${PROJECT_BINARY_DIR}/src/parser/nmodl/stack.hh
  COMMAND ${BISON_EXECUTABLE} ARGS -d -o ${PROJECT_BINARY_DIR}/src/parser/nmodl/nmodl_parser.cpp
          ${NMODL_PROJECT_SOURCE_DIR}/src/parser/nmodl.yy
  DEPENDS ${NMODL_PROJECT_SOURCE_DIR}/src/parser/nmodl.yy pyastgen
  COMMENT "-- NMODL : GENERATING NMODL_CORE PARSER WITH BISON! --")

# command to generate verbatim parser
add_custom_command(
  OUTPUT ${PROJECT_BINARY_DIR}/src/parser/verbatim_parser.cpp
         ${PROJECT_BINARY_DIR}/src/parser/verbatim_parser.hpp
  COMMAND ${BISON_EXECUTABLE} ARGS -d -o ${PROJECT_BINARY_DIR}/src/parser/verbatim_parser.cpp
          ${NMODL_PROJECT_SOURCE_DIR}/src/parser/verbatim.yy
  DEPENDS ${NMODL_PROJECT_SOURCE_DIR}/src/parser/verbatim.yy
  COMMENT "-- NMODL : GENERATING VERBATIM PARSER WITH BISON! --")

# command to generate differential equation parser
add_custom_command(
  OUTPUT ${PROJECT_BINARY_DIR}/src/parser/diffeq/diffeq_parser.cpp
         ${PROJECT_BINARY_DIR}/src/parser/diffeq/diffeq_parser.hpp
         ${PROJECT_BINARY_DIR}/src/parser/diffeq/stack.hh
  COMMAND ${BISON_EXECUTABLE} ARGS -d -o ${PROJECT_BINARY_DIR}/src/parser/diffeq/diffeq_parser.cpp
          ${NMODL_PROJECT_SOURCE_DIR}/src/parser/diffeq.yy
  DEPENDS ${NMODL_PROJECT_SOURCE_DIR}/src/parser/diffeq.yy
          ${NMODL_PROJECT_SOURCE_DIR}/src/parser/diffeq_context.hpp
          ${NMODL_PROJECT_SOURCE_DIR}/src/parser/diffeq_context.cpp
          ${NMODL_PROJECT_SOURCE_DIR}/src/parser/diffeq_helper.hpp
  COMMENT "-- NMODL : GENERATING DIFFERENTIAL EQUATION PARSER WITH BISON! --")

# command to generate C (11) parser
add_custom_command(
  OUTPUT ${PROJECT_BINARY_DIR}/src/parser/c/c11_parser.cpp
         ${PROJECT_BINARY_DIR}/src/parser/c/c11_parser.hpp
         ${PROJECT_BINARY_DIR}/src/parser/c/stack.hh
  COMMAND ${BISON_EXECUTABLE} ARGS -d -o ${PROJECT_BINARY_DIR}/src/parser/c/c11_parser.cpp
          ${NMODL_PROJECT_SOURCE_DIR}/src/parser/c11.yy
  DEPENDS ${NMODL_PROJECT_SOURCE_DIR}/src/parser/c11.yy
  COMMENT "-- NMODL : GENERATING C (11) PARSER WITH BISON! --")

# command to generate Units parser
add_custom_command(
  OUTPUT ${PROJECT_BINARY_DIR}/src/parser/unit/unit_parser.cpp
         ${PROJECT_BINARY_DIR}/src/parser/unit/unit_parser.hpp
         ${PROJECT_BINARY_DIR}/src/parser/unit/stack.hh
  COMMAND ${BISON_EXECUTABLE} ARGS -d -o ${PROJECT_BINARY_DIR}/src/parser/unit/unit_parser.cpp
          ${NMODL_PROJECT_SOURCE_DIR}/src/parser/unit.yy
  DEPENDS ${NMODL_PROJECT_SOURCE_DIR}/src/parser/unit.yy
  COMMENT "-- NMODL : GENERATING UNIT PARSER WITH BISON! --")

# command to generate nmodl lexer
add_custom_command(
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/nmodl_base_lexer.cpp
         ${CMAKE_CURRENT_BINARY_DIR}/nmodl_base_lexer.hpp
  COMMAND ${FLEX_EXECUTABLE} ARGS ${CMAKE_CURRENT_SOURCE_DIR}/nmodl.ll
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/
  DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/nmodl.ll ${CMAKE_CURRENT_SOURCE_DIR}/nmodl_utils.hpp
  COMMENT "-- NMODL : GENERATING NMODL LEXER WITH FLEX! --")

# command to generate verbatim lexer
add_custom_command(
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/verbatim_lexer.cpp
         ${CMAKE_CURRENT_BINARY_DIR}/verbatim_lexer.hpp
  COMMAND ${FLEX_EXECUTABLE} ARGS ${CMAKE_CURRENT_SOURCE_DIR}/verbatim.l
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/verbatim.l
  COMMENT "-- NMODL : GENERATING VERBATIM LEXER WITH FLEX! --")

# command to generate differential equation lexer
add_custom_command(
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/diffeq_base_lexer.cpp
         ${CMAKE_CURRENT_BINARY_DIR}/diffeq_base_lexer.hpp
  COMMAND ${FLEX_EXECUTABLE} ARGS ${CMAKE_CURRENT_SOURCE_DIR}/diffeq.ll
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/diffeq.ll
  COMMENT "-- NMODL : GENERATING DIFFERENTIAL EQUATION LEXER WITH FLEX! --")

# command to generate C (11) lexer
add_custom_command(
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/c11_base_lexer.cpp
         ${CMAKE_CURRENT_BINARY_DIR}/c11_base_lexer.hpp
  COMMAND ${FLEX_EXECUTABLE} ARGS ${CMAKE_CURRENT_SOURCE_DIR}/c11.ll
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/c11.ll
  COMMENT "-- NMODL : GENERATING C(11) LEXER WITH FLEX! --")

# command to generate Units lexer
add_custom_command(
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/unit_base_lexer.cpp
         ${CMAKE_CURRENT_BINARY_DIR}/unit_base_lexer.hpp
  COMMAND ${FLEX_EXECUTABLE} ARGS ${CMAKE_CURRENT_SOURCE_DIR}/unit.ll
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/unit.ll
  COMMENT "-- NMODL : GENERATING UNIT LEXER WITH FLEX! --")
# =============================================================================
# Libraries & executables
# =============================================================================

add_library(lexer_obj OBJECT ${LEXER_SOURCE_FILES} ${BISON_GENERATED_SOURCE_FILES}
                             ${AST_GENERATED_SOURCES} ${UNIT_SOURCE_FILES})

set_property(TARGET lexer_obj PROPERTY POSITION_INDEPENDENT_CODE ON)

add_library(lexer STATIC $<TARGET_OBJECTS:lexer_obj>)
target_link_libraries(lexer fmt::fmt)

if(NOT NMODL_AS_SUBPROJECT)
  add_executable(nmodl_lexer main_nmodl.cpp)
  add_executable(c_lexer main_c.cpp)
  add_executable(units_lexer main_units.cpp)

  target_link_libraries(nmodl_lexer lexer util)
  target_link_libraries(c_lexer lexer util)
  target_link_libraries(units_lexer lexer util)
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

add_custom_target(parser-gen DEPENDS ${BISON_GENERATED_SOURCE_FILES})
