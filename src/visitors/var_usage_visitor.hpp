#ifndef VAR_USAGE_VISITOR_HPP
#define VAR_USAGE_VISITOR_HPP

#include <string>

#include "ast/ast.hpp"
#include "visitors/ast_visitor.hpp"

/**
 * \class VarUsageVisitor
 * \brief Check if variable is used in given block
 *
 * \todo : check if macro is considered as variable
 */

class VarUsageVisitor : public AstVisitor {
  private:
    /// variable to check usage
    std::string var_name;
    bool used = false;

  public:
    VarUsageVisitor() = default;

    bool variable_used(ast::Node* node, std::string name);

    virtual void visit_name(ast::Name* node) override;
};

#endif
