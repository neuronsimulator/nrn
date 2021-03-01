/*************************************************************************
 * Copyright (C) 2018-2019 Blue Brain Project
 *
 * This file is part of NMODL distributed under the terms of the GNU
 * Lesser General Public License. See top-level LICENSE file for details.
 *************************************************************************/

#include "visitors/sympy_solver_visitor.hpp"
#include "visitors/sympy_replace_solutions_visitor.hpp"

#include "ast/all.hpp"
#include "codegen/codegen_naming.hpp"
#include "pybind/pyembed.hpp"
#include "symtab/symbol.hpp"
#include "utils/logger.hpp"
#include "utils/string_utils.hpp"
#include "visitors/visitor_utils.hpp"


namespace pywrap = nmodl::pybind_wrappers;

namespace nmodl {
namespace visitor {

using symtab::syminfo::NmodlType;

using nmodl::utils::UseNumbersInString;

void SympySolverVisitor::init_block_data(ast::Node* node) {
    // clear any previous data
    expression_statements.clear();
    eq_system.clear();
    state_vars_in_block.clear();
    last_expression_statement = nullptr;
    block_with_expression_statements = nullptr;
    eq_system_is_valid = true;
    conserve_equation.clear();
    // get set of local block vars & global vars
    vars = global_vars;
    if (auto symtab = node->get_statement_block()->get_symbol_table()) {
        auto localvars = symtab->get_variables_with_properties(NmodlType::local_var);
        for (const auto& localvar: localvars) {
            std::string var_name = localvar->get_name();
            if (localvar->is_array()) {
                var_name += "[" + std::to_string(localvar->get_length()) + "]";
            }
            vars.insert(var_name);
        }
    }
    const auto& fcall_nodes = collect_nodes(*node->get_statement_block(),
                                            {ast::AstNodeType::FUNCTION_CALL});
    for (const auto& call: fcall_nodes) {
        function_calls.insert(call->get_node_name());
    }
}

void SympySolverVisitor::init_state_vars_vector() {
    state_vars.clear();
    for (const auto& state_var: all_state_vars) {
        if (state_vars_in_block.find(state_var) != state_vars_in_block.cend()) {
            state_vars.push_back(state_var);
        }
    }
}

void SympySolverVisitor::replace_diffeq_expression(ast::DiffEqExpression& expr,
                                                   const std::string& new_expr) {
    auto new_statement = create_statement(new_expr);
    auto new_expr_statement = std::dynamic_pointer_cast<ast::ExpressionStatement>(new_statement);
    auto new_bin_expr = std::dynamic_pointer_cast<ast::BinaryExpression>(
        new_expr_statement->get_expression());
    expr.set_expression(std::move(new_bin_expr));
}

void SympySolverVisitor::check_expr_statements_in_same_block() {
    /// all ode/kinetic/(non)linear statements (typically) appear in the same statement block
    /// if this is not the case, for now return an error (and should instead use fallback solver)
    if (block_with_expression_statements != nullptr &&
        block_with_expression_statements != current_statement_block) {
        logger->warn(
            "SympySolverVisitor :: Coupled equations are appearing in different blocks - not "
            "supported");
        eq_system_is_valid = false;
    }
    block_with_expression_statements = current_statement_block;
}

ast::StatementVector::const_iterator SympySolverVisitor::get_solution_location_iterator(
    const ast::StatementVector& statements) {
    // find out where to insert solutions in statement block
    // returns iterator pointing to the first element after the last (non)linear eq
    // so if there are no such elements, it returns statements.end()
    auto it = statements.begin();
    if (last_expression_statement != nullptr) {
        while ((it != statements.end()) &&
               (std::dynamic_pointer_cast<ast::ExpressionStatement>(*it).get() !=
                last_expression_statement)) {
            logger->debug("SympySolverVisitor :: {} != {}",
                          to_nmodl(*it),
                          to_nmodl(*last_expression_statement));
            ++it;
        }
        if (it != statements.end()) {
            logger->debug("SympySolverVisitor :: {} == {}",
                          to_nmodl(std::dynamic_pointer_cast<ast::ExpressionStatement>(*it)),
                          to_nmodl(*last_expression_statement));
            ++it;
        }
    }
    return it;
}

/**
 * Check if provided statement is local variable declaration statement
 * @param statement AST node representing statement in the MOD file
 * @return True if statement is local variable declaration else False
 *
 * Statement declaration could be wrapped into another statement type like
 * expression statement and hence we try to look inside if it's really a
 * variable declaration.
 */
static bool is_local_statement(const std::shared_ptr<ast::Statement>& statement) {
    if (statement->is_local_list_statement()) {
        return true;
    }
    if (statement->is_expression_statement()) {
        auto e_statement = std::dynamic_pointer_cast<ast::ExpressionStatement>(statement);
        auto expression = e_statement->get_expression();
        if (expression->is_local_list_statement()) {
            return true;
        }
    }
    return false;
}

std::string& SympySolverVisitor::replaceAll(std::string& context,
                                            const std::string& from,
                                            const std::string& to) const {
    std::size_t lookHere = 0;
    std::size_t foundHere;
    while ((foundHere = context.find(from, lookHere)) != std::string::npos) {
        context.replace(foundHere, from.size(), to);
        lookHere = foundHere + to.size();
    }
    return context;
}

std::vector<std::string> SympySolverVisitor::filter_string_vector(
    const std::vector<std::string>& original_vector,
    const std::string& original_string,
    const std::string& substitution_string) const {
    std::vector<std::string> filtered_vector;
    for (auto element: original_vector) {
        std::string filtered_element = replaceAll(element, original_string, substitution_string);
        filtered_vector.push_back(filtered_element);
    }
    return filtered_vector;
}

void SympySolverVisitor::construct_eigen_solver_block(
    const std::vector<std::string>& pre_solve_statements,
    const std::vector<std::string>& solutions,
    bool linear) {
    // Provide random string to append to X, J, Jm and F matrices that
    // are produced by sympy
    std::string unique_X = suffix_random_string(vars, "X");
    std::string unique_J = suffix_random_string(vars, "J");
    std::string unique_Jm = suffix_random_string(vars, "Jm");
    std::string unique_F = suffix_random_string(vars, "F");

    // filter solutions for matrices named "X", "J", "Jm" and "F" and change them to
    // unique_X, unique_J, unique_Jm and unique_F respectively
    auto solutions_filtered = filter_string_vector(solutions, "X[", unique_X + "[");
    solutions_filtered = filter_string_vector(solutions_filtered, "J[", unique_J + "[");
    solutions_filtered = filter_string_vector(solutions_filtered, "Jm[", unique_Jm + "[");
    solutions_filtered = filter_string_vector(solutions_filtered, "F[", unique_F + "[");

    for (const auto& sol: solutions_filtered) {
        logger->debug("SympySolverVisitor :: -> adding statement: {}", sol);
    }

    std::vector<std::string> pre_solve_statements_and_setup_x_eqs(pre_solve_statements);
    std::vector<std::string> update_statements;
    for (int i = 0; i < state_vars.size(); i++) {
        auto update_state = state_vars[i] + " = " + unique_X + "[" + std::to_string(i) + "]";
        auto setup_x = unique_X + "[" + std::to_string(i) + "] = " + state_vars[i];

        pre_solve_statements_and_setup_x_eqs.push_back(setup_x);
        update_statements.push_back(update_state);
        logger->debug("SympySolverVisitor :: setup_", unique_X, ": {}", setup_x);
        logger->debug("SympySolverVisitor :: update_state: {}", update_state);
    }

    visitor::SympyReplaceSolutionsVisitor solution_replacer(
        pre_solve_statements_and_setup_x_eqs,
        solutions_filtered,
        expression_statements,
        visitor::SympyReplaceSolutionsVisitor::ReplacePolicy::GREEDY,
        state_vars.size() + 1,
        "");
    solution_replacer.visit_statement_block(*block_with_expression_statements);

    // split in the various blocks for eigen
    auto n_state_vars = std::make_shared<ast::Integer>(state_vars.size(), nullptr);

    const auto& statements = block_with_expression_statements->get_statements();

    ast::StatementVector variable_statements;    // LOCAL //
    ast::StatementVector initialize_statements;  // pre_solve_statements //
    ast::StatementVector setup_x_statements;     // old_x = x, X[0] = x //
    ast::StatementVector functor_statements;   // J[0]_row * X = F[0], additional assignments during
                                               // computation //
    ast::StatementVector finalize_statements;  // assignments at the end //
    const size_t sr_begin = solution_replacer.replaced_statements_begin();
    const size_t sr_end = solution_replacer.replaced_statements_end();

    // initialize and edge case where the system of equations is empty
    for (size_t idx = 0; idx < statements.size(); ++idx) {
        auto& s = statements[idx];
        if (is_local_statement(s)) {
            variable_statements.push_back(s);
        } else if (sr_begin == statements.size()) {
            initialize_statements.push_back(s);
        } else if (idx < sr_begin) {
            initialize_statements.push_back(s);
        }
    }

    if (sr_begin != statements.size()) {
        initialize_statements.insert(initialize_statements.end(),
                                     statements.begin() + sr_begin,
                                     statements.begin() + sr_begin + pre_solve_statements.size());
        setup_x_statements =
            ast::StatementVector(statements.begin() + sr_begin + pre_solve_statements.size(),
                                 statements.begin() + sr_begin + pre_solve_statements.size() +
                                     state_vars.size());
        functor_statements = ast::StatementVector(statements.begin() + sr_begin +
                                                      pre_solve_statements.size() +
                                                      state_vars.size(),
                                                  statements.begin() + sr_end);
        finalize_statements = ast::StatementVector(statements.begin() + sr_end, statements.end());
    }

    const size_t total_statements_size = variable_statements.size() + initialize_statements.size() +
                                         setup_x_statements.size() + functor_statements.size() +
                                         finalize_statements.size();
    if (statements.size() != total_statements_size) {
        logger->error(
            "SympySolverVisitor :: statement number missmatch ({} =/= {}) during splitting before "
            "creation of "
            "eigen "
            "solver block.",
            statements.size(),
            total_statements_size);
        return;
    }

    auto variable_block = std::make_shared<ast::StatementBlock>(std::move(variable_statements));
    auto initialize_block = std::make_shared<ast::StatementBlock>(std::move(initialize_statements));
    auto update_state_block = create_statement_block(update_statements);
    auto finalize_block = std::make_shared<ast::StatementBlock>(std::move(finalize_statements));
    if (linear) {
        /// functor and initialize block converge in the same block
        setup_x_statements.insert(setup_x_statements.end(),
                                  functor_statements.begin(),
                                  functor_statements.end());
        auto setup_x_block = std::make_shared<ast::StatementBlock>(std::move(setup_x_statements));
        auto solver_block = std::make_shared<ast::EigenLinearSolverBlock>(n_state_vars,
                                                                          variable_block,
                                                                          initialize_block,
                                                                          setup_x_block,
                                                                          update_state_block,
                                                                          finalize_block);
        /// replace statement block with solver block as it contains all statements
        ast::StatementVector solver_block_statements{
            std::make_shared<ast::ExpressionStatement>(solver_block)};
        block_with_expression_statements->set_statements(std::move(solver_block_statements));
    } else {
        /// create eigen newton solver block
        auto setup_x_block = std::make_shared<ast::StatementBlock>(std::move(setup_x_statements));
        auto functor_block = std::make_shared<ast::StatementBlock>(std::move(functor_statements));
        auto solver_block = std::make_shared<ast::EigenNewtonSolverBlock>(n_state_vars,
                                                                          variable_block,
                                                                          initialize_block,
                                                                          setup_x_block,
                                                                          functor_block,
                                                                          update_state_block,
                                                                          finalize_block);
        /// replace statement block with solver block as it contains all statements
        ast::StatementVector solver_block_statements{
            std::make_shared<ast::ExpressionStatement>(solver_block)};
        block_with_expression_statements->set_statements(std::move(solver_block_statements));
    }
}


void SympySolverVisitor::solve_linear_system(const std::vector<std::string>& pre_solve_statements) {
    // construct ordered vector of state vars used in linear system
    init_state_vars_vector();
    // call sympy linear solver
    bool small_system = (eq_system.size() <= SMALL_LINEAR_SYSTEM_MAX_STATES);
    auto solver = pywrap::EmbeddedPythonLoader::get_instance().api()->create_sls_executor();
    solver->eq_system = eq_system;
    solver->state_vars = state_vars;
    solver->vars = vars;
    solver->small_system = small_system;
    solver->elimination = elimination;
    // this is necessary after we destroy the solver
    const auto tmp_unique_prefix = suffix_random_string(vars, "tmp");
    solver->tmp_unique_prefix = tmp_unique_prefix;
    solver->function_calls = function_calls;
    (*solver)();
    // returns a vector of solutions, i.e. new statements to add to block:
    auto solutions = solver->solutions;
    // and a vector of new local variables that need to be declared in the block:
    auto new_local_vars = solver->new_local_vars;
    // may also return a python exception message:
    auto exception_message = solver->exception_message;
    // destroy solver
    pywrap::EmbeddedPythonLoader::get_instance().api()->destroy_sls_executor(solver);
    if (!exception_message.empty()) {
        logger->warn("SympySolverVisitor :: solve_lin_system python exception: " +
                     exception_message);
        return;
    }
    // find out where to insert solutions in statement block
    const auto& statements = block_with_expression_statements->get_statements();
    auto it = get_solution_location_iterator(statements);
    if (small_system) {
        // for small number of state vars, linear solver
        // directly returns solution by solving symbolically at compile time
        logger->debug("SympySolverVisitor :: Solving *small* linear system of eqs");
        // declare new local vars
        if (!new_local_vars.empty()) {
            for (const auto& new_local_var: new_local_vars) {
                logger->debug("SympySolverVisitor :: -> declaring new local variable: {}",
                              new_local_var);
                add_local_variable(*block_with_expression_statements, new_local_var);
            }
        }
        visitor::SympyReplaceSolutionsVisitor solution_replacer(
            pre_solve_statements,
            solutions,
            expression_statements,
            visitor::SympyReplaceSolutionsVisitor::ReplacePolicy::VALUE,
            1,
            tmp_unique_prefix);
        solution_replacer.visit_statement_block(*block_with_expression_statements);
    } else {
        // otherwise it returns a linear matrix system to solve
        logger->debug("SympySolverVisitor :: Constructing linear newton solve block");
        construct_eigen_solver_block(pre_solve_statements, solutions, true);
    }
}

void SympySolverVisitor::solve_non_linear_system(
    const std::vector<std::string>& pre_solve_statements) {
    // construct ordered vector of state vars used in non-linear system
    init_state_vars_vector();
    // call sympy non-linear solver

    auto solver = pywrap::EmbeddedPythonLoader::get_instance().api()->create_nsls_executor();
    solver->eq_system = eq_system;
    solver->state_vars = state_vars;
    solver->vars = vars;
    solver->function_calls = function_calls;
    (*solver)();
    // returns a vector of solutions, i.e. new statements to add to block:
    auto solutions = solver->solutions;
    // may also return a python exception message:
    auto exception_message = solver->exception_message;
    pywrap::EmbeddedPythonLoader::get_instance().api()->destroy_nsls_executor(solver);
    if (!exception_message.empty()) {
        logger->warn("SympySolverVisitor :: solve_non_lin_system python exception: " +
                     exception_message);
        return;
    }
    logger->debug("SympySolverVisitor :: Constructing eigen newton solve block");
    construct_eigen_solver_block(pre_solve_statements, solutions, false);
}

void SympySolverVisitor::visit_var_name(ast::VarName& node) {
    if (collect_state_vars) {
        std::string var_name = node.get_node_name();
        if (node.get_name()->is_indexed_name()) {
            auto index_name = std::dynamic_pointer_cast<ast::IndexedName>(node.get_name());
            var_name +=
                "[" +
                std::to_string(
                    std::dynamic_pointer_cast<ast::Integer>(index_name->get_length())->eval()) +
                "]";
        }
        // if var_name is a state var, add it to set
        if (std::find(all_state_vars.cbegin(), all_state_vars.cend(), var_name) !=
            all_state_vars.cend()) {
            logger->debug("SympySolverVisitor :: adding state var: {}", var_name);
            state_vars_in_block.insert(var_name);
        }
    }
}

void SympySolverVisitor::visit_diff_eq_expression(ast::DiffEqExpression& node) {
    const auto& lhs = node.get_expression()->get_lhs();
    const auto& rhs = node.get_expression()->get_rhs();

    if (!lhs->is_var_name()) {
        logger->warn("SympySolverVisitor :: LHS of differential equation is not a VariableName");
        return;
    }
    auto lhs_name = std::dynamic_pointer_cast<ast::VarName>(lhs)->get_name();
    if ((lhs_name->is_indexed_name() &&
         !std::dynamic_pointer_cast<ast::IndexedName>(lhs_name)->get_name()->is_prime_name()) ||
        (!lhs_name->is_indexed_name() && !lhs_name->is_prime_name())) {
        logger->warn("SympySolverVisitor :: LHS of differential equation is not a PrimeName");
        return;
    }

    check_expr_statements_in_same_block();

    const auto node_as_nmodl = to_nmodl_for_sympy(node);
    auto diffeq_solver = pywrap::EmbeddedPythonLoader::get_instance().api()->create_des_executor();
    diffeq_solver->node_as_nmodl = node_as_nmodl;
    diffeq_solver->dt_var = codegen::naming::NTHREAD_DT_VARIABLE;
    diffeq_solver->vars = vars;
    diffeq_solver->use_pade_approx = use_pade_approx;
    diffeq_solver->function_calls = function_calls;
    diffeq_solver->method = solve_method;
    (*diffeq_solver)();
    if (solve_method == codegen::naming::EULER_METHOD) {
        // replace x' = f(x) differential equation
        // with forwards Euler timestep:
        // x = x + f(x) * dt
        logger->debug("SympySolverVisitor :: EULER - solving: {}", node_as_nmodl);
    } else if (solve_method == codegen::naming::CNEXP_METHOD) {
        // replace x' = f(x) differential equation
        // with analytic solution for x(t+dt) in terms of x(t)
        // x = ...
        logger->debug("SympySolverVisitor :: CNEXP - solving: {}", node_as_nmodl);
    } else {
        // for other solver methods: just collect the ODEs & return
        std::string eq_str = to_nmodl_for_sympy(node);
        std::string var_name = lhs_name->get_node_name();
        if (lhs_name->is_indexed_name()) {
            auto index_name = std::dynamic_pointer_cast<ast::IndexedName>(lhs_name);
            var_name +=
                "[" +
                std::to_string(
                    std::dynamic_pointer_cast<ast::Integer>(index_name->get_length())->eval()) +
                "]";
        }
        logger->debug("SympySolverVisitor :: adding ODE system: {}", eq_str);
        eq_system.push_back(eq_str);
        logger->debug("SympySolverVisitor :: adding state var: {}", var_name);
        state_vars_in_block.insert(var_name);
        expression_statements.insert(current_expression_statement);
        last_expression_statement = current_expression_statement;
        return;
    }

    // replace ODE with solution in AST
    auto solution = diffeq_solver->solution;
    logger->debug("SympySolverVisitor :: -> solution: {}", solution);

    auto exception_message = diffeq_solver->exception_message;
    pywrap::EmbeddedPythonLoader::get_instance().api()->destroy_des_executor(diffeq_solver);
    if (!exception_message.empty()) {
        logger->warn("SympySolverVisitor :: python exception: " + exception_message);
        return;
    }

    if (!solution.empty()) {
        replace_diffeq_expression(node, solution);
    } else {
        logger->warn("SympySolverVisitor :: solution to differential equation not possible");
    }
}

void SympySolverVisitor::visit_conserve(ast::Conserve& node) {
    // Replace ODE for state variable on LHS of CONSERVE statement with
    // algebraic expression on RHS (see p244 of NEURON book)
    logger->debug("SympySolverVisitor :: CONSERVE statement: {}", to_nmodl(node));
    expression_statements.insert(&node);
    std::string conserve_equation_statevar;
    if (node.get_react()->is_react_var_name()) {
        conserve_equation_statevar = node.get_react()->get_node_name();
    }
    if (std::find(all_state_vars.cbegin(), all_state_vars.cend(), conserve_equation_statevar) ==
        all_state_vars.cend()) {
        logger->error(
            "SympySolverVisitor :: Invalid CONSERVE statement for DERIVATIVE block, LHS should be "
            "a state variable, instead found: {}. Ignoring CONSERVE statement",
            to_nmodl(node.get_react()));
        return;
    }
    auto conserve_equation_str = to_nmodl_for_sympy(*node.get_expr());
    logger->debug("SympySolverVisitor :: --> replace ODE for state var {} with equation {}",
                  conserve_equation_statevar,
                  conserve_equation_str);
    conserve_equation[conserve_equation_statevar] = conserve_equation_str;
}

void SympySolverVisitor::visit_derivative_block(ast::DerivativeBlock& node) {
    /// clear information from previous block, get global vars + block local vars
    init_block_data(&node);

    // get user specified solve method for this block
    solve_method = derivative_block_solve_method[node.get_node_name()];

    // visit each differential equation:
    //  - for CNEXP or EULER, each equation is independent & is replaced with its solution
    //  - otherwise, each equation is added to eq_system
    node.visit_children(*this);

    if (eq_system_is_valid && !eq_system.empty()) {
        // solve system of ODEs in eq_system
        logger->debug("SympySolverVisitor :: Solving {} system of ODEs", solve_method);

        // construct implicit Euler equations from ODEs
        std::vector<std::string> pre_solve_statements;
        for (auto& eq: eq_system) {
            auto split_eq = stringutils::split_string(eq, '=');
            auto x_prime_split = stringutils::split_string(split_eq[0], '\'');
            auto x = stringutils::trim(x_prime_split[0]);
            std::string x_array_index = "";
            std::string x_array_index_i = "";
            if (x_prime_split.size() > 1 && stringutils::trim(x_prime_split[1]).size() > 2) {
                x_array_index = stringutils::trim(x_prime_split[1]);
                x_array_index_i = "_" + x_array_index.substr(1, x_array_index.size() - 2);
            }
            std::string state_var_name = x + x_array_index;
            auto var_eq_pair = conserve_equation.find(state_var_name);
            if (var_eq_pair != conserve_equation.cend()) {
                // replace the ODE for this state var with corresponding CONSERVE equation
                eq = state_var_name + " = " + var_eq_pair->second;
                logger->debug(
                    "SympySolverVisitor :: -> instead of Euler eq using CONSERVE equation: {} = {}",
                    state_var_name,
                    var_eq_pair->second);
            } else {
                // no CONSERVE equation, construct Euler equation
                auto dxdt = stringutils::trim(split_eq[1]);


                const auto old_x = suffix_random_string(vars, "old_" + x + x_array_index_i);
                // declare old_x
                logger->debug("SympySolverVisitor :: -> declaring new local variable: {}", old_x);
                add_local_variable(*block_with_expression_statements, old_x);
                // assign old_x = x
                pre_solve_statements.push_back(old_x + " = " + x + x_array_index);
                // replace ODE with Euler equation
                eq = x + x_array_index + " = " + old_x + " + " +
                     codegen::naming::NTHREAD_DT_VARIABLE + " * (" + dxdt + ")";
                logger->debug("SympySolverVisitor :: -> constructed Euler eq: {}", eq);
            }
        }

        if (solve_method == codegen::naming::SPARSE_METHOD) {
            solve_linear_system(pre_solve_statements);
        } else if (solve_method == codegen::naming::DERIVIMPLICIT_METHOD) {
            solve_non_linear_system(pre_solve_statements);
        } else {
            logger->error("SympySolverVisitor :: Solve method {} not supported", solve_method);
        }
    }
}

void SympySolverVisitor::visit_lin_equation(ast::LinEquation& node) {
    check_expr_statements_in_same_block();
    std::string lin_eq = to_nmodl_for_sympy(*node.get_left_linxpression());
    lin_eq += " = ";
    lin_eq += to_nmodl_for_sympy(*node.get_linxpression());
    eq_system.push_back(lin_eq);
    expression_statements.insert(current_expression_statement);
    last_expression_statement = current_expression_statement;
    logger->debug("SympySolverVisitor :: adding linear eq: {}", lin_eq);
    collect_state_vars = true;
    node.visit_children(*this);
    collect_state_vars = false;
}

void SympySolverVisitor::visit_linear_block(ast::LinearBlock& node) {
    logger->debug("SympySolverVisitor :: found LINEAR block: {}", node.get_node_name());

    /// clear information from previous block, get global vars + block local vars
    init_block_data(&node);

    // collect linear equations
    node.visit_children(*this);

    if (eq_system_is_valid && !eq_system.empty()) {
        solve_linear_system();
    }
}

void SympySolverVisitor::visit_non_lin_equation(ast::NonLinEquation& node) {
    check_expr_statements_in_same_block();
    std::string non_lin_eq = to_nmodl_for_sympy(*node.get_lhs());
    non_lin_eq += " = ";
    non_lin_eq += to_nmodl_for_sympy(*node.get_rhs());
    eq_system.push_back(non_lin_eq);
    expression_statements.insert(current_expression_statement);
    last_expression_statement = current_expression_statement;
    logger->debug("SympySolverVisitor :: adding non-linear eq: {}", non_lin_eq);
    collect_state_vars = true;
    node.visit_children(*this);
    collect_state_vars = false;
}

void SympySolverVisitor::visit_non_linear_block(ast::NonLinearBlock& node) {
    logger->debug("SympySolverVisitor :: found NONLINEAR block: {}", node.get_node_name());

    /// clear information from previous block, get global vars + block local vars
    init_block_data(&node);

    // collect non-linear equations
    node.visit_children(*this);

    if (eq_system_is_valid && !eq_system.empty()) {
        solve_non_linear_system();
    }
}

void SympySolverVisitor::visit_expression_statement(ast::ExpressionStatement& node) {
    auto prev_expression_statement = current_expression_statement;
    current_expression_statement = &node;
    node.visit_children(*this);
    current_expression_statement = prev_expression_statement;
}

void SympySolverVisitor::visit_statement_block(ast::StatementBlock& node) {
    auto prev_statement_block = current_statement_block;
    current_statement_block = &node;
    node.visit_children(*this);
    current_statement_block = prev_statement_block;
}

void SympySolverVisitor::visit_program(ast::Program& node) {
    derivative_block_solve_method.clear();

    global_vars = get_global_vars(node);

    // get list of solve statements with names & methods
    const auto& solve_block_nodes = collect_nodes(node, {ast::AstNodeType::SOLVE_BLOCK});
    for (const auto& block: solve_block_nodes) {
        if (auto block_ptr = std::dynamic_pointer_cast<const ast::SolveBlock>(block)) {
            const auto& block_name = block_ptr->get_block_name()->get_value()->eval();
            if (block_ptr->get_method()) {
                // Note: solve method name is an optional parameter
                // LINEAR and NONLINEAR blocks do not have solve method specified
                const auto& solve_method = block_ptr->get_method()->get_value()->eval();
                logger->debug("SympySolverVisitor :: Found SOLVE statement: using {} for {}",
                              solve_method,
                              block_name);
                derivative_block_solve_method[block_name] = solve_method;
            }
        }
    }

    // get set of all state vars
    all_state_vars.clear();
    if (auto symtab = node.get_symbol_table()) {
        auto statevars = symtab->get_variables_with_properties(NmodlType::state_var);
        for (const auto& v: statevars) {
            std::string var_name = v->get_name();
            if (v->is_array()) {
                for (int i = 0; i < v->get_length(); ++i) {
                    std::string var_name_i = var_name + "[" + std::to_string(i) + "]";
                    all_state_vars.push_back(var_name_i);
                }
            } else {
                all_state_vars.push_back(var_name);
            }
        }
    }

    node.visit_children(*this);
}

}  // namespace visitor
}  // namespace nmodl
