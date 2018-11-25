#include <vector>
#include <string>

#include "utils/string_utils.hpp"
#include "symtab/symbol_properties.hpp"

using namespace symtab::details;

/// check if any property is set
bool has_property(const SymbolInfo& obj, NmodlInfo property) {
    return static_cast<bool>(obj & property);
}

bool has_status(const SymbolStatus& obj, Status state) {
    return static_cast<bool>(obj & state);
}

/// helper function to convert properties to string
std::vector<std::string> to_string_vector(const SymbolInfo& obj) {
    std::vector<std::string> properties;

    if (has_property(obj, NmodlInfo::local_var)) {
        properties.emplace_back("local");
    }

    if (has_property(obj, NmodlInfo::global_var)) {
        properties.emplace_back("global");
    }

    if (has_property(obj, NmodlInfo::range_var)) {
        properties.emplace_back("range");
    }

    if (has_property(obj, NmodlInfo::param_assign)) {
        properties.emplace_back("parameter");
    }

    if (has_property(obj, NmodlInfo::pointer_var)) {
        properties.emplace_back("pointer");
    }

    if (has_property(obj, NmodlInfo::bbcore_pointer_var)) {
        properties.emplace_back("bbcore_pointer");
    }

    if (has_property(obj, NmodlInfo::extern_var)) {
        properties.emplace_back("extern");
    }

    if (has_property(obj, NmodlInfo::prime_name)) {
        properties.emplace_back("prime_name");
    }

    if (has_property(obj, NmodlInfo::dependent_def)) {
        properties.emplace_back("dependent_def");
    }

    if (has_property(obj, NmodlInfo::unit_def)) {
        properties.emplace_back("unit_def");
    }

    if (has_property(obj, NmodlInfo::read_ion_var)) {
        properties.emplace_back("read_ion");
    }

    if (has_property(obj, NmodlInfo::write_ion_var)) {
        properties.emplace_back("write_ion");
    }

    if (has_property(obj, NmodlInfo::nonspe_cur_var)) {
        properties.emplace_back("nonspe_cur");
    }

    if (has_property(obj, NmodlInfo::electrode_cur_var)) {
        properties.emplace_back("electrode_cur");
    }

    if (has_property(obj, NmodlInfo::section_var)) {
        properties.emplace_back("section");
    }

    if (has_property(obj, NmodlInfo::argument)) {
        properties.emplace_back("argument");
    }

    if (has_property(obj, NmodlInfo::function_block)) {
        properties.emplace_back("function_block");
    }

    if (has_property(obj, NmodlInfo::procedure_block)) {
        properties.emplace_back("procedure_block");
    }

    if (has_property(obj, NmodlInfo::derivative_block)) {
        properties.emplace_back("derivative_block");
    }

    if (has_property(obj, NmodlInfo::linear_block)) {
        properties.emplace_back("linear_block");
    }

    if (has_property(obj, NmodlInfo::non_linear_block)) {
        properties.emplace_back("non_linear_block");
    }

    /** todo: temporarily disabled due to property limit in enum
    if (has_property(obj, NmodlInfo::discrete_block)) {
        properties.emplace_back("discrete_block");
    }
     */

    if (has_property(obj, NmodlInfo::constant_var)) {
        properties.emplace_back("constant");
    }

    if (has_property(obj, NmodlInfo::partial_block)) {
        properties.emplace_back("partial_block");
    }

    if (has_property(obj, NmodlInfo::kinetic_block)) {
        properties.emplace_back("kinetic_block");
    }

    if (has_property(obj, NmodlInfo::function_table_block)) {
        properties.emplace_back("function_table_block");
    }

    if (has_property(obj, NmodlInfo::factor_def)) {
        properties.emplace_back("factor_def");
    }

    if (has_property(obj, NmodlInfo::extern_neuron_variable)) {
        properties.emplace_back("extern_neuron_var");
    }

    if (has_property(obj, NmodlInfo::extern_method)) {
        properties.emplace_back("extern_method");
    }

    if (has_property(obj, NmodlInfo::state_var)) {
        properties.emplace_back("state_var");
    }

    if (has_property(obj, NmodlInfo::to_solve)) {
        properties.emplace_back("to_solve");
    }

    if (has_property(obj, NmodlInfo::useion)) {
        properties.emplace_back("ion");
    }

    if (has_property(obj, NmodlInfo::thread_safe)) {
        properties.emplace_back("thread_safe");
    }

    return properties;
}

std::vector<std::string> to_string_vector(const SymbolStatus& obj) {
    std::vector<std::string> status;

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
    return status;
}

std::ostream& operator<<(std::ostream& os, const SymbolInfo& obj) {
    os << to_string(obj);
    return os;
}


std::ostream& operator<<(std::ostream& os, const SymbolStatus& obj) {
    os << to_string(obj);
    return os;
}
