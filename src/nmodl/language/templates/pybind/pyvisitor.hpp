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
 * \file
 * \brief Visitors extending base visitors for Python interface
 */

#include <nanobind/nanobind.h>
#include <nanobind/trampoline.h>

#include "ast/ast.hpp"
#include "visitors/visitor.hpp"
#include "visitors/ast_visitor.hpp"

using namespace nmodl;
using namespace visitor;

/**
 * \brief Class mirroring nmodl::visitor::Visitor for Python bindings
 *
 * Slot count is an upper bound on AST node types (visit_* overrides).
 */
class PyVisitor: public Visitor {
  public:
    NB_TRAMPOLINE(Visitor, 256);

    {% for node in nodes %
    }
    void visit_{{node.class_name | snake_case}}(ast::{{node.class_name}} & node) override;
    { % endfor % }
};


/**
 * \brief Class mirroring nmodl::visitor::AstVisitor for Python bindings
 */
class PyAstVisitor: public AstVisitor {
  public:
    NB_TRAMPOLINE(AstVisitor, 256);

    {% for node in nodes %
    }
    void visit_{{node.class_name | snake_case}}(ast::{{node.class_name}} & node) override;
    { % endfor % }
};

/**
 * \brief Class mirroring nmodl::visitor::ConstVisitor for Python bindings
 */
class PyConstVisitor: public ConstVisitor {
  public:
    NB_TRAMPOLINE(ConstVisitor, 256);

    {% for node in nodes %
    }
    void visit_{{node.class_name | snake_case}}(const ast::{{node.class_name}} & node) override;
    { % endfor % }
};


/**
 * \brief Class mirroring nmodl::visitor::ConstAstVisitor for Python bindings
 */
class PyConstAstVisitor: public ConstAstVisitor {
  public:
    NB_TRAMPOLINE(ConstAstVisitor, 256);

    {% for node in nodes %
    }
    void visit_{{node.class_name | snake_case}}(const ast::{{node.class_name}} & node) override;
    { % endfor % }
};
