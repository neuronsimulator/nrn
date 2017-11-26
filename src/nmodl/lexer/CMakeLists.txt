#=============================================================================
# Various project components and their source files
#=============================================================================
set (BISON_GENERATED_SOURCE_FILES
    ${PROJECT_SOURCE_DIR}/src/parser/nmodl_parser.cpp
    ${PROJECT_SOURCE_DIR}/src/parser/verbatim_parser.cpp
)

set(AST_SOURCE_FILES
    ${PROJECT_SOURCE_DIR}/src/ast/ast.cpp
)

set(NMODL_DRIVER_FILES
    ${PROJECT_SOURCE_DIR}/src/parser/nmodl_driver.hpp
    ${PROJECT_SOURCE_DIR}/src/parser/nmodl_driver.cpp
)

set_source_files_properties(
    ${PROJECT_SOURCE_DIR}/src/ast/ast.cpp
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
    ${NMODL_DRIVER_FILES}
)

#=============================================================================
# Lexer & Parser commands
#=============================================================================
# command to generate nmodl parser
add_custom_command (
    COMMAND ${BISON_EXECUTABLE}
    ARGS -d -o ${PROJECT_SOURCE_DIR}/src/parser/nmodl_parser.cpp
    ${PROJECT_SOURCE_DIR}/src/parser/nmodl.yy
    OUTPUT ${PROJECT_SOURCE_DIR}/src/parser/nmodl_parser.cpp
    OUTPUT ${PROJECT_SOURCE_DIR}/src/parser/nmodl_parser.hpp
    OUTPUT ${PROJECT_SOURCE_DIR}/src/parser/location.hh
    OUTPUT ${PROJECT_SOURCE_DIR}/src/parser/position.hh
    OUTPUT ${PROJECT_SOURCE_DIR}/src/parser/stack.hh
    DEPENDS ${PROJECT_SOURCE_DIR}/src/parser/nmodl.yy
    DEPENDS pyastgen
    COMMENT "-- NOCMODL : GENERATING NMODL_CORE PARSER WITH BISON! --"
)

# command to generate verbatim parser
add_custom_command (
    COMMAND ${BISON_EXECUTABLE}
    ARGS -d -o ${PROJECT_SOURCE_DIR}/src/parser/verbatim_parser.cpp
    ${PROJECT_SOURCE_DIR}/src/parser/verbatim.y
    OUTPUT ${PROJECT_SOURCE_DIR}/src/parser/verbatim_parser.cpp
    OUTPUT ${PROJECT_SOURCE_DIR}/src/parser/verbatim_parser.hpp
    COMMENT "-- NOCMODL : GENERATING VERBATIM PARSER WITH BISON! --"
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
    COMMENT "-- NOCMODL : GENERATING NMODL LEXER WITH FLEX! --"
)

# command to generate verbatim lexer
add_custom_command(
    COMMAND ${FLEX_EXECUTABLE}
    ARGS ${CMAKE_CURRENT_SOURCE_DIR}/verbatim.l
    OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/verbatim_lexer.cpp
    OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/verbatim_lexer.hpp
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/verbatim.l
    COMMENT "-- NOCMODL : GENERATING VERBATIM LEXER WITH FLEX! --"
)

#=============================================================================
# Libraries & executables
#=============================================================================
add_library(lexer
            STATIC
            ${LEXER_SOURCE_FILES}
            ${BISON_GENERATED_SOURCE_FILES}
            ${AST_SOURCE_FILES})

add_executable(nocmodl_lexer main.cpp)
target_link_libraries(nocmodl_lexer lexer)

#=============================================================================
# Files for clang-format
#=============================================================================
set(FILES_FOR_CLANG_FORMAT
    ${LEXER_SOURCE_FILES}
    ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
    ${FILES_FOR_CLANG_FORMAT}
    PARENT_SCOPE
)