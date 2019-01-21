#=============================================================================
# pybind targets
#=============================================================================
set_source_files_properties( ${AUTO_GENERATED_FILES} PROPERTIES GENERATED TRUE)


set(PYNMODL_SOURCES
        ${PROJECT_SOURCE_DIR}/src/ast/ast.cpp
        ${PROJECT_SOURCE_DIR}/src/lexer/modtoken.cpp
        ${PROJECT_SOURCE_DIR}/src/symtab/symbol.cpp
        ${PROJECT_SOURCE_DIR}/src/symtab/symbol_properties.cpp
        ${PROJECT_SOURCE_DIR}/src/symtab/symbol_table.cpp
        ${PROJECT_SOURCE_DIR}/src/visitors/ast_visitor.cpp
        ${PROJECT_SOURCE_DIR}/src/utils/common_utils.cpp
        ${PROJECT_SOURCE_DIR}/src/utils/table_data.cpp
        ${PROJECT_SOURCE_DIR}/src/pybind/pyast.cpp
        ${PROJECT_SOURCE_DIR}/src/pybind/pyvisitor.cpp
        ${PROJECT_SOURCE_DIR}/src/pybind/pynmodl.cpp)

set(PYNMODL_HEADERS
        ${PROJECT_SOURCE_DIR}/src/ast/ast.hpp
        ${PROJECT_SOURCE_DIR}/src/ast/ast_decl.hpp
        ${PROJECT_SOURCE_DIR}/src/ast/ast_common.hpp
        ${PROJECT_SOURCE_DIR}/src/lexer/modtoken.hpp
        ${PROJECT_SOURCE_DIR}/src/symtab/symbol.hpp
        ${PROJECT_SOURCE_DIR}/src/symtab/symbol_properties.hpp
        ${PROJECT_SOURCE_DIR}/src/symtab/symbol_table.hpp
        ${PROJECT_SOURCE_DIR}/src/visitors/ast_visitor.hpp
        ${PROJECT_SOURCE_DIR}/src/visitors/visitor.hpp
        ${PROJECT_SOURCE_DIR}/src/utils/common_utils.hpp
        ${PROJECT_SOURCE_DIR}/src/utils/string_utils.hpp
        ${PROJECT_SOURCE_DIR}/src/utils/table_data.hpp
        ${PROJECT_SOURCE_DIR}/src/pybind/pyast.hpp
        ${PROJECT_SOURCE_DIR}/src/pybind/pyvisitor.hpp)


pybind11_add_module(_nmodl ${PYNMODL_HEADERS} ${PYNMODL_SOURCES})
target_link_libraries(_nmodl PRIVATE lexer
                             PRIVATE visitor
                             PRIVATE printer)

add_dependencies(_nmodl pyastgen)
add_dependencies(_nmodl nmodl_parser)
