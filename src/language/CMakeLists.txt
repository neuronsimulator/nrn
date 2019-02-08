# =============================================================================
# Command to generate AST/Visitor classes from language definition
# =============================================================================
set_source_files_properties(${AUTO_GENERATED_FILES} PROPERTIES GENERATED TRUE)

file(GLOB TEMPLATE_FILES "${PROJECT_SOURCE_DIR}/src/language/templates/*.cpp"
          "${PROJECT_SOURCE_DIR}/src/language/templates/*.hpp")

file(GLOB PYCODE "${PROJECT_SOURCE_DIR}/src/language/*.py")

if(CLANG_FORMAT_FOUND AND CLANG_FORMAT_VERSION_MAJOR GREATER_EQUAL 4)
    set(CODE_GENERATOR_OPTS --clang-format=${CLANG_FORMAT_EXECUTABLE})
endif()

add_custom_command(
    OUTPUT ${AUTO_GENERATED_FILES}
    COMMAND ${PYTHON_EXECUTABLE}
            ARGS ${PROJECT_SOURCE_DIR}/src/language/code_generator.py
                 ${CODE_GENERATOR_OPTS}
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/src/language
    DEPENDS ${PROJECT_SOURCE_DIR}/src/language/nmodl.yaml
    DEPENDS ${PYCODE}
    DEPENDS ${TEMPLATE_FILES}
    COMMENT "-- NMODL : GENERATING AST CLASSES WITH PYTHON GENERATOR! --")
unset(CODE_GENERATOR_OPTS)

# =============================================================================
# Target to propagate dependencies properly to lexer
# =============================================================================
add_custom_target(pyastgen DEPENDS ${PROJECT_SOURCE_DIR}/src/ast/ast.cpp)
