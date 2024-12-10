/*
 * Copyright 2023 Blue Brain Project, EPFL.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "symtab/symbol.hpp"
#include "utils/logger.hpp"
#include <ast/ast.hpp>

namespace nmodl {
namespace symtab {

using syminfo::NmodlType;
using syminfo::Status;


bool Symbol::is_variable() const noexcept {
    // if symbol has one of the following property then it
    // is considered as variable in the NMODL
    // clang-format off
        NmodlType var_properties = NmodlType::local_var
                                    | NmodlType::global_var
                                    | NmodlType::range_var
                                    | NmodlType::param_assign
                                    | NmodlType::pointer_var
                                    | NmodlType::bbcore_pointer_var
                                    | NmodlType::extern_var
                                    | NmodlType::assigned_definition
                                    | NmodlType::read_ion_var
                                    | NmodlType::write_ion_var
                                    | NmodlType::nonspecific_cur_var
                                    | NmodlType::electrode_cur_var
                                    | NmodlType::argument
                                    | NmodlType::extern_neuron_variable;
    // clang-format on
    return has_any_property(var_properties);
}

std::string Symbol::to_string() const {
    std::string s(name);
    if (properties != NmodlType::empty) {
        s += fmt::format(" [Properties : {}]", syminfo::to_string(properties));
    }
    if (status != Status::empty) {
        s += fmt::format(" [Status : {}]", syminfo::to_string(status));
    }
    return s;
}

std::vector<ast::Ast*> Symbol::get_nodes_by_type(
    std::initializer_list<ast::AstNodeType> l) const noexcept {
    std::vector<ast::Ast*> _nodes;
    for (const auto& n: nodes) {
        for (const auto& m: l) {
            if (n->get_node_type() == m) {
                _nodes.push_back(n);
                break;
            }
        }
    }
    return _nodes;
}

}  // namespace symtab
}  // namespace nmodl
