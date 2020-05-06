# =============================================================================
# pybind targets
# =============================================================================
set_source_files_properties(${AUTO_GENERATED_FILES} PROPERTIES GENERATED TRUE)

# build nmodl python module under lib/python/nmodl
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib/python/nmodl)

foreach(
  file
  ast.py
  dsl.py
  ode.py
  symtab.py
  visitor.py
  __init__.py)
  list(APPEND NMODL_PYTHON_FILES_IN ${NMODL_PROJECT_SOURCE_DIR}/nmodl/${file})
  list(APPEND NMODL_PYTHON_FILES_OUT ${PROJECT_BINARY_DIR}/nmodl/${file})
endforeach()

set(PYNMODL_SOURCES
    ${PROJECT_BINARY_DIR}/src/pybind/pyast.cpp ${PROJECT_BINARY_DIR}/src/pybind/pyvisitor.cpp
    ${PROJECT_BINARY_DIR}/src/pybind/pysymtab.cpp
    ${NMODL_PROJECT_SOURCE_DIR}/src/pybind/pynmodl.cpp)

set(PYNMODL_HEADERS
    ${PROJECT_BINARY_DIR}/src/ast/ast_decl.hpp
    ${NMODL_PROJECT_SOURCE_DIR}/src/ast/ast_common.hpp
    ${PROJECT_BINARY_DIR}/src/visitors/visitor.hpp
    ${NMODL_PROJECT_SOURCE_DIR}/src/pybind/pybind_utils.hpp
    ${PROJECT_BINARY_DIR}/src/pybind/pyast.hpp
    ${PROJECT_BINARY_DIR}/src/pybind/pyvisitor.hpp)

# Note that LTO causes link time errors with GCC 8. To avoid this, we disable LTO for pybind using
# NO_EXTRAS. See #266.
pybind11_add_module(
  _nmodl
  NO_EXTRAS
  ${PYNMODL_HEADERS}
  ${PYNMODL_SOURCES}
  $<TARGET_OBJECTS:symtab_obj>
  $<TARGET_OBJECTS:visitor_obj>
  $<TARGET_OBJECTS:lexer_obj>
  $<TARGET_OBJECTS:util_obj>
  $<TARGET_OBJECTS:printer_obj>)

add_dependencies(_nmodl pyastgen)
add_dependencies(_nmodl lexer_obj)
add_dependencies(_nmodl util_obj)

target_link_libraries(_nmodl PRIVATE fmt::fmt)

add_custom_target(copy_python_files ALL DEPENDS ${NMODL_PYTHON_FILES_OUT})
add_custom_command(
  OUTPUT ${NMODL_PYTHON_FILES_OUT}
  COMMAND ${CMAKE_COMMAND} -E copy_directory ${NMODL_PROJECT_SOURCE_DIR}/nmodl
          ${PROJECT_BINARY_DIR}/nmodl
  COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:_nmodl> ${PROJECT_BINARY_DIR}/nmodl
  DEPENDS ${NMODL_PYTHON_FILES_IN} $<TARGET_FILE:_nmodl>
  COMMENT "-- COPYING NMODL PYTHON FILES --")

# =============================================================================
# Copy python binding components and examples into build directory
# =============================================================================
file(GLOB NMODL_PYTHON_HELPER_FILES "${NMODL_PROJECT_SOURCE_DIR}/nmodl/*.py")
file(COPY ${NMODL_PYTHON_HELPER_FILES} DESTINATION ${CMAKE_BINARY_DIR}/lib/python/nmodl/)
file(COPY ${NMODL_PROJECT_SOURCE_DIR}/nmodl/ext DESTINATION ${CMAKE_BINARY_DIR}/lib/python/nmodl/)

# =============================================================================
# Install python binding components
# =============================================================================
install(DIRECTORY ${CMAKE_BINARY_DIR}/lib/ DESTINATION lib)
