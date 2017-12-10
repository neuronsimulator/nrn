set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/test)

include_directories(
    ${PROJECT_SOURCE_DIR}/src/ext/catch
)

#=============================================================================
# Test executables
#=============================================================================
add_executable (testmodtoken modtoken/modtoken.cpp)
add_executable (testlexer lexer/tokens.cpp)
add_executable (testvisitor visitor/visitor.cpp)
add_executable (testprinter printer/printer.cpp)
add_executable (testsymtab symtab/symbol_table.cpp)

target_link_libraries(testmodtoken lexer)
target_link_libraries(testlexer lexer)
target_link_libraries(testvisitor lexer printer visitor)
target_link_libraries(testprinter printer)
target_link_libraries(testsymtab symtab lexer)

add_test (NAME ModToken COMMAND testmodtoken)
add_test (NAME Lexer COMMAND testlexer)
add_test (NAME Visitor COMMAND testvisitor)
add_test (NAME Printer COMMAND testprinter)
add_test (NAME Symtab COMMAND testsymtab)

#=============================================================================
# Files for clang-format
#=============================================================================
set(FILES_FOR_CLANG_FORMAT
    ${CMAKE_CURRENT_SOURCE_DIR}/modtoken/modtoken.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/lexer/tokens.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/visitor/visitor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/printer/printer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/symtab/symbol_table.cpp
    ${FILES_FOR_CLANG_FORMAT}
    PARENT_SCOPE
)