#pragma once

#include "ast/ast.hpp"
#include "visitors/visitor.hpp"
#include "visitors/ast_visitor.hpp"
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>


class PyVisitor : public Visitor {
public:
    using Visitor::Visitor;

    {% for node in nodes %}
    void visit_{{ node.class_name|snake_case }}(ast::{{ node.class_name }}* node) override;
    {% endfor %}
};

/* Python interface of basic visitor implementation */
class PyAstVisitor : public AstVisitor {
public:
    using AstVisitor::AstVisitor;

    {% for node in nodes %}
    void visit_{{ node.class_name|snake_case }}(ast::{{ node.class_name }}* node) override;
    {% endfor %}
};
