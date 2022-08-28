# =============================================================================
# Visitor sources
# =============================================================================
set_source_files_properties(${VISITORS_GENERATED_SOURCES} PROPERTIES GENERATED TRUE)

# =============================================================================
# Visitor library and executable
# =============================================================================
include_directories(${PYBIND11_INCLUDE_DIR} ${PYTHON_INCLUDE_DIRS})
add_library(
  visitor STATIC
  after_cvode_to_cnexp_visitor.cpp
  constant_folder_visitor.cpp
  defuse_analyze_visitor.cpp
  global_var_visitor.cpp
  implicit_argument_visitor.cpp
  indexedname_visitor.cpp
  inline_visitor.cpp
  kinetic_block_visitor.cpp
  local_to_assigned_visitor.cpp
  local_var_rename_visitor.cpp
  localize_visitor.cpp
  loop_unroll_visitor.cpp
  neuron_solve_visitor.cpp
  perf_visitor.cpp
  rename_visitor.cpp
  semantic_analysis_visitor.cpp
  solve_block_visitor.cpp
  steadystate_visitor.cpp
  sympy_conductance_visitor.cpp
  sympy_replace_solutions_visitor.cpp
  sympy_solver_visitor.cpp
  units_visitor.cpp
  var_usage_visitor.cpp
  verbatim_var_rename_visitor.cpp
  verbatim_visitor.cpp
  visitor_utils.cpp
  ${VISITORS_GENERATED_SOURCES})
set_property(TARGET visitor PROPERTY POSITION_INDEPENDENT_CODE ON)
add_dependencies(visitor pywrapper)
target_link_libraries(visitor PUBLIC lexer util)

# ~~~
# pybind11::embed adds PYTHON_LIBRARIES to target_link_libraries. To avoid link to
# libpython, we can use `pybind11::module` interface library from pybind11.
# ~~~
if(NOT LINK_AGAINST_PYTHON)
  target_link_libraries(visitor PRIVATE pybind11::module)
else()
  target_link_libraries(visitor PRIVATE pybind11::embed)
endif()

if(NOT NMODL_AS_SUBPROJECT)
  add_executable(nmodl_visitor main.cpp)

  target_link_libraries(
    nmodl_visitor
    CLI11::CLI11
    printer
    visitor
    symtab
    util
    lexer
    ${NMODL_WRAPPER_LIBS})
endif()

# =============================================================================
# Install executable
# =============================================================================
if(NOT NMODL_AS_SUBPROJECT)
  install(
    TARGETS nmodl_visitor
    DESTINATION ${NMODL_INSTALL_DIR_SUFFIX}bin/visitor
    CONFIGURATIONS Debug)
endif()
