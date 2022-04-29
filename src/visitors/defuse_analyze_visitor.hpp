/*************************************************************************
 * Copyright (C) 2018-2022 Blue Brain Project
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

#include "printer/json_printer.hpp"
#include "symtab/decl.hpp"
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
    /// global or local variable is conditionally defined
    CD,
    /// local variable is used
    LU,
    /// local variable is defined
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

/**
 * Variable type processed by DefUseAnalyzeVisitor
 *
 * DUVariableType::Local means that we are looking for LD, LU and CD DUStates, while Global means we
 * are looking for U, D and CD DUStates.
 */
enum class DUVariableType { Local, Global };

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

    explicit DUInstance(DUState state,
                        const std::shared_ptr<const ast::BinaryExpression> binary_expression)
        : state(state)
        , binary_expression(binary_expression) {}

    /// analyze all children and return "effective" usage
    DUState eval(DUVariableType variable_type) const;

    /// if, elseif and else evaluation
    DUState sub_block_eval(DUVariableType variable_type) const;

    /// evaluate global usage i.e. with [D,U] states of children
    DUState conditional_block_eval(DUVariableType variable_type) const;

    void print(printer::JSONPrinter& printer) const;

    /** \brief binary expression in which the variable is used/defined
     *
     * We use the binary expression because it is used in both:
     *
     * - x = ... // expression statement, binary expression
     * - IF (x == 0) // not an expression statement, binary expression
     *
     * We also want the outermost binary expression. Thus, we do not keep track
     * of the interior ones. For example:
     *
     * \f$ tau = tau + 1 \f$
     *
     * we want to return the full statement, not only \f$ tau + 1 \f$
     */
    std::shared_ptr<const ast::BinaryExpression> binary_expression;
};


/**
 * \class DUChain
 * \brief Def-Use chain for an AST node
 */
class DUChain {
  public:
    /// name of the node
    std::string name;

    /// type of variable
    DUVariableType variable_type;

    /// def-use chain for a variable
    std::vector<DUInstance> chain;

    DUChain() = default;
    DUChain(std::string name, DUVariableType type)
        : name(std::move(name))
        , variable_type(type) {}

    /// return "effective" usage of a variable
    DUState eval() const;

    /// return json representation
    std::string to_string(bool compact = true) const;
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
class DefUseAnalyzeVisitor: protected ConstAstVisitor {
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

    /// variable type (Local or Global)
    DUVariableType variable_type;

    /// indicate that there is unsupported construct encountered
    bool unsupported_node = false;

    /// ignore verbatim blocks
    bool ignore_verbatim = false;

    /// starting visiting lhs of assignment statement
    bool visiting_lhs = false;

    std::shared_ptr<const ast::BinaryExpression> current_binary_expression = nullptr;

    void process_variable(const std::string& name);
    void process_variable(const std::string& name, int index);

    void update_defuse_chain(const std::string& name);
    void visit_unsupported_node(const ast::Node& node);
    void visit_with_new_chain(const ast::Node& node, DUState state);
    void start_new_chain(DUState state);

  public:
    DefUseAnalyzeVisitor() = delete;

    explicit DefUseAnalyzeVisitor(symtab::SymbolTable& symtab)
        : global_symtab(&symtab) {}

    DefUseAnalyzeVisitor(symtab::SymbolTable& symtab, bool ignore_verbatim)
        : global_symtab(&symtab)
        , ignore_verbatim(ignore_verbatim) {}

    void visit_binary_expression(const ast::BinaryExpression& node) override;
    void visit_if_statement(const ast::IfStatement& node) override;
    void visit_function_call(const ast::FunctionCall& node) override;
    void visit_statement_block(const ast::StatementBlock& node) override;
    void visit_verbatim(const ast::Verbatim& node) override;

    /**
     * /\name unsupported statements
     * we aren't sure how to handle this "yet" and hence variables
     * used in any of the below statements are handled separately
     * \{
     */

    void visit_reaction_statement(const ast::ReactionStatement& node) override;

    void visit_non_lin_equation(const ast::NonLinEquation& node) override;

    void visit_lin_equation(const ast::LinEquation& node) override;

    void visit_partial_boundary(const ast::PartialBoundary& node) override;

    void visit_from_statement(const ast::FromStatement& node) override;

    void visit_conserve(const ast::Conserve& node) override;

    void visit_var_name(const ast::VarName& node) override;

    void visit_name(const ast::Name& node) override;

    void visit_indexed_name(const ast::IndexedName& node) override;

    /** \} */

    /**
     * /\name statements
     * nodes that should not be used for def-use chain analysis
     * \{
     */

    void visit_conductance_hint(const ast::ConductanceHint& node) override;

    void visit_local_list_statement(const ast::LocalListStatement& node) override;

    void visit_argument(const ast::Argument& node) override;

    /** \} */

    /// compute def-use chain for a variable within the node
    DUChain analyze(const ast::Ast& node, const std::string& name);
};

/** @} */  // end of visitor_classes

}  // namespace visitor
}  // namespace nmodl
