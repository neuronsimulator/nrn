# lexer library links with all parser related files
# so need to have parser as a separate library
add_executable(nocmodl_parser main.cpp)
target_link_libraries(nocmodl_parser lexer)