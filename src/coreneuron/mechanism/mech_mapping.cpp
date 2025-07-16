/*
# =============================================================================
# Copyright (c) 2016 - 2021 Blue Brain Project/EPFL
#
# See top-level LICENSE file for details.
# =============================================================================
*/

#include <cstring>
#include <cstdlib>
#include <iostream>
#include <map>

#include "coreneuron/mechanism/mech_mapping.hpp"
#include "coreneuron/mechanism/mechanism.hpp"
#include "coreneuron/permute/data_layout.hpp"

namespace coreneuron {
using Offset = size_t;
using MechId = int;
using VariableName = const char*;

struct cmp_str {
    bool operator()(char const* a, char const* b) const {
        return std::strcmp(a, b) < 0;
    }
};

/*
 * Structure that map variable names of mechanisms to their value's location (offset) in memory
 */
using MechNamesMapping = std::map<MechId, std::map<VariableName, Offset, cmp_str>>;
static MechNamesMapping mechNamesMapping;

double* get_var_location_from_var_name(int mech_id,
                                        const std::string_view mech_name,
                                       const char* variable_name,
                                       Memb_list* ml,
                                       int node_index) {
    const auto mech_it = mechNamesMapping.find(mech_id);
    if (mech_it == mechNamesMapping.end()) {
        std::cerr << "DEBUG: mech_id " << mech_id << " not found.\n";
        std::cerr << "DEBUG: Current MechNamesMapping keys:\n";
        for (const auto& kv : mechNamesMapping) {
            std::cerr << "  mech_id: " << kv.first << "\n";
        }
        throw std::runtime_error("No variable name mapping exists for mechanism id: " + std::to_string(mech_id));
    }

    const auto& mech = mech_it->second;
    auto offset_it = mech.find(variable_name);
    if (offset_it == mech.end()) {
        std::cerr << "DEBUG: variable_name '" << variable_name << "' not found for mech_id " << mech_id << ".\n";
        std::cerr << "DEBUG: Available variable names for mech_id " << mech_id << ":\n";
        for (const auto& var_kv : mech) {
            std::cerr << "  variable_name: '" << var_kv.first << "', offset: " << var_kv.second << "\n";
        }
        throw std::runtime_error(std::string("No value associated to variable name: '") + variable_name + "'");
    }

    const int ix = get_data_index(node_index, offset_it->second, mech_id, ml);
    return &(ml->data[ix]);
}

// double* get_var_location_from_var_name(int mech_id,
//                                        const char* variable_name,
//                                        Memb_list* ml,
//                                        int node_index) {
//     const auto mech_it = mechNamesMapping.find(mech_id);
//     if (mech_it == mechNamesMapping.end()) {
//         throw std::runtime_error("No variable name mapping exists for mechanism id: " + std::to_string(mech_id));
//     }
//     const auto& mech = mech_it->second;
//     auto offset_it = mech.find(variable_name);
//     if (offset_it == mech.end()) {
//         throw std::runtime_error(std::string("No value associated to variable name: '") +
//                             variable_name + "'");
//     }

//     const int ix = get_data_index(node_index, offset_it->second, mech_id, ml);
//     return &(ml->data[ix]);
// }


void register_all_variables_offsets(int mech_id, SerializedNames variable_names) {
    int idx = 0;
    int nb_parsed_variables = 0;
    int current_categorie = 1;
    while (current_categorie < NB_MECH_VAR_CATEGORIES) {
        if (variable_names[idx]) {
                        mechNamesMapping[mech_id][variable_names[idx]] = nb_parsed_variables;
            nb_parsed_variables++;
        } else {
            current_categorie++;
        }
        idx++;
    }
    idx++;
}

}  // namespace coreneuron
