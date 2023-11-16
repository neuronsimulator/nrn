/*
# =============================================================================
# Copyright (c) 2016 - 2021 Blue Brain Project/EPFL
#
# See top-level LICENSE file for details.
# =============================================================================
*/

#pragma once

#include "nrnoc/multicore.h"

namespace neuron {
// determine ml->_permute and permute the ml->nodeindices accordingly
void permute_nodeindices(Memb_list* ml, int* permute);

// sort ml fields in increasing node index order
void sort_ml(Memb_list* ml);

// vec values >= 0 updated according to permutation
void node_permute(int* vec, int n, int* permute);

// moves values to new location but does not change those values
void permute_ptr(int* vec, int n, int* permute);

void permute_data(double* vec, int n, int* permute);
void permute_ml(Memb_list* ml, int type, NrnThread& nt);
int nrn_index_permute(int, int type, Memb_list* ml);

int type_of_ntdata(NrnThread&, int index, bool reset);
}  // namespace neuron
