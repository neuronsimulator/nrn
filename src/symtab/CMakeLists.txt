#=============================================================================
# Visitor sources
#=============================================================================
set(SYMTAB_SOURCE_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/symbol.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/symbol.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/symbol_properties.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/symbol_properties.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/symbol_table.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/symbol_table.cpp
)

#=============================================================================
# Symbol table library
#=============================================================================
add_library(symtab
            STATIC
            ${SYMTAB_SOURCE_FILES}
)

add_dependencies(symtab lexer)

#=============================================================================
# Files for clang-format
#=============================================================================
set(FILES_FOR_CLANG_FORMAT
    ${SYMTAB_SOURCE_FILES}
    ${FILES_FOR_CLANG_FORMAT}
    PARENT_SCOPE
)