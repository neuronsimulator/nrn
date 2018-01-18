#include "visitors/rename_visitor.hpp"

/// rename matching variable
void RenameVisitor::visit_name(ast::Name* node) {
    std::string name = node->get_name();
    if (name == var_name) {
        node->value->set(new_var_name);
    }
}

/** Prime name has member order which is an integer. In theory
 * integer could be "macro name" and hence could end-up renaming
 * macro. In practice this won't be an issue as we order is set
 * by parser. To be safe we are only renaming prime variable.
 */
void RenameVisitor::visit_prime_name(ast::PrimeName* node) {
    node->visit_children(this);
}
