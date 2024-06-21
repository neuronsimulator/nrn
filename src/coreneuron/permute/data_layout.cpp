/*
# =============================================================================
# Copyright (c) 2016 - 2021 Blue Brain Project/EPFL
#
# See top-level LICENSE file for details.
# =============================================================================
*/

#include "coreneuron/coreneuron.hpp"
#include "coreneuron/permute/data_layout.hpp"
#include "coreneuron/mechanism/mechanism.hpp"
#include "coreneuron/permute/node_permute.h"
#include "coreneuron/mechanism/membfunc.hpp"

namespace coreneuron {
/*
 * Return the index to mechanism variable based Original input files are organized in AoS
 */
int get_data_index(int node_index, int variable_index, int mtype, Memb_list* ml) {
    return variable_index * ml->_nodecount_padded + node_index;
}
}  // namespace coreneuron
