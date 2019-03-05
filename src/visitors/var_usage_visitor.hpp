/*************************************************************************
 * Copyright (C) 2018-2019 Blue Brain Project
 *
 * This file is part of NMODL distributed under the terms of the GNU
 * Lesser General Public License. See top-level LICENSE file for details.
 *************************************************************************/

#pragma once

#include <string>

#include "ast/ast.hpp"
#include "visitors/ast_visitor.hpp"


namespace nmodl {

/**
 * \class VarUsageVisitor
 * \brief Check if variable is used in given block
 *
 * \todo : check if macro is considered as variable
 */

class VarUsageVisitor: public AstVisitor {
  private:
    /// variable to check usage
    std::string var_name;
    bool used = false;

  public:
    VarUsageVisitor() = default;

    bool variable_used(ast::Node* node, std::string name);

    virtual void visit_name(ast::Name* node) override;
};

}  // namespace nmodl