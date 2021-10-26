/*
# =============================================================================
# Copyright (c) 2016 - 2021 Blue Brain Project/EPFL
#
# See top-level LICENSE file for details.
# =============================================================================
*/

#ifndef node_permute_h
#define node_permute_h

#include "coreneuron/sim/multicore.hpp"

namespace coreneuron {
// determine ml->_permute and permute the ml->nodeindices accordingly
void permute_nodeindices(Memb_list* ml, int* permute);

// vec values >= 0 updated according to permutation
void node_permute(int* vec, int n, int* permute);

// moves values to new location but does not change those values
void permute_ptr(int* vec, int n, int* permute);

void permute_data(double* vec, int n, int* permute);
void permute_ml(Memb_list* ml, int type, NrnThread& nt);
int nrn_index_permute(int, int type, Memb_list* ml);

int* inverse_permute(int* p, int n);
}  // namespace coreneuron
#endif
