/*
# =============================================================================
# Copyright (c) 2016 - 2021 Blue Brain Project/EPFL
#
# See top-level LICENSE file for details.
# =============================================================================.
*/
// Beware changing this to #pragma once, we rely on this file shadowing the
// equivalent file from NEURON.
#ifndef nrnran123_h
#define nrnran123_h

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

#include <Random123/philox.h>
#include <inttypes.h>

#ifdef __CUDACC__
#define CORENRN_HOST_DEVICE __host__ __device__
#else
#define CORENRN_HOST_DEVICE
#endif

// Is there actually any harm leaving the pragma in when DISABLE_OPENACC is true?
#if defined(_OPENACC) && !defined(DISABLE_OPENACC)
#define CORENRN_HOST_DEVICE_ACC CORENRN_HOST_DEVICE _Pragma("acc routine seq")
#else
#define CORENRN_HOST_DEVICE_ACC CORENRN_HOST_DEVICE
#endif

// Some files are compiled with DISABLE_OPENACC, and some builds have no GPU
// support at all. In these two cases, request that the random123 state is
// allocated using new/delete instead of CUDA unified memory.
#if (defined(__CUDACC__) || defined(_OPENACC)) && !defined(DISABLE_OPENACC)
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

struct nrnran123_array4x32 {
    uint32_t v[4];
};

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
CORENRN_HOST_DEVICE_ACC void nrnran123_getseq(nrnran123_State*, uint32_t* seq, char* which);
CORENRN_HOST_DEVICE_ACC void nrnran123_getids(nrnran123_State*, uint32_t* id1, uint32_t* id2);
CORENRN_HOST_DEVICE_ACC void nrnran123_getids3(nrnran123_State*,
                                               uint32_t* id1,
                                               uint32_t* id2,
                                               uint32_t* id3);
CORENRN_HOST_DEVICE_ACC uint32_t nrnran123_ipick(nrnran123_State*); /* uniform 0 to 2^32-1 */

/* this could be called from openacc parallel construct */
CORENRN_HOST_DEVICE_ACC double nrnran123_dblpick(nrnran123_State*); /* uniform open interval (0,1)*/
/* nrnran123_dblpick minimum value is 2.3283064e-10 and max value is 1-min */

/* this could be called from openacc parallel construct (in INITIAL block) */
CORENRN_HOST_DEVICE_ACC void nrnran123_setseq(nrnran123_State*, uint32_t seq, char which);

CORENRN_HOST_DEVICE_ACC double nrnran123_negexp(nrnran123_State*); /* mean 1.0 */
/* nrnran123_negexp min value is 2.3283064e-10, max is 22.18071 */

/* missing declaration in coreneuron */
CORENRN_HOST_DEVICE_ACC double nrnran123_normal(nrnran123_State*);

CORENRN_HOST_DEVICE_ACC double nrnran123_gauss(nrnran123_State*); /* mean 0.0, std 1.0 */

/* more fundamental (stateless) (though the global index is still used) */
CORENRN_HOST_DEVICE_ACC nrnran123_array4x32 nrnran123_iran(uint32_t seq,
                                                           uint32_t id1,
                                                           uint32_t id2);
CORENRN_HOST_DEVICE_ACC double nrnran123_uint2dbl(uint32_t);
}  // namespace coreneuron

#endif
