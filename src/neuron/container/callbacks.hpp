#pragma once
#include <cassert>     // assert
#include <cstddef>     // std::byte
#include <functional>  // std::function
#include <tuple>       // std::tuple
#include <vector>      // std::vector
namespace neuron::container {
/**
 * @brief Data required to compute old -> new raw pointer mappings.
 */
struct ptr_update_data {
    [[nodiscard]] bool empty() {
        return m_permutations.empty() && m_reallocations.empty();
    }

    void clear() {
        m_permutations.clear();
        m_reallocations.clear();
    }

    void prepare();

    /**
     * @brief Register that `old_ptr` should be updated to `new_ptr`.
     *
     * This can easily be handled as a special case of reallocation, including if new_ptr is
     * nullptr.
     */
    template <typename T, typename U>
    void old_new_pair(T* old_ptr, U new_ptr) {
        static_assert(std::is_same_v<T*, U> || std::is_same_v<U, std::nullptr_t>);
        m_reallocations.emplace_back(static_cast<std::byte*>(static_cast<void*>(old_ptr)),
                                     static_cast<std::byte*>(static_cast<void*>(new_ptr)),
                                     sizeof(T),
                                     1);
    }

    /**
     * @brief Register that `data` is about to be permuted according to `reverse_permutation`
     *
     * `data` is the start of a range of `size_of_data` elements of size `size_of_type` that are
     * about to be reverse-permuted with `reverse_permutation`. The old i-th element of the range
     * will move to offset `reverse_permutation[i]`.
     */
    template <typename T>
    void permuted_vector(T* data, std::size_t* reverse_permutation, std::size_t size_of_data) {
        m_permutations.emplace_back(static_cast<std::byte*>(static_cast<void*>(data)),
                                    reverse_permutation,
                                    sizeof(T),
                                    size_of_data);
    }

    /**
     * @brief Register that a set of ranges has been reallocated
     *
     * A range of `old_size` elements of size `size_of_type` used to be stored at `old_data` and are
     * now stored at `new_data`.
     */
    void reallocation_data(std::vector<std::tuple<std::byte* /* old_data */,
                                                  std::byte* /* new_data */,
                                                  std::size_t /* size_of_type */,
                                                  std::size_t /* old_size */>> ptr_update_data) {
        assert(m_reallocations.empty());
        m_reallocations = std::move(ptr_update_data);
    }

    /**
     * @brief Calculate the new address corresponding to `ptr`
     */
    template <typename T>
    T* operator()(T* ptr) const {
        return static_cast<T*>(recalculate_ptr(ptr, sizeof(T)));
    }

  private:
    void* recalculate_ptr(void*, std::size_t) const;
    std::vector<std::tuple<std::byte*, std::size_t*, std::size_t, std::size_t>> m_permutations{};
    std::vector<std::tuple<std::byte*, std::byte*, std::size_t, std::size_t>> m_reallocations{};
};
using ptr_update_callback_t = std::function<void()>;
namespace detail {
extern ptr_update_data ptr_transformations;
extern std::vector<ptr_update_callback_t> ptr_update_callbacks;
}  // namespace detail
[[nodiscard]] inline bool ptr_update_callbacks_exist() {
    return !detail::ptr_update_callbacks.empty();
}
inline void register_ptr_update_callback(ptr_update_callback_t callback) {
    detail::ptr_update_callbacks.push_back(std::move(callback));
}
inline void remove_ptr_update_callbacks() {
    detail::ptr_update_callbacks.clear();
}
template <typename F>
void invoke_ptr_update_callbacks(F&& f) {
    if (detail::ptr_update_callbacks.empty()) {
        return;
    }
    assert(detail::ptr_transformations.empty());
    try {
        std::invoke(std::forward<F>(f), detail::ptr_transformations);
        detail::ptr_transformations.prepare();
        for (auto callback: detail::ptr_update_callbacks) {
            callback();
        }
    } catch (...) {
        detail::ptr_transformations.clear();
        throw;
    }
    detail::ptr_transformations.clear();
}
template <typename T>
T* recalculate_ptr(T* ptr) {
    return detail::ptr_transformations(ptr);
}
}  // namespace neuron::container
