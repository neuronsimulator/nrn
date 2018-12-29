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

    /// name of the cnexp method
    const std::string cnexp_method = "cnexp";

    /// name of the derivimplicit method
    const std::string derivimplicit_method = "derivimplicit";

    /// name of the euler method
    const std::string euler_method = "euler";

    symtab::SymbolTable* program_symtab = nullptr;

  public:
    CnexpSolveVisitor() = default;

    void visit_solve_block(ast::SolveBlock* node) override;
    void visit_derivative_block(ast::DerivativeBlock* node) override;
    void visit_binary_expression(ast::BinaryExpression* node) override;
    void visit_program(ast::Program* node) override;
};

#endif
