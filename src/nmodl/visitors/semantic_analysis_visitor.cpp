/*
 * Copyright 2023 Blue Brain Project, EPFL.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "fmt/ranges.h"

#include "visitors/semantic_analysis_visitor.hpp"
#include "ast/breakpoint_block.hpp"
#include "ast/function_block.hpp"
#include "ast/function_call.hpp"
#include "ast/function_table_block.hpp"
#include "ast/independent_block.hpp"
#include "ast/net_receive_block.hpp"
#include "ast/procedure_block.hpp"
#include "ast/program.hpp"
#include "ast/range_var.hpp"
#include "ast/statement_block.hpp"
#include "ast/string.hpp"
#include "ast/suffix.hpp"
#include "ast/binary_expression.hpp"
#include "ast/table_statement.hpp"
#include "symtab/symbol_properties.hpp"
#include "utils/logger.hpp"
#include "visitors/visitor_utils.hpp"

namespace nmodl {
namespace visitor {


bool SemanticAnalysisVisitor::check_table_vars(const ast::Program& node) {
    // check that we do not have any duplicate TABLE statement variables in PROCEDUREs
    const auto& procedure_nodes = collect_nodes(node, {ast::AstNodeType::PROCEDURE_BLOCK});
    std::unordered_set<std::string> procedure_vars{};
    for (const auto& proc_node: procedure_nodes) {
        const auto& table_nodes = collect_nodes(*proc_node, {ast::AstNodeType::TABLE_STATEMENT});
        for (const auto& table_node: table_nodes) {
            const auto& table_vars =
                std::dynamic_pointer_cast<const ast::TableStatement>(table_node)->get_table_vars();
            for (const auto& table_var: table_vars) {
                const auto& [var_name,
                             inserted] = procedure_vars.insert(table_var->get_node_name());
                if (!inserted) {
                    logger->critical(
                        fmt::format("SemanticAnalysisVisitor :: TABLE statement variable {} used "
                                    "in multiple tables",
                                    *var_name));
                    check_fail = true;
                }
            }
        }
    }

    return check_fail;
}

static bool check_function_has_return_statement(const ast::FunctionBlock& node) {
    // check a given FUNCTION has a return statement, and return true if yes, false if no
    const auto& func_name = node.get_node_name();
    // get all binary expressions that are of the form x = y
    const auto& binary_expr_nodes = collect_nodes(node, {ast::AstNodeType::BINARY_EXPRESSION});
    for (const auto& binary_expr_node: binary_expr_nodes) {
        const auto& expr = std::dynamic_pointer_cast<const ast::BinaryExpression>(binary_expr_node);
        const auto& lhs = expr->get_lhs();
        const auto& op = expr->get_op();
        if (op.eval() == "=" && lhs->get_node_name() == func_name) {
            return true;
        }
    }
    return false;
}

static bool check_function_has_verbatim_block(const ast::FunctionBlock& node) {
    // check a given FUNCTION has a VERBATIM block; those can interfere with detecting return
    // statements
    const auto& func_name = node.get_node_name();
    const auto& verbatim_blocks = collect_nodes(node, {ast::AstNodeType::VERBATIM});
    return !verbatim_blocks.empty();
}


void SemanticAnalysisVisitor::check_functions_have_return_statements(const ast::Program& node) {
    // check that all functions have a return statement, i.e. that there is a statement of the form
    // <funcname> = <expr> somewhere in each FUNCTION block
    const auto& function_nodes = collect_nodes(node, {ast::AstNodeType::FUNCTION_BLOCK});
    for (const auto& func_node: function_nodes) {
        const auto& func = std::dynamic_pointer_cast<const ast::FunctionBlock>(func_node);
        const auto& has_return_statement = check_function_has_return_statement(*func);
        const auto& has_verbatim_block = check_function_has_verbatim_block(*func);
        if (!has_return_statement) {
            if (!has_verbatim_block) {
                logger->warn(fmt::format(
                    "SemanticAnalysisVisitor :: FUNCTION {} does not have a return statement",
                    func->get_node_name()));
            } else {
                logger->warn(
                    fmt::format("SemanticAnalysisVisitor :: FUNCTION {} does not have an explicit "
                                "return statement, but VERBATIM block detected (possible return "
                                "statement in VERBATIM block?)",
                                func->get_node_name()));
            }
        }
    }
}


bool SemanticAnalysisVisitor::check_name_conflict(const ast::Program& node) {
    // check that there are no RANGE variables which have the same name as a FUNCTION or PROCEDURE
    const auto& range_nodes = collect_nodes(node, {ast::AstNodeType::RANGE_VAR});
    std::set<std::string> range_vars{};
    for (const auto& range_node: range_nodes) {
        range_vars.insert(range_node->get_node_name());
    }

    const auto& function_nodes =
        collect_nodes(node, {ast::AstNodeType::FUNCTION_BLOCK, ast::AstNodeType::PROCEDURE_BLOCK});
    std::set<std::string> func_vars{};
    for (const auto& function_node: function_nodes) {
        func_vars.insert(function_node->get_node_name());
    }

    std::vector<std::string> result;
    std::set_intersection(func_vars.begin(),
                          func_vars.end(),
                          range_vars.begin(),
                          range_vars.end(),
                          std::back_inserter(result));
    const auto& ret_value = !result.empty();
    if (ret_value) {
        logger->critical("Duplicate name(s) found: {}", fmt::join(result, ", "));
    }
    return ret_value;
}

bool SemanticAnalysisVisitor::check(const ast::Program& node) {
    check_fail = false;
    program_symtab = node.get_symbol_table();

    /// <-- This code is for check 2
    const auto& suffix_node = collect_nodes(node, {ast::AstNodeType::SUFFIX});
    if (!suffix_node.empty()) {
        const auto& suffix = std::dynamic_pointer_cast<const ast::Suffix>(suffix_node[0]);
        const auto& type = suffix->get_type()->get_node_name();
        is_point_process = (type == "POINT_PROCESS" || type == "ARTIFICIAL_CELL");
    }
    /// -->

    check_fail = check_fail || check_table_vars(node) || check_name_conflict(node);

    /// <-- This code is for check 4
    using namespace symtab::syminfo;
    const auto& with_prop = NmodlType::read_ion_var | NmodlType::write_ion_var;

    const auto& sym_table = node.get_symbol_table();
    assert(sym_table != nullptr);

    // get all ion variables
    const auto& ion_variables = sym_table->get_variables_with_properties(with_prop, false);

    /// make sure ion variables aren't redefined in a `CONSTANT` block.
    for (const auto& var: ion_variables) {
        if (var->has_any_property(NmodlType::constant_var)) {
            logger->critical(
                fmt::format("SemanticAnalysisVisitor :: ion variable {} from the USEION statement "
                            "can not be re-declared in a CONSTANT block",
                            var->get_name()));
            check_fail = true;
        }
    }
    /// -->

    check_functions_have_return_statements(node);

    visit_program(node);
    return check_fail;
}

void SemanticAnalysisVisitor::visit_program(const ast::Program& node) {
    /// <-- This code is for check 8
    const auto& derivative_block_nodes = collect_nodes(node, {ast::AstNodeType::DERIVATIVE_BLOCK});
    if (derivative_block_nodes.size() > 1) {
        logger->critical("It is not supported to have several DERIVATIVE blocks");
        check_fail = true;
    }
    /// -->
    node.visit_children(*this);
}

void SemanticAnalysisVisitor::visit_procedure_block(const ast::ProcedureBlock& node) {
    /// <-- This code is for check 1
    in_procedure = true;
    one_arg_in_procedure_function = node.get_parameters().size() == 1;
    node.visit_children(*this);
    in_procedure = false;
    /// -->
}

void SemanticAnalysisVisitor::visit_function_block(const ast::FunctionBlock& node) {
    /// <-- This code is for check 1
    in_function = true;
    one_arg_in_procedure_function = node.get_parameters().size() == 1;
    node.visit_children(*this);
    in_function = false;
    /// -->
}

void SemanticAnalysisVisitor::visit_name(const ast::Name& node) {
    /// <-- This code is a portion of  check 9
    // There are only two contexts where a random_var is allowed. As the first arg of a random
    // function or as an item in the RANDOM declaration.
    // Only the former needs checking.
    auto name = node.get_node_name();

    // only check for variables exist in the symbol table (e.g. SUFFIX has type Name but it's not
    // variable)
    // if variable is not RANDOM then nothing to check for it
    auto symbol = program_symtab->lookup(name);
    if (!symbol || !symbol->has_any_property(symtab::syminfo::NmodlType::random_var)) {
        return;
    }

    auto parent = node.get_parent();

    // if it's RANDOM var declaration in NEURON block then nothing to do
    if (parent && parent->is_random_var()) {
        return;
    }

    if (parent && parent->is_var_name()) {
        parent = parent->get_parent();
        if (parent && parent->is_function_call()) {
            auto fname = parent->get_node_name();
            // if function is a random function then check if the current
            // name is the function's first argument
            if (is_random_construct_function(fname)) {
                auto rfun = dynamic_cast<ast::FunctionCall*>(parent);
                const auto& arguments = rfun->get_arguments();
                if (!arguments.empty() && arguments.front()->is_var_name() &&
                    arguments.front()->get_node_name() == name) {
                    // if this is a first argument to function then there
                    // is no problem
                    node.visit_children(*this);
                    return;
                }
            }
        }
    }

    // Otherwise, we have an error
    auto position = node.get_token()->position();
    logger->critical(
        fmt::format("SemanticAnalysisVisitor :: RANDOM variable {} at {}"
                    " can be used only as the first arg of a random function",
                    node.get_node_name(),
                    position));
    check_fail = true;

    node.visit_children(*this);
    /// -->
}

void SemanticAnalysisVisitor::visit_function_call(const ast::FunctionCall& node) {
    /// <-- This code is a portion of  check 9
    //  The first arg of a RANDOM function must be a random_var
    // Otherwise it's an error
    auto fname = node.get_node_name();
    auto position = node.get_name()->get_token()->position();

    if (is_random_construct_function(fname)) {
        const auto& arguments = node.get_arguments();
        if (!arguments.empty()) {
            auto arg0 = arguments.front();
            if (arg0->is_var_name()) {
                auto name = arg0->get_node_name();
                auto symbol = program_symtab->lookup(name);
                if (symbol->has_any_property(symtab::syminfo::NmodlType::random_var)) {
                    node.visit_children(*this);
                    return;
                }
            }
        }
        logger->critical(
            fmt::format("SemanticAnalysisVisitor :: random function {} at {} :: The first arg must "
                        "be a random variable",
                        fname,
                        position));
        check_fail = true;
    }

    if (is_nrn_pointing(fname)) {
        if (size_t args_size = node.get_arguments().size(); args_size != 1) {
            logger->critical(
                fmt::format("nrn_pointing excepts exactly one argument, got: {}", args_size));
            check_fail = true;
        }
    }

    if (is_nrn_state_disc(fname)) {
        // check that a call to `state_discontinuity` has exactly 2 arguments
        if (size_t args_size = node.get_arguments().size(); args_size != 2) {
            logger->critical("state_discontinuity accepts exactly two arguments, got: {}",
                             args_size);
            check_fail = true;
            return;
        }
        // check that the first arg is a variable
        const auto& first = node.get_arguments()[0];
        if (!first->is_var_name()) {
            logger->critical(
                "state_discontinuity first arg must be a variable, not a compound expression");
            check_fail = true;
            return;
        }
        if (!in_net_receive_block) {
            logger->warn(
                "Use of state_discontinuity is not thread safe except in a NET_RECEIVE block");
        }
    }

    node.visit_children(*this);
    /// -->
}

void SemanticAnalysisVisitor::visit_table_statement(const ast::TableStatement& tableStmt) {
    /// <-- This code is for check 1
    if ((in_function || in_procedure) && !one_arg_in_procedure_function) {
        logger->critical(
            "SemanticAnalysisVisitor :: The procedure or function containing the TABLE statement "
            "should contains exactly one argument.");
        check_fail = true;
    }
    /// -->
    /// <-- This code is for check 3
    const auto& table_vars = tableStmt.get_table_vars();
    if (in_function && !table_vars.empty()) {
        logger->critical(
            "SemanticAnalysisVisitor :: TABLE statement in FUNCTION cannot have a table name "
            "list.");
    }
    if (in_procedure && table_vars.empty()) {
        logger->critical(
            "SemanticAnalysisVisitor :: TABLE statement in PROCEDURE must have a table name list.");
    }
    /// -->
}

void SemanticAnalysisVisitor::visit_destructor_block(const ast::DestructorBlock& /* node */) {
    /// <-- This code is for check 2
    if (!is_point_process) {
        logger->warn(
            "SemanticAnalysisVisitor :: This mod file is not point process but contains a "
            "destructor.");
        check_fail = true;
    }
    /// -->
}

void SemanticAnalysisVisitor::visit_independent_block(const ast::IndependentBlock& node) {
    /// <-- This code is for check 5
    for (const auto& n: node.get_variables()) {
        if (n->get_value()->get_value() != "t") {
            logger->warn(
                "SemanticAnalysisVisitor :: '{}' cannot be used as an independent variable, only "
                "'t' is allowed.",
                n->get_value()->get_value());
        }
    }
    /// -->
}

void SemanticAnalysisVisitor::visit_function_table_block(const ast::FunctionTableBlock& node) {
    /// <-- This code is for check 7
    if (node.get_parameters().size() < 1) {
        logger->critical(
            "SemanticAnalysisVisitor :: Function table '{}' must have one or more arguments.",
            node.get_node_name());
        check_fail = true;
    }
    /// -->
}

void SemanticAnalysisVisitor::visit_protect_statement(const ast::ProtectStatement& /* node */) {
    /// <-- This code is for check 6
    if (accel_backend) {
        logger->error("PROTECT statement is not supported with GPU execution");
    }
    if (in_mutex) {
        logger->warn("SemanticAnalysisVisitor :: Find a PROTECT inside a already locked part.");
    }
    /// -->
}

void SemanticAnalysisVisitor::visit_mutex_lock(const ast::MutexLock& /* node */) {
    /// <-- This code is for check 6
    if (accel_backend) {
        logger->error("MUTEXLOCK statement is not supported with GPU execution");
    }
    if (in_mutex) {
        logger->warn("SemanticAnalysisVisitor :: Found a MUTEXLOCK inside an already locked part.");
    }
    in_mutex = true;
    /// -->
}

void SemanticAnalysisVisitor::visit_net_receive_block(const ast::NetReceiveBlock& node) {
    in_net_receive_block = true;
    node.visit_children(*this);
    in_net_receive_block = false;
}

void SemanticAnalysisVisitor::visit_mutex_unlock(const ast::MutexUnlock& /* node */) {
    /// <-- This code is for check 6
    if (accel_backend) {
        logger->error("MUTEXUNLOCK statement is not supported with GPU execution");
    }
    if (!in_mutex) {
        logger->warn("SemanticAnalysisVisitor :: Found a MUTEXUNLOCK outside a locked part.");
    }
    in_mutex = false;
    /// -->
}

}  // namespace visitor
}  // namespace nmodl
