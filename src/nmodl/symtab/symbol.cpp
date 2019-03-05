/*************************************************************************
 * Copyright (C) 2018-2019 Blue Brain Project
 *
 * This file is part of NMODL distributed under the terms of the GNU
 * Lesser General Public License. See top-level LICENSE file for details.
 *************************************************************************/

#include "symtab/symbol.hpp"
#include "fmt/format.h"


using namespace fmt::literals;

namespace nmodl {
namespace symtab {

using syminfo::NmodlType;
using syminfo::Status;

/** if symbol has only extern_token property : this is true
 *  for symbols which are external variables from neuron
 *  like v, t, dt etc.
 * \todo: check if we should check two properties using has_property
 *        instead of exact comparisons
 */
bool Symbol::is_external_symbol_only() {
    return (properties == NmodlType::extern_neuron_variable ||
            properties == NmodlType::extern_method);
}

/// check if symbol has any of the common properties

bool Symbol::has_properties(NmodlType new_properties) {
    return static_cast<bool>(properties & new_properties);
}

/// check if symbol has all of the given properties
bool Symbol::has_all_properties(NmodlType new_properties) {
    return ((properties & new_properties) == new_properties);
}


/// check if symbol has any of the status
bool Symbol::has_any_status(Status new_status) {
    return static_cast<bool>(status & new_status);
}

/// check if symbol has all of the status
bool Symbol::has_all_status(Status new_status) {
    return ((status & new_status) == new_status);
}

/// add new properties to symbol with bitwise or
void Symbol::add_properties(NmodlType new_properties) {
    properties |= new_properties;
}

void Symbol::add_property(NmodlType property) {
    properties |= property;
}

/** Prime variable will appear in different block and could have
 *  multiple derivative orders. We have to store highest order.
 *
 * \todo Check if analysis passes need more information.
 */
void Symbol::set_order(int new_order) {
    if (new_order > order) {
        order = new_order;
    }
}

/// check if symbol is of variable type in nmodl
bool Symbol::is_variable() {
    // clang-format off
        NmodlType var_properties = NmodlType::local_var
                                    | NmodlType::global_var
                                    | NmodlType::range_var
                                    | NmodlType::param_assign
                                    | NmodlType::pointer_var
                                    | NmodlType::bbcore_pointer_var
                                    | NmodlType::extern_var
                                    | NmodlType::dependent_def
                                    | NmodlType::read_ion_var
                                    | NmodlType::write_ion_var
                                    | NmodlType::nonspecific_cur_var
                                    | NmodlType::electrode_cur_var
                                    | NmodlType::section_var
                                    | NmodlType::argument
                                    | NmodlType::extern_neuron_variable;
    // clang-format on
    return has_properties(var_properties);
}

std::string Symbol::to_string() {
    std::string s(name);
    if (properties != NmodlType::empty) {
        s += " [Properties : {}]"_format(syminfo::to_string(properties));
    }
    if (status != Status::empty) {
        s += " [Status : {}]"_format(syminfo::to_string(status));
    }
    return s;
}

}  // namespace symtab
}  // namespace nmodl
