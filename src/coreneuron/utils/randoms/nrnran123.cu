/*
# =============================================================================
# Copyright (c) 2016 - 2021 Blue Brain Project/EPFL
#
# See top-level LICENSE file for details.
# =============================================================================.
*/
#include "coreneuron/utils/nrnmutdec.h"
#include "coreneuron/utils/randoms/nrnran123.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include <mutex>

// In a GPU build this file will be compiled by NVCC as CUDA code
// In a CPU build this file will be compiled by a C++ compiler as C++ code
#ifdef __CUDACC__
#define CORENRN_MANAGED __managed__
#else
#define CORENRN_MANAGED
#endif

namespace {
/* global data structure per process */
CORENRN_MANAGED philox4x32_key_t g_k = {{0}};
OMP_Mutex g_instance_count_mutex;
std::size_t g_instance_count{};
constexpr double SHIFT32 = 1.0 / 4294967297.0; /* 1/(2^32 + 1) */
}  // namespace

namespace coreneuron {
std::size_t nrnran123_instance_count() {
    return g_instance_count;
}

#ifdef _OPENMP
static MUTDEC void nrnran123_mutconstruct() {
    if (!mut_) {
        MUTCONSTRUCT(1);
    }
}
#else
void nrnran123_mutconstruct() {}
#endif

/* if one sets the global, one should reset all the stream sequences. */
CORENRN_HOST_DEVICE uint32_t nrnran123_get_globalindex() {
    return g_k.v[0];
}

CORENRN_HOST_DEVICE void nrnran123_getseq(nrnran123_State* s, uint32_t* seq, char* which) {
    *seq = s->c.v[0];
    *which = s->which_;
}

CORENRN_HOST_DEVICE void nrnran123_setseq(nrnran123_State* s, uint32_t seq, char which) {
    if (which > 3) {
        s->which_ = 0;
    } else {
        s->which_ = which;
    }
    s->c.v[0] = seq;
    s->r = philox4x32(s->c, g_k);
}

CORENRN_HOST_DEVICE void nrnran123_getids(nrnran123_State* s, uint32_t* id1, uint32_t* id2) {
    *id1 = s->c.v[2];
    *id2 = s->c.v[3];
}

CORENRN_HOST_DEVICE void nrnran123_getids3(nrnran123_State* s,
                                           uint32_t* id1,
                                           uint32_t* id2,
                                           uint32_t* id3) {
    *id3 = s->c.v[1];
    *id1 = s->c.v[2];
    *id2 = s->c.v[3];
}

CORENRN_HOST_DEVICE uint32_t nrnran123_ipick(nrnran123_State* s) {
    uint32_t rval;
    char which = s->which_;
    rval = s->r.v[which++];
    if (which > 3) {
        which = 0;
        s->c.v[0]++;
        s->r = philox4x32(s->c, g_k);
    }
    s->which_ = which;
    return rval;
}

CORENRN_HOST_DEVICE double nrnran123_dblpick(nrnran123_State* s) {
    return nrnran123_uint2dbl(nrnran123_ipick(s));
}

CORENRN_HOST_DEVICE double nrnran123_negexp(nrnran123_State* s) {
    /* min 2.3283064e-10 to max 22.18071 */
    return -log(nrnran123_dblpick(s));
}

/* at cost of a cached  value we could compute two at a time. */
CORENRN_HOST_DEVICE double nrnran123_normal(nrnran123_State* s) {
    double w, x, y;
    double u1, u2;

    do {
        u1 = nrnran123_dblpick(s);
        u2 = nrnran123_dblpick(s);
        u1 = 2. * u1 - 1.;
        u2 = 2. * u2 - 1.;
        w = (u1 * u1) + (u2 * u2);
    } while (w > 1);

    y = sqrt((-2. * log(w)) / w);
    x = u1 * y;
    return x;
}

CORENRN_HOST_DEVICE double nrnran123_uint2dbl(uint32_t u) {
    /* 0 to 2^32-1 transforms to double value in open (0,1) interval */
    /* min 2.3283064e-10 to max (1 - 2.3283064e-10) */
    return ((double) u + 1.0) * SHIFT32;
}

/* nrn123 streams are created from cpu launcher routine */
void nrnran123_set_globalindex(uint32_t gix) {
    g_k.v[0] = gix;
}

nrnran123_State* nrnran123_newstream3(uint32_t id1,
                                      uint32_t id2,
                                      uint32_t id3,
                                      bool use_unified_memory) {
    nrnran123_State* s{nullptr};
    if (use_unified_memory) {
#ifdef __CUDACC__
        {
            auto const code = cudaMallocManaged(&s, sizeof(nrnran123_State));
            assert(code == cudaSuccess);
        }
        {
            auto const code = cudaMemset(s, 0, sizeof(nrnran123_State));
            assert(code == cudaSuccess);
        }
#else
        throw std::runtime_error("Tried to use CUDA unified memory in a non-GPU build.");
#endif
    } else {
        s = new nrnran123_State{};
    }
    s->c.v[0] = 0;
    s->c.v[1] = id3;
    s->c.v[2] = id1;
    s->c.v[3] = id2;
    nrnran123_setseq(s, 0, 0);
    {
        std::lock_guard<OMP_Mutex> _{g_instance_count_mutex};
        ++g_instance_count;
    }
    return s;
}

/* nrn123 streams are destroyed from cpu launcher routine */
void nrnran123_deletestream(nrnran123_State* s, bool use_unified_memory) {
    {
        std::lock_guard<OMP_Mutex> _{g_instance_count_mutex};
        --g_instance_count;
    }
    if (use_unified_memory) {
#ifdef __CUDACC__
        cudaFree(s);
#else
        throw std::runtime_error("Tried to use CUDA unified memory in a non-GPU build.");
#endif
    } else {
        delete s;
    }
}
}  // namespace coreneuron
