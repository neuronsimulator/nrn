#=============================================================================
# Various project components and their source files
#=============================================================================
set (BISON_GENERATED_SOURCE_FILES
    ${PROJECT_SOURCE_DIR}/src/parser/nmodl/nmodl_parser.cpp
    ${PROJECT_SOURCE_DIR}/src/parser/verbatim_parser.cpp
    ${PROJECT_SOURCE_DIR}/src/parser/diffeq/diffeq_parser.cpp
    ${PROJECT_SOURCE_DIR}/src/parser/c/c11_parser.cpp
)

set(AST_SOURCE_FILES
    ${PROJECT_SOURCE_DIR}/src/ast/ast.hpp
    ${PROJECT_SOURCE_DIR}/src/ast/ast.cpp
)

set(NMODL_DRIVER_FILES
    ${PROJECT_SOURCE_DIR}/src/parser/nmodl_driver.hpp
    ${PROJECT_SOURCE_DIR}/src/parser/nmodl_driver.cpp
)

set(DIFFEQ_DRIVER_FILES
    ${PROJECT_SOURCE_DIR}/src/parser/diffeq_driver.hpp
    ${PROJECT_SOURCE_DIR}/src/parser/diffeq_driver.cpp
    ${PROJECT_SOURCE_DIR}/src/parser/diffeq_context.hpp
    ${PROJECT_SOURCE_DIR}/src/parser/diffeq_context.cpp
    ${PROJECT_SOURCE_DIR}/src/parser/diffeq_helper.hpp
)

set(C_DRIVER_FILES
    ${PROJECT_SOURCE_DIR}/src/parser/c11_driver.hpp
    ${PROJECT_SOURCE_DIR}/src/parser/c11_driver.cpp
)

set_source_files_properties(
    ${AST_SOURCE_FILES}
    PROPERTIES GENERATED TRUE
)

set(LEXER_SOURCE_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/modl.h
    ${CMAKE_CURRENT_SOURCE_DIR}/token_mapping.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/token_mapping.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/nmodl_utils.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/nmodl_utils.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/modtoken.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/modtoken.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/nmodl_base_lexer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/verbatim_lexer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/diffeq_base_lexer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/c11_base_lexer.cpp
    ${NMODL_DRIVER_FILES}
    ${DIFFEQ_DRIVER_FILES}
    ${C_DRIVER_FILES}
)

#=============================================================================
# Directories for parsers (as they need to be in separate directories)
#=============================================================================
file(MAKE_DIRECTORY ${PROJECT_SOURCE_DIR}/src/parser/nmodl)
file(MAKE_DIRECTORY ${PROJECT_SOURCE_DIR}/src/parser/diffeq)
file(MAKE_DIRECTORY ${PROJECT_SOURCE_DIR}/src/parser/c)

#=============================================================================
# Lexer & Parser commands
#=============================================================================
# command to generate nmodl parser
add_custom_command (
    COMMAND ${BISON_EXECUTABLE}
    ARGS -d -o ${PROJECT_SOURCE_DIR}/src/parser/nmodl/nmodl_parser.cpp
    ${PROJECT_SOURCE_DIR}/src/parser/nmodl.yy
    OUTPUT ${PROJECT_SOURCE_DIR}/src/parser/nmodl/nmodl_parser.cpp
    OUTPUT ${PROJECT_SOURCE_DIR}/src/parser/nmodl/nmodl_parser.hpp
    OUTPUT ${PROJECT_SOURCE_DIR}/src/parser/nmodl/location.hh
    OUTPUT ${PROJECT_SOURCE_DIR}/src/parser/nmodl/position.hh
    OUTPUT ${PROJECT_SOURCE_DIR}/src/parser/nmodl/stack.hh
    DEPENDS ${PROJECT_SOURCE_DIR}/src/parser/nmodl.yy
    DEPENDS pyastgen
    COMMENT "-- NMODL : GENERATING NMODL_CORE PARSER WITH BISON! --"
)

# command to generate verbatim parser
add_custom_command (
    COMMAND ${BISON_EXECUTABLE}
    ARGS -d -o ${PROJECT_SOURCE_DIR}/src/parser/verbatim_parser.cpp
    ${PROJECT_SOURCE_DIR}/src/parser/verbatim.y
    OUTPUT ${PROJECT_SOURCE_DIR}/src/parser/verbatim_parser.cpp
    OUTPUT ${PROJECT_SOURCE_DIR}/src/parser/verbatim_parser.hpp
    DEPENDS ${PROJECT_SOURCE_DIR}/src/parser/verbatim.y
    COMMENT "-- NMODL : GENERATING VERBATIM PARSER WITH BISON! --"
)

# command to generate differential equation parser
add_custom_command (
    COMMAND ${BISON_EXECUTABLE}
    ARGS -d -o ${PROJECT_SOURCE_DIR}/src/parser/diffeq/diffeq_parser.cpp
    ${PROJECT_SOURCE_DIR}/src/parser/diffeq.yy
    OUTPUT ${PROJECT_SOURCE_DIR}/src/parser/diffeq/diffeq_parser.cpp
    OUTPUT ${PROJECT_SOURCE_DIR}/src/parser/diffeq/diffeq_parser.hpp
    OUTPUT ${PROJECT_SOURCE_DIR}/src/parser/diffeq/location.hh
    OUTPUT ${PROJECT_SOURCE_DIR}/src/parser/diffeq/position.hh
    OUTPUT ${PROJECT_SOURCE_DIR}/src/parser/diffeq/stack.hh
    DEPENDS ${PROJECT_SOURCE_DIR}/src/parser/diffeq.yy
    DEPENDS ${PROJECT_SOURCE_DIR}/src/parser/diffeq_context.hpp
    DEPENDS ${PROJECT_SOURCE_DIR}/src/parser/diffeq_context.cpp
    DEPENDS ${PROJECT_SOURCE_DIR}/src/parser/diffeq_helper.hpp
    COMMENT "-- NMODL : GENERATING DIFFERENTIAL EQUATION PARSER WITH BISON! --"
)

# command to generate C (11) parser
add_custom_command (
    COMMAND ${BISON_EXECUTABLE}
    ARGS -d -o ${PROJECT_SOURCE_DIR}/src/parser/c/c11_parser.cpp
    ${PROJECT_SOURCE_DIR}/src/parser/c11.yy
    OUTPUT ${PROJECT_SOURCE_DIR}/src/parser/c/c11_parser.cpp
    OUTPUT ${PROJECT_SOURCE_DIR}/src/parser/c/c11_parser.hpp
    OUTPUT ${PROJECT_SOURCE_DIR}/src/parser/c/location.hh
    OUTPUT ${PROJECT_SOURCE_DIR}/src/parser/c/position.hh
    OUTPUT ${PROJECT_SOURCE_DIR}/src/parser/c/stack.hh
    DEPENDS ${PROJECT_SOURCE_DIR}/src/parser/c11.yy
    COMMENT "-- NMODL : GENERATING C (11) PARSER WITH BISON! --"
)

# command to generate nmodl lexer
add_custom_command(
    COMMAND ${FLEX_EXECUTABLE}
    ARGS ${CMAKE_CURRENT_SOURCE_DIR}/nmodl.ll
    OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/nmodl_base_lexer.cpp
    OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/nmodl_base_lexer.hpp
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/nmodl.ll
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/nmodl_utils.hpp
    COMMENT "-- NMODL : GENERATING NMODL LEXER WITH FLEX! --"
)

# command to generate verbatim lexer
add_custom_command(
    COMMAND ${FLEX_EXECUTABLE}
    ARGS ${CMAKE_CURRENT_SOURCE_DIR}/verbatim.l
    OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/verbatim_lexer.cpp
    OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/verbatim_lexer.hpp
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/verbatim.l
    COMMENT "-- NMODL : GENERATING VERBATIM LEXER WITH FLEX! --"
)

# command to generate differential equation lexer
add_custom_command(
    COMMAND ${FLEX_EXECUTABLE}
    ARGS ${CMAKE_CURRENT_SOURCE_DIR}/diffeq.ll
    OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/diffeq_base_lexer.cpp
    OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/diffeq_base_lexer.hpp
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/diffeq.ll
    COMMENT "-- NMODL : GENERATING DIFFERENTIAL EQUATION LEXER WITH FLEX! --"
)

# command to generate C (11) lexer
add_custom_command(
    COMMAND ${FLEX_EXECUTABLE}
    ARGS ${CMAKE_CURRENT_SOURCE_DIR}/c11.ll
    OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/c11_base_lexer.cpp
    OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/c11_base_lexer.hpp
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/c11.ll
    COMMENT "-- NMODL : GENERATING C(11) LEXER WITH FLEX! --"
)

#=============================================================================
# Libraries & executables
#=============================================================================
add_library(lexer
            STATIC
            ${LEXER_SOURCE_FILES}
            ${BISON_GENERATED_SOURCE_FILES}
            ${AST_SOURCE_FILES})

add_executable(nmodl_lexer main_nmodl.cpp)
add_executable(c_lexer main_c.cpp)

target_link_libraries(nmodl_lexer lexer)
target_link_libraries(c_lexer lexer)

#=============================================================================
# Files for clang-format
#=============================================================================
set(FILES_FOR_CLANG_FORMAT
    ${LEXER_SOURCE_FILES}
    ${CMAKE_CURRENT_SOURCE_DIR}/main_nmodl.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/main_c.cpp
    ${DIFFEQ_DRIVER_FILES}
    ${C_DRIVER_FILES}
    ${FILES_FOR_CLANG_FORMAT}
    PARENT_SCOPE
)
