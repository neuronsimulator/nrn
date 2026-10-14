#pragma once

/**
 * Template parameters:
 *   T      - the element type
 *   Mutex  - if true, alloc/hpfree/free_all are protected by a mutex
 *
 * Constructor parameters:
 *   count    - initial number of slots
 *   subcount - subcount; each slot contains this many subobjects
 *
 * Usage:
 *   Pool<T>            - single-object pool, no mutex
 *   Pool<T, true>      - single-object pool with mutex protection
 *   Pool<T>(n, subcount) - array pool with subcount, no mutex
 *   Pool<T, true>(n)   - single-object pool with mutex
 *
 * Objects that define a clear() method will have it called during free_all().
 */

#include "memory.hpp"  // nrn_cacheline_calloc

#include <cassert>
#include <cstdlib>
#include <mutex>
#include <type_traits>
#include <vector>

// Detect if T has a clear() method
template <typename T, typename = void>
struct pool_has_clear: std::false_type {};

template <typename T>
struct pool_has_clear<T, std::void_t<decltype(std::declval<T>().clear())>>: std::true_type {};

namespace detail {

struct NullMutex {
    void lock() {}
    void unlock() {}
};

}  // namespace detail

template <typename T, bool Mutex = false>
class Pool {
  public:
    /**
     * count Initial number of items in the pool
     * subcount; the `item` may also be an array; this sets the number of elements in the
     * array
     */
    explicit Pool(long count, long subcount = 1);
    ~Pool();

    Pool(const Pool&) = delete;
    Pool& operator=(const Pool&) = delete;

    T* alloc();
    void hpfree(T*);
    void free_all();

    /** Grow pool by count items. */
    void grow(long count);

    /** Number of outstanding allocations. */
    long nget() const {
        return nget_;
    }

    /** Total number of allocations, over the lifetime of the pool. */
    long total_allocs() const {
        return total_allocs_;
    }

    long size() const {
        return count_;
    }

    long subcount() const {
        return subcount_;
    }

    /** Check if pointer is valid (points to an aligned element in any chain segment). */
    int is_valid_ptr(void* v) const;

  private:
    T* allocate_pool(long count);
    void deallocate_pool(T* p);

    using mutex_type = std::conditional_t<Mutex, std::recursive_mutex, detail::NullMutex>;

    std::vector<T*> freelist_;
    T* pool_{};
    long pool_size_{};
    long count_{};
    long subcount_{1};
    long nget_{};
    long total_allocs_{};
    Pool* chain_{};
    Pool* chainlast_{this};
    mutable mutex_type mut_;
};

template <typename T, bool Mutex>
T* Pool<T, Mutex>::allocate_pool(long count) {
    T* p{};
    if (subcount_ > 1) {
        p = static_cast<T*>(
            nrn_cacheline_calloc(reinterpret_cast<void**>(&p), count * subcount_, sizeof(T)));
    } else {
        p = new T[count];
    }
    return p;
}

template <typename T, bool Mutex>
void Pool<T, Mutex>::deallocate_pool(T* p) {
    if (subcount_ > 1) {
        free(p);
    } else {
        delete[] p;
    }
}

template <typename T, bool Mutex>
Pool<T, Mutex>::Pool(long count, long subcount)
    : count_(count)
    , subcount_(subcount) {
    pool_ = allocate_pool(count_);
    pool_size_ = count_;
    freelist_.reserve(count_);
    for (long i = 0; i < count_; ++i) {
        freelist_.push_back(pool_ + i * subcount_);
    }
}

template <typename T, bool Mutex>
Pool<T, Mutex>::~Pool() {
    delete chain_;
    deallocate_pool(pool_);
}

template <typename T, bool Mutex>
void Pool<T, Mutex>::grow(long count) {
    std::lock_guard<mutex_type> lock(mut_);
    Pool* p = new Pool(count, subcount_);
    chainlast_->chain_ = p;
    chainlast_ = p;

    for (long j = 0; j < count; ++j) {
        freelist_.push_back(p->pool_ + j * subcount_);
    }
    count_ += count;
}

template <typename T, bool Mutex>
T* Pool<T, Mutex>::alloc() {
    std::lock_guard<mutex_type> lock(mut_);
    if (freelist_.empty()) {
        grow(std::max(1L, static_cast<long>(freelist_.capacity())));
    }
    T* item = freelist_.back();
    freelist_.pop_back();
    ++nget_;
    ++total_allocs_;
    return item;
}

template <typename T, bool Mutex>
void Pool<T, Mutex>::hpfree(T* item) {
    std::lock_guard<mutex_type> lock(mut_);
    assert(nget_ > 0);
    //freelist_.push_back(item);
    freelist_.insert(freelist_.begin(), item);
    --nget_;
}

template <typename T, bool Mutex>
void Pool<T, Mutex>::free_all() {
    std::lock_guard<mutex_type> lock(mut_);
    Pool* pp;
    nget_ = 0;
    freelist_.clear();
    for (pp = this; pp; pp = pp->chain_) {
        for (long i = 0; i < pp->pool_size_; ++i) {
            freelist_.push_back(pp->pool_ + i * subcount_);
            if constexpr (pool_has_clear<T>::value) {
                (pp->pool_ + i * subcount_)->clear();
            }
        }
    }
}

template <typename T, bool Mutex>
int Pool<T, Mutex>::is_valid_ptr(void* v) const {
    const Pool* pp;
    long item_size = static_cast<long>(sizeof(T)) * subcount_;
    for (pp = this; pp; pp = pp->chain_) {
        void* vp = static_cast<void*>(pp->pool_);
        if (v >= vp && v < static_cast<void*>(pp->pool_ + pp->pool_size_ * subcount_)) {
            if ((static_cast<char*>(v) - static_cast<char*>(vp)) % item_size == 0) {
                return 1;
            } else {
                return 0;
            }
        }
    }
    return 0;
}
