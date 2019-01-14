#=============================================================================
# Command to generate AST/Visitor classes from language definition
#=============================================================================
set ( AUTOGEN_FILES ${PROJECT_SOURCE_DIR}/src/ast/ast.hpp
        ${PROJECT_SOURCE_DIR}/src/ast/ast_decl.hpp
        ${PROJECT_SOURCE_DIR}/src/ast/ast.cpp
        ${PROJECT_SOURCE_DIR}/src/visitors/visitor.hpp
        ${PROJECT_SOURCE_DIR}/src/visitors/ast_visitor.hpp
        ${PROJECT_SOURCE_DIR}/src/visitors/ast_visitor.cpp
        ${PROJECT_SOURCE_DIR}/src/visitors/json_visitor.hpp
        ${PROJECT_SOURCE_DIR}/src/visitors/json_visitor.cpp
        ${PROJECT_SOURCE_DIR}/src/visitors/symtab_visitor.hpp
        ${PROJECT_SOURCE_DIR}/src/visitors/symtab_visitor.cpp
        ${PROJECT_SOURCE_DIR}/src/visitors/nmodl_visitor.hpp
        ${PROJECT_SOURCE_DIR}/src/visitors/nmodl_visitor.cpp
        )

file ( GLOB TEMPLATE_FILES
        "${PROJECT_SOURCE_DIR}/src/language/templates/*.cpp"
        "${PROJECT_SOURCE_DIR}/src/language/templates/*.hpp"
        )

file ( GLOB PYCODE "${PROJECT_SOURCE_DIR}/src/language/*.py" )


if(CLANG_FORMAT_FOUND)
    add_custom_command (
            OUTPUT ${AUTOGEN_FILES}
            COMMAND ${PYTHON_EXECUTABLE} ARGS ${PROJECT_SOURCE_DIR}/src/language/code_generator.py
            COMMAND ${CLANG_FORMAT_EXECUTABLE} ARGS -i --style=file ${AUTOGEN_FILES}
            WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/src/language
            DEPENDS ${PROJECT_SOURCE_DIR}/src/language/nmodl.yaml
            DEPENDS ${PYCODE}
            DEPENDS ${TEMPLATE_FILES}
            COMMENT "-- NMODLX : GENERATING AST CLASSES WITH PYTHON GENERATOR! --"
            )
else()
        add_custom_command (
                OUTPUT ${AUTOGEN_FILES}
                COMMAND ${PYTHON_EXECUTABLE} ARGS ${PROJECT_SOURCE_DIR}/src/language/code_generator.py
                WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/src/language
                DEPENDS ${PROJECT_SOURCE_DIR}/src/language/nmodl.yaml
                DEPENDS ${PYCODE}
                DEPENDS ${TEMPLATE_FILES}
                COMMENT "-- NMODLX : GENERATING AST CLASSES WITH PYTHON GENERATOR! --"
        )
endif()
#=============================================================================
# Target to propogate dependencies properly to lexer
#=============================================================================
add_custom_target (pyastgen DEPENDS ${PROJECT_SOURCE_DIR}/src/ast/ast.cpp)