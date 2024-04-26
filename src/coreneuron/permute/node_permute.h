/*
# =============================================================================
# Copyright (c) 2016 - 2021 Blue Brain Project/EPFL
#
# See top-level LICENSE file for details.
# =============================================================================
*/

#pragma once

#if CORENRN_BUILD
#include "coreneuron/sim/multicore.hpp"
#else
#include "nrnoc/multicore.h"
#endif

#if CORENRN_BUILD
namespace coreneuron {
#else
namespace neuron {
#endif
// determine ml->_permute and permute the ml->nodeindices accordingly
void permute_nodeindices(Memb_list* ml, int* permute);

#if CORENRN_BUILD
// vec values >= 0 updated according to permutation
void node_permute(int* vec, int n, int* permute);
#else   // not CORENRN_BUILD
// sort ml fields in increasing node index order
void sort_ml(Memb_list* ml);

/*
 * Update indices vector so that the parent-child relation is kept valid
 * after applying a permutation to the vector `vec`.
 * This is done by updating vec values >= 0 according to the permutation.
 */
void update_parent_index(int* vec, int vec_size, const std::vector<int>& permute);
#endif  // not CORENRN_BUILD

// moves values to new location but does not change those values
void permute_ptr(int* vec, int n, int* permute);

void permute_data(double* vec, int n, int* permute);
void permute_ml(Memb_list* ml, int type, NrnThread& nt);

int* inverse_permute(int* p, int n);

int type_of_ntdata(NrnThread&, int index, bool reset);
}  // namespace coreneuron
