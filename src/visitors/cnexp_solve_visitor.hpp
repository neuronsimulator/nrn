#ifndef CNEXP_SOLVE_VISITOR_HPP
#define CNEXP_SOLVE_VISITOR_HPP

#include <string>

#include "ast/ast.hpp"
#include "visitors/ast_visitor.hpp"

/**
 * \class CnexpSolveVisitor
 * \brief Visitor that solves and replaces ODEs using cnexp method
 *
 * This pass solves ODEs in derivative block if cnexp method is used.
 * The original ODEs get replaced with the solution. This transformation
 * is performed at ast level. This is useful for performance modeling
 * purpose where we want to measure performance metrics using perfvisitor
 * pass.
 */

class CnexpSolveVisitor : public AstVisitor {
  private:
    /// method specified in solve block
    std::string solve_method;

    /// true while visiting derivative block
    bool derivative_block = false;

    /// name of the cnexp methoda in nmodl
    const std::string cnexp_method = "cnexp";

  public:
    CnexpSolveVisitor() = default;

    void visit_solve_block(SolveBlock* node) override;
    void visit_derivative_block(DerivativeBlock* node) override;
    void visit_binary_expression(BinaryExpression* node) override;
};

#endif
