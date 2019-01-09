#include "visitors/ast_visitor.hpp"

{% for node in nodes %}
void AstVisitor::visit_{{ node.class_name|snake_case }}({{ node.class_name }}* node) {
    node->visit_children(this);
}

{% endfor %}
