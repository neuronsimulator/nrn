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
#include "coreneuron/utils/utils.hpp"

namespace coreneuron {
using Offset = size_t;
using MechId = int;
using VariableName = const std::string_view;


/*
 * Structure that map variable names of mechanisms to their value's location (offset) in memory
 */
using MechNamesMapping = std::map<MechId, std::map<VariableName, Offset>>;
static MechNamesMapping mechNamesMapping;

double* get_var_location_from_var_name(int mech_id,
                                       const std::string_view mech_name,
                                       const std::string_view variable_name,
                                       Memb_list* ml,
                                       int node_index) {
    const auto mech_it = mechNamesMapping.find(mech_id);
    if (mech_it == mechNamesMapping.end()) {
        std::cerr << "No variable name mapping exists for mechanism id: " << mech_id << std::endl;
        nrn_abort(1);
    }

    const auto& mech = mech_it->second;
    auto offset_it = mech.find(variable_name);
    if (offset_it == mech.end()) {
        // Try fallback with variable_name + "_" + mech_name. Used necessary for i_pas
        std::string fallback_name = std::string(variable_name) + "_" + std::string(mech_name);
        offset_it = mech.find(fallback_name);

        if (offset_it == mech.end()) {
            std::cerr << "No value associated to variable name: '" << variable_name
                      << "' or fallback '" << fallback_name << "'";
            nrn_abort(1);
        }
    }

    const int ix = get_data_index(node_index, offset_it->second, mech_id, ml);
    return &(ml->data[ix]);
}

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
