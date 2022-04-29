/*************************************************************************
 * Copyright (C) 2018-2022 Blue Brain Project
 *
 * This file is part of NMODL distributed under the terms of the GNU
 * Lesser General Public License. See top-level LICENSE file for details.
 *************************************************************************/

#include "visitors/indexedname_visitor.hpp"
#include "ast/binary_expression.hpp"
#include "visitors/visitor_utils.hpp"

namespace nmodl {
namespace visitor {

void IndexedNameVisitor::visit_indexed_name(ast::IndexedName& node) {
    indexed_name = nmodl::get_indexed_name(node);
}

void IndexedNameVisitor::visit_diff_eq_expression(ast::DiffEqExpression& node) {
    node.visit_children(*this);
    const auto& bin_exp = std::static_pointer_cast<ast::BinaryExpression>(node.get_expression());
    auto lhs = bin_exp->get_lhs();
    auto rhs = bin_exp->get_rhs();
    dependencies = nmodl::statement_dependencies(lhs, rhs);
}

void IndexedNameVisitor::visit_program(ast::Program& node) {
    node.visit_children(*this);
}
std::pair<std::string, std::unordered_set<std::string>> IndexedNameVisitor::get_dependencies() {
    return dependencies;
}
std::string IndexedNameVisitor::get_indexed_name() {
    return indexed_name;
}

}  // namespace visitor
}  // namespace nmodl
