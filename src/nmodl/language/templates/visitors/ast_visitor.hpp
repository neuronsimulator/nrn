/*
 * Copyright 2023 Blue Brain Project, EPFL.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

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

#include "visitors/visitor.hpp"


namespace nmodl {
namespace visitor {

/**
 * \ingroup visitor_classes
 * \{
 */

/**
 * \brief Concrete visitor for all AST classes
 */
class AstVisitor: public Visitor {
  public:
    {% for node in nodes %}
    void visit_{{ node.class_name|snake_case }}(ast::{{ node.class_name }}& node) override;
    {% endfor %}
};

/**
 * \brief Concrete constant visitor for all AST classes
 */
class ConstAstVisitor: public ConstVisitor {
  public:
    {% for node in nodes %}
    void visit_{{ node.class_name|snake_case }}(const ast::{{ node.class_name }}& node) override;
    {% endfor %}
};

/** \} */  // end of visitor_classes

}  // namespace visitor
}  // namespace nmodl

