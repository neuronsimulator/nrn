/*
# =============================================================================
# Copyright (C) 2016-2021 Blue Brain Project
#
# See top-level LICENSE file for details.
# =============================================================================
*/

#include "coreneuron/coreneuron.hpp"
#include "coreneuron/io/nrn2core_direct.h"
#include "coreneuron/sim/multicore.hpp"
#include "coreneuron/nrniv/nrniv_decl.h"
#include "coreneuron/io/core2nrn_data_return.hpp"

/** @brief, Information from NEURON to help with copying data to NEURON.
 *  Info for copying voltage, i_membrane_, and mechanism data.
 *  See implementaton in
 *  nrn/src/nrniv/nrnbbcore_write.cpp:nrnthreads_type_return.
 *  Return is size of either the returned data pointer or the number
 *  of pointers in mdata. tid is the thread index.
 */
size_t (*nrn2core_type_return_)(int type, int tid, double*& data, double**& mdata);

namespace coreneuron {

/** @brief permuted array copied to unpermuted array
 *  If permute is NULL then just a copy
 */
static void inverse_permute_copy(size_t n, double* permuted_src, double* dest, int* permute) {
    if (permute) {
        for (size_t i = 0; i < n; ++i) {
            dest[i] = permuted_src[permute[i]];
        }
    } else {
        std::copy(permuted_src, permuted_src + n, dest);
    }
}

/** @brief SoA permuted mechanism data copied to unpermuted AoS data.
 *  dest is an array of n pointers to the beginning of each sz length array.
 *  src is a contiguous array of sz segments of size stride. The stride
 *  may be slightly greater than n for purposes of alignment.
 *  Each of the sz segments of src are permuted.
 */
static void soa2aos_inverse_permute_copy(size_t n,
                                         int sz,
                                         int stride,
                                         double* src,
                                         double** dest,
                                         int* permute) {
    // src is soa and permuted. dest is n pointers to sz doubles (aos).
    for (size_t instance = 0; instance < n; ++instance) {
        double* d = dest[instance];
        double* s = src + permute[instance];
        for (int i = 0; i < sz; ++i) {
            d[i] = s[i * stride];
        }
    }
}

/** @brief SoA unpermuted mechanism data copied to unpermuted AoS data.
 *  dest is an array of n pointers to the beginning of each sz length array.
 *  src is a contiguous array of sz segments of size stride. The stride
 *  may be slightly greater than n for purposes of alignment.
 *  Each of the sz segments of src have the same order as the n pointers
 *  of dest.
 */
static void soa2aos_unpermuted_copy(size_t n, int sz, int stride, double* src, double** dest) {
    // src is soa and permuted. dest is n pointers to sz doubles (aos).
    for (size_t instance = 0; instance < n; ++instance) {
        double* d = dest[instance];
        double* s = src + instance;
        for (int i = 0; i < sz; ++i) {
            d[i] = s[i * stride];
        }
    }
}

/** @brief AoS mechanism data copied to AoS data.
 *  dest is an array of n pointers to the beginning of each sz length array.
 *  src is a contiguous array of n segments of size sz.
 */
static void aos2aos_copy(size_t n, int sz, double* src, double** dest) {
    for (size_t instance = 0; instance < n; ++instance) {
        double* d = dest[instance];
        double* s = src + (instance * sz);
        std::copy(s, s + sz, d);
    }
}

/** @brief copy data back to NEURON.
 *  Copies t, voltage, i_membrane_ if it used, and mechanism param data.
 */
void core2nrn_data_return() {
    if (!nrn2core_type_return_) {
        return;
    }
    for (int tid = 0; tid < nrn_nthread; ++tid) {
        size_t n = 0;
        double* data = nullptr;
        double** mdata = nullptr;
        NrnThread& nt = nrn_threads[tid];

        n = (*nrn2core_type_return_)(0, tid, data, mdata);  // 0 means time
        if (n) {                                            // not the empty thread
            data[0] = nt._t;
        }

        if (nt.end) {  // transfer voltage and possibly i_membrane_
            n = (*nrn2core_type_return_)(voltage, tid, data, mdata);
            assert(n == size_t(nt.end) && data);
            inverse_permute_copy(n, nt._actual_v, data, nt._permute);

            if (nt.nrn_fast_imem) {
                n = (*nrn2core_type_return_)(i_membrane_, tid, data, mdata);
                assert(n == size_t(nt.end) && data);
                inverse_permute_copy(n, nt.nrn_fast_imem->nrn_sav_rhs, data, nt._permute);
            }
        }

        for (NrnThreadMembList* tml = nt.tml; tml; tml = tml->next) {
            int mtype = tml->index;
            Memb_list* ml = tml->ml;
            n = (*nrn2core_type_return_)(mtype, tid, data, mdata);
            assert(n == size_t(ml->nodecount) && mdata);
            if (n == 0) {
                continue;
            }
            // NEURON is AoS, CoreNEURON may be SoA and may be permuted.
            // On the NEURON side, the data is actually contiguous because of
            // cache_efficient, but that may not be the case for ARTIFICIAL_CELL.
            // For initial implementation simplicity, use the mdata info which gives
            // a double* for each param_size mech instance.
            int* permute = ml->_permute;
            double* cndat = ml->data;
            int layout = corenrn.get_mech_data_layout()[mtype];
            int sz = corenrn.get_prop_param_size()[mtype];
            if (layout == Layout::SoA) {
                int stride = ml->_nodecount_padded;
                if (permute) {
                    soa2aos_inverse_permute_copy(n, sz, stride, cndat, mdata, permute);
                } else {
                    soa2aos_unpermuted_copy(n, sz, stride, cndat, mdata);
                }
            } else { /* AoS */
                aos2aos_copy(n, sz, cndat, mdata);
            }
        }
    }
}

}  // namespace coreneuron
