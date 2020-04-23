/*************************************************************************
 * Copyright (C) 2018-2019 Blue Brain Project
 *
 * This file is part of NMODL distributed under the terms of the GNU
 * Lesser General Public License. See top-level LICENSE file for details.
 *************************************************************************/

///
/// THIS FILE IS GENERATED AT BUILD TIME AND SHALL NOT BE EDITED.
///

#pragma once

/**
 *
 * \dir
 * \brief Auto generated visitors
 *
 * \file
 * \brief \copybrief nmodl::visitor::AstVisitor
 */

#include "ast/ast.hpp"
#include "visitors/visitor.hpp"


namespace nmodl {
namespace visitor {

/**
 * @ingroup visitor_classes
 * @{
 */

/**
 * \brief Concrete visitor for all AST classes
 *
 * This class defines interface for all concrete visitors implementation.
 * Note that this class only provides interface that could be implemented
 * by concrete visitors like ast::AstVisitor.
 */
class AstVisitor : public Visitor {
  public:
    {% for node in nodes %}
    void visit_{{ node.class_name|snake_case }}(ast::{{ node.class_name }}& node) override;
    {% endfor %}
};

/** @} */  // end of visitor_classes

}  // namespace visitor
}  // namespace nmodl

