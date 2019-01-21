#pragma once


/* Abstract base class for all visitor implementations */
class Visitor {

    public:
        virtual ~Visitor() = default;

        {% for node in nodes %}
        virtual void visit_{{ node.class_name|snake_case }}(ast::{{ node.class_name }}* node) = 0;
        {% endfor %}
};
