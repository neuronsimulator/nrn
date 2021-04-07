/*************************************************************************
 * Copyright (C) 2018-2021 Blue Brain Project
 *
 * This file is part of NMODL distributed under the terms of the GNU
 * Lesser General Public License. See top-level LICENSE file for details.
 *************************************************************************/

#include "visitors/sympy_replace_solutions_visitor.hpp"
#include "visitors/lookup_visitor.hpp"

#include "ast/all.hpp"
#include "utils/logger.hpp"
#include "visitors/visitor_utils.hpp"

namespace nmodl {
namespace visitor {

using namespace fmt::literals;

/**
 * \details SympyReplaceSolutionsVisitor tells us that a new equation appear and, depending where
 * it is located, it can determine if it is part of the main system of equations or is something
 * else. Every time we are out of the system and we print a new equation that is in the system
 * we update the counter. \ref in_system follows, with lag, \param is_in_system and every time
 * they are false and true respectively we detect a switch.
 *
 * \param is_in_system is a bool provided from outside that tells us if a new equation is indeed
 * part of the main system of equations
 */
void SympyReplaceSolutionsVisitor::InterleavesCounter::new_equation(const bool is_in_system) {
    n_interleaves += (!in_system && is_in_system);  // count an interleave only if in_system ==
                                                    // false and is_in_system == true
    in_system = is_in_system;                       // update in_system
}

SympyReplaceSolutionsVisitor::SympyReplaceSolutionsVisitor(
    const std::vector<std::string>& pre_solve_statements,
    const std::vector<std::string>& solutions,
    const std::unordered_set<ast::Statement*>& to_be_removed,
    const ReplacePolicy policy,
    const size_t n_next_equations,
    const std::string& tmp_unique_prefix)
    : pre_solve_statements(pre_solve_statements.begin(), pre_solve_statements.end(), 2)
    , to_be_removed(&to_be_removed)
    , policy(policy)
    , n_next_equations(n_next_equations)
    , replaced_statements_range(-1, -1) {
    // if tmp_unique_prefix we do not expect tmp_statements
    const auto ss_tmp_delimeter =
        tmp_unique_prefix.empty()
            ? solutions.begin()
            : std::find_if(solutions.begin(),
                           solutions.end(),
                           [&tmp_unique_prefix](const std::string& statement) {
                               return statement.substr(0, tmp_unique_prefix.size()) !=
                                      tmp_unique_prefix;
                           });
    tmp_statements = StatementDispenser(solutions.begin(), ss_tmp_delimeter, -1);
    solution_statements = StatementDispenser(ss_tmp_delimeter, solutions.end(), -1);

    replacements.clear();
    is_top_level_statement_block = true;
}


void SympyReplaceSolutionsVisitor::visit_statement_block(ast::StatementBlock& node) {
    const bool current_is_top_level_statement_block =
        is_top_level_statement_block;  // we mark it down since we are going to change it for
    // visiting the children
    is_top_level_statement_block = false;

    if (current_is_top_level_statement_block) {
        logger->debug("SympyReplaceSolutionsVisitor :: visit statements. Matching policy: {}",
                      (policy == ReplacePolicy::VALUE ? "VALUE" : "GREEDY"));
        interleaves_counter = InterleavesCounter();
        node.visit_children(*this);

        if (!solution_statements.tags.empty() && policy == ReplacePolicy::VALUE) {
            logger->debug(
                "SympyReplaceSolutionsVisitor :: not all solutions were replaced. Policy: GREEDY");
            interleaves_counter = InterleavesCounter();
            policy = ReplacePolicy::GREEDY;
            node.visit_children(*this);

            if (interleaves_counter.n() > 0) {
                logger->warn(
                    "SympyReplaceSolutionsVisitor :: Found ambiguous system of equations "
                    "interleaved with {} assignment statements. I do not know what equations go "
                    "before and what "
                    "equations go after the assignment statements. Either put all the equations "
                    "that need to be solved "
                    "in the form: x = f(...) and with distinct variable assignments or do not "
                    "interleave the system with assignments.",
                    interleaves_counter.n());
            }
        }
    } else {
        node.visit_children(*this);
    }

    const auto& old_statements = node.get_statements();

    ast::StatementVector new_statements;
    new_statements.reserve(2 * old_statements.size());
    for (const auto& old_statement: old_statements) {
        const auto& replacement_ptr = replacements.find(old_statement);
        if (replacement_ptr != replacements.end()) {
            if (replaced_statements_range.first == -1) {
                replaced_statements_range.first = new_statements.size();
            }

            new_statements.insert(new_statements.end(),
                                  replacement_ptr->second.begin(),
                                  replacement_ptr->second.end());

            replaced_statements_range.second = new_statements.size();

            logger->debug("SympyReplaceSolutionsVisitor :: erasing {}", to_nmodl(old_statement));
            for (const auto& replacement: replacement_ptr->second) {
                logger->debug("SympyReplaceSolutionsVisitor :: adding {}", to_nmodl(replacement));
            }
        } else if (to_be_removed == nullptr ||
                   to_be_removed->find(&(*old_statement)) == to_be_removed->end()) {
            logger->debug("SympyReplaceSolutionsVisitor :: found {}, nothing to do",
                          to_nmodl(old_statement));
            new_statements.emplace_back(std::move(old_statement));
        } else {
            logger->debug("SympyReplaceSolutionsVisitor :: erasing {}", to_nmodl(old_statement));
        }
    }

    if (current_is_top_level_statement_block) {
        if (!solution_statements.tags.empty()) {
            std::ostringstream ss;
            for (const auto ii: solution_statements.tags) {
                ss << to_nmodl(solution_statements.statements[ii]) << '\n';
            }
            throw std::runtime_error(
                "Not all solutions were replaced! Sympy returned {} equations but I could not find "
                "a place "
                "for all of them. In particular, the following equations remain to be replaced "
                "somewhere:\n{}This is "
                "probably a bug and I invite you to report it to a developer. Possible causes:\n"
                " - I did not do a GREEDY pass and some solutions could not be replaced by VALUE\n "
                "sympy "
                "returned more equations than what we expected\n - There is a bug in the GREEDY "
                "pass\n - some "
                "solutions were replaced but not untagged"_format(
                    solution_statements.statements.size(), ss.str()));
        }

        if (replaced_statements_range.first == -1) {
            replaced_statements_range.first = new_statements.size();
        }
        if (replaced_statements_range.second == -1) {
            replaced_statements_range.second = new_statements.size();
        }
    }

    node.set_statements(std::move(new_statements));
}

void SympyReplaceSolutionsVisitor::try_replace_tagged_statement(
    const ast::Node& node,
    const std::shared_ptr<ast::Expression>& get_lhs(const ast::Node& node),
    const std::shared_ptr<ast::Expression>& get_rhs(const ast::Node& node)) {
    interleaves_counter.new_equation(true);

    const auto& statement = std::static_pointer_cast<ast::Statement>(
        node.get_parent()->get_shared_ptr());

    // do not visit if already marked
    if (replacements.find(statement) != replacements.end()) {
        return;
    }


    switch (policy) {
    case ReplacePolicy::VALUE: {
        const auto dependencies = statement_dependencies(get_lhs(node), get_rhs(node));
        const auto& key = dependencies.first;
        const auto& vars = dependencies.second;

        if (solution_statements.is_var_assigned_here(key)) {
            logger->debug("SympyReplaceSolutionsVisitor :: marking for replacement {}",
                          to_nmodl(statement));

            ast::StatementVector new_statements;

            pre_solve_statements.emplace_back_all_tagged_statements(new_statements);
            tmp_statements.emplace_back_all_tagged_statements(new_statements);
            solution_statements.try_emplace_back_tagged_statement(new_statements, key);

            replacements.emplace(statement, new_statements);
        }
        break;
    }
    case ReplacePolicy::GREEDY: {
        logger->debug("SympyReplaceSolutionsVisitor :: marking for replacement {}",
                      to_nmodl(statement));

        ast::StatementVector new_statements;

        pre_solve_statements.emplace_back_all_tagged_statements(new_statements);
        tmp_statements.emplace_back_all_tagged_statements(new_statements);
        solution_statements.emplace_back_next_tagged_statements(new_statements, n_next_equations);

        replacements.emplace(statement, new_statements);
        break;
    }
    }
}


void SympyReplaceSolutionsVisitor::visit_diff_eq_expression(ast::DiffEqExpression& node) {
    logger->debug("SympyReplaceSolutionsVisitor :: visit {}", to_nmodl(node));
    auto get_lhs = [](const ast::Node& node) -> const std::shared_ptr<ast::Expression>& {
        return static_cast<const ast::DiffEqExpression&>(node).get_expression()->get_lhs();
    };

    auto get_rhs = [](const ast::Node& node) -> const std::shared_ptr<ast::Expression>& {
        return static_cast<const ast::DiffEqExpression&>(node).get_expression()->get_rhs();
    };

    try_replace_tagged_statement(node, get_lhs, get_rhs);
}

void SympyReplaceSolutionsVisitor::visit_lin_equation(ast::LinEquation& node) {
    logger->debug("SympyReplaceSolutionsVisitor :: visit {}", to_nmodl(node));
    auto get_lhs = [](const ast::Node& node) -> const std::shared_ptr<ast::Expression>& {
        return static_cast<const ast::LinEquation&>(node).get_left_linxpression();
    };

    auto get_rhs = [](const ast::Node& node) -> const std::shared_ptr<ast::Expression>& {
        return static_cast<const ast::LinEquation&>(node).get_left_linxpression();
    };

    try_replace_tagged_statement(node, get_lhs, get_rhs);
}


void SympyReplaceSolutionsVisitor::visit_non_lin_equation(ast::NonLinEquation& node) {
    logger->debug("SympyReplaceSolutionsVisitor :: visit {}", to_nmodl(node));
    auto get_lhs = [](const ast::Node& node) -> const std::shared_ptr<ast::Expression>& {
        return static_cast<const ast::NonLinEquation&>(node).get_lhs();
    };

    auto get_rhs = [](const ast::Node& node) -> const std::shared_ptr<ast::Expression>& {
        return static_cast<const ast::NonLinEquation&>(node).get_rhs();
    };

    try_replace_tagged_statement(node, get_lhs, get_rhs);
}


void SympyReplaceSolutionsVisitor::visit_binary_expression(ast::BinaryExpression& node) {
    logger->debug("SympyReplaceSolutionsVisitor :: visit {}", to_nmodl(node));
    if (node.get_op().get_value() == ast::BinaryOp::BOP_ASSIGN && node.get_lhs()->is_var_name()) {
        interleaves_counter.new_equation(false);

        const auto& var =
            std::static_pointer_cast<ast::VarName>(node.get_lhs())->get_name()->get_node_name();
        pre_solve_statements.tag_dependant_statements(var);
        tmp_statements.tag_dependant_statements(var);
    }
}


SympyReplaceSolutionsVisitor::StatementDispenser::StatementDispenser(
    const std::vector<std::string>::const_iterator& statements_str_beg,
    const std::vector<std::string>::const_iterator& statements_str_end,
    const int error_on_n_flushes)
    : statements(create_statements(statements_str_beg, statements_str_end))
    , error_on_n_flushes(error_on_n_flushes) {
    tag_all_statements();
    build_maps();
}


/**
 * \details CHere we construct a map variable -> affected equations. In other words this map tells
 * me what equations need to be updated when I change a particular variable. To do that we build a a
 * graph of dependencies var -> vars and in the mean time we reduce it to the root variables. This
 * is ensured by the fact that the tmp variables are sorted so that the next tmp variable may depend
 * on the previous one. Since it is a relation of equivalence (if an equation depends on a variable,
 * it needs to be updated if the variable changes), we build the two maps at the same time.
 *
 * An example:
 *
 *  \code{.mod}
 *  tmp0 = x + a
 *  tmp1 = tmp0 + b
 *  tmp2 = y
 *  \endcode
 *
 * dependency_map should be (the order of the equation is unimportant since we are building
 * a map):
 *
 * - tmp0 : x, a
 * - tmp1 : x, a, b
 * - tmp2 : y
 *
 * and the var2statement map should be (the order of the following equations is unimportant
 * since we are building a map. The number represents the index of the original equations):
 *
 * - x : 0, 1
 * - y : 2
 * - a : 0, 1
 * - b : 1
 *
 */
void SympyReplaceSolutionsVisitor::StatementDispenser::build_maps() {
    for (size_t ii = 0; ii < statements.size(); ++ii) {
        const auto& statement = statements[ii];

        if (statement->is_expression_statement()) {
            const auto& e_statement =
                std::static_pointer_cast<ast::ExpressionStatement>(statement)->get_expression();
            if (e_statement->is_binary_expression()) {
                const auto& bin_exp = std::static_pointer_cast<ast::BinaryExpression>(e_statement);
                const auto& dependencies = statement_dependencies(bin_exp->get_lhs(),
                                                                  bin_exp->get_rhs());

                const auto& key = dependencies.first;
                const auto& vars = dependencies.second;
                if (!key.empty()) {
                    var2statement.emplace(key, ii);
                    for (const auto& var: vars) {
                        const auto& var_already_inserted = dependency_map.find(var);
                        if (var_already_inserted != dependency_map.end()) {
                            dependency_map[key].insert(var_already_inserted->second.begin(),
                                                       var_already_inserted->second.end());
                            for (const auto& root_var: var_already_inserted->second) {
                                var2dependants[root_var].insert(ii);
                            }
                        } else {
                            dependency_map[key].insert(var);
                            var2dependants[var].insert(ii);
                        }
                    }
                }
            }
        }
    }

    logger->debug("SympyReplaceSolutionsVisitor::StatementDispenser :: var2dependants map");
    for (const auto& entry: var2dependants) {
        logger->debug("SympyReplaceSolutionsVisitor::StatementDispenser :: var `{}` used in:",
                      entry.first);
        for (const auto ii: entry.second) {
            logger->debug("SympyReplaceSolutionsVisitor::StatementDispenser ::    -> {}",
                          to_nmodl(statements[ii]));
        }
    }
    logger->debug("SympyReplaceSolutionsVisitor::StatementDispenser :: var2statement map");
    for (const auto& entry: var2statement) {
        logger->debug("SympyReplaceSolutionsVisitor::StatementDispenser :: var `{}` defined in:",
                      entry.first);
        logger->debug("SympyReplaceSolutionsVisitor::StatementDispenser ::    -> {}",
                      to_nmodl(statements[entry.second]));
    }
}

bool SympyReplaceSolutionsVisitor::StatementDispenser::try_emplace_back_tagged_statement(
    ast::StatementVector& new_statements,
    const std::string& var) {
    auto ptr = var2statement.find(var);
    bool emplaced = false;
    if (ptr != var2statement.end()) {
        const auto ii = ptr->second;
        const auto tag_ptr = tags.find(ii);
        if (tag_ptr != tags.end()) {
            new_statements.emplace_back(statements[ii]->clone());
            tags.erase(tag_ptr);
            emplaced = true;

            logger->debug(
                "SympyReplaceSolutionsVisitor::StatementDispenser :: adding to replacement rule {}",
                to_nmodl(statements[ii]));
        } else {
            logger->error(
                "SympyReplaceSolutionsVisitor::StatementDispenser :: tried adding to replacement "
                "rule {} but statement is not "
                "tagged",
                to_nmodl(statements[ii]));
        }
    }
    return emplaced;
}

size_t SympyReplaceSolutionsVisitor::StatementDispenser::emplace_back_next_tagged_statements(
    ast::StatementVector& new_statements,
    const size_t n_next_statements) {
    size_t counter = 0;
    for (size_t next_statement_ii = 0;
         next_statement_ii < statements.size() && counter < n_next_statements;
         ++next_statement_ii) {
        const auto tag_ptr = tags.find(next_statement_ii);
        if (tag_ptr != tags.end()) {
            logger->debug(
                "SympyReplaceSolutionsVisitor::StatementDispenser :: adding to replacement rule {}",
                to_nmodl(statements[next_statement_ii]));
            new_statements.emplace_back(statements[next_statement_ii]->clone());
            tags.erase(tag_ptr);
            ++counter;
        }
    }
    return counter;
}

size_t SympyReplaceSolutionsVisitor::StatementDispenser::emplace_back_all_tagged_statements(
    ast::StatementVector& new_statements) {
    for (const auto ii: tags) {
        new_statements.emplace_back(statements[ii]->clone());
        logger->debug(
            "SympyReplaceSolutionsVisitor::StatementDispenser :: adding to replacement rule {}",
            to_nmodl(statements[ii]));
    }

    n_flushes += (!tags.empty());
    if (error_on_n_flushes > 0 && n_flushes >= error_on_n_flushes) {
        throw std::runtime_error(
            "SympyReplaceSolutionsVisitor::StatementDispenser :: State variable assignment(s) "
            "interleaved in system "
            "of "
            "equations/differential equations. It is not allowed due to possible numerical "
            "instability and undefined "
            "behavior. Erase the assignment statement(s) or move them before/after the"
            " set of equations/differential equations.");
    }

    const auto n_replacements = tags.size();

    tags.clear();

    return n_replacements;
}

size_t SympyReplaceSolutionsVisitor::StatementDispenser::tag_dependant_statements(
    const std::string& var) {
    auto ptr = var2dependants.find(var);
    size_t n = 0;
    if (ptr != var2dependants.end()) {
        for (const auto ii: ptr->second) {
            const auto pos = tags.insert(ii);
            if (pos.second) {
                logger->debug("SympyReplaceSolutionsVisitor::StatementDispenser :: tagging {}",
                              to_nmodl(statements[ii]));
            }
            ++n;
        }
    }
    return n;
}

void SympyReplaceSolutionsVisitor::StatementDispenser::tag_all_statements() {
    logger->debug("SympyReplaceSolutionsVisitor::StatementDispenser :: tagging all statements");
    for (size_t i = 0; i < statements.size(); ++i) {
        tags.insert(i);
        logger->debug("SympyReplaceSolutionsVisitor::StatementDispenser :: tagging {}",
                      to_nmodl(statements[i]));
    }
}


}  // namespace visitor
}  // namespace nmodl
