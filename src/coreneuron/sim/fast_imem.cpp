/*
Copyright (c) 2019, Blue Brain Project
All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors
   may be used to endorse or promote products derived from this software
   without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "coreneuron/nrnconf.h"
#include "coreneuron/sim/fast_imem.hpp"
#include "coreneuron/utils/memory.h"
#include "coreneuron/mpi/nrnmpi.h"
#include "coreneuron/utils/nrnoc_aux.hpp"

namespace coreneuron {

extern int nrn_nthread;
extern NrnThread *nrn_threads;
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
            nt->nrn_fast_imem = (NrnFastImem*)ecalloc(1, sizeof(NrnFastImem));
            nt->nrn_fast_imem->nrn_sav_rhs = (double*)emalloc_align(n * sizeof(double));
            nt->nrn_fast_imem->nrn_sav_d = (double*)emalloc_align(n * sizeof(double));
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
    for (int i = i1; i < i3 ; ++i) {
        fast_imem_rhs[i] = (fast_imem_d[i]*vec_rhs[i] + fast_imem_rhs[i])*vec_area[i]*0.01;
    }
}

}

