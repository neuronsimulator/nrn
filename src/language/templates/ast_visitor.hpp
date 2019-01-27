#pragma once

#include "ast/ast.hpp"
#include "visitors/visitor.hpp"


/* Basic visitor implementation */
class AstVisitor : public Visitor {

    public:
        {% for node in nodes %}
        virtual void visit_{{ node.class_name|snake_case }}(ast::{{ node.class_name }}* node) override;
        {% endfor %}
};
