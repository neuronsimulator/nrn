/*************************************************************************
 * Copyright (C) 2018-2019 Blue Brain Project
 *
 * This file is part of NMODL distributed under the terms of the GNU
 * Lesser General Public License. See top-level LICENSE file for details.
 *************************************************************************/

#pragma once

/**
 * \file
 * \brief \copybrief nmodl::visitor::DefUseAnalyzeVisitor
 */

#include <map>
#include <stack>

#include "ast/ast.hpp"
#include "printer/json_printer.hpp"
#include "symtab/symbol_table.hpp"
#include "utils/logger.hpp"
#include "visitors/ast_visitor.hpp"
#include "visitors/visitor_utils.hpp"


namespace nmodl {
namespace visitor {

/// Represent a state in Def-Use chain
enum class DUState {
    /// global variable is used
    U,
    /// global variable is defined
    D,
    /// global variable is conditionally defined
    CD,
    /// local variable is used
    LU,
    /// local variable is used
    LD,
    /// state not known
    UNKNOWN,
    /// conditional block
    CONDITIONAL_BLOCK,
    /// if sub-block
    IF,
    /// elseif sub-block
    ELSEIF,
    /// else sub-block
    ELSE,
    /// variable is not used
    NONE
};

std::ostream& operator<<(std::ostream& os, DUState state);

/**
 * \class DUInstance
 * \brief Represent use of a variable in the statement
 *
 * For a given variable, say tau, when we have statement like `a = tau + c + tau`
 * then def-use is simply `[DUState::U, DUState::U]`. But if we have if-else block
 * like:
 *
 * \code{.mod}
 *      IF (...) {
 *          a = tau
 *          tau = c + d
 *      } ELSE {
 *          tau = b
 *      }
 * \endcode
 *
 * Then to know the effective result, we have to analyze def-use in `IF` and `ELSE`
 * blocks i.e. if variable is used in any of the if-elseif-else block then it needs
 * to mark as `DUState::U`. Hence we keep the track of all children in case of
 * statements like if-else.
 */
class DUInstance {
  public:
    /// state of the usage
    DUState state;

    /// usage of variable in case of if like statements
    std::vector<DUInstance> children;

    explicit DUInstance(DUState state)
        : state(state) {}

    /// analyze all children and return "effective" usage
    DUState eval();

    /// if, elseif and else evaluation
    DUState sub_block_eval();

    /// evaluate global usage i.e. with [D,U] states of children
    DUState conditional_block_eval();

    void print(printer::JSONPrinter& printer);
};


/**
 * \class DUChain
 * \brief Def-Use chain for an AST node
 */
class DUChain {
  public:
    /// name of the node
    std::string name;

    /// def-use chain for a variable
    std::vector<DUInstance> chain;

    DUChain() = default;
    explicit DUChain(std::string name)
        : name(std::move(name)) {}

    /// return "effective" usage of a variable
    DUState eval();

    /// return json representation
    std::string to_string(bool compact = true);
};


/**
 * @addtogroup visitor_classes
 * @{
 */

/**
 * \class DefUseAnalyzeVisitor
 * \brief %Visitor to return Def-Use chain for a given variable in the block/node
 *
 * Motivation: For global to local variable transformation aka localizer
 * pass, we need to compute Def-Use chains for all global variables. For
 * example, if we have variable usage like:
 *
 * \code{.mod}
 *      NEURON {
 *          RANGE tau, beta
 *      }
 *
 *      DERIVATIVE states() {
 *          ...
 *          x = alpha
 *          beta = x + y
 *          ....
 *          z = beta
 *      }
 *
 *      INITIAL {
 *         ...
 *         beta = x + y
 *         z = beta * 0.1
 *         alpha = x * y
 *      }
 * \endcode
 *
 * In the above example if we look at variable beta then it's defined before it's
 * usage and hence it can be safely made local. But variable alpha is used in first
 * block and defined in second block. Hence it can't be made local. A variable can
 * be made local if it is defined before it's usage (in all global blocks). We exclude
 * procedures/functions because we expect inlining to be done prior to this pass.
 *
 * The analysis of if-elseif-else needs special attention because the def-use chain
 * needs re-cursive evaluation in order to find end-result. For example:
 *
 * \code{.mod}
 *      IF(...) {
 *          beta = y
 *      } ELSE {
 *          IF(...) {
 *              beta = x
 *          } ELSE {
 *              x = beta
 *          }
 *      }
 * \endcode
 *
 * For if-else statements, in the above example, if the variable is used
 * in any of the if-elseif-else part then it is considered as "used". And
 * this is done recursively from innermost level to the top.
 */
class DefUseAnalyzeVisitor: public AstVisitor {
  private:
    /// symbol table containing global variables
    symtab::SymbolTable* global_symtab = nullptr;

    /// def-use chain currently being constructed
    std::vector<DUInstance>* current_chain = nullptr;

    /// symbol table for current statement block (or of parent block if doesn't have)
    /// should be initialized by caller somehow
    symtab::SymbolTable* current_symtab = nullptr;

    /// symbol tables in call hierarchy
    std::stack<symtab::SymbolTable*> symtab_stack;

    /// variable for which to construct def-use chain
    std::string variable_name;

    /// indicate that there is unsupported construct encountered
    bool unsupported_node = false;

    /// ignore verbatim blocks
    bool ignore_verbatim = false;

    /// starting visiting lhs of assignment statement
    bool visiting_lhs = false;

    void process_variable(const std::string& name);
    void process_variable(const std::string& name, int index);

    void update_defuse_chain(const std::string& name);
    void visit_unsupported_node(ast::Node* node);
    void visit_with_new_chain(ast::Node* node, DUState state);
    void start_new_chain(DUState state);

  public:
    DefUseAnalyzeVisitor() = delete;

    explicit DefUseAnalyzeVisitor(symtab::SymbolTable* symtab)
        : global_symtab(symtab) {}

    DefUseAnalyzeVisitor(symtab::SymbolTable* symtab, bool ignore_verbatim)
        : global_symtab(symtab)
        , ignore_verbatim(ignore_verbatim) {}

    void visit_binary_expression(ast::BinaryExpression& node) override;
    void visit_if_statement(ast::IfStatement& node) override;
    void visit_function_call(ast::FunctionCall& node) override;
    void visit_statement_block(ast::StatementBlock& node) override;
    void visit_verbatim(ast::Verbatim& node) override;

    /// unsupported statements : we aren't sure how to handle this "yet" and
    /// hence variables used in any of the below statements are handled separately

    void visit_reaction_statement(ast::ReactionStatement& node) override {
        visit_unsupported_node(&node);
    }

    void visit_non_lin_equation(ast::NonLinEquation& node) override {
        visit_unsupported_node(&node);
    }

    void visit_lin_equation(ast::LinEquation& node) override {
        visit_unsupported_node(&node);
    }

    void visit_partial_boundary(ast::PartialBoundary& node) override {
        visit_unsupported_node(&node);
    }

    void visit_from_statement(ast::FromStatement& node) override {
        visit_unsupported_node(&node);
    }

    void visit_conserve(ast::Conserve& node) override {
        visit_unsupported_node(&node);
    }

    void visit_var_name(ast::VarName& node) override {
        const std::string& variable = to_nmodl(node);
        process_variable(variable);
    }

    void visit_name(ast::Name& node) override {
        const std::string& variable = to_nmodl(node);
        process_variable(variable);
    }

    void visit_indexed_name(ast::IndexedName& node) override {
        const auto& name = node.get_node_name();
        const auto& length = node.get_length();

        /// index should be an integer (e.g. after constant folding)
        /// if this is not the case and then we can't determine exact
        /// def-use chain
        if (!length->is_integer()) {
            /// check if variable name without index part is same
            auto variable_name_prefix = variable_name.substr(0, variable_name.find('['));
            if (name == variable_name_prefix) {
                update_defuse_chain(variable_name_prefix);
                const std::string& text = to_nmodl(node);
                nmodl::logger->info("index used to access variable is not known : {} ", text);
            }
            return;
        }
        auto index = std::dynamic_pointer_cast<ast::Integer>(length);
        process_variable(name, index->eval());
    }

    /// statements / nodes that should not be used for def-use chain analysis

    void visit_conductance_hint(ast::ConductanceHint& /*node*/) override {}

    void visit_local_list_statement(ast::LocalListStatement& /*node*/) override {}

    void visit_argument(ast::Argument& /*node*/) override {}

    /// compute def-use chain for a variable within the node
    DUChain analyze(ast::Ast* node, const std::string& name);
};

/** @} */  // end of visitor_classes

}  // namespace visitor
}  // namespace nmodl
