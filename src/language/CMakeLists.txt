# generate AST/Visitor classes from language definition
add_custom_command (
    COMMAND ${PYTHON_EXECUTABLE}
    ARGS ${PROJECT_SOURCE_DIR}/src/language/code_generator.py
    OUTPUT ${PROJECT_SOURCE_DIR}/src/ast/ast.hpp
    OUTPUT ${PROJECT_SOURCE_DIR}/src/ast/ast.cpp
    OUTPUT ${PROJECT_SOURCE_DIR}/src/visitors/visitor.hpp
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/src/language
    DEPENDS ${PROJECT_SOURCE_DIR}/src/language/nmodl.yaml
    DEPENDS ${PROJECT_SOURCE_DIR}/src/language/argument.py
    DEPENDS ${PROJECT_SOURCE_DIR}/src/language/ast.py
    DEPENDS ${PROJECT_SOURCE_DIR}/src/language/parser.py
    DEPENDS ${PROJECT_SOURCE_DIR}/src/language/node_types.py
    DEPENDS ${PROJECT_SOURCE_DIR}/src/language/code_generator.py
    DEPENDS ${PROJECT_SOURCE_DIR}/src/language/base_printer.py
    DEPENDS ${PROJECT_SOURCE_DIR}/src/language/ast_printer.py
    COMMENT "-- NMODLX : GENERATING AST CLASSES WITH PYTHON GENERATOR! --"
)

add_custom_target (pyastgen DEPENDS ${PROJECT_SOURCE_DIR}/src/ast/ast.cpp)