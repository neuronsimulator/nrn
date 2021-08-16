/*
# =============================================================================
# Copyright (c) 2016 - 2021 Blue Brain Project/EPFL
#
# See top-level LICENSE file for details.
# =============================================================================.
*/
#include "coreneuron/utils/memory.h"
#include "coreneuron/utils/nrnmutdec.h"
#include "coreneuron/utils/randoms/nrnran123.h"

#include <cmath>
#include <iostream>
#include <memory>
#include <mutex>

// In a GPU build this file will be compiled by NVCC as CUDA code
// In a CPU build this file will be compiled by a C++ compiler as C++ code
#ifdef __CUDACC__
#define CORENRN_DEVICE __device__
#else
#define CORENRN_DEVICE
#endif

namespace {
/* global data structure per process */
using g_k_allocator_t = coreneuron::unified_allocator<philox4x32_key_t>;
std::unique_ptr<philox4x32_key_t, coreneuron::alloc_deleter<g_k_allocator_t>> g_k;

// In a GPU build we need a device-side global pointer to this global state.
// This is set to the same unified memory address as `g_k` in
// `setup_global_state()` if the GPU is enabled. It would be cleaner to use
// __managed__ here, but unfortunately that does not work on machines that do
// not have a GPU.
#ifdef __CUDACC__
CORENRN_DEVICE philox4x32_key_t* g_k_dev;
#endif

OMP_Mutex g_instance_count_mutex;
std::size_t g_instance_count{};

constexpr double SHIFT32 = 1.0 / 4294967297.0; /* 1/(2^32 + 1) */

void setup_global_state() {
    if (g_k) {
        // Already initialised, nothing to do
        return;
    }
    g_k = coreneuron::allocate_unique<philox4x32_key_t>(g_k_allocator_t{});
#ifdef __CUDACC__
    if (coreneuron::unified_memory_enabled()) {
        // Set the device-side global g_k_dev to point to the newly-allocated
        // unified memory. If this is false, g_k is just a host pointer and
        // there is no point initialising the device global to it.
        {
            auto* k = g_k.get();
            auto const code = cudaMemcpyToSymbol(g_k_dev, &k, sizeof(k));
            assert(code == cudaSuccess);
        }
        // Make sure g_k_dev is updated.
        {
            auto const code = cudaDeviceSynchronize();
            assert(code == cudaSuccess);
        }
    }
#endif
}

/** @brief Get the Random123 global state from either host or device code.
 */
CORENRN_HOST_DEVICE philox4x32_key_t& get_global_state() {
    philox4x32_key_t* ret{nullptr};
#ifdef __CUDA_ARCH__
    // Called from device code
    ret = g_k_dev;
#else
    // Called from host code
    ret = g_k.get();
#endif
    assert(ret);
    return *ret;
}
}  // namespace

namespace coreneuron {
std::size_t nrnran123_instance_count() {
    return g_instance_count;
}

/* if one sets the global, one should reset all the stream sequences. */
uint32_t nrnran123_get_globalindex() {
    setup_global_state();
    return get_global_state().v[0];
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
    s->r = philox4x32(s->c, get_global_state());
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
        s->r = philox4x32(s->c, get_global_state());
    }
    s->which_ = which;
    return rval;
}

CORENRN_HOST_DEVICE double nrnran123_dblpick(nrnran123_State* s) {
    return nrnran123_uint2dbl(nrnran123_ipick(s));
}

CORENRN_HOST_DEVICE double nrnran123_negexp(nrnran123_State* s) {
    /* min 2.3283064e-10 to max 22.18071 */
    return -std::log(nrnran123_dblpick(s));
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

    y = std::sqrt((-2. * log(w)) / w);
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
    setup_global_state();
    // If the global seed is changing then we shouldn't have any active streams.
    {
        std::lock_guard<OMP_Mutex> _{g_instance_count_mutex};
        if (g_instance_count != 0 && nrnmpi_myid == 0) {
            std::cout
                << "nrnran123_set_globalindex(" << gix
                << ") called when a non-zero number of Random123 streams (" << g_instance_count
                << ") were active. This is not safe, some streams will remember the old value ("
                << get_global_state().v[0] << ')';
        }
    }
    get_global_state().v[0] = gix;
}

/** @brief Allocate a new Random123 stream.
 *  @todo  It would be nicer if the API return type was
 *  std::unique_ptr<nrnran123_State, ...not specified...>, so we could use a
 *  custom allocator/deleter and avoid the (fragile) need for matching
 *  nrnran123_deletestream calls. See `g_k` for an example.
 */
nrnran123_State* nrnran123_newstream3(uint32_t id1,
                                      uint32_t id2,
                                      uint32_t id3,
                                      bool use_unified_memory) {
    // The `use_unified_memory` argument is an implementation detail to keep the
    // old behaviour that some Random123 streams that are known to only be used
    // from the CPU are allocated using new/delete instead of unified memory.
    // See OPENACC_EXCLUDED_FILES in coreneuron/CMakeLists.txt. If we dropped
    // this feature then we could always use coreneuron::unified_allocator.
#ifndef CORENEURON_ENABLE_GPU
    if (use_unified_memory) {
        throw std::runtime_error("Tried to use CUDA unified memory in a non-GPU build.");
    }
#endif
    nrnran123_State* s{nullptr};
    if (use_unified_memory) {
        s = coreneuron::allocate_unique<nrnran123_State>(
                coreneuron::unified_allocator<nrnran123_State>{})
                .release();
    } else {
        s = new nrnran123_State{};
    }
    s->c.v[0] = 0;
    s->c.v[1] = id3;
    s->c.v[2] = id1;
    s->c.v[3] = id2;
    nrnran123_setseq(s, 0, 0);
    {
        // TODO: can I assert something useful about the instance count going
        // back to zero anywhere? Or that it is zero when some operations happen?
        std::lock_guard<OMP_Mutex> _{g_instance_count_mutex};
        ++g_instance_count;
    }
    return s;
}

/* nrn123 streams are destroyed from cpu launcher routine */
void nrnran123_deletestream(nrnran123_State* s, bool use_unified_memory) {
#ifndef CORENEURON_ENABLE_GPU
    if (use_unified_memory) {
        throw std::runtime_error("Tried to use CUDA unified memory in a non-GPU build.");
    }
#endif
    {
        std::lock_guard<OMP_Mutex> _{g_instance_count_mutex};
        --g_instance_count;
    }
    if (use_unified_memory) {
        std::unique_ptr<nrnran123_State,
                        coreneuron::alloc_deleter<coreneuron::unified_allocator<nrnran123_State>>>
            _{s};
    } else {
        delete s;
    }
}
}  // namespace coreneuron
