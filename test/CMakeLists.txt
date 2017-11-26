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

target_link_libraries(testmodtoken lexer)
target_link_libraries(testlexer lexer)
target_link_libraries(testvisitor lexer printer visitor)
target_link_libraries(testprinter printer)

add_test (NAME ModToken COMMAND testmodtoken)
add_test (NAME Lexer COMMAND testlexer)
add_test (NAME Visitor COMMAND testvisitor)
add_test (NAME Printer COMMAND testprinter)