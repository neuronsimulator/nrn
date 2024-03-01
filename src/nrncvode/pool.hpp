#pragma once

// create and manage a vector of objects as a memory pool of those objects
// the object must have a void clear() method which takes care of any
// data the object contains which should be deleted upon free_all.
// clear() is NOT called on deallocate, only on free_all.

// the pool doubles in size every time a new pool is added.

#include <list>
#include <vector>
#include <stdexcept>
#include <mutex>

template <typename T>
class MutexPool {
  public:
    using value_type = T;
    MutexPool();
    T* allocate(std::size_t = 1);
    void deallocate(T*, std::size_t = 1);
    void free_all();

    // For now this pool only allow allocation of 1 byte at a time
    std::size_t max_size() const {
        return 1;
    }

  private:
    // Add a new vector to the pools_ with the double of size of the previous one
    // Reset put_ and get_
    void grow();
    // A ring-buffer covering all the pools_
    // put_ is the place where to put the next freed item
    // get_ is the place where to get a new item
    std::vector<T*> items_{};
    std::size_t get_{};
    std::size_t put_{};
    // Number of items currently allocated in the pools
    // When this number equal the total number of elements it will be time to grow()
    std::size_t nget_{};
    // A vector of all the pools_
    std::vector<std::vector<T>> pools_{};
#if NRN_ENABLE_THREADS
    std::recursive_mutex mut_;
#endif
};

template <typename T>
MutexPool<T>::MutexPool() {
    constexpr std::size_t count = 1000;
    auto* ptr = pools_.emplace_back(count).data();
    items_.resize(count);
    for (std::size_t i = 0; i < count; ++i) {
        items_[i] = ptr + i;
    }
}

template <typename T>
void MutexPool<T>::grow() {
    std::size_t total_size = items_.size();

    // Everything is already allocated so reset put_ to the beginning of items
    // and set get_ to the new space allocated pointing to the new pool
    put_ = 0;
    get_ = total_size;

    items_.resize(2 * total_size);

    auto* ptr = pools_.emplace_back(total_size).data();
    for (std::size_t i = total_size, j = 0; j < total_size; ++i, ++j) {
        items_[i] = ptr + j;
    }
}

template <typename T>
T* MutexPool<T>::allocate(std::size_t n) {
#if NRN_ENABLE_THREADS
    std::lock_guard<std::recursive_mutex> l(mut_);
#endif
    if (n != 1) {
        throw std::runtime_error("MutexPool allocator can only allocate one object at a time");
    }
    if (nget_ == items_.size()) {
        grow();
    }
    ++nget_;
    T* item = items_[get_];
    get_ = (++get_) % items_.size();

    return item;
}

template <typename T>
void MutexPool<T>::deallocate(T* item, std::size_t) {
#if NRN_ENABLE_THREADS
    std::lock_guard<std::recursive_mutex> l(mut_);
#endif
    --nget_;
    items_[put_] = item;
    put_ = (++put_) % items_.size();
}

template <typename T>
void MutexPool<T>::free_all() {
#if NRN_ENABLE_THREADS
    std::lock_guard<std::recursive_mutex> l(mut_);
#endif
    get_ = 0;
    put_ = 0;
    for (auto& item: items_) {
        item->clear();
    }
}
