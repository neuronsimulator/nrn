/*************************************************************************
 * Copyright (C) 2018-2022 Blue Brain Project
 *
 * This file is part of NMODL distributed under the terms of the GNU
 * Lesser General Public License. See top-level LICENSE file for details.
 *************************************************************************/

///
/// THIS FILE IS GENERATED AT BUILD TIME AND SHALL NOT BE EDITED.
///

#pragma once

/**
 * \file
 * \brief Visitors extending base visitors for Python interface
 */

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "ast/ast.hpp"
#include "visitors/visitor.hpp"
#include "visitors/ast_visitor.hpp"

using namespace nmodl;
using namespace visitor;

/**
 * \brief Class mirroring nmodl::visitor::Visitor for Python bindings
 *
 * \details \copydetails nmodl::visitor::Visitor
 *
 * This class is used to interface nmodl::visitor::Visitor with the Python
 * world using `pybind11`.
 */
class PyVisitor : public Visitor {
public:
    using Visitor::Visitor;

    {% for node in nodes %}
    void visit_{{ node.class_name|snake_case }}(ast::{{ node.class_name }}& node) override;
    {% endfor %}
};


/**
 * \brief Class mirroring nmodl::visitor::AstVisitor for Python bindings
 *
 * \details \copydetails nmodl::visitor::AstVisitor
 *
 * This class is used to interface nmodl::visitor::AstVisitor with the Python
 * world using `pybind11`.
 */
class PyAstVisitor : public AstVisitor {
public:
    using AstVisitor::AstVisitor;

    {% for node in nodes %}
    void visit_{{ node.class_name|snake_case }}(ast::{{ node.class_name }}& node) override;
    {% endfor %}
};

/**
 * \brief Class mirroring nmodl::visitor::ConstVisitor for Python bindings
 *
 * \details \copydetails nmodl::visitor::ConstVisitor
 *
 * This class is used to interface nmodl::visitor::ConstVisitor with the Python
 * world using `pybind11`.
 */
class PyConstVisitor : public ConstVisitor {
public:
    using ConstVisitor::ConstVisitor;

    {% for node in nodes %}
    void visit_{{ node.class_name|snake_case }}(const ast::{{ node.class_name }}& node) override;
    {% endfor %}
};


/**
 * \brief Class mirroring nmodl::visitor::ConstAstVisitor for Python bindings
 *
 * \details \copydetails nmodl::visitor::ConstAstVisitor
 *
 * This class is used to interface nmodl::visitor::ConstAstVisitor with the Python
 * world using `pybind11`.
 */
class PyConstAstVisitor : public ConstAstVisitor {
public:
    using ConstAstVisitor::ConstAstVisitor;

    {% for node in nodes %}
    void visit_{{ node.class_name|snake_case }}(const ast::{{ node.class_name }}& node) override;
    {% endfor %}
};

