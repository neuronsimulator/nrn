#include <vector>
#include <string>

#include "utils/string_utils.hpp"
#include "symtab/symbol_properties.hpp"

using namespace symtab::details;

bool has_property(const SymbolInfo& obj, NmodlInfo property) {
    return static_cast<bool>(obj & property);
}

std::vector<std::string> to_string_vector(const SymbolInfo& obj) {
    std::vector<std::string> properties;

    if (has_property(obj, NmodlInfo::local_var)) {
        properties.push_back("local");
    }

    if (has_property(obj, NmodlInfo::global_var)) {
        properties.push_back("global");
    }

    if (has_property(obj, NmodlInfo::range_var)) {
        properties.push_back("range");
    }

    if (has_property(obj, NmodlInfo::param_assign)) {
        properties.push_back("parameter");
    }

    if (has_property(obj, NmodlInfo::pointer_var)) {
        properties.push_back("pointer");
    }

    if (has_property(obj, NmodlInfo::bbcore_pointer_var)) {
        properties.push_back("bbcore_pointer");
    }

    if (has_property(obj, NmodlInfo::extern_var)) {
        properties.push_back("extern");
    }

    if (has_property(obj, NmodlInfo::prime_name)) {
        properties.push_back("prime_name");
    }

    if (has_property(obj, NmodlInfo::dependent_def)) {
        properties.push_back("dependent_def");
    }

    if (has_property(obj, NmodlInfo::unit_def)) {
        properties.push_back("unit_def");
    }

    if (has_property(obj, NmodlInfo::read_ion_var)) {
        properties.push_back("read_ion");
    }

    if (has_property(obj, NmodlInfo::write_ion_var)) {
        properties.push_back("write_ion");
    }

    if (has_property(obj, NmodlInfo::nonspe_cur_var)) {
        properties.push_back("nonspe_cur");
    }

    if (has_property(obj, NmodlInfo::electrode_cur_var)) {
        properties.push_back("electrode_cur");
    }

    if (has_property(obj, NmodlInfo::section_var)) {
        properties.push_back("section");
    }

    if (has_property(obj, NmodlInfo::argument)) {
        properties.push_back("argument");
    }

    if (has_property(obj, NmodlInfo::function_block)) {
        properties.push_back("function_block");
    }

    if (has_property(obj, NmodlInfo::procedure_block)) {
        properties.push_back("procedure_block");
    }

    if (has_property(obj, NmodlInfo::derivative_block)) {
        properties.push_back("derivative_block");
    }

    if (has_property(obj, NmodlInfo::linear_block)) {
        properties.push_back("linear_block");
    }

    if (has_property(obj, NmodlInfo::non_linear_block)) {
        properties.push_back("non_linear_block");
    }

    if (has_property(obj, NmodlInfo::discrete_block)) {
        properties.push_back("discrete_block");
    }

    if (has_property(obj, NmodlInfo::partial_block)) {
        properties.push_back("partial_block");
    }

    if (has_property(obj, NmodlInfo::kinetic_block)) {
        properties.push_back("kinetic_block");
    }

    if (has_property(obj, NmodlInfo::function_table_block)) {
        properties.push_back("function_table_block");
    }

    if (has_property(obj, NmodlInfo::factor_def)) {
        properties.push_back("factor_def");
    }

    if (has_property(obj, NmodlInfo::extern_token)) {
        properties.push_back("extern_token");
    }

    return properties;
}

std::ostream& operator<<(std::ostream& os, const SymbolInfo& obj) {
    os << to_string(obj);
    return os;
}

std::string to_string(const SymbolInfo& obj) {
    auto properties = to_string_vector(obj);
    std::string text;
    for (const auto& property : properties) {
        text += property + " ";
    }

    // remove extra whitespace at the end
    stringutils::trim(text);
    return text;
}
