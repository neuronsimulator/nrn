# =============================================================================
# Command to generate AST/Visitor classes from language definition
# =============================================================================
set_source_files_properties(${NMODL_GENERATED_SOURCES} PROPERTIES GENERATED TRUE)

# clang-format is handled by the HPC coding conventions scripts, which also handle generating the
# .clang-format configuration file. It's important that we only try to format code if formatting was
# enabled and NMODL's .clang-format exists, otherwise clang-format will search too far up the
# directory tree and find the wrong configuration file. This can break compilation.
if(NMODL_CLANG_FORMAT OR NMODL_FORMATTING)
  set(CODE_GENERATOR_OPTS -v --clang-format=${ClangFormat_EXECUTABLE})
  foreach(clang_format_opt ${NMODL_ClangFormat_OPTIONS} --style=file)
    list(APPEND CODE_GENERATOR_OPTS --clang-format-opts=${clang_format_opt})
  endforeach()
endif()

add_custom_command(
  OUTPUT ${NMODL_GENERATED_SOURCES}
  COMMAND ${PYTHON_EXECUTABLE} ARGS ${CMAKE_CURRENT_SOURCE_DIR}/code_generator.py
          ${CODE_GENERATOR_OPTS} --base-dir ${PROJECT_BINARY_DIR}/src
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  DEPENDS ${CODE_GENERATOR_PY_FILES}
  DEPENDS ${CODE_GENERATOR_YAML_FILES}
  DEPENDS ${CODE_GENERATOR_JINJA_FILES}
  DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/templates/code_generator.cmake
  COMMENT "-- NMODL : GENERATING AST CLASSES WITH PYTHON GENERATOR! --")
unset(CODE_GENERATOR_OPTS)

# =============================================================================
# Target to propagate dependencies properly to lexer
# =============================================================================
add_custom_target(pyastgen DEPENDS ${PROJECT_BINARY_DIR}/src/ast/ast.cpp)
