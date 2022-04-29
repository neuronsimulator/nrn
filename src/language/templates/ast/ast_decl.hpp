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

#include <memory>
#include <vector>

/// \file
/// \brief Auto generated  AST node types and aliases declaration

namespace nmodl {
namespace ast {

/// forward declaration of ast nodes

struct Ast;
{% for node in nodes %}
class {{ node.class_name }};
{% endfor %}

/**
 * @defgroup ast_type AST Node Types
 * @ingroup ast
 * @brief Enum node types for all ast nodes
 * @{
 */

/**
 * \brief Enum type for every AST node type
 *
 * Every node in the ast has associated type represented by this
 * enum class.
 *
 * \sa ast::Ast::get_node_type ast::Ast::get_node_type_name
 */
enum class AstNodeType {
    {% for node in nodes %}
    {{ node.class_name|snake_case|upper }}, ///< type of ast::{{ node.class_name }}
    {% endfor %}
};

/** @} */  // end of ast_type

/**
 * @defgroup ast_vec_type AST Vector Type Aliases
 * @ingroup ast
 * @brief Vector type alias for AST node
 * @{
 */
{% for node in nodes %}
using {{ node.class_name }}Vector = std::vector<std::shared_ptr<{{ node.class_name }}>>;
{% endfor %}

/** @} */  // end of ast_vec_type

}  // namespace ast
}  // namespace nmodl

