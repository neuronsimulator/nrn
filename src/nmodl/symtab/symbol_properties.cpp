/*************************************************************************
 * Copyright (C) 2018-2022 Blue Brain Project
 *
 * This file is part of NMODL distributed under the terms of the GNU
 * Lesser General Public License. See top-level LICENSE file for details.
 *************************************************************************/

#include <string>
#include <vector>

#include "symtab/symbol_properties.hpp"
#include "utils/string_utils.hpp"


namespace nmodl {
namespace symtab {
namespace syminfo {


std::vector<std::string> to_string_vector(const NmodlType& obj) {
    std::vector<std::string> properties;

    // do nothing for NmodlType::empty

    if (has_property(obj, NmodlType::local_var)) {
        properties.emplace_back("local");
    }

    if (has_property(obj, NmodlType::global_var)) {
        properties.emplace_back("global");
    }

    if (has_property(obj, NmodlType::range_var)) {
        properties.emplace_back("range");
    }

    if (has_property(obj, NmodlType::param_assign)) {
        properties.emplace_back("parameter");
    }

    if (has_property(obj, NmodlType::pointer_var)) {
        properties.emplace_back("pointer");
    }

    if (has_property(obj, NmodlType::bbcore_pointer_var)) {
        properties.emplace_back("bbcore_pointer");
    }

    if (has_property(obj, NmodlType::extern_var)) {
        properties.emplace_back("extern");
    }

    if (has_property(obj, NmodlType::prime_name)) {
        properties.emplace_back("prime_name");
    }

    if (has_property(obj, NmodlType::assigned_definition)) {
        properties.emplace_back("assigned_definition");
    }

    if (has_property(obj, NmodlType::unit_def)) {
        properties.emplace_back("unit_def");
    }

    if (has_property(obj, NmodlType::read_ion_var)) {
        properties.emplace_back("read_ion");
    }

    if (has_property(obj, NmodlType::write_ion_var)) {
        properties.emplace_back("write_ion");
    }

    if (has_property(obj, NmodlType::nonspecific_cur_var)) {
        properties.emplace_back("nonspecific_cur_var");
    }

    if (has_property(obj, NmodlType::electrode_cur_var)) {
        properties.emplace_back("electrode_cur");
    }

    if (has_property(obj, NmodlType::section_var)) {
        properties.emplace_back("section");
    }

    if (has_property(obj, NmodlType::argument)) {
        properties.emplace_back("argument");
    }

    if (has_property(obj, NmodlType::function_block)) {
        properties.emplace_back("function_block");
    }

    if (has_property(obj, NmodlType::procedure_block)) {
        properties.emplace_back("procedure_block");
    }

    if (has_property(obj, NmodlType::derivative_block)) {
        properties.emplace_back("derivative_block");
    }

    if (has_property(obj, NmodlType::linear_block)) {
        properties.emplace_back("linear_block");
    }

    if (has_property(obj, NmodlType::non_linear_block)) {
        properties.emplace_back("non_linear_block");
    }

    if (has_property(obj, NmodlType::table_statement_var)) {
        properties.emplace_back("table_statement_var");
    }

    if (has_property(obj, NmodlType::table_assigned_var)) {
        properties.emplace_back("table_assigned_var");
    }

    if (has_property(obj, NmodlType::constant_var)) {
        properties.emplace_back("constant");
    }

    if (has_property(obj, NmodlType::partial_block)) {
        properties.emplace_back("partial_block");
    }

    if (has_property(obj, NmodlType::kinetic_block)) {
        properties.emplace_back("kinetic_block");
    }

    if (has_property(obj, NmodlType::function_table_block)) {
        properties.emplace_back("function_table_block");
    }

    if (has_property(obj, NmodlType::factor_def)) {
        properties.emplace_back("factor_def");
    }

    if (has_property(obj, NmodlType::extern_neuron_variable)) {
        properties.emplace_back("extern_neuron_var");
    }

    if (has_property(obj, NmodlType::extern_method)) {
        properties.emplace_back("extern_method");
    }

    if (has_property(obj, NmodlType::state_var)) {
        properties.emplace_back("state_var");
    }

    if (has_property(obj, NmodlType::to_solve)) {
        properties.emplace_back("to_solve");
    }

    if (has_property(obj, NmodlType::useion)) {
        properties.emplace_back("ion");
    }

    if (has_property(obj, NmodlType::discrete_block)) {
        properties.emplace_back("discrete_block");
    }

    if (has_property(obj, NmodlType::define)) {
        properties.emplace_back("define");
    }

    if (has_property(obj, NmodlType::codegen_var)) {
        properties.emplace_back("codegen_var");
    }

    return properties;
}

std::vector<std::string> to_string_vector(const Status& obj) {
    std::vector<std::string> status;

    // do nothing for Status::empty

    if (has_status(obj, Status::localized)) {
        status.emplace_back("localized");
    }

    if (has_status(obj, Status::globalized)) {
        status.emplace_back("globalized");
    }

    if (has_status(obj, Status::inlined)) {
        status.emplace_back("inlined");
    }

    if (has_status(obj, Status::renamed)) {
        status.emplace_back("renamed");
    }

    if (has_status(obj, Status::created)) {
        status.emplace_back("created");
    }

    if (has_status(obj, Status::from_state)) {
        status.emplace_back("from_state");
    }

    if (has_status(obj, Status::thread_safe)) {
        status.emplace_back("thread_safe");
    }

    return status;
}

std::ostream& operator<<(std::ostream& os, const NmodlType& obj) {
    os << to_string(obj);
    return os;
}

std::ostream& operator<<(std::ostream& os, const Status& obj) {
    os << to_string(obj);
    return os;
}

}  // namespace syminfo
}  // namespace symtab
}  // namespace nmodl
