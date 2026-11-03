# =============================================================================
# Symbol table library
# =============================================================================
add_library(symtab STATIC symbol.cpp symbol_properties.cpp symbol_table.cpp)
set_property(TARGET symtab PROPERTY POSITION_INDEPENDENT_CODE ON)
target_link_libraries(symtab PRIVATE util)
add_dependencies(symtab lexer)
