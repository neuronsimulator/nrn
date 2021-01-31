/*
# =============================================================================
# Copyright (c) 2016 - 2021 Blue Brain Project/EPFL
#
# See top-level LICENSE file for details.
# =============================================================================.
*/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "coreneuron/utils/randoms/nrnran123.h"
namespace coreneuron {
/* global data structure per process */
__device__ static const double SHIFT32 = 1.0 / 4294967297.0; /* 1/(2^32 + 1) */
__device__ static philox4x32_key_t k = {{0}};
__device__ static unsigned int instance_count_ = 0;
__device__ size_t nrnran123_instance_count() {
    return instance_count_;
}

__device__ size_t nrnran123_state_size() {
    return sizeof(nrnran123_State);
}

__global__ void nrnran123_set_globalindex(uint32_t gix) {
    k.v[0] = gix;
}

/* if one sets the global, one should reset all the stream sequences. */
__device__ uint32_t nrnran123_get_globalindex() {
    return k.v[0];
}

__global__ void nrnran123_setup_cuda_newstream(nrnran123_State* s,
                                               uint32_t id1,
                                               uint32_t id2,
                                               uint32_t id3) {
    s->c.v[0] = 0;
    s->c.v[1] = id3;
    s->c.v[2] = id1;
    s->c.v[3] = id2;
    nrnran123_setseq(s, 0, 0);
    atomicAdd(&instance_count_, 1);
}

__global__ void nrnran123_cuda_deletestream(nrnran123_State* s) {
    atomicSub(&instance_count_, 1);
}

__device__ void nrnran123_getseq(nrnran123_State* s, uint32_t* seq, unsigned char* which) {
    *seq = s->c.v[0];
    *which = s->which_;
}

__device__ void nrnran123_setseq(nrnran123_State* s, uint32_t seq, unsigned char which) {
    if (which > 3) {
        s->which_ = 0;
    } else {
        s->which_ = which;
    }
    s->c.v[0] = seq;
    s->r = philox4x32(s->c, k);
}

__device__ void nrnran123_getids(nrnran123_State* s, uint32_t* id1, uint32_t* id2) {
    *id1 = s->c.v[2];
    *id2 = s->c.v[3];
}

__device__ void nrnran123_getids3(nrnran123_State* s, uint32_t* id1, uint32_t* id2, uint32_t* id3) {
    *id3 = s->c.v[1];
    *id1 = s->c.v[2];
    *id2 = s->c.v[3];
}

__device__ uint32_t nrnran123_ipick(nrnran123_State* s) {
    uint32_t rval;
    unsigned char which = s->which_;
    rval = s->r.v[which++];
    if (which > 3) {
        which = 0;
        s->c.v[0]++;
        s->r = philox4x32(s->c, k);
    }
    s->which_ = which;
    return rval;
}

__device__ double nrnran123_dblpick(nrnran123_State* s) {
    return nrnran123_uint2dbl(nrnran123_ipick(s));
}

__device__ double nrnran123_negexp(nrnran123_State* s) {
    /* min 2.3283064e-10 to max 22.18071 */
    return -log(nrnran123_dblpick(s));
}

/* at cost of a cached  value we could compute two at a time. */
__device__ double nrnran123_normal(nrnran123_State* s) {
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

__device__ double nrnran123_uint2dbl(uint32_t u) {
    /* 0 to 2^32-1 transforms to double value in open (0,1) interval */
    /* min 2.3283064e-10 to max (1 - 2.3283064e-10) */
    return ((double)u + 1.0) * SHIFT32;
}

/* nrn123 streams are created from cpu launcher routine */
nrnran123_State* nrnran123_newstream(uint32_t id1, uint32_t id2) {
    return nrnran123_newstream3(id1, id2, 0);
}

nrnran123_State* nrnran123_newstream3(uint32_t id1, uint32_t id2, uint32_t id3) {
    nrnran123_State* s;

    cudaMalloc((void**)&s, sizeof(nrnran123_State));
    cudaMemset((void*)s, 0, sizeof(nrnran123_State));

    nrnran123_setup_cuda_newstream<<<1, 1>>>(s, id1, id2, id3);
    cudaDeviceSynchronize();

    return s;
}

/* nrn123 streams are destroyed from cpu launcher routine */
void nrnran123_deletestream(nrnran123_State* s) {
    nrnran123_cuda_deletestream<<<1, 1>>>(s);
    cudaDeviceSynchronize();

    cudaFree(s);
}

/* set global index for random123 stream on gpu */
void nrnran123_set_gpu_globalindex(uint32_t gix) {
    nrnran123_set_globalindex<<<1,1>>>(gix);
    cudaDeviceSynchronize();
}

} //namespace coreneuron
