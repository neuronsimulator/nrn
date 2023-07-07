/*
 * Copyright 2023 Blue Brain Project, EPFL.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "common_utils.hpp"

#include <array>
#include <cassert>
#include <cerrno>
#include <fstream>
#include <iostream>
#include <random>
#include <stdexcept>
#include <string>
#include <string_view>
#include <sys/stat.h>

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32) && !defined(__CYGWIN__)
#define IS_WINDOWS
#endif

namespace nmodl {
namespace utils {

std::string generate_random_string(const int len, UseNumbersInString use_numbers) {
    std::string s(len, 0);
    constexpr std::size_t number_of_numbers{10};
    constexpr std::string_view alphanum{
        "0123456789"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz"};
    std::random_device dev;
    std::mt19937 rng(dev());
    std::uniform_int_distribution<std::mt19937::result_type> dist(use_numbers ? 0
                                                                              : number_of_numbers,
                                                                  alphanum.size() - 1);
    for (int i = 0; i < len; ++i) {
        s[i] = alphanum[dist(rng)];
    }
    return s;
}

}  // namespace utils
}  // namespace nmodl
