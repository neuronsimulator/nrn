/*
# =============================================================================
# Copyright (c) 2016 - 2021 Blue Brain Project/EPFL
#
# See top-level LICENSE file for details.
# =============================================================================
*/

#include "coreneuron/nrnconf.h"
#include "coreneuron/sim/fast_imem.hpp"
#include "coreneuron/utils/memory.h"
#include "coreneuron/mpi/nrnmpi.h"
#include "coreneuron/utils/nrnoc_aux.hpp"

namespace coreneuron {

extern int nrn_nthread;
extern NrnThread* nrn_threads;
bool nrn_use_fast_imem;

void fast_imem_free() {
    for (auto nt = nrn_threads; nt < nrn_threads + nrn_nthread; ++nt) {
        if (nt->nrn_fast_imem) {
            free(nt->nrn_fast_imem->nrn_sav_rhs);
            free(nt->nrn_fast_imem->nrn_sav_d);
            free(nt->nrn_fast_imem);
            nt->nrn_fast_imem = nullptr;
        }
    }
}

void nrn_fast_imem_alloc() {
    if (nrn_use_fast_imem) {
        fast_imem_free();
        for (auto nt = nrn_threads; nt < nrn_threads + nrn_nthread; ++nt) {
            int n = nt->end;
            nt->nrn_fast_imem = (NrnFastImem*) ecalloc(1, sizeof(NrnFastImem));
            nt->nrn_fast_imem->nrn_sav_rhs = (double*) ecalloc_align(n, sizeof(double));
            nt->nrn_fast_imem->nrn_sav_d = (double*) ecalloc_align(n, sizeof(double));
        }
    }
}

void nrn_calc_fast_imem(NrnThread* nt) {
    int i1 = 0;
    int i3 = nt->end;

    double* vec_rhs = nt->_actual_rhs;
    double* vec_area = nt->_actual_area;

    double* fast_imem_d = nt->nrn_fast_imem->nrn_sav_d;
    double* fast_imem_rhs = nt->nrn_fast_imem->nrn_sav_rhs;
#pragma acc parallel loop present(vec_rhs,     \
                                  vec_area,    \
                                  fast_imem_d, \
                                  fast_imem_rhs) if (nt->compute_gpu) async(nt->stream_id)
    for (int i = i1; i < i3; ++i) {
        fast_imem_rhs[i] = (fast_imem_d[i] * vec_rhs[i] + fast_imem_rhs[i]) * vec_area[i] * 0.01;
    }
}

}  // namespace coreneuron
