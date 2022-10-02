#
# THIS FILE IS GENERATED AND SHALL NOT BE EDITED.
#

# cmake-format: off
set(CODE_GENERATOR_JINJA_FILES
    ${PROJECT_SOURCE_DIR}/src/language/templates/ast/all.hpp
    ${PROJECT_SOURCE_DIR}/src/language/templates/ast/ast.cpp
    ${PROJECT_SOURCE_DIR}/src/language/templates/ast/ast.hpp
    ${PROJECT_SOURCE_DIR}/src/language/templates/ast/ast_decl.hpp
    ${PROJECT_SOURCE_DIR}/src/language/templates/ast/node.hpp
    ${PROJECT_SOURCE_DIR}/src/language/templates/ast/node_class.template
    ${PROJECT_SOURCE_DIR}/src/language/templates/pybind/pyast.cpp
    ${PROJECT_SOURCE_DIR}/src/language/templates/pybind/pyast.hpp
    ${PROJECT_SOURCE_DIR}/src/language/templates/pybind/pynode.cpp
    ${PROJECT_SOURCE_DIR}/src/language/templates/pybind/pysymtab.cpp
    ${PROJECT_SOURCE_DIR}/src/language/templates/pybind/pyvisitor.cpp
    ${PROJECT_SOURCE_DIR}/src/language/templates/pybind/pyvisitor.hpp
    ${PROJECT_SOURCE_DIR}/src/language/templates/visitors/ast_visitor.cpp
    ${PROJECT_SOURCE_DIR}/src/language/templates/visitors/ast_visitor.hpp
    ${PROJECT_SOURCE_DIR}/src/language/templates/visitors/checkparent_visitor.cpp
    ${PROJECT_SOURCE_DIR}/src/language/templates/visitors/checkparent_visitor.hpp
    ${PROJECT_SOURCE_DIR}/src/language/templates/visitors/json_visitor.cpp
    ${PROJECT_SOURCE_DIR}/src/language/templates/visitors/json_visitor.hpp
    ${PROJECT_SOURCE_DIR}/src/language/templates/visitors/lookup_visitor.cpp
    ${PROJECT_SOURCE_DIR}/src/language/templates/visitors/lookup_visitor.hpp
    ${PROJECT_SOURCE_DIR}/src/language/templates/visitors/nmodl_visitor.cpp
    ${PROJECT_SOURCE_DIR}/src/language/templates/visitors/nmodl_visitor.hpp
    ${PROJECT_SOURCE_DIR}/src/language/templates/visitors/symtab_visitor.cpp
    ${PROJECT_SOURCE_DIR}/src/language/templates/visitors/symtab_visitor.hpp
    ${PROJECT_SOURCE_DIR}/src/language/templates/visitors/visitor.hpp
)

set(CODE_GENERATOR_PY_FILES
    ${PROJECT_SOURCE_DIR}/src/language/argument.py
    ${PROJECT_SOURCE_DIR}/src/language/code_generator.py
    ${PROJECT_SOURCE_DIR}/src/language/node_info.py
    ${PROJECT_SOURCE_DIR}/src/language/nodes.py
    ${PROJECT_SOURCE_DIR}/src/language/parser.py
    ${PROJECT_SOURCE_DIR}/src/language/utils.py
)

set(CODE_GENERATOR_YAML_FILES
    ${PROJECT_SOURCE_DIR}/src/language/codegen.yaml
    ${PROJECT_SOURCE_DIR}/src/language/nmodl.yaml
)

set(AST_GENERATED_SOURCES
    ${PROJECT_BINARY_DIR}/src/ast/after_block.hpp
    ${PROJECT_BINARY_DIR}/src/ast/all.hpp
    ${PROJECT_BINARY_DIR}/src/ast/argument.hpp
    ${PROJECT_BINARY_DIR}/src/ast/assigned_block.hpp
    ${PROJECT_BINARY_DIR}/src/ast/assigned_definition.hpp
    ${PROJECT_BINARY_DIR}/src/ast/ast.cpp
    ${PROJECT_BINARY_DIR}/src/ast/ast.hpp
    ${PROJECT_BINARY_DIR}/src/ast/ast_decl.hpp
    ${PROJECT_BINARY_DIR}/src/ast/ba_block.hpp
    ${PROJECT_BINARY_DIR}/src/ast/ba_block_type.hpp
    ${PROJECT_BINARY_DIR}/src/ast/bbcore_pointer.hpp
    ${PROJECT_BINARY_DIR}/src/ast/bbcore_pointer_var.hpp
    ${PROJECT_BINARY_DIR}/src/ast/before_block.hpp
    ${PROJECT_BINARY_DIR}/src/ast/binary_expression.hpp
    ${PROJECT_BINARY_DIR}/src/ast/binary_operator.hpp
    ${PROJECT_BINARY_DIR}/src/ast/block.hpp
    ${PROJECT_BINARY_DIR}/src/ast/block_comment.hpp
    ${PROJECT_BINARY_DIR}/src/ast/boolean.hpp
    ${PROJECT_BINARY_DIR}/src/ast/breakpoint_block.hpp
    ${PROJECT_BINARY_DIR}/src/ast/compartment.hpp
    ${PROJECT_BINARY_DIR}/src/ast/conductance_hint.hpp
    ${PROJECT_BINARY_DIR}/src/ast/conserve.hpp
    ${PROJECT_BINARY_DIR}/src/ast/constant_block.hpp
    ${PROJECT_BINARY_DIR}/src/ast/constant_statement.hpp
    ${PROJECT_BINARY_DIR}/src/ast/constant_var.hpp
    ${PROJECT_BINARY_DIR}/src/ast/constructor_block.hpp
    ${PROJECT_BINARY_DIR}/src/ast/define.hpp
    ${PROJECT_BINARY_DIR}/src/ast/derivative_block.hpp
    ${PROJECT_BINARY_DIR}/src/ast/derivimplicit_callback.hpp
    ${PROJECT_BINARY_DIR}/src/ast/destructor_block.hpp
    ${PROJECT_BINARY_DIR}/src/ast/diff_eq_expression.hpp
    ${PROJECT_BINARY_DIR}/src/ast/discrete_block.hpp
    ${PROJECT_BINARY_DIR}/src/ast/double.hpp
    ${PROJECT_BINARY_DIR}/src/ast/double_unit.hpp
    ${PROJECT_BINARY_DIR}/src/ast/eigen_linear_solver_block.hpp
    ${PROJECT_BINARY_DIR}/src/ast/eigen_newton_solver_block.hpp
    ${PROJECT_BINARY_DIR}/src/ast/electrode_cur_var.hpp
    ${PROJECT_BINARY_DIR}/src/ast/electrode_current.hpp
    ${PROJECT_BINARY_DIR}/src/ast/else_if_statement.hpp
    ${PROJECT_BINARY_DIR}/src/ast/else_statement.hpp
    ${PROJECT_BINARY_DIR}/src/ast/expression.hpp
    ${PROJECT_BINARY_DIR}/src/ast/expression_statement.hpp
    ${PROJECT_BINARY_DIR}/src/ast/extern_var.hpp
    ${PROJECT_BINARY_DIR}/src/ast/external.hpp
    ${PROJECT_BINARY_DIR}/src/ast/factor_def.hpp
    ${PROJECT_BINARY_DIR}/src/ast/float.hpp
    ${PROJECT_BINARY_DIR}/src/ast/for_netcon.hpp
    ${PROJECT_BINARY_DIR}/src/ast/from_statement.hpp
    ${PROJECT_BINARY_DIR}/src/ast/function_block.hpp
    ${PROJECT_BINARY_DIR}/src/ast/function_call.hpp
    ${PROJECT_BINARY_DIR}/src/ast/function_table_block.hpp
    ${PROJECT_BINARY_DIR}/src/ast/global.hpp
    ${PROJECT_BINARY_DIR}/src/ast/global_var.hpp
    ${PROJECT_BINARY_DIR}/src/ast/identifier.hpp
    ${PROJECT_BINARY_DIR}/src/ast/if_statement.hpp
    ${PROJECT_BINARY_DIR}/src/ast/include.hpp
    ${PROJECT_BINARY_DIR}/src/ast/independent_block.hpp
    ${PROJECT_BINARY_DIR}/src/ast/independent_definition.hpp
    ${PROJECT_BINARY_DIR}/src/ast/indexed_name.hpp
    ${PROJECT_BINARY_DIR}/src/ast/initial_block.hpp
    ${PROJECT_BINARY_DIR}/src/ast/integer.hpp
    ${PROJECT_BINARY_DIR}/src/ast/kinetic_block.hpp
    ${PROJECT_BINARY_DIR}/src/ast/lag_statement.hpp
    ${PROJECT_BINARY_DIR}/src/ast/limits.hpp
    ${PROJECT_BINARY_DIR}/src/ast/lin_equation.hpp
    ${PROJECT_BINARY_DIR}/src/ast/line_comment.hpp
    ${PROJECT_BINARY_DIR}/src/ast/linear_block.hpp
    ${PROJECT_BINARY_DIR}/src/ast/local_list_statement.hpp
    ${PROJECT_BINARY_DIR}/src/ast/local_var.hpp
    ${PROJECT_BINARY_DIR}/src/ast/lon_difuse.hpp
    ${PROJECT_BINARY_DIR}/src/ast/model.hpp
    ${PROJECT_BINARY_DIR}/src/ast/mutex_lock.hpp
    ${PROJECT_BINARY_DIR}/src/ast/mutex_unlock.hpp
    ${PROJECT_BINARY_DIR}/src/ast/name.hpp
    ${PROJECT_BINARY_DIR}/src/ast/net_receive_block.hpp
    ${PROJECT_BINARY_DIR}/src/ast/neuron_block.hpp
    ${PROJECT_BINARY_DIR}/src/ast/node.hpp
    ${PROJECT_BINARY_DIR}/src/ast/non_lin_equation.hpp
    ${PROJECT_BINARY_DIR}/src/ast/non_linear_block.hpp
    ${PROJECT_BINARY_DIR}/src/ast/nonspecific.hpp
    ${PROJECT_BINARY_DIR}/src/ast/nonspecific_cur_var.hpp
    ${PROJECT_BINARY_DIR}/src/ast/nrn_state_block.hpp
    ${PROJECT_BINARY_DIR}/src/ast/number.hpp
    ${PROJECT_BINARY_DIR}/src/ast/number_range.hpp
    ${PROJECT_BINARY_DIR}/src/ast/ontology_statement.hpp
    ${PROJECT_BINARY_DIR}/src/ast/param_assign.hpp
    ${PROJECT_BINARY_DIR}/src/ast/param_block.hpp
    ${PROJECT_BINARY_DIR}/src/ast/paren_expression.hpp
    ${PROJECT_BINARY_DIR}/src/ast/pointer.hpp
    ${PROJECT_BINARY_DIR}/src/ast/pointer_var.hpp
    ${PROJECT_BINARY_DIR}/src/ast/prime_name.hpp
    ${PROJECT_BINARY_DIR}/src/ast/procedure_block.hpp
    ${PROJECT_BINARY_DIR}/src/ast/program.hpp
    ${PROJECT_BINARY_DIR}/src/ast/protect_statement.hpp
    ${PROJECT_BINARY_DIR}/src/ast/range.hpp
    ${PROJECT_BINARY_DIR}/src/ast/range_var.hpp
    ${PROJECT_BINARY_DIR}/src/ast/react_var_name.hpp
    ${PROJECT_BINARY_DIR}/src/ast/reaction_operator.hpp
    ${PROJECT_BINARY_DIR}/src/ast/reaction_statement.hpp
    ${PROJECT_BINARY_DIR}/src/ast/read_ion_var.hpp
    ${PROJECT_BINARY_DIR}/src/ast/solution_expression.hpp
    ${PROJECT_BINARY_DIR}/src/ast/solve_block.hpp
    ${PROJECT_BINARY_DIR}/src/ast/state_block.hpp
    ${PROJECT_BINARY_DIR}/src/ast/statement.hpp
    ${PROJECT_BINARY_DIR}/src/ast/statement_block.hpp
    ${PROJECT_BINARY_DIR}/src/ast/string.hpp
    ${PROJECT_BINARY_DIR}/src/ast/suffix.hpp
    ${PROJECT_BINARY_DIR}/src/ast/table_statement.hpp
    ${PROJECT_BINARY_DIR}/src/ast/thread_safe.hpp
    ${PROJECT_BINARY_DIR}/src/ast/unary_expression.hpp
    ${PROJECT_BINARY_DIR}/src/ast/unary_operator.hpp
    ${PROJECT_BINARY_DIR}/src/ast/unit.hpp
    ${PROJECT_BINARY_DIR}/src/ast/unit_block.hpp
    ${PROJECT_BINARY_DIR}/src/ast/unit_def.hpp
    ${PROJECT_BINARY_DIR}/src/ast/unit_state.hpp
    ${PROJECT_BINARY_DIR}/src/ast/update_dt.hpp
    ${PROJECT_BINARY_DIR}/src/ast/useion.hpp
    ${PROJECT_BINARY_DIR}/src/ast/valence.hpp
    ${PROJECT_BINARY_DIR}/src/ast/var_name.hpp
    ${PROJECT_BINARY_DIR}/src/ast/verbatim.hpp
    ${PROJECT_BINARY_DIR}/src/ast/watch.hpp
    ${PROJECT_BINARY_DIR}/src/ast/watch_statement.hpp
    ${PROJECT_BINARY_DIR}/src/ast/while_statement.hpp
    ${PROJECT_BINARY_DIR}/src/ast/wrapped_expression.hpp
    ${PROJECT_BINARY_DIR}/src/ast/write_ion_var.hpp
)

set(PYBIND_GENERATED_SOURCES
    ${PROJECT_BINARY_DIR}/src/pybind/pyast.cpp
    ${PROJECT_BINARY_DIR}/src/pybind/pyast.hpp
    ${PROJECT_BINARY_DIR}/src/pybind/pynode_0.cpp
    ${PROJECT_BINARY_DIR}/src/pybind/pynode_1.cpp
    ${PROJECT_BINARY_DIR}/src/pybind/pysymtab.cpp
    ${PROJECT_BINARY_DIR}/src/pybind/pyvisitor.cpp
    ${PROJECT_BINARY_DIR}/src/pybind/pyvisitor.hpp
)

set(VISITORS_GENERATED_SOURCES
    ${PROJECT_BINARY_DIR}/src/visitors/ast_visitor.cpp
    ${PROJECT_BINARY_DIR}/src/visitors/ast_visitor.hpp
    ${PROJECT_BINARY_DIR}/src/visitors/checkparent_visitor.cpp
    ${PROJECT_BINARY_DIR}/src/visitors/checkparent_visitor.hpp
    ${PROJECT_BINARY_DIR}/src/visitors/json_visitor.cpp
    ${PROJECT_BINARY_DIR}/src/visitors/json_visitor.hpp
    ${PROJECT_BINARY_DIR}/src/visitors/lookup_visitor.cpp
    ${PROJECT_BINARY_DIR}/src/visitors/lookup_visitor.hpp
    ${PROJECT_BINARY_DIR}/src/visitors/nmodl_visitor.cpp
    ${PROJECT_BINARY_DIR}/src/visitors/nmodl_visitor.hpp
    ${PROJECT_BINARY_DIR}/src/visitors/symtab_visitor.cpp
    ${PROJECT_BINARY_DIR}/src/visitors/symtab_visitor.hpp
    ${PROJECT_BINARY_DIR}/src/visitors/visitor.hpp
)

set(NMODL_GENERATED_SOURCES
    ${AST_GENERATED_SOURCES}
    ${PYBIND_GENERATED_SOURCES}
    ${VISITORS_GENERATED_SOURCES}
)
# cmake-format: on
