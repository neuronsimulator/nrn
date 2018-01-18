set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/test)

include_directories(
    ${PROJECT_SOURCE_DIR}/src/ext/catch
    ${PROJECT_SOURCE_DIR}/test
)

#=============================================================================
# Common input data library
#=============================================================================
add_library(test_util
            STATIC
            ${CMAKE_CURRENT_SOURCE_DIR}/utils/nmodl_constructs.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/utils/test_utils.cpp
)

#=============================================================================
# Test executables
#=============================================================================
add_executable (testmodtoken modtoken/modtoken.cpp)
add_executable (testlexer lexer/tokens.cpp)
add_executable (testparser parser/parser.cpp)
add_executable (testvisitor visitor/visitor.cpp)
add_executable (testprinter printer/printer.cpp)
add_executable (testsymtab symtab/symbol_table.cpp)

target_link_libraries(testmodtoken lexer)
target_link_libraries(testlexer lexer)
target_link_libraries(testparser lexer test_util)
target_link_libraries(testvisitor symtab lexer visitor util test_util printer)
target_link_libraries(testprinter printer)
target_link_libraries(testsymtab symtab lexer util)

add_test (NAME ModToken COMMAND testmodtoken)
add_test (NAME Lexer COMMAND testlexer)
add_test (NAME Parser COMMAND testparser)
add_test (NAME Visitor COMMAND testvisitor)
add_test (NAME Printer COMMAND testprinter)
add_test (NAME Symtab COMMAND testsymtab)

#=============================================================================
# Files for clang-format
#=============================================================================
set(FILES_FOR_CLANG_FORMAT
    ${CMAKE_CURRENT_SOURCE_DIR}/utils/nmodl_constructs.h
    ${CMAKE_CURRENT_SOURCE_DIR}/utils/nmodl_constructs.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/utils/test_utils.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/modtoken/modtoken.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/lexer/tokens.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/parser/parser.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/visitor/visitor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/printer/printer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/symtab/symbol_table.cpp
    ${FILES_FOR_CLANG_FORMAT}
    PARENT_SCOPE
)
