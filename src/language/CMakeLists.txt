#=============================================================================
# Command to generate AST/Visitor classes from language definition
#=============================================================================
add_custom_command (
    COMMAND ${PYTHON_EXECUTABLE}
    ARGS ${PROJECT_SOURCE_DIR}/src/language/code_generator.py
    OUTPUT ${PROJECT_SOURCE_DIR}/src/ast/ast.hpp
    OUTPUT ${PROJECT_SOURCE_DIR}/src/ast/ast.cpp
    OUTPUT ${PROJECT_SOURCE_DIR}/src/visitors/visitor.hpp
    OUTPUT ${PROJECT_SOURCE_DIR}/src/visitors/ast_visitor.hpp
    OUTPUT ${PROJECT_SOURCE_DIR}/src/visitors/ast_visitor.cpp
    OUTPUT ${PROJECT_SOURCE_DIR}/src/visitors/json_visitor.hpp
    OUTPUT ${PROJECT_SOURCE_DIR}/src/visitors/json_visitor.cpp
    OUTPUT ${PROJECT_SOURCE_DIR}/src/visitors/symtab_visitor.hpp
    OUTPUT ${PROJECT_SOURCE_DIR}/src/visitors/symtab_visitor.cpp
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/src/language
    DEPENDS ${PROJECT_SOURCE_DIR}/src/language/nmodl.yaml
    DEPENDS ${PROJECT_SOURCE_DIR}/src/language/argument.py
    DEPENDS ${PROJECT_SOURCE_DIR}/src/language/nodes.py
    DEPENDS ${PROJECT_SOURCE_DIR}/src/language/parser.py
    DEPENDS ${PROJECT_SOURCE_DIR}/src/language/node_info.py
    DEPENDS ${PROJECT_SOURCE_DIR}/src/language/code_generator.py
    DEPENDS ${PROJECT_SOURCE_DIR}/src/language/printer.py
    DEPENDS ${PROJECT_SOURCE_DIR}/src/language/ast_printer.py
    DEPENDS ${PROJECT_SOURCE_DIR}/src/language/visitors_printer.py
    COMMENT "-- NMODLX : GENERATING AST CLASSES WITH PYTHON GENERATOR! --"
)

#=============================================================================
# Target to propogate dependencies properly to lexer
#=============================================================================
add_custom_target (pyastgen DEPENDS ${PROJECT_SOURCE_DIR}/src/ast/ast.cpp)