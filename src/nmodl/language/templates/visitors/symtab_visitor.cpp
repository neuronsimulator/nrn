/*************************************************************************
 * Copyright (C) 2018-2022 Blue Brain Project
 *
 * This file is part of NMODL distributed under the terms of the GNU
 * Lesser General Public License. See top-level LICENSE file for details.
 *************************************************************************/

///
/// THIS FILE IS GENERATED AT BUILD TIME AND SHALL NOT BE EDITED.
///

#include "symtab/symbol_table.hpp"
#include "visitors/symtab_visitor.hpp"
#include "visitors/symtab_visitor_helper.hpp"


namespace nmodl {
namespace visitor {

using namespace ast;
using symtab::syminfo::NmodlType;

{% for node in nodes %}
{% if node.is_symtab_method_required and not node.is_symbol_helper_node %}
{% set typename = node.class_name|snake_case %}
{% set propname = "NmodlType::" + typename %}
void SymtabVisitor::visit_{{ typename }}({{ node.class_name }}& node) {
    {% if node.is_symbol_var_node %}
    setup_symbol(&node, {{ propname }});
    {% elif node.is_program_node %}
    setup_symbol_table_for_program_block(&node);
    {% elif node.is_global_block_node %}
    setup_symbol_table_for_global_block(&node);
    {% elif node.is_symbol_block_node %}
    add_model_symbol_with_property(&node, {{ propname }});
    setup_symbol_table_for_scoped_block(&node, node.get_node_name());
    {% else %}
    setup_symbol_table_for_scoped_block(&node, node.get_node_type_name());
    {% endif %}
}

{% endif %}
{% endfor %}

}  // namespace visitor
}  // namespace nmodl

