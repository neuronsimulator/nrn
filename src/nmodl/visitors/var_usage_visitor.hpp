/*
 * Copyright 2023 Blue Brain Project, EPFL.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

/**
 * \file
 * \brief \copybrief nmodl::visitor::VarUsageVisitor
 */

#include <string>

#include "visitors/ast_visitor.hpp"


namespace nmodl {
namespace visitor {

/**
 * \addtogroup visitor_classes
 * \{
 */

/**
 * \class VarUsageVisitor
 * \brief Check if variable is used in given block
 *
 * \todo Check if macro is considered as variable
 */

class VarUsageVisitor: protected ConstAstVisitor {
  private:
    /// variable to check usage
    std::string var_name;
    bool used = false;

    void visit_name(const ast::Name& node) override;

  public:
    VarUsageVisitor() = default;

    bool variable_used(const ast::Node& node, std::string name);
};

/** \} */  // end of visitor_classes

}  // namespace visitor
}  // namespace nmodl
