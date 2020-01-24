# =============================================================================
# Visitor sources
# =============================================================================
set(SYMTAB_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/symbol.cpp ${CMAKE_CURRENT_SOURCE_DIR}/symbol_properties.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/symbol_table.cpp)

set(SYMTAB_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/symbol.hpp ${CMAKE_CURRENT_SOURCE_DIR}/symbol_properties.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/symbol_table.hpp)

# =============================================================================
# Symbol table library
# =============================================================================
add_library(symtab_obj OBJECT ${SYMTAB_SOURCES} ${SYMTAB_HEADERS})
set_property(TARGET symtab_obj PROPERTY POSITION_INDEPENDENT_CODE ON)

add_dependencies(symtab_obj lexer_obj)

add_library(symtab STATIC $<TARGET_OBJECTS:symtab_obj>)

add_dependencies(symtab lexer)
