# =============================================================================
# Command to generate AST/Visitor classes from language definition
# =============================================================================
set_source_files_properties(${AUTO_GENERATED_FILES} PROPERTIES GENERATED TRUE)

file(GLOB TEMPLATE_FILES "${PROJECT_SOURCE_DIR}/src/language/templates/*.cpp"
          "${PROJECT_SOURCE_DIR}/src/language/templates/*.hpp")

file(GLOB PYCODE "${PROJECT_SOURCE_DIR}/src/language/*.py")

if(ClangFormat_FOUND AND (NOT ClangFormat_VERSION_MAJOR LESS 4))
  set(CODE_GENERATOR_OPTS --clang-format=${ClangFormat_EXECUTABLE})
  if(nmodl_ClangFormat_OPTIONS)
    set(CODE_GENERATOR_OPTS ${CODE_GENERATOR_OPTS} --clang-format-opts ${nmodl_ClangFormat_OPTIONS})
  endif(nmodl_ClangFormat_OPTIONS)
endif()

add_custom_command(OUTPUT ${AUTO_GENERATED_FILES}
                   COMMAND ${PYTHON_EXECUTABLE}
                           ARGS ${PROJECT_SOURCE_DIR}/src/language/code_generator.py
                                ${CODE_GENERATOR_OPTS}
                           --base-dir ${PROJECT_BINARY_DIR}/src
                   WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/src/language
                   DEPENDS ${PROJECT_SOURCE_DIR}/src/language/nmodl.yaml
                   DEPENDS ${PROJECT_SOURCE_DIR}/src/language/codegen.yaml
                   DEPENDS ${PYCODE}
                   DEPENDS ${TEMPLATE_FILES}
                   COMMENT "-- NMODL : GENERATING AST CLASSES WITH PYTHON GENERATOR! --")
unset(CODE_GENERATOR_OPTS)

# =============================================================================
# Target to propagate dependencies properly to lexer
# =============================================================================
add_custom_target(pyastgen DEPENDS ${PROJECT_BINARY_DIR}/src/ast/ast.cpp)
