/*
# =============================================================================
# Copyright (C) 2016-2021 Blue Brain Project
#
# See top-level LICENSE file for details.
# =============================================================================.
*/

#ifndef _H_MEMORY_
#define _H_MEMORY_

#include <cstdint>
#include <cstring>

#include "coreneuron/utils/nrn_assert.h"

#if !defined(NRN_SOA_BYTE_ALIGN)
// for layout 0, every range variable array must be aligned by at least 16 bytes (the size of the
// simd memory bus)
#define NRN_SOA_BYTE_ALIGN (8 * sizeof(double))
#endif

/// for gpu builds with unified memory support
#if (defined(__CUDACC__) || defined(UNIFIED_MEMORY))

#include <cuda_runtime_api.h>

// TODO : error handling for CUDA routines
inline void alloc_memory(void*& pointer, size_t num_bytes, size_t /*alignment*/) {
    cudaMallocManaged(&pointer, num_bytes);
}

inline void calloc_memory(void*& pointer, size_t num_bytes, size_t /*alignment*/) {
    alloc_memory(pointer, num_bytes, 64);
    cudaMemset(pointer, 0, num_bytes);
}

inline void free_memory(void* pointer) {
    cudaFree(pointer);
}

/**
 * A base class providing overloaded new and delete operators for CUDA allocation
 *
 * Classes that should be allocated on the GPU should inherit from this class. Additionally they
 * may need to implement a special copy-construtor. This is documented here:
 * \link: https://devblogs.nvidia.com/unified-memory-in-cuda-6/
 */
class MemoryManaged {
  public:
  void *operator new(size_t len) {
    void *ptr;
    cudaMallocManaged(&ptr, len);
    cudaDeviceSynchronize();
    return ptr;
  }

  void *operator new[](size_t len) {
    void *ptr;
    cudaMallocManaged(&ptr, len);
    cudaDeviceSynchronize();
    return ptr;
  }

  void operator delete(void *ptr) {
    cudaDeviceSynchronize();
    cudaFree(ptr);
  }

  void operator delete[](void *ptr) {
    cudaDeviceSynchronize();
    cudaFree(ptr);
  }
};


/// for cpu builds use posix memalign
#else
class MemoryManaged {
    // does nothing by default
};

#include <stdlib.h>

inline void alloc_memory(void*& pointer, size_t num_bytes, size_t alignment) {
#if defined(MINGW)
    nrn_assert( (pointer = _aligned_malloc(num_bytes, alignment)) != nullptr);
#else
    nrn_assert(posix_memalign(&pointer, alignment, num_bytes) == 0);
#endif
}

inline void calloc_memory(void*& pointer, size_t num_bytes, size_t alignment) {
    alloc_memory(pointer, num_bytes, alignment);
    memset(pointer, 0, num_bytes);
}

inline void free_memory(void* pointer) {
    free(pointer);
}

#endif

namespace coreneuron {

/** Independent function to compute the needed chunkding,
    the chunk argument is the number of doubles the chunk is chunkded upon.
*/
template <int chunk>
inline int soa_padded_size(int cnt, int layout) {
    int imod = cnt % chunk;
    if (layout == 1)
        return cnt;
    if (imod) {
        int idiv = cnt / chunk;
        return (idiv + 1) * chunk;
    }
    return cnt;
}

/** Check for the pointer alignment.
 */
inline bool is_aligned(void* pointer, size_t alignment) {
    return (((uintptr_t)(const void*)(pointer)) % (alignment) == 0);
}

/** Allocate the aligned memory.
 */
inline void* emalloc_align(size_t size, size_t alignment = NRN_SOA_BYTE_ALIGN) {
    void* memptr;
    alloc_memory(memptr, size, alignment);
    nrn_assert(is_aligned(memptr, alignment));
    return memptr;
}

/** Allocate the aligned memory and set it to 0.
 */
inline void* ecalloc_align(size_t n, size_t size, size_t alignment = NRN_SOA_BYTE_ALIGN) {
    void* p;
    if (n == 0) {
        return nullptr;
    }
    calloc_memory(p, n * size, alignment);
    nrn_assert(is_aligned(p, alignment));
    return p;
}
}  // namespace coreneuron

#endif
