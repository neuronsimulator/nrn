/*
# =============================================================================
# Copyright (c) 2016 - 2022 Blue Brain Project/EPFL
#
# See top-level LICENSE file for details.
# =============================================================================.
*/
#pragma once

/* interface to Random123 */
/* http://www.thesalmons.org/john/random123/papers/random123sc11.pdf */

/*
The 4x32 generators utilize a uint32x4 counter and uint32x4 key to transform
into an almost cryptographic quality uint32x4 random result.
There are many possibilites for balancing the sharing of the internal
state instances while reserving a uint32 counter for the stream sequence
and reserving other portions of the counter vector for stream identifiers
and global index used by all streams.

We currently provide a single instance by default in which the policy is
to use the 0th counter uint32 as the stream sequence, words 2 and 3 as the
stream identifier, and word 0 of the key as the global index. Unused words
are constant uint32 0.

It is also possible to use Random123 directly without reference to this
interface. See Random123-1.02/docs/html/index.html
of the full distribution available from
http://www.deshawresearch.com/resources_random123.html
*/

#ifdef __bgclang__
#define R123_USE_MULHILO64_MULHI_INTRIN 0
#define R123_USE_GNU_UINT128            1
#endif

#include "coreneuron/utils/offload.hpp"

#include <Random123/philox.h>
#include <inttypes.h>

#include <cmath>

#if defined(CORENEURON_ENABLE_GPU)
#define CORENRN_RAN123_USE_UNIFIED_MEMORY true
#else
#define CORENRN_RAN123_USE_UNIFIED_MEMORY false
#endif

namespace coreneuron {

struct nrnran123_State {
    philox4x32_ctr_t c;
    philox4x32_ctr_t r;
    char which_;
};

}  // namespace coreneuron

/** @brief Provide a helper function in global namespace that is declared target for OpenMP
 * offloading to function correctly with NVHPC
 */
nrn_pragma_acc(routine seq)
nrn_pragma_omp(declare target)
philox4x32_ctr_t coreneuron_random123_philox4x32_helper(coreneuron::nrnran123_State* s);
nrn_pragma_omp(end declare target)

namespace coreneuron {
void nrnran123_initialise_global_state_on_device();
void nrnran123_destroy_global_state_on_device();

/* global index. eg. run number */
/* all generator instances share this global index */
void nrnran123_set_globalindex(uint32_t gix);
uint32_t nrnran123_get_globalindex();

// Utilities used for calculating model size, only called from the CPU.
std::size_t nrnran123_instance_count();
inline std::size_t nrnran123_state_size() {
    return sizeof(nrnran123_State);
}

/* routines for creating and deleting streams are called from cpu */
nrnran123_State* nrnran123_newstream3(uint32_t id1,
                                      uint32_t id2,
                                      uint32_t id3,
                                      bool use_unified_memory = CORENRN_RAN123_USE_UNIFIED_MEMORY);
inline nrnran123_State* nrnran123_newstream(
    uint32_t id1,
    uint32_t id2,
    bool use_unified_memory = CORENRN_RAN123_USE_UNIFIED_MEMORY) {
    return nrnran123_newstream3(id1, id2, 0, use_unified_memory);
}
void nrnran123_deletestream(nrnran123_State* s,
                            bool use_unified_memory = CORENRN_RAN123_USE_UNIFIED_MEMORY);

/* minimal data stream */
constexpr void nrnran123_getseq(nrnran123_State* s, uint32_t* seq, char* which) {
    *seq = s->c.v[0];
    *which = s->which_;
}
constexpr void nrnran123_getids(nrnran123_State* s, uint32_t* id1, uint32_t* id2) {
    *id1 = s->c.v[2];
    *id2 = s->c.v[3];
}
constexpr void nrnran123_getids3(nrnran123_State* s, uint32_t* id1, uint32_t* id2, uint32_t* id3) {
    *id3 = s->c.v[1];
    *id1 = s->c.v[2];
    *id2 = s->c.v[3];
}

// Uniform 0 to 2*32-1
inline uint32_t nrnran123_ipick(nrnran123_State* s) {
    char which = s->which_;
    uint32_t rval{s->r.v[int{which++}]};
    if (which > 3) {
        which = 0;
        s->c.v[0]++;
        s->r = coreneuron_random123_philox4x32_helper(s);
    }
    s->which_ = which;
    return rval;
}

constexpr double nrnran123_uint2dbl(uint32_t u) {
    constexpr double SHIFT32 = 1.0 / 4294967297.0; /* 1/(2^32 + 1) */
    /* 0 to 2^32-1 transforms to double value in open (0,1) interval */
    /* min 2.3283064e-10 to max (1 - 2.3283064e-10) */
    return (static_cast<double>(u) + 1.0) * SHIFT32;
}

// Uniform open interval (0,1), minimum value is 2.3283064e-10 and max value is 1-min
inline double nrnran123_dblpick(nrnran123_State* s) {
    return nrnran123_uint2dbl(nrnran123_ipick(s));
}

// same as dblpick
inline double nrnran123_uniform(nrnran123_State* s) {
    return nrnran123_uint2dbl(nrnran123_ipick(s));
}

inline double nrnran123_uniform(nrnran123_State* s, double low, double high) {
    return low + nrnran123_uint2dbl(nrnran123_ipick(s)) * (high - low);
}

/* this could be called from openacc parallel construct (in INITIAL block) */
inline void nrnran123_setseq(nrnran123_State* s, uint32_t seq, char which) {
    if (which > 3) {
        s->which_ = 0;
    } else {
        s->which_ = which;
    }
    s->c.v[0] = seq;
    s->r = coreneuron_random123_philox4x32_helper(s);
}

/* this could be called from openacc parallel construct (in INITIAL block) */
inline void nrnran123_setseq(nrnran123_State* s, double seq34) {
    if (seq34 < 0.0) {
        seq34 = 0.0;
    }
    if (seq34 > double(0XffffffffffLL)) {
        seq34 = 0.0;
    }

    // at least 64 bits even on 32 bit machine (could be more)
    unsigned long long x = ((unsigned long long) seq34) & 0X3ffffffffLL;
    char which = x & 0X3;
    uint32_t seq = x >> 2;
    nrnran123_setseq(s, seq, which);
}

// nrnran123_negexp min value is 2.3283064e-10, max is 22.18071, mean 1.0
inline double nrnran123_negexp(nrnran123_State* s) {
    return -std::log(nrnran123_dblpick(s));
}

/* at cost of a cached  value we could compute two at a time. */
inline double nrnran123_normal(nrnran123_State* s) {
    double w, u1;
    do {
        u1 = nrnran123_dblpick(s);
        double u2{nrnran123_dblpick(s)};
        u1 = 2. * u1 - 1.;
        u2 = 2. * u2 - 1.;
        w = (u1 * u1) + (u2 * u2);
    } while (w > 1);
    double y{std::sqrt((-2. * std::log(w)) / w)};
    return u1 * y;
}

// nrnran123_gauss, nrnran123_iran were declared but not defined in CoreNEURON
// nrnran123_array4x32 was declared but not used in CoreNEURON
}  // namespace coreneuron
