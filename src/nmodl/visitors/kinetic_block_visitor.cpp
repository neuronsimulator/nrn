/*************************************************************************
 * Copyright (C) 2018-2019 Blue Brain Project
 *
 * This file is part of NMODL distributed under the terms of the GNU
 * Lesser General Public License. See top-level LICENSE file for details.
 *************************************************************************/

#include <iostream>

#include "fmt/format.h"
#include "kinetic_block_visitor.hpp"
#include "symtab/symbol.hpp"
#include "utils/logger.hpp"
#include "utils/string_utils.hpp"
#include "visitor_utils.hpp"


namespace nmodl {
namespace visitor {

using symtab::syminfo::NmodlType;
using namespace fmt::literals;

void KineticBlockVisitor::process_reac_var(const std::string& varname, int count) {
    // lookup index of state var
    const auto it = state_var_index.find(varname);
    if (it == state_var_index.cend()) {
        // not a state var
        // so this is a constant variable in the reaction statement
        // this should be included in the fluxes:
        if (in_reaction_statement_lhs) {
            non_state_var_fflux[i_statement] = varname;
            logger->debug("KineticBlockVisitor :: adding non-state fflux[{}] \"{}\"",
                          i_statement,
                          varname);
        } else {
            non_state_var_bflux[i_statement] = varname;
            logger->debug("KineticBlockVisitor :: adding non-state bflux[{}] \"{}\"",
                          i_statement,
                          varname);
        }
        // but as it is not a state var, no ODE should be generated for it, i.e. no nu_L or nu_R
        // entry.
    } else {
        // found state var index
        int i_statevar = it->second;
        if (in_reaction_statement_lhs) {
            // set element of nu_L matrix
            rate_eqs.nu_L[i_statement][i_statevar] += count;
            logger->debug("KineticBlockVisitor :: nu_L[{}][{}] += {}",
                          i_statement,
                          i_statevar,
                          count);
        } else {
            // set element of nu_R matrix
            rate_eqs.nu_R[i_statement][i_statevar] += count;
            logger->debug("KineticBlockVisitor :: nu_R[{}][{}] += {}",
                          i_statement,
                          i_statevar,
                          count);
        }
    }
}

void KineticBlockVisitor::visit_conserve(ast::Conserve* node) {
    // NOTE! CONSERVE statement "implicitly takes into account COMPARTMENT factors on LHS"
    // see p244 of NEURON book
    // need to ensure that we do this in the same way: presumably need
    // to either multiply or divide state vars on LHS of conserve statement by their COMPARTMENT
    // factors?
    logger->debug("KineticBlockVisitor :: CONSERVE statement ignored (for now): {} = {}",
                  to_nmodl(node->get_react().get()),
                  to_nmodl(node->get_expr().get()));
    statements_to_remove.insert(node);
}

void KineticBlockVisitor::visit_compartment(ast::Compartment* node) {
    // COMPARTMENT block has an expression, and a list of state vars it applies to.
    // For each state var, the rhs of the differential eq should be divided by the expression.
    // Here we store the expressions in the compartment_factors vector
    auto expr = node->get_expression();
    std::string expression = to_nmodl(expr.get());
    logger->debug("KineticBlockVisitor :: COMPARTMENT expr: {}", expression);
    for (const auto name_ptr: node->get_names()) {
        std::string var_name = name_ptr->get_node_name();
        const auto it = state_var_index.find(var_name);
        if (it != state_var_index.cend()) {
            int var_index = it->second;
            compartment_factors[var_index] = expression;
            logger->debug(
                "KineticBlockVisitor :: COMPARTMENT factor {} for state var {} (index {})",
                expression,
                var_name,
                var_index);
        }
    }
    // add COMPARTMENT state to list of statements to remove
    // since we don't want this statement to be present in the final DERIVATIVE block
    statements_to_remove.insert(node);
}

void KineticBlockVisitor::visit_reaction_operator(ast::ReactionOperator* node) {
    auto reaction_op = node->get_value();
    if (reaction_op == ast::ReactionOp::LTMINUSGT) {
        // <->
        // reversible reaction
        // we go from visiting the lhs to visiting the rhs of the reaction statement
        in_reaction_statement_lhs = false;
    }
}

void KineticBlockVisitor::visit_react_var_name(ast::ReactVarName* node) {
    // react_var_name node contains var_name and integer
    // var_name is the state variable which we convert to an index
    // integer is the value to be added to the stochiometric matrix at this index
    if (in_reaction_statement) {
        auto varname = to_nmodl(node->get_name().get());
        int count = node->get_value() ? node->get_value()->eval() : 1;
        process_reac_var(varname, count);
    }
}

void KineticBlockVisitor::visit_reaction_statement(ast::ReactionStatement* node) {
    statements_to_remove.insert(node);

    auto reaction_op = node->get_op().get_value();
    // special case for << statements
    if (reaction_op == ast::ReactionOp::LTLT) {
        logger->debug("KineticBlockVisitor :: '<<' reaction statement: {}", to_nmodl(node));
        // statements involving the "<<" operator
        // must have a single state var on lhs
        // and a single expression on rhs that corresponds to d{state var}/dt
        // So if x is a state var, then
        // ~ x << (a*b)
        // translates to the ODE contribution x' += a*b
        auto lhs = node->get_reaction1();

        /// check if reaction statement is a single state variable
        bool single_state_var = true;
        if (lhs->is_react_var_name()) {
            auto value = std::dynamic_pointer_cast<ast::ReactVarName>(lhs)->get_value();
            if (value && (value->eval() != 1)) {
                single_state_var = false;
            }
        }
        if (!lhs->is_react_var_name() || !single_state_var) {
            logger->warn(
                "KineticBlockVisitor :: LHS of \"<<\" reaction statement must be a single state "
                "var, but instead found {}: ignoring this statement",
                to_nmodl(lhs.get()));
            return;
        }
        auto rhs = node->get_expression1();
        std::string varname = to_nmodl(lhs.get());
        // get index of state var
        const auto it = state_var_index.find(varname);
        if (it != state_var_index.cend()) {
            int var_index = it->second;
            std::string expr = to_nmodl(rhs.get());
            if (!additive_terms[var_index].empty()) {
                additive_terms[var_index] += " + ";
            }
            // add to additive terms for this state var
            additive_terms[var_index] += "({})"_format(expr);
            logger->debug("KineticBlockVisitor :: '<<' reaction statement: {}' += {}",
                          varname,
                          expr);
        }
        return;
    }

    // forwards reaction rate
    auto kf = node->get_expression1();
    // backwards reaction rate
    auto kb = node->get_expression2();

    // add reaction rates to vectors kf, kb
    auto kf_str = to_nmodl(kf.get());
    logger->debug("KineticBlockVisitor :: k_f[{}] = {}", i_statement, kf_str);
    rate_eqs.k_f.emplace_back(kf_str);

    if (kb) {
        // kf is always defined, but for statements with operator "->" kb is not
        auto kb_str = to_nmodl(kb.get());
        logger->debug("KineticBlockVisitor :: k_b[{}] = {}", i_statement, kb_str);
        rate_eqs.k_b.emplace_back(kb_str);
    } else {
        rate_eqs.k_b.emplace_back();
    }

    // add empty non state var fluxes for this statement
    non_state_var_fflux.emplace_back();
    non_state_var_bflux.emplace_back();

    // add a row of zeros to the stochiometric matrices
    rate_eqs.nu_L.emplace_back(std::vector<int>(state_var_count, 0));
    rate_eqs.nu_R.emplace_back(std::vector<int>(state_var_count, 0));

    // visit each term in reaction statement and
    // add the corresponding integer to the new row in the matrix
    in_reaction_statement = true;
    in_reaction_statement_lhs = true;
    node->visit_children(this);
    in_reaction_statement = false;

    // generate fluxes
    modfile_fflux = rate_eqs.k_f.back();
    modfile_bflux = rate_eqs.k_b.back();

    // contribution from state vars
    for (int j = 0; j < state_var_count; ++j) {
        std::string multiply_var = std::string("*").append(state_var[j]);
        int nu_L = rate_eqs.nu_L[i_statement][j];
        while (nu_L-- > 0) {
            modfile_fflux += multiply_var;
        }
        int nu_R = rate_eqs.nu_R[i_statement][j];
        while (nu_R-- > 0) {
            modfile_bflux += multiply_var;
        }
    }
    // contribution from non-state vars
    if (!non_state_var_fflux[i_statement].empty()) {
        modfile_fflux += std::string("*").append(non_state_var_fflux[i_statement]);
    }
    if (!non_state_var_bflux[i_statement].empty()) {
        modfile_bflux += std::string("*").append(non_state_var_bflux[i_statement]);
    }
    fflux.emplace_back(modfile_fflux);
    bflux.emplace_back(modfile_bflux);

    // for substituting into modfile, empty flux should be 0
    if (modfile_fflux.empty()) {
        modfile_fflux = "0";
    }
    if (modfile_bflux.empty()) {
        modfile_bflux = "0";
    }

    logger->debug("KineticBlockVisitor :: fflux[{}] = {}", i_statement, fflux[i_statement]);
    logger->debug("KineticBlockVisitor :: bflux[{}] = {}", i_statement, bflux[i_statement]);

    // increment statement counter
    ++i_statement;
}

std::shared_ptr<ast::Expression> create_expr(const std::string& str_expr) {
    auto statement = create_statement("dummy = " + str_expr);
    auto expr = std::dynamic_pointer_cast<ast::ExpressionStatement>(statement)->get_expression();
    return std::dynamic_pointer_cast<ast::BinaryExpression>(expr)->get_rhs();
}

void KineticBlockVisitor::visit_wrapped_expression(ast::WrappedExpression* node) {
    // If a wrapped expression contains a variable with name "f_flux" or "b_flux",
    // this variable should be replaced by the expression for the corresponding flux
    // which depends on the previous reaction statement. The current expressions are
    // stored as strings in "modfile_fflux" and "modfile_bflux"
    if (node->get_expression()->is_name()) {
        auto var_name = std::dynamic_pointer_cast<ast::Name>(node->get_expression());
        if (var_name->get_node_name() == "f_flux") {
            auto expr = create_expr(modfile_fflux);
            logger->debug("KineticBlockVisitor :: replacing f_flux with {}", to_nmodl(expr.get()));
            node->set_expression(std::move(expr));
        } else if (var_name->get_node_name() == "b_flux") {
            auto expr = create_expr(modfile_bflux);
            logger->debug("KineticBlockVisitor :: replacing b_flux with {}", to_nmodl(expr.get()));
            node->set_expression(std::move(expr));
        }
    }
    node->visit_children(this);
}

void KineticBlockVisitor::visit_statement_block(ast::StatementBlock* node) {
    auto prev_statement_block = current_statement_block;
    current_statement_block = node;
    node->visit_children(this);
    // remove processed statements from current statement block
    remove_statements_from_block(current_statement_block, statements_to_remove);
    current_statement_block = prev_statement_block;
}

void KineticBlockVisitor::visit_kinetic_block(ast::KineticBlock* node) {
    rate_eqs.nu_L.clear();
    rate_eqs.nu_R.clear();
    rate_eqs.k_f.clear();
    rate_eqs.k_b.clear();
    fflux.clear();
    bflux.clear();
    odes.clear();
    modfile_fflux = "0";
    modfile_bflux = "0";

    // allocate these vectors with empty strings
    compartment_factors = std::vector<std::string>(state_var_count);
    additive_terms = std::vector<std::string>(state_var_count);
    i_statement = 0;

    // construct stoichiometric matrices and fluxes
    node->visit_children(this);

    // number of reaction statements
    int Ni = static_cast<int>(rate_eqs.k_f.size());

    // number of ODEs (= number of state vars)
    int Nj = state_var_count;

    // generate ODEs
    for (int j = 0; j < Nj; ++j) {
        // rhs of ODE eq
        std::string ode_rhs = additive_terms[j];
        for (int i = 0; i < Ni; ++i) {
            int delta_nu = rate_eqs.nu_R[i][j] - rate_eqs.nu_L[i][j];
            if (delta_nu != 0) {
                // if not the first RHS term, add + sign first
                if (!ode_rhs.empty()) {
                    ode_rhs += " + ";
                }
                if (bflux[i].empty()) {
                    ode_rhs += "({}*({}))"_format(delta_nu, fflux[i]);
                } else if (fflux[i].empty()) {
                    ode_rhs += "({}*(-{}))"_format(delta_nu, bflux[i]);
                } else {
                    ode_rhs += "({}*({}-{}))"_format(delta_nu, fflux[i], bflux[i]);
                }
            }
        }
        // divide by COMPARTMENT factor if present
        if (!compartment_factors[j].empty()) {
            ode_rhs = "({})/({})"_format(ode_rhs, compartment_factors[j]);
        }
        // if rhs of ODE is not empty, add to list of ODEs
        if (!ode_rhs.empty()) {
            auto state_var_split = stringutils::split_string(state_var[j], '[');
            std::string var_str = state_var_split[0];
            std::string index_str;
            if (state_var_split.size() > 1) {
                index_str = "[" + state_var_split[1];
            }
            odes.push_back("{}'{} = {}"_format(var_str, index_str, ode_rhs));
        }
    }

    for (const auto& ode: odes) {
        logger->debug("KineticBlockVisitor :: ode : {}", ode);
    }

    auto kinetic_statement_block = node->get_statement_block();
    // remove any remaining kinetic statements
    remove_statements_from_block(kinetic_statement_block.get(), statements_to_remove);
    // add new statements
    for (const auto& ode: odes) {
        logger->debug("KineticBlockVisitor :: -> adding statement: {}", ode);
        kinetic_statement_block->addStatement(create_statement(ode));
    }

    // store pointer to kinetic block
    kinetic_blocks.push_back(node);
}

void KineticBlockVisitor::visit_program(ast::Program* node) {
    statements_to_remove.clear();
    current_statement_block = nullptr;

    // get state variables - assign an index to each
    state_var_index.clear();
    state_var.clear();
    state_var_count = 0;
    if (auto symtab = node->get_symbol_table()) {
        auto statevars = symtab->get_variables_with_properties(NmodlType::state_var);
        for (const auto& v: statevars) {
            std::string var_name = v->get_name();
            if (v->is_array()) {
                // for array state vars we need to add each element of the array separately
                var_name += "[";
                for (int i = 0; i < v->get_length(); ++i) {
                    std::string var_name_i = var_name + std::to_string(i) + "]";
                    logger->debug("KineticBlockVisitor :: state_var_index[{}] = {}",
                                  var_name_i,
                                  state_var_count);
                    state_var_index[var_name_i] = state_var_count++;
                    state_var.push_back(var_name_i);
                }
            } else {
                logger->debug("KineticBlockVisitor :: state_var_index[{}] = {}",
                              var_name,
                              state_var_count);
                state_var_index[var_name] = state_var_count++;
                state_var.push_back(var_name);
            }
        }
    }

    // replace reaction statements within each kinetic block with equivalent ODEs
    node->visit_children(this);

    // change KINETIC blocks -> DERIVATIVE blocks
    auto blocks = node->get_blocks();
    for (auto* kinetic_block: kinetic_blocks) {
        for (auto it = blocks.begin(); it != blocks.end(); ++it) {
            if (it->get() == kinetic_block) {
                auto dblock = std::make_shared<ast::DerivativeBlock>(
                    std::move(kinetic_block->get_name()),
                    std::move(kinetic_block->get_statement_block()));
                ModToken tok{};
                dblock->set_token(tok);
                *it = dblock;
            }
        }
    }
    node->set_blocks(std::move(blocks));
}

}  // namespace visitor
}  // namespace nmodl
