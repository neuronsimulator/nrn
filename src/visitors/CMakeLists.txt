# =============================================================================
# Visitor sources
# =============================================================================
set(VISITOR_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/neuron_solve_visitor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/neuron_solve_visitor.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/constant_folder_visitor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/constant_folder_visitor.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/after_cvode_to_cnexp_visitor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/after_cvode_to_cnexp_visitor.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/defuse_analyze_visitor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/defuse_analyze_visitor.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/global_var_visitor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/global_var_visitor.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/inline_visitor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/inline_visitor.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/kinetic_block_visitor.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/kinetic_block_visitor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/local_to_assigned_visitor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/local_to_assigned_visitor.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/local_var_rename_visitor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/local_var_rename_visitor.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/localize_visitor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/localize_visitor.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/loop_unroll_visitor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/loop_unroll_visitor.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/nmodl_visitor_helper.ipp
    ${CMAKE_CURRENT_SOURCE_DIR}/solve_block_visitor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/solve_block_visitor.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/perf_visitor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/perf_visitor.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/rename_visitor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/rename_visitor.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/steadystate_visitor.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/steadystate_visitor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/sympy_conductance_visitor.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/sympy_conductance_visitor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/sympy_solver_visitor.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/sympy_solver_visitor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/sympy_replace_solutions_visitor.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/sympy_replace_solutions_visitor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/symtab_visitor_helper.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/units_visitor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/units_visitor.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/var_usage_visitor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/var_usage_visitor.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/verbatim_var_rename_visitor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/verbatim_var_rename_visitor.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/verbatim_visitor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/verbatim_visitor.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/visitor_utils.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/visitor_utils.hpp)

set_source_files_properties(${VISITORS_GENERATED_SOURCES} PROPERTIES GENERATED TRUE)

# =============================================================================
# Visitor library and executable
# =============================================================================
include_directories(${PYBIND11_INCLUDE_DIR} ${PYTHON_INCLUDE_DIRS})
add_library(visitor_obj OBJECT ${VISITOR_SOURCES} ${VISITORS_GENERATED_SOURCES})
set_property(TARGET visitor_obj PROPERTY POSITION_INDEPENDENT_CODE ON)

add_dependencies(visitor_obj lexer_obj)
add_library(visitor STATIC $<TARGET_OBJECTS:visitor_obj>)

add_dependencies(visitor lexer util pywrapper)

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
