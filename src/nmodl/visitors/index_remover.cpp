#include "index_remover.hpp"

#include "utils/logger.hpp"
#include "visitors/visitor_utils.hpp"

namespace nmodl {
namespace visitor {

IndexRemover::IndexRemover(std::string index, int value)
    : index(std::move(index))
    , value(value) {}

/// if expression we are visiting is `Name` then return new `Integer` node
std::shared_ptr<ast::Expression> IndexRemover::replace_for_name(
    const std::shared_ptr<ast::Expression>& node) const {
    if (node->is_name()) {
        auto name = std::dynamic_pointer_cast<ast::Name>(node);
        if (name->get_node_name() == index) {
            return std::make_shared<ast::Integer>(value, nullptr);
        }
    }
    return node;
}

void IndexRemover::visit_binary_expression(ast::BinaryExpression& node) {
    node.visit_children(*this);
    if (under_indexed_name) {
        /// first recursively replaces children
        /// replace lhs & rhs if they have matching index variable
        auto lhs = replace_for_name(node.get_lhs());
        auto rhs = replace_for_name(node.get_rhs());
        node.set_lhs(std::move(lhs));
        node.set_rhs(std::move(rhs));
    }
}

void IndexRemover::visit_indexed_name(ast::IndexedName& node) {
    under_indexed_name = true;
    node.visit_children(*this);
    /// once all children are replaced, do the same for index
    auto length = replace_for_name(node.get_length());
    node.set_length(std::move(length));
    under_indexed_name = false;
}

}  // namespace visitor
}  // namespace nmodl
