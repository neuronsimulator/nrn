#pragma once

#include <algorithm>
#include <functional>
#include <tuple>

template <typename T,
          typename F,
          typename value_type = typename T::value_type,
          typename TIter = decltype(std::begin(std::declval<T>())),
          typename = decltype(std::end(std::declval<T>()))>
void apply_to_first(T&& iterable, value_type&& value, F&& f) {
    auto it = std::find(std::begin(std::forward<T>(iterable)),
                        std::end(std::forward<T>(iterable)),
                        std::forward<value_type>(value));
    if (it != std::end(std::forward<T>(iterable))) {
        f(it);
    }
}

template <typename T, typename value_type = typename T::value_type>
void erase_first(T&& iterable, value_type&& value) {
    apply_to_first(std::forward<T>(iterable),
                   std::forward<value_type>(value),
                   [&iterable](const auto& it) { iterable.erase(it); });
}

template <typename T,
          typename TIter = decltype(std::begin(std::declval<T>())),
          typename = decltype(std::begin(std::declval<T>())),
          typename = decltype(std::end(std::declval<T>()))>
constexpr auto range(T&& iterable) {
    struct iterator {
        std::size_t i;
        TIter iter;
        bool operator!=(const iterator& other) const {
            return iter != other.iter;
        }
        void operator++() {
            ++i;
            ++iter;
        }
        auto operator*() const {
            return i;
        }
    };
    struct iterable_wrapper {
        T iterable;
        auto begin() {
            return iterator{0, std::begin(iterable)};
        }
        auto end() {
            return iterator{0, std::end(iterable)};
        }
    };
    return iterable_wrapper{std::forward<T>(iterable)};
}

template <typename T,
          typename TIter = decltype(std::rbegin(std::declval<T>())),
          typename = decltype(std::rend(std::declval<T>()))>
constexpr auto reverse(T&& iterable) {
    struct iterator {
        TIter iter;
        bool operator!=(const iterator& other) const {
            return iter != other.iter;
        }
        void operator++() {
            ++iter;
        }
        auto&& operator*() const {
            return *iter;
        }
    };
    struct iterable_wrapper {
        T iterable;
        auto begin() {
            return iterator{std::rbegin(iterable)};
        }
        auto end() {
            return iterator{std::rend(iterable)};
        }
    };
    return iterable_wrapper{std::forward<T>(iterable)};
}

template <typename T,
          typename TIter = decltype(std::begin(std::declval<T>())),
          typename = decltype(std::end(std::declval<T>()))>
constexpr auto enumerate(T&& iterable) {
    struct iterator {
        std::size_t i;
        TIter iter;
        bool operator!=(const iterator& other) const {
            return iter != other.iter;
        }
        void operator++() {
            ++i;
            ++iter;
        }
        auto operator*() const {
            return std::tie(i, *iter);
        }
    };
    struct iterable_wrapper {
        T iterable;
        auto begin() {
            return iterator{0, std::begin(iterable)};
        }
        auto end() {
            return iterator{0, std::end(iterable)};
        }
    };
    return iterable_wrapper{std::forward<T>(iterable)};
}

template <typename T,
          typename TIter = decltype(std::rbegin(std::declval<T>())),
          typename = decltype(std::rend(std::declval<T>()))>
constexpr auto renumerate(T&& iterable) {
    struct iterator {
        std::size_t i;
        TIter iter;
        bool operator!=(const iterator& other) const {
            return iter != other.iter;
        }
        void operator++() {
            --i;
            ++iter;
        }
        auto operator*() const {
            return std::tie(i, *iter);
        }
    };
    struct iterable_wrapper {
        T iterable;
        auto begin() {
            return iterator{std::size(iterable) - 1, std::rbegin(iterable)};
        }
        auto end() {
            return iterator{std::size(iterable) - 1, std::rend(iterable)};
        }
    };
    return iterable_wrapper{std::forward<T>(iterable)};
}
