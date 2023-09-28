/*
# =============================================================================
# Copyright (c) 2016 - 2022 Blue Brain Project/EPFL
#
# See top-level LICENSE file for details.
# =============================================================================.
*/

#pragma once

#include <cstdint>
#include <cstring>
#include <cstdlib>
#include <memory>

#include "coreneuron/utils/nrn_assert.h"
#include "coreneuron/nrniv/nrniv_decl.h"

#if !defined(NRN_SOA_BYTE_ALIGN)
// for layout 0, every range variable array must be aligned by at least 16 bytes (the size of the
// simd memory bus)
#define NRN_SOA_BYTE_ALIGN (8 * sizeof(double))
#endif

namespace coreneuron {
/**
 * @brief Check if GPU support is enabled.
 *
 * This returns true if GPU support was enabled at compile time and at runtime
 * via coreneuron.gpu = True and/or --gpu, otherwise it returns false.
 */
bool gpu_enabled();

/** @brief Allocate unified memory in GPU builds iff GPU enabled, otherwise new
 */
void* allocate_unified(std::size_t num_bytes);

/** @brief Deallocate memory allocated by `allocate_unified`.
 */
void deallocate_unified(void* ptr, std::size_t num_bytes);

/** @brief C++ allocator that uses [de]allocate_unified.
 */
template <typename T>
struct unified_allocator {
    using value_type = T;

    unified_allocator() = default;

    template <typename U>
    unified_allocator(unified_allocator<U> const&) noexcept {}

    value_type* allocate(std::size_t n) {
        return static_cast<value_type*>(allocate_unified(n * sizeof(value_type)));
    }

    void deallocate(value_type* p, std::size_t n) noexcept {
        deallocate_unified(p, n * sizeof(value_type));
    }
};

template <typename T, typename U>
bool operator==(unified_allocator<T> const&, unified_allocator<U> const&) noexcept {
    return true;
}

template <typename T, typename U>
bool operator!=(unified_allocator<T> const& x, unified_allocator<U> const& y) noexcept {
    return !(x == y);
}

/** @brief Allocator-aware deleter for use with std::unique_ptr.
 *
 *  This is copied from https://stackoverflow.com/a/23132307. See also
 *  http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2017/p0316r0.html,
 *  http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2020/p0211r3.html, and
 *  boost::allocate_unique<...>.
 *  Hopefully std::allocate_unique will be included in C++23.
 */
template <typename Alloc>
struct alloc_deleter {
    alloc_deleter() = default;  // OL210813 addition
    alloc_deleter(const Alloc& a)
        : a(a) {}

    using pointer = typename std::allocator_traits<Alloc>::pointer;

    void operator()(pointer p) const {
        Alloc aa(a);
        std::allocator_traits<Alloc>::destroy(aa, std::addressof(*p));
        std::allocator_traits<Alloc>::deallocate(aa, p, 1);
    }

  private:
    Alloc a;
};

template <typename T, typename Alloc, typename... Args>
auto allocate_unique(const Alloc& alloc, Args&&... args) {
    using AT = std::allocator_traits<Alloc>;
    static_assert(std::is_same<typename AT::value_type, std::remove_cv_t<T>>{}(),
                  "Allocator has the wrong value_type");

    Alloc a(alloc);
    auto p = AT::allocate(a, 1);
    try {
        AT::construct(a, std::addressof(*p), std::forward<Args>(args)...);
        using D = alloc_deleter<Alloc>;
        return std::unique_ptr<T, D>(p, D(a));
    } catch (...) {
        AT::deallocate(a, p, 1);
        throw;
    }
}
}  // namespace coreneuron

/// for gpu builds with unified memory support
#ifdef CORENEURON_UNIFIED_MEMORY

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
    void* operator new(size_t len) {
        void* ptr;
        cudaMallocManaged(&ptr, len);
        cudaDeviceSynchronize();
        return ptr;
    }

    void* operator new[](size_t len) {
        void* ptr;
        cudaMallocManaged(&ptr, len);
        cudaDeviceSynchronize();
        return ptr;
    }

    void operator delete(void* ptr) {
        cudaDeviceSynchronize();
        cudaFree(ptr);
    }

    void operator delete[](void* ptr) {
        cudaDeviceSynchronize();
        cudaFree(ptr);
    }
};


/// for cpu builds use posix memalign
#else
class MemoryManaged {
    // does nothing by default
};

#include <cstdlib>

inline void alloc_memory(void*& pointer, size_t num_bytes, size_t alignment) {
    size_t fill = 0;
    if (alignment > 0) {
        if (num_bytes % alignment != 0) {
            size_t multiple = num_bytes / alignment;
            fill = alignment * (multiple + 1) - num_bytes;
        }
        nrn_assert((pointer = std::aligned_alloc(alignment, num_bytes + fill)) != nullptr);
    } else {
        nrn_assert((pointer = std::malloc(num_bytes)) != nullptr);
    }
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
    if (layout == Layout::AoS)
        return cnt;
    if (imod) {
        int idiv = cnt / chunk;
        return (idiv + 1) * chunk;
    }
    return cnt;
}

/** Check for the pointer alignment.
 */
inline bool is_aligned(void* pointer, std::size_t alignment) {
    return (reinterpret_cast<std::uintptr_t>(pointer) % alignment) == 0;
}

/**
 * Allocate aligned memory. This will be unified memory if the corresponding
 * CMake option is set. This must be freed with the free_memory method.
 *
 * \param size      Size of buffer to allocate in bytes.
 * \param alignment Memory alignment, defaults to NRN_SOA_BYTE_ALIGN. Pass 0 for no alignment.
 */
inline void* emalloc_align(size_t size, size_t alignment = NRN_SOA_BYTE_ALIGN) {
    void* memptr;
    alloc_memory(memptr, size, alignment);
    if (alignment != 0) {
        nrn_assert(is_aligned(memptr, alignment));
    }
    return memptr;
}

/**
 * Allocate the aligned memory and set it to 0. This will be unified memory if
 * the corresponding CMake option is set. This must be freed with the
 * free_memory method.
 *
 * \param n         Number of objects to allocate
 * \param size      Size of buffer for each object to allocate in bytes.
 * \param alignment Memory alignment, defaults to NRN_SOA_BYTE_ALIGN. Pass 0 for no alignment.
 *
 * \note the allocated size will be \code n*size
 */
inline void* ecalloc_align(size_t n, size_t size, size_t alignment = NRN_SOA_BYTE_ALIGN) {
    void* p;
    if (n == 0) {
        return nullptr;
    }
    calloc_memory(p, n * size, alignment);
    if (alignment != 0) {
        nrn_assert(is_aligned(p, alignment));
    }
    return p;
}
}  // namespace coreneuron
