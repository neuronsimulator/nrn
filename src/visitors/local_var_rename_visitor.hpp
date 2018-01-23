#ifndef LOCAL_VAR_RENAME_VISITOR_HPP
#define LOCAL_VAR_RENAME_VISITOR_HPP

#include <stack>
#include <map>

#include "ast/ast.hpp"
#include "visitors/ast_visitor.hpp"
#include "symtab/symbol_table.hpp"

/**
 * \class LocalVarRenameVisitor
 * \brief Visitor to rename local variables conflicting with global scope
 *
 * Motivation: During inlining we have to do data-flow-analysis. Consider
 * below example:
 *
 *      NEURON {
 *          RANGE tau, beta
 *      }
 *
 *      DERIVATIVE states() {
 *          LOCAL tau
 *          ...
 *          rates()
 *      }
 *
 *      PROCEDURE rates() {
 *          tau = beta * 0.12 * some_var
 *      }
 *
 * When rates() will be inlined into states(), local definition of tau will
 * conflict with range variable tau. Hence we can't just copy the statements.
 * Dataflow analysis could be done at the time of inlining. Other way is to run
 * this pass before inlining and pre-rename any local-global variable conflicts.
 * As we are renaming local variables only, it's safe and there are no side effects.
 *
 * \todo: currently we are renaming variables even if there is no inlining candidates.
 * In this case ideally we should not rename.
 */

class LocalVarRenameVisitor : public AstVisitor {
  private:
    /// non-null symbol table in the scope hierarchy
    std::shared_ptr<symtab::SymbolTable> symtab;

    /// symbol tables in case of nested blocks
    std::stack<std::shared_ptr<symtab::SymbolTable>> symtab_stack;

    /// variables currently being renamed and their count
    std::map<std::string, int> renamed_variables;

  public:
    LocalVarRenameVisitor() = default;
    virtual void visit_statement_block(ast::StatementBlock* node) override;
};

#endif
