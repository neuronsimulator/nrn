#pragma once

// create and manage a vector of objects as a memory pool of those objects
// the object must have a void clear() method which takes care of any
// data the object contains which should be deleted upon free_all.
// clear() is NOT called on free, only on free_all.

// the chain of Pool
// is only for extra items in a pool_ and no other fields are used.
// the pool doubles in size every time a chain Pool is added.

#include <nrnmutdec.h>

#include <list>
#include <vector>

template <typename T, std::size_t Count = 1000>
class MutexPool {
  public:
    MutexPool();
    ~MutexPool();
    T* alloc();
    void hpfree(T*);
    void free_all();

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
    MUTDEC
};

template <typename T, std::size_t Count>
MutexPool<T, Count>::MutexPool() {
    auto* ptr = pools_.emplace_back(Count).data();
    items_.resize(Count);
    for (std::size_t i = 0; i < Count; ++i) {
        items_[i] = ptr + i;
    }
    MUTCONSTRUCT(1)
}

template <typename T, std::size_t Count>
void MutexPool<T, Count>::grow() {
    std::size_t total_size = items_.size();

    // Everything is already allocated so reset put_ to the beginning of items
    // and set get_ to the new space allocated pointing to the new pool
    put_ = 0;
    get_ = total_size;

    items_.resize(2 * total_size);

    auto* ptr = pools_.emplace_back(total_size).data();
    for (std::size_t i = total_size, j = 0; j < 2 * total_size; ++i, ++j) {
        items_[i] = ptr + j;
    }
}

template <typename T, std::size_t Count>
MutexPool<T, Count>::~MutexPool() {
    MUTDESTRUCT
}

template <typename T, std::size_t Count>
T* MutexPool<T, Count>::alloc() {
    MUTLOCK
    if (nget_ == items_.size()) {
        grow();
    }
    ++nget_;
    T* item = items_[get_];
    get_ = (++get_) % items_.size();

    MUTUNLOCK
    return item;
}

template <typename T, std::size_t Count>
void MutexPool<T, Count>::hpfree(T* item) {
    MUTLOCK
    --nget_;
    items_[put_] = item;
    put_ = (++put_) % items_.size();
    MUTUNLOCK
}

template <typename T, std::size_t Count>
void MutexPool<T, Count>::free_all() {
    MUTLOCK
    get_ = 0;
    put_ = 0;
    for (auto& item: items_) {
        item->clear();
    }
    MUTUNLOCK
}
