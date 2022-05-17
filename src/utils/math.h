#pragma once

#include <cmath>
namespace utils {
template <typename T, typename U, typename V>
bool equal(T x, U y, V e) {
    return std::abs(x - y) < e;
}
}

