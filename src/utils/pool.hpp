#pragma once

// create and manage a vector of objects as a memory pool of those objects
// the object must have a void clear() method which takes care of any
// data the object contains which should be deleted upon free_all.
// clear() is NOT called on deallocate, only on free_all.

// the pool doubles in size every time a new pool is added.

#include <vector>
#include <stdexcept>
#include <functional>

template <typename T>
class Pool {
  public:
    using value_type = T;
    Pool();
    template <typename F>
    void set_function(F f) {
        clear_ = f;
    }
    T* allocate(std::size_t = 1);
    void deallocate(T*, std::size_t = 1);
    void free_all();

    // For now this pool only allow allocation of 1 byte at a time
    std::size_t max_size() const {
        return 1;
    }

    bool is_valid_ptr(const T*) const;

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

    std::function<void(T*)> clear_{};
};

template <typename T>
Pool<T>::Pool() {
    constexpr std::size_t count = 1000;
    auto& ptr = pools_.emplace_back(count);
    items_.resize(count);
    std::transform(ptr.begin(), ptr.end(), items_.begin(), [](auto& it) { return &it; });
}

template <typename T>
void Pool<T>::grow() {
    const std::size_t total_size = items_.size();

    // Everything is already allocated so reset put_ to the beginning of items
    // and set get_ to the new space allocated pointing to the new pool
    put_ = 0;
    get_ = total_size;

    items_.resize(2 * total_size);

    auto& ptr = pools_.emplace_back(total_size);
    std::transform(ptr.begin(), ptr.end(), items_.begin() + total_size, [](auto& it) {
        return &it;
    });
}

template <typename T>
T* Pool<T>::allocate(std::size_t n) {
    if (n != 1) {
        throw std::runtime_error("Pool allocator can only allocate one object at a time");
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
void Pool<T>::deallocate(T* item, std::size_t) {
    --nget_;
    items_[put_] = item;
    put_ = (++put_) % items_.size();
}

template <typename T>
void Pool<T>::free_all() {
    get_ = 0;
    put_ = 0;
    nget_ = 0;
    // Populate again items_ with real ones
    auto it = items_.begin();
    for (auto& pool: pools_) {
        std::transform(pool.begin(), pool.end(), it, [](auto& i) { return &i; });
        it += pool.size();
    }
    if (clear_) {
        for (auto& item: items_) {
            std::invoke(clear_, item);
        }
    }
}

template <typename T>
bool Pool<T>::is_valid_ptr(const T* p) const {
    for (const auto& pool: pools_) {
        if (p >= &pool.front() && p <= &pool.back()) {
            // Check that the pointer is on a value and not in the middle
            return (p - &pool.front()) % sizeof(T) == 0;
        }
    }
    return false;
}
