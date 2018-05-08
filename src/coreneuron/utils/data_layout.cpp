#include "coreneuron/utils/data_layout.hpp"
#include "coreneuron/nrnoc/nrnoc_ml.h"
#include "coreneuron/nrniv/node_permute.h"
#include "coreneuron/nrnoc/nrnoc_decl.h"
#include "coreneuron/nrnoc/membfunc.h"

namespace coreneuron {
/*
 * Return the index to mechanism variable based Original input files are organized in AoS
 */
int get_data_index(int node_index, int variable_index, int mtype, Memb_list* ml) {
    int layout = nrn_mech_data_layout_[mtype];
    if (layout == AOS_LAYOUT) {
        return variable_index + node_index * nrn_prop_dparam_size_[mtype];
    }
    assert(layout == SOA_LAYOUT);
    return variable_index * ml->_nodecount_padded + node_index;
}
} //namespace coreneuron
