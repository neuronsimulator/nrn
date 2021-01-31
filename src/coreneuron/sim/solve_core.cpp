/*
# =============================================================================
# Copyright (c) 2016 - 2021 Blue Brain Project/EPFL
#
# See top-level LICENSE file for details.
# =============================================================================.
*/

#include "coreneuron/nrnconf.h"
#include "coreneuron/permute/cellorder.hpp"
#include "coreneuron/sim/multicore.hpp"
namespace coreneuron {
bool use_solve_interleave;

static void triang(NrnThread*), bksub(NrnThread*);

/* solve the matrix equation */
void nrn_solve_minimal(NrnThread* _nt) {
    if (use_solve_interleave) {
        solve_interleaved(_nt->id);
    } else {
        triang(_nt);
        bksub(_nt);
    }
}

/** TODO loops are executed seq in OpenACC just for debugging, remove it! */

/* triangularization of the matrix equations */
static void triang(NrnThread* _nt) {
    int i2 = _nt->ncell;
    int i3 = _nt->end;

    double* vec_a = &(VEC_A(0));
    double* vec_b = &(VEC_B(0));
    double* vec_d = &(VEC_D(0));
    double* vec_rhs = &(VEC_RHS(0));
    int* parent_index = _nt->_v_parent_index;

#if defined(_OPENACC)
    int stream_id = _nt->stream_id;
#endif
    /** @todo: just for benchmarking, otherwise produces wrong results */
    // clang-format off

    #pragma acc parallel loop seq present(      \
        vec_a[0:i3], vec_b[0:i3], vec_d[0:i3],  \
        vec_rhs[0:i3], parent_index[0:i3])      \
        async(stream_id) if (_nt->compute_gpu)
    // clang-format on
    for (int i = i3 - 1; i >= i2; --i) {
        double p = vec_a[i] / vec_d[i];
        vec_d[parent_index[i]] -= p * vec_b[i];
        vec_rhs[parent_index[i]] -= p * vec_rhs[i];
    }
}

/* back substitution to finish solving the matrix equations */
static void bksub(NrnThread* _nt) {
    int i1 = 0;
    int i2 = i1 + _nt->ncell;
    int i3 = _nt->end;

    double* vec_b = &(VEC_B(0));
    double* vec_d = &(VEC_D(0));
    double* vec_rhs = &(VEC_RHS(0));
    int* parent_index = _nt->_v_parent_index;

#if defined(_OPENACC)
    int stream_id = _nt->stream_id;
#endif
    /** @todo: just for benchmarking, otherwise produces wrong results */
    // clang-format off

    #pragma acc parallel loop seq present(      \
        vec_d[0:i2], vec_rhs[0:i2])             \
        async(stream_id) if (_nt->compute_gpu)
    // clang-format on
    for (int i = i1; i < i2; ++i) {
        vec_rhs[i] /= vec_d[i];
    }

    /** @todo: just for benchmarking, otherwise produces wrong results */
    // clang-format off

    #pragma acc parallel loop seq present(          \
        vec_b[0:i3], vec_d[0:i3], vec_rhs[0:i3],    \
        parent_index[0:i3]) async(stream_id)        \
        if (_nt->compute_gpu)
    for (int i = i2; i < i3; ++i) {
        vec_rhs[i] -= vec_b[i] * vec_rhs[parent_index[i]];
        vec_rhs[i] /= vec_d[i];
    }

    #pragma acc wait(stream_id)
    // clang-format on
}
}  // namespace coreneuron
