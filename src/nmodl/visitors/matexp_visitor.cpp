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

namespace nmodl {
namespace visitor {


std::shared_ptr<ast::Name> mangle_name(const std::string& block_name, bool steadystate) {
    const std::string mangled_name = block_name + (steadystate ? "_steadystate" : "_matexp");
    return std::make_shared<ast::Name>(std::make_shared<ast::String>(mangled_name));
}


void nonlinear_reaction_error() {
    logger->error("MatexpVisitor :: Error : Non-linear equation detected");
    throw std::invalid_argument("Non-linear equation detected");
}


template <typename T>
bool vector_contains(const std::vector<T>& vec, const T& value) {
    return std::find(vec.begin(), vec.end(), value) != vec.end();
}


// Helper class for removing CONSERVE statements once they're no longer needed
class RemoveConserveVisitor: public AstVisitor {
  public:
    void visit_statement_block(ast::StatementBlock& node) override {
        node.visit_children(*this);
        const auto& statements = node.get_statements();
        for (auto iter = statements.begin(); iter != statements.end(); iter++) {
            if ((*iter)->is_conserve()) {
                node.erase_statement(iter--);
            }
        }
    }
};


void MatexpVisitor::visit_program(ast::Program& node) {
    states = node.get_symbol_table()->get_variables(symtab::syminfo::NmodlType::state_var);
    //
    node.visit_children(*this);
    //
    for (const auto& block: solved_blocks) {
        node.emplace_back_node(block);
    }
    // Include the matrix exponential function
    const auto& blocks = node.get_blocks();
    const auto& matexp_impl = std::make_shared<ast::Verbatim>(std::make_shared<ast::String>(
        "\n#undef g \n #undef F \n #undef t \n #include <unsupported/Eigen/MatrixFunctions>\n"));
    node.insert_node(blocks.begin(), matexp_impl);
    // Remove solved KINETIC blocks
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


void MatexpVisitor::visit_solve_block(ast::SolveBlock& node) {
    // Filter for solve statements that use the matexp solver method
    const auto& solve_method = node.get_method();
    const auto& steadystate_method = node.get_steadystate();
    const auto& matexp_method = codegen::naming::MATEXP_METHOD;
    const auto is_method_matexp = [](auto method) {
        return method && method->get_value()->eval() == matexp_method;
    };
    const bool solve = is_method_matexp(solve_method);
    const bool steadystate = is_method_matexp(steadystate_method);
    const auto& block_name = node.get_block_name()->get_node_name();
    if (!(solve || steadystate)) {
        keep_blocks.push_back(block_name);
        return;
    }
    // Save the block name for later reference
    if (solve) {
        solve_blocks.push_back(block_name);
    }
    if (steadystate) {
        steadystate_blocks.push_back(block_name);
    }
    // Clear the solver method
    node.set_method(nullptr);
    node.set_steadystate(nullptr);
    // Replace the KINETIC block with the solved PROCEDURE block
    node.set_block_name(mangle_name(block_name, steadystate));
}


void MatexpVisitor::visit_kinetic_block(ast::KineticBlock& node) {
    // Filter for blocks with corresponding SOLVE statements
    const auto& block_name = node.get_name()->get_node_name();
    const bool solve = vector_contains(solve_blocks, block_name);
    const bool steadystate = vector_contains(steadystate_blocks, block_name);
    //
    if (steadystate) {
        solve_kinetic_block(node, true);
    }
    if (solve) {
        solve_kinetic_block(node, false);
    }
}


void MatexpVisitor::solve_kinetic_block(const ast::KineticBlock& node, bool steadystate) {
    // Make a copy of the statement block. Do not mutate the original
    const auto& statement_block = std::make_shared<ast::StatementBlock>(
        *node.get_statement_block());
    // Make the new PROCEDURE block
    const auto& block_name = node.get_name()->get_node_name();
    const auto& new_name = mangle_name(block_name, steadystate);
    ast::ArgumentVector args;
    const auto& unit = std::make_shared<ast::Unit>(std::make_shared<ast::String>(""));
    const auto& new_block =
        std::make_shared<ast::ProcedureBlock>(new_name, args, unit, statement_block);
    solved_blocks.push_back(new_block);
    logger->debug("MatexpVisitor :: Creating PROCEDURE \"{}\"", to_nmodl(new_name));
    // Get ready to write C++ code
    const std::string float_type = codegen::naming::DEFAULT_FLOAT_TYPE;
    const std::string n_states = std::to_string(states.size());
    const std::string vector_type = "Eigen::Matrix<" + float_type + ", " + n_states + ", 1>";
    const std::string matrix_type = "Eigen::Matrix<" + float_type + ", " + n_states + ", " +
                                    n_states + ">";
    // Setup the Jacobian matrix
    const auto& begining = statement_block->get_statements().begin();
    statement_block->insert_statement(
        begining,
        create_statement("VERBATIM\n" + matrix_type + " nmodl_eigen_jm = " + matrix_type +
                         "::Zero();\n" + float_type + "* nmodl_eigen_j = nmodl_eigen_jm.data();\n" +
                         "ENDVERBATIM\n"));
    // Convert the reaction statements into assignments to the Jacobian matrix
    dt = steadystate ? "1000000000" : "dt";
    in_kinetic_block = true;
    this->visit_statement_block(*statement_block);
    in_kinetic_block = false;
    // Find the conserved sum, if specified
    std::string conserve = "";
    const auto conserve_statements = collect_nodes(node, {ast::AstNodeType::CONSERVE});
    if (conserve_statements.size() > 0) {
        // Check that the conserve statement is the sum of state variables
        const auto stmt = std::dynamic_pointer_cast<const ast::Conserve>(conserve_statements[0]);
        const auto react = stmt->get_react();
        const auto vars = collect_nodes(*react, {ast::AstNodeType::NAME});
        const bool primes = node_exists(*react, {ast::AstNodeType::PRIME_NAME});
        std::vector<std::string> var_names;
        std::vector<std::string> state_names;
        for (const auto& var: vars) {
            var_names.push_back(to_nmodl(var));
        }
        for (const auto& state: states) {
            state_names.push_back(state->get_name());
        }
        std::sort(var_names.begin(), var_names.end());
        std::sort(state_names.begin(), state_names.end());
        if (var_names != state_names || primes) {
            logger->error("MatexpVisitor :: Error : Ignoring irregular CONSERVE statement");
        } else {
            conserve = to_nmodl(stmt->get_expr());
        }
    }
    if (conserve_statements.size() > 1) {
        logger->error("MatexpVisitor :: Error : multiple CONSERVE statements unsupported");
    }
    //
    std::string conserve_cpp = "";
    if (conserve == "1") {
        conserve_cpp = "nmodl_eigen_ym /= nmodl_eigen_ym.sum();\n";
    } else if (conserve.size()) {
        conserve_cpp = "nmodl_eigen_ym *= (" + conserve + ") / nmodl_eigen_ym.sum();\n";
    }
    // Remove CONSERVE statements
    RemoveConserveVisitor().visit_statement_block(*statement_block);
    // Create the state vector
    statement_block->emplace_back_statement(
        create_statement("VERBATIM\n" + vector_type + " nmodl_eigen_xm;\n" + float_type +
                         "* nmodl_eigen_x = nmodl_eigen_xm.data();\n" + "ENDVERBATIM\n"));
    // Assign to the state vector
    if (steadystate && conserve.size()) {
        for (int i = 0; i < states.size(); i++) {
            statement_block->emplace_back_statement(create_statement(
                "nmodl_eigen_x[" + std::to_string(i) + "] = (" + conserve + ") / " + n_states));
        }
    } else {
        for (int i = 0; i < states.size(); i++) {
            statement_block->emplace_back_statement(create_statement(
                "nmodl_eigen_x[" + std::to_string(i) + "] = " + states[i]->get_name()));
        }
    }
    // Run the solver
    statement_block->emplace_back_statement(create_statement(
        "VERBATIM\n" + vector_type + " nmodl_eigen_ym = nmodl_eigen_jm.exp() * nmodl_eigen_xm;\n" +
        conserve_cpp + float_type + "* nmodl_eigen_y = nmodl_eigen_ym.data();\n" +
        "ENDVERBATIM\n"));
    // Save the results
    for (int i = 0; i < states.size(); i++) {
        statement_block->emplace_back_statement(create_statement(
            states[i]->get_name() + " = nmodl_eigen_y[" + std::to_string(i) + "]"));
    }
}


void MatexpVisitor::visit_statement_block(ast::StatementBlock& node) {
    // Iterate through the children using indices instead of pointers,
    // so that more statements can be added to the list
    const auto& statements = node.get_statements();
    for (int index = 0; index < statements.size(); index++) {
        statements[index]->accept(*this);
    }
}


// Argument node is a reactant or product expression
std::string get_state_var_name(const std::shared_ptr<ast::Ast>& node) {
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


int MatexpVisitor::get_state_index(const std::string& state_name) {
    for (int index = 0; index < states.size(); index++) {
        if (state_name == states[index]->get_name()) {
            return index;
        }
    }
    throw std::invalid_argument("no such state");
}


int find_node(const nmodl::ast::StatementVector& statements, const nmodl::ast::Statement* node) {
    for (int index = 0; index < statements.size(); index++) {
        const nmodl::ast::Statement* cursor = statements[index].get();
        if (cursor == node) {
            return index;
        }
    }
    throw std::runtime_error("broken ast");
}


void MatexpVisitor::visit_reaction_statement(ast::ReactionStatement& node) {
    if (!in_kinetic_block) {
        return;
    }
    // Unpack the reaction data
    const auto& op = node.get_op().get_value();
    const auto& lhs = node.get_reaction1();
    const auto& rhs = node.get_reaction2();
    const auto& kf = node.get_expression1();  // forwards reaction rate
    const auto& kb = node.get_expression2();  // backwards reaction rate
    // Find and remove this reaction statement
    const auto& statement_block = node.get_parent()->get_parent()->get_statement_block();
    const auto& statements = statement_block->get_statements();
    int statement_index = find_node(statements, &node);
    statement_block->erase_statement(std::begin(statements) + statement_index);
    // Check for invalid kinetic models
    if (op == ast::ReactionOp::LTLT) {
        nonlinear_reaction_error();
    } else if (op == ast::ReactionOp::MINUSGT) {
        // Find the state vector indices
        const auto lhs_name = get_state_var_name(lhs);
        const auto lhs_idx = get_state_index(lhs_name);
        // Calculate the Jacobian matrix indices
        const int jf_src_idx = lhs_idx + states.size() * lhs_idx;
        // Write NMODL to assign to the Jacobian matrix
        const std::string jf_src = "nmodl_eigen_j[" + std::to_string(jf_src_idx) + "]";
        const std::string kf_nmodl = "(" + to_nmodl(kf) + ")";
        const std::string jf_n_string = jf_src + " = " + jf_src + " - " + kf_nmodl + " * " + dt;
        const auto& jf_n = create_statement(jf_n_string);
        // Replace the reaction statement with an assignment to the Jaconbian
        statement_block->insert_statement(std::begin(statements) + statement_index, jf_n);
    } else if (op == ast::ReactionOp::LTMINUSGT) {
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
        const std::string kf_nmodl = "(" + to_nmodl(kf) + ")";
        const std::string kb_nmodl = "(" + to_nmodl(kb) + ")";
        const std::string jf_n_string = jf_src + " = " + jf_src + " - " + kf_nmodl + " * " + dt;
        const std::string jf_p_string = jf_dst + " = " + jf_dst + " + " + kf_nmodl + " * " + dt;
        const std::string jb_n_string = jb_src + " = " + jb_src + " - " + kb_nmodl + " * " + dt;
        const std::string jb_p_string = jb_dst + " = " + jb_dst + " + " + kb_nmodl + " * " + dt;
        const auto& jf_n = create_statement(jf_n_string);
        const auto& jf_p = create_statement(jf_p_string);
        const auto& jb_n = create_statement(jb_n_string);
        const auto& jb_p = create_statement(jb_p_string);
        // Replace the reaction statement with the four new statements
        for (const auto& stmt: {jf_n, jf_p, jb_n, jb_p}) {
            statement_block->insert_statement(std::begin(statements) + statement_index++, stmt);
        }
    }
}

}  // namespace visitor
}  // namespace nmodl
