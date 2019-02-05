#ifndef DIFF_EQ_VISITOR_HPP
#define DIFF_EQ_VISITOR_HPP

#include <vector>

#include "ast/ast.hpp"
#include "visitors/ast_visitor.hpp"

/**
 * \class DifferentialEquationVisitor
 * \brief Visitor for differential equations in derivative block
 *
 * Simple example of visitor to find out all differential
 * equations in a MOD file.
 */

class DifferentialEquationVisitor : public AstVisitor {
  private:
    std::vector<ast::DiffEqExpression*> equations;

  public:
    DifferentialEquationVisitor() = default;

    void visit_diff_eq_expression(ast::DiffEqExpression* node) override;

    std::vector<ast::DiffEqExpression*> get_equations() {
        return equations;
    }
};

#endif
