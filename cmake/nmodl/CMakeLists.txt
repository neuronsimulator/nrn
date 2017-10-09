cmake_minimum_required(VERSION 3.1.0 FATAL_ERROR)

project(nocmodl CXX)
set(PROJECT_VERSION_MAJOR 0)
set(PROJECT_VERSION_MINOR 1)

set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

message(STATUS "CHECKING FOR FLEX/BISON/PYTHON")

find_package(PythonInterp REQUIRED)
find_package(BISON 3.0 REQUIRED)
find_package(FLEX 2.6 REQUIRED)

list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)
include(GitRevision)

include_directories(${PROJECT_SOURCE_DIR}/src)

set(PROJECT_VERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR})

# generate source file with version number
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/src/version/version.cpp.in
               ${CMAKE_CURRENT_BINARY_DIR}/version.cpp @ONLY)

# command to generate lexer
add_custom_command(
    COMMAND ${FLEX_EXECUTABLE}
    ARGS ${CMAKE_CURRENT_SOURCE_DIR}/src/lexer/nmodl.l
    OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/src/lexer/nmodl_lexer.cpp
    OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/src/lexer/nmodl_lexer.hpp
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/lexer/lexer_utils.hpp
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/src/lexer/
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/lexer/nmodl.l
    COMMENT "-- NOCMODL : GENERATING NMODL LEXER WITH FLEX! --"
)

# command to generate parser
add_custom_command(
    COMMAND ${BISON_EXECUTABLE}
    ARGS -d -o ${CMAKE_CURRENT_SOURCE_DIR}/src/parser/nmodl_parser.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/parser/nmodl.y
    OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/src/parser/nmodl_parser.cpp
    OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/src/parser/nmodl_parser.hpp
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/parser/nmodl.y
    COMMENT "-- NOCMODL : GENERATING NMODL PARSER WITH BISON! --"
)

set(LEXER_SOURCE_FILES
    src/lexer/init.cpp
    src/lexer/list.cpp
    src/lexer/nmodl_lexer.cpp
    src/lexer/nmodl.l
)

set(PARSER_SOURCE_FILES
    src/parser/nmodl_parser.cpp
    src/parser/nmodl.y
    src/ast/astutils.hpp
    src/ast/ast.hpp
    src/ast/ast.cpp
)


add_executable(${PROJECT_NAME}
               ${LEXER_SOURCE_FILES}
               ${PARSER_SOURCE_FILES}
               src/main.cpp
               ${CMAKE_CURRENT_BINARY_DIR}/version.cpp)

add_custom_target(clangformat
                  COMMAND
                  clang-format -i
                  --style=file ${FILES_FOR_CLANG_FORMAT})
