/*
# =============================================================================
# Copyright (C) 2016-2021 Blue Brain Project
#
# See top-level LICENSE file for details.
# =============================================================================.
*/

#include <stdlib.h>
#include <math.h>
#include "coreneuron/utils/randoms/nrnran123.h"
#include "coreneuron/nrnconf.h"
#include "coreneuron/utils/nrnmutdec.h"
#include "coreneuron/utils/nrnoc_aux.hpp"

namespace coreneuron {
static const double SHIFT32 = 1.0 / 4294967297.0; /* 1/(2^32 + 1) */

static philox4x32_key_t k = {{0}};

static size_t instance_count_ = 0;
size_t nrnran123_instance_count() {
    return instance_count_;
}

size_t nrnran123_state_size() {
    return sizeof(nrnran123_State);
}

void nrnran123_set_globalindex(uint32_t gix) {
    k.v[0] = gix;
#if (defined(__CUDACC__) || defined(_OPENACC))
    nrnran123_set_gpu_globalindex(gix);
#endif
}

/* if one sets the global, one should reset all the stream sequences. */
uint32_t nrnran123_get_globalindex() {
    return k.v[0];
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

nrnran123_State* nrnran123_newstream(uint32_t id1, uint32_t id2) {
    return nrnran123_newstream3(id1, id2, 0);
}

nrnran123_State* nrnran123_newstream3(uint32_t id1, uint32_t id2, uint32_t id3) {
    nrnran123_State* s = (nrnran123_State*) ecalloc(sizeof(nrnran123_State), 1);
    s->c.v[1] = id3;
    s->c.v[2] = id1;
    s->c.v[3] = id2;
    nrnran123_setseq(s, 0, 0);
    MUTLOCK
    ++instance_count_;
    MUTUNLOCK
    return s;
}

void nrnran123_deletestream(nrnran123_State* s) {
    MUTLOCK
    --instance_count_;
    MUTUNLOCK
    free(s);
}

void nrnran123_getseq(nrnran123_State* s, uint32_t* seq, unsigned char* which) {
    *seq = s->c.v[0];
    *which = s->which_;
}

void nrnran123_setseq(nrnran123_State* s, uint32_t seq, unsigned char which) {
    if (which > 3) {
        s->which_ = 0;
    } else {
        s->which_ = which;
    }
    s->c.v[0] = seq;
    s->r = philox4x32(s->c, k);
}

void nrnran123_getids(nrnran123_State* s, uint32_t* id1, uint32_t* id2) {
    *id1 = s->c.v[2];
    *id2 = s->c.v[3];
}

void nrnran123_getids3(nrnran123_State* s, uint32_t* id1, uint32_t* id2, uint32_t* id3) {
    *id3 = s->c.v[1];
    *id1 = s->c.v[2];
    *id2 = s->c.v[3];
}

uint32_t nrnran123_ipick(nrnran123_State* s) {
    uint32_t rval;
    unsigned char which = s->which_;
    assert(which < 4);
    rval = s->r.v[which++];
    if (which > 3) {
        which = 0;
        s->c.v[0]++;
        s->r = philox4x32(s->c, k);
    }
    s->which_ = which;
    return rval;
}

double nrnran123_dblpick(nrnran123_State* s) {
    return nrnran123_uint2dbl(nrnran123_ipick(s));
}

double nrnran123_negexp(nrnran123_State* s) {
    /* min 2.3283064e-10 to max 22.18071 */
    return -log(nrnran123_dblpick(s));
}

/* at cost of a cached  value we could compute two at a time. */
double nrnran123_normal(nrnran123_State* s) {
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

double nrnran123_uint2dbl(uint32_t u) {
    /* 0 to 2^32-1 transforms to double value in open (0,1) interval */
    /* min 2.3283064e-10 to max (1 - 2.3283064e-10) */
    return ((double) u + 1.0) * SHIFT32;
}
}  // namespace coreneuron
