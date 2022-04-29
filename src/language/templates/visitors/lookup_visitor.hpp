/*************************************************************************
 * Copyright (C) 2018-2022 Blue Brain Project
 *
 * This file is part of NMODL distributed under the terms of the GNU
 * Lesser General Public License. See top-level LICENSE file for details.
 *************************************************************************/

///
/// THIS FILE IS GENERATED AT BUILD TIME AND SHALL NOT BE EDITED.
///

#pragma once

/**
 * \file
 * \brief \copybrief nmodl::visitor::AstLookupVisitor
 */

#include "visitors/visitor.hpp"

namespace nmodl {
namespace visitor {

/**
 * \addtogroup visitor_classes
 * \{
 */

/**
 * \class AstLookupVisitor
 * \brief %Visitor to find AST nodes based on their types
 */
template <typename DefaultVisitor>
class MetaAstLookupVisitor: public DefaultVisitor {
    static const bool is_const_visitor = std::is_same<ConstVisitor, DefaultVisitor>::value;

    template <typename T>
    struct identity {
        using type = T;
    };

    template <typename T>
    using visit_arg_trait =
        typename std::conditional<is_const_visitor, std::add_const<T>, identity<T>>::type;
    using ast_t = typename visit_arg_trait<ast::Ast>::type;
    using nodes_t = std::vector<std::shared_ptr<ast_t>>;

  private:
    /// node types to search in the ast
    std::vector<ast::AstNodeType> types;

    /// matching nodes found in the ast
    std::vector<std::shared_ptr<ast_t>> nodes;

  public:
    MetaAstLookupVisitor() = default;

    MetaAstLookupVisitor(ast::AstNodeType type)
        : types{type} {}

    MetaAstLookupVisitor(const std::vector<ast::AstNodeType>& types)
        : types(types) {}

    const nodes_t& lookup(ast_t& node);

    const nodes_t& lookup(ast_t& node, ast::AstNodeType type);

    const nodes_t& lookup(ast_t& node, const std::vector<ast::AstNodeType>& t_types);

    const nodes_t& get_nodes() const noexcept {
        return nodes;
    }

    void clear() {
        types.clear();
        nodes.clear();
    }

  protected:
    // clang-format off
    {% for node in nodes %}
    void visit_{{ node.class_name|snake_case }}(typename visit_arg_trait<ast::{{ node.class_name }}>::type& node) override;
    {% endfor %}
    // clang-format on
};

using AstLookupVisitor = MetaAstLookupVisitor<Visitor>;
using ConstAstLookupVisitor = MetaAstLookupVisitor<ConstVisitor>;

// explicit template instantiation declarations
extern template class MetaAstLookupVisitor<Visitor>;
extern template class MetaAstLookupVisitor<ConstVisitor>;

/** \} */  // end of visitor_classes

}  // namespace visitor
}  // namespace nmodl

