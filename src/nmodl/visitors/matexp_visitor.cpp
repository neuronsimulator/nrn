/*
 * Copyright 2025 David McDougall
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "visitors/matexp_visitor.hpp"

#include "ast/all.hpp"
#include "codegen/codegen_naming.hpp"
#include "utils/logger.hpp"
#include "visitors/visitor_utils.hpp"

#include <algorithm>

namespace nmodl {
namespace visitor {


template <typename T>
static bool vector_contains(const std::vector<T>& vec, const T& value) {
    return std::find(vec.begin(), vec.end(), value) != vec.end();
}


void MatexpVisitor::visit_program(ast::Program& node) {
    // Make lists of all KineticBlock's and SolveBlock's in the program
    node.visit_children(*this);

    states = node.get_symbol_table()->get_variables(symtab::syminfo::NmodlType::state_var);

    // Replace solve-steadystate-matexp statements with their MatexpBlock solution
    for (const auto& solve_block: steadystate_blocks) {
        replace_solve_block(*solve_block, true);
    }

    // Remove solve-method-matexp statements and append their MatexpBlock
    // solution to the end of the file.
    for (const auto& solve_block: solve_blocks) {
        node.emplace_back_node(remove_solve_block(*solve_block, false));
    }

    // Remove solved KINETIC blocks
    const auto& blocks = node.get_blocks();
    for (auto iter = blocks.begin(); iter != blocks.end(); iter++) {
        if ((*iter)->is_kinetic_block()) {
            const auto kinetic_block = std::dynamic_pointer_cast<ast::KineticBlock>(*iter);
            const auto& block_name = kinetic_block->get_name()->get_node_name();
            bool keep = vector_contains(keep_blocks, block_name);
            if (!keep) {
                node.erase_node(iter--);
                logger->debug("MatexpVisitor :: Removing solved KINETIC block \"{}\"", block_name);
            }
        }
    }
}


/// Populate the lists of solve-block statements in the program
void MatexpVisitor::visit_solve_block(ast::SolveBlock& node) {
    // Find solve statements that use the matexp solver method
    const auto& solve_method = node.get_method();
    const auto& steadystate_method = node.get_steadystate();
    const auto& matexp_method = codegen::naming::MATEXP_METHOD;
    const auto is_method_matexp = [](auto method) {
        return method && method->get_value()->eval() == matexp_method;
    };
    const bool solve = is_method_matexp(solve_method);
    const bool steadystate = is_method_matexp(steadystate_method);
    // Save the block for later reference
    if (solve) {
        solve_blocks.push_back(&node);
    } else if (steadystate) {
        steadystate_blocks.push_back(&node);
    } else {
        keep_blocks.push_back(node.get_block_name()->get_node_name());
    }
}


/// Populate the list of kinetic blocks in the program
void MatexpVisitor::visit_kinetic_block(ast::KineticBlock& node) {
    kinetic_blocks.push_back(&node);
}


// Helper class for finding, processing, and removing CONSERVE statements
class ExtractConserveVisitor: public AstVisitor {
  public:
    std::shared_ptr<ast::Expression> conserve_sum;
    std::vector<std::shared_ptr<symtab::Symbol>> states;

    explicit ExtractConserveVisitor(std::vector<std::shared_ptr<symtab::Symbol>>& states) {
        this->states = states;
        this->conserve_sum = nullptr;
    }
    void visit_statement_block(ast::StatementBlock& node) override {
        node.visit_children(*this);
        const auto& statements = node.get_statements();
        for (auto iter = statements.begin(); iter != statements.end(); iter++) {
            if ((*iter)->is_conserve()) {
                node.erase_statement(iter--);
            }
        }
    }
    void visit_conserve(ast::Conserve& node) override {
        if (conserve_sum) {
            logger->error("MatexpVisitor :: Error : multiple CONSERVE statements unsupported");
            return;
        }
        // Make a sorted list of state names
        std::vector<std::string> state_names;
        for (const auto& state: states) {
            state_names.push_back(state->get_name());
        }
        std::sort(state_names.begin(), state_names.end());
        // Unpack the conserve statement
        const auto expr = node.get_expr();
        const auto react = node.get_react();
        const auto vars = collect_nodes(*react,
                                        {
                                            ast::AstNodeType::NAME,
                                        });
        const bool primes = node_exists(*react,
                                        {
                                            ast::AstNodeType::PRIME_NAME,
                                        });
        // Check that the conserved value is the sum of the state variables
        std::vector<std::string> var_names;
        for (const auto& var: vars) {
            var_names.push_back(to_nmodl(var));
        }
        std::sort(var_names.begin(), var_names.end());
        if (var_names != state_names || primes) {
            logger->error("MatexpVisitor :: Error : Ignoring irregular CONSERVE statement");
            return;
        }
        conserve_sum = expr;
    }
};


// Replace the given solve statement with a MatexpBlock
void MatexpVisitor::replace_solve_block(const ast::SolveBlock& node, bool steadystate) {
    const auto& name = node.get_block_name()->get_node_name();
    const auto& block = find_kinetic_block(name);
    const auto& solution = solve_kinetic_block(*block, steadystate);
    ast::Ast* parent = node.get_parent();
    if (!parent->is_expression_statement()) {
        throw std::runtime_error("broken ast");
    }
    ((ast::ExpressionStatement*) parent)->set_expression(solution);
}


// Remove the given solve-block statement and return the MatexpBlock solution
std::shared_ptr<ast::MatexpBlock> MatexpVisitor::remove_solve_block(const ast::SolveBlock& node,
                                                                    bool steadystate) {
    const auto& name = node.get_block_name()->get_node_name();
    const auto& block = find_kinetic_block(name);
    const auto& solution = solve_kinetic_block(*block, steadystate);
    ast::Ast* parent = node.get_parent();
    if (!parent->is_expression_statement()) {
        throw std::runtime_error("broken ast");
    }
    if (!parent->get_parent()->is_statement_block()) {
        throw std::runtime_error("broken ast");
    }
    const auto statement_block = (ast::StatementBlock*) parent->get_parent();
    auto statements = statement_block->get_statements();
    for (auto iter = statements.begin(); iter != statements.end(); iter++) {
        if (iter->get() == parent) {
            statements.erase(iter--);
            statement_block->set_statements(statements);
            return solution;
        }
    }
    throw std::runtime_error("broken ast");
}


// Search the kinetic_blocks vector for the given block
ast::KineticBlock* MatexpVisitor::find_kinetic_block(const std::string& block_name) {
    for (const auto& block: kinetic_blocks) {
        if (block->get_node_name() == block_name) {
            return block;
        }
    }
    throw std::runtime_error("cannot find the block '{" + block_name + "}' to solve it");
}


// Convert a KineticBlock into a MatexpBlock
std::shared_ptr<ast::MatexpBlock> MatexpVisitor::solve_kinetic_block(const ast::KineticBlock& node,
                                                                     bool steadystate) {
    // Make a copy of the statement block, do not modify original.
    const auto& jacobian_block = std::make_shared<ast::StatementBlock>(*node.get_statement_block());
    // Convert the reaction statements into assignments to the Jacobian matrix
    in_jacobian_block = true;
    this->visit_statement_block(*jacobian_block);
    in_jacobian_block = false;

    ExtractConserveVisitor conserve_visitor(states);
    conserve_visitor.visit_statement_block(*jacobian_block);

    return std::make_shared<ast::MatexpBlock>(std::make_shared<ast::Boolean>(steadystate),
                                              jacobian_block,
                                              conserve_visitor.conserve_sum);
}


void MatexpVisitor::visit_statement_block(ast::StatementBlock& node) {
    // Iterate through the children using indices instead of pointers,
    // so that more statements can be added to the list
    for (int index = 0; index < node.get_statements().size(); index++) {
        node.get_statements()[index]->accept(*this);
    }
}


static void nonlinear_reaction_error() {
    logger->error("MatexpVisitor :: Error : Non-linear equation detected");
    throw std::invalid_argument("Non-linear equation detected");
}


// Argument node is a reactant or product expression
static std::string get_state_var_name(const std::shared_ptr<ast::Ast>& node) {
    const auto reaction_var_name = std::dynamic_pointer_cast<ast::ReactVarName>(node);
    if (!reaction_var_name) {
        nonlinear_reaction_error();
    }
    const auto coefficient = reaction_var_name->get_value();
    if (coefficient && coefficient->eval() != 1) {
        nonlinear_reaction_error();
    }
    return reaction_var_name->get_node_name();
}


// Returns an index into the "state" vector
int MatexpVisitor::get_state_index(const std::string& state_name) {
    for (int index = 0; index < states.size(); index++) {
        if (state_name == states[index]->get_name()) {
            return index;
        }
    }
    throw std::invalid_argument("no such state");
}


// Get the index of a statement in a statement-vector
static int find_node(const nmodl::ast::StatementVector& statements, const nmodl::ast::Node* node) {
    for (int index = 0; index < statements.size(); index++) {
        const nmodl::ast::Node* cursor = statements[index].get();
        if (cursor == node) {
            return index;
        }
    }
    throw std::runtime_error("broken ast");
}


// Convert a decay reaction statement "->" into equivalent assignments to the Jacobian matrix
std::shared_ptr<ast::Statement> MatexpVisitor::transform_decay_statement(
    std::shared_ptr<ast::Expression> lhs,
    std::shared_ptr<ast::Expression> kf) {
    const auto lhs_name = get_state_var_name(lhs);
    const auto lhs_idx = get_state_index(lhs_name);
    // Calculate the Jacobian matrix indices
    const int jf_src_idx = lhs_idx + states.size() * lhs_idx;
    // Write NMODL to assign to the Jacobian matrix
    const std::string jf_src = "nmodl_eigen_j[" + std::to_string(jf_src_idx) + "]";
    const std::string kf_nmodl = to_nmodl(kf);
    return create_statement(jf_src + " = " + jf_src + " - (" + kf_nmodl + ") * dt");
}


// Convert a reaction statement "<->" into equivalent assignments to the Jacobian matrix
std::vector<std::shared_ptr<ast::Statement>> MatexpVisitor::transform_reaction_statement(
    std::shared_ptr<ast::Expression> lhs,
    std::shared_ptr<ast::Expression> rhs,
    std::shared_ptr<ast::Expression> kf,
    std::shared_ptr<ast::Expression> kb) {
    // Find the state vector indices
    const auto lhs_name = get_state_var_name(lhs);
    const auto rhs_name = get_state_var_name(rhs);
    const auto lhs_idx = get_state_index(lhs_name);
    const auto rhs_idx = get_state_index(rhs_name);
    // Calculate the Jacobian matrix indices
    const int jf_src_idx = lhs_idx + states.size() * lhs_idx;
    const int jf_dst_idx = lhs_idx * states.size() + rhs_idx;
    const int jb_src_idx = rhs_idx * states.size() + rhs_idx;
    const int jb_dst_idx = lhs_idx + states.size() * rhs_idx;
    // Create four new statements assigning to the Jacobian matrix
    const std::string jf_src = "nmodl_eigen_j[" + std::to_string(jf_src_idx) + "]";
    const std::string jf_dst = "nmodl_eigen_j[" + std::to_string(jf_dst_idx) + "]";
    const std::string jb_src = "nmodl_eigen_j[" + std::to_string(jb_src_idx) + "]";
    const std::string jb_dst = "nmodl_eigen_j[" + std::to_string(jb_dst_idx) + "]";
    const std::string kf_nmodl = to_nmodl(kf);
    const std::string kb_nmodl = to_nmodl(kb);
    const std::string jf_n_string = jf_src + " = " + jf_src + " - (" + kf_nmodl + ") * dt";
    const std::string jf_p_string = jf_dst + " = " + jf_dst + " + (" + kf_nmodl + ") * dt";
    const std::string jb_n_string = jb_src + " = " + jb_src + " - (" + kb_nmodl + ") * dt";
    const std::string jb_p_string = jb_dst + " = " + jb_dst + " + (" + kb_nmodl + ") * dt";
    const auto& jf_n = create_statement(jf_n_string);
    const auto& jf_p = create_statement(jf_p_string);
    const auto& jb_n = create_statement(jb_n_string);
    const auto& jb_p = create_statement(jb_p_string);
    return {jf_n, jf_p, jb_n, jb_p};
}


// Visit reaction statements inside of kinetic blocks which we are actively solving
void MatexpVisitor::visit_reaction_statement(ast::ReactionStatement& node) {
    if (!in_jacobian_block) {
        return;
    }
    // Unpack the reaction data
    const auto& op = node.get_op().get_value();
    const auto& lhs = node.get_reaction1();
    const auto& rhs = node.get_reaction2();
    const auto& kf = node.get_expression1();  // forwards reaction rate
    const auto& kb = node.get_expression2();  // backwards reaction rate
    // Get the parent statement block
    if (!node.get_parent()->is_statement_block()) {
        throw std::runtime_error("broken ast");
    }
    const auto statement_block = (ast::StatementBlock*) node.get_parent();
    const auto& statements = statement_block->get_statements();
    // Find and remove this reaction statement
    int statement_index = find_node(statements, &node);
    statement_block->erase_statement(std::begin(statements) + statement_index);
    // Check for invalid kinetic models
    if (op == ast::ReactionOp::LTLT) {
        nonlinear_reaction_error();
    }
    // Replace reaction statements with assignments to the Jaconbian
    else if (op == ast::ReactionOp::MINUSGT) {
        const auto jf_n = transform_decay_statement(lhs, kf);
        statement_block->insert_statement(std::begin(statements) + statement_index, jf_n);
    } else if (op == ast::ReactionOp::LTMINUSGT) {
        for (const auto& stmt: transform_reaction_statement(lhs, rhs, kf, kb)) {
            statement_block->insert_statement(std::begin(statements) + statement_index++, stmt);
        }
    }
}

}  // namespace visitor
}  // namespace nmodl
