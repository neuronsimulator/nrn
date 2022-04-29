/*************************************************************************
 * Copyright (C) 2018-2022 Blue Brain Project
 *
 * This file is part of NMODL distributed under the terms of the GNU
 * Lesser General Public License. See top-level LICENSE file for details.
 *************************************************************************/

///
/// THIS FILE IS GENERATED AT BUILD TIME AND SHALL NOT BE EDITED.
///

#include "visitors/lookup_visitor.hpp"

#include <algorithm>

#include "ast/all.hpp"


namespace nmodl {
namespace visitor {

using namespace ast;

{% for node in nodes %}
template <typename DefaultVisitor>
void MetaAstLookupVisitor<DefaultVisitor>::visit_{{ node.class_name|snake_case }}(typename visit_arg_trait<{{ node.class_name }}>::type& node) {
    const auto type = node.get_node_type();
    if (std::find(types.begin(), types.end(), type) != types.end()) {
        nodes.push_back(node.get_shared_ptr());
    }
    node.visit_children(*this);
}
{% endfor %}

template <typename DefaultVisitor>
const typename MetaAstLookupVisitor<DefaultVisitor>::nodes_t&
MetaAstLookupVisitor<DefaultVisitor>::lookup(typename MetaAstLookupVisitor<DefaultVisitor>::ast_t& node,
                                             const std::vector<AstNodeType>& t_types) {
    clear();
    this->types = t_types;
    node.accept(*this);
    return nodes;
}

template <typename DefaultVisitor>
const typename MetaAstLookupVisitor<DefaultVisitor>::nodes_t&
MetaAstLookupVisitor<DefaultVisitor>::lookup(typename MetaAstLookupVisitor<DefaultVisitor>::ast_t& node,
                                             AstNodeType type) {
    clear();
    this->types.push_back(type);
    node.accept(*this);
    return nodes;
}

template <typename DefaultVisitor>
const typename MetaAstLookupVisitor<DefaultVisitor>::nodes_t&
MetaAstLookupVisitor<DefaultVisitor>::lookup(typename MetaAstLookupVisitor<DefaultVisitor>::ast_t& node) {
    nodes.clear();
    node.accept(*this);
    return nodes;
}

// explicit template instantiation definitions
template class MetaAstLookupVisitor<Visitor>;
template class MetaAstLookupVisitor<ConstVisitor>;

}  // namespace visitor
}  // namespace nmodl
