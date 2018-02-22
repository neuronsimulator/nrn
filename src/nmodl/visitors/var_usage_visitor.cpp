#include "visitors/var_usage_visitor.hpp"

/// rename matching variable
void VarUsageVisitor::visit_name(ast::Name* node) {
    std::string name = node->get_name();
    if (name == var_name) {
        used = true;
    }
}

bool VarUsageVisitor::variable_used(ast::Node* node, std::string name) {
    used = false;
    var_name = name;
    node->visit_children(this);
    return used;
}