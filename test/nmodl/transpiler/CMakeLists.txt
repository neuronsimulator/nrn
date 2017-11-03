include_directories(
    ${PROJECT_SOURCE_DIR}/src/ext/catch
)

add_executable (testmodtoken modtoken/modtoken.cpp)
add_executable (testlexer lexer/tokens.cpp)

target_link_libraries(testmodtoken lexer)
target_link_libraries(testlexer lexer)