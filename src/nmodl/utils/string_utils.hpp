/*************************************************************************
 * Copyright (C) 2018-2019 Blue Brain Project
 *
 * This file is part of NMODL distributed under the terms of the GNU
 * Lesser General Public License. See top-level LICENSE file for details.
 *************************************************************************/

#pragma once

/**
 * \file
 * \brief Implement string manipulation functions
 *
 * String trimming and manipulation functions based on
 * stackoverflow.com/questions/216823/whats-the-best-way-to-trim-stdstring
 */

#include <algorithm>
#include <cctype>
#include <cmath>
#include <functional>
#include <sstream>
#include <vector>

#include <spdlog/spdlog.h>

namespace nmodl {
/// string utility functions
namespace stringutils {

/**
 * @addtogroup utils
 * @{
 */

/// text alignment when printing in the tabular form
enum class text_alignment { left, right, center };

/// Trim from start
static inline std::string& ltrim(std::string& s) {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](int c) { return !std::isspace(c); }));
    return s;
}

/// Trim from end
static inline std::string& rtrim(std::string& s) {
    s.erase(std::find_if(s.rbegin(), s.rend(), [](int c) { return !std::isspace(c); }).base(),
            s.end());
    return s;
}

/// Trim from both ends
static inline std::string& trim(std::string& s) {
    return ltrim(rtrim(s));
}

static inline void remove_character(std::string& str, const char c) {
    str.erase(std::remove(str.begin(), str.end(), c), str.end());
}

/// Remove leading newline for the string read by grammar
static inline std::string& trim_newline(std::string& s) {
    remove_character(s, '\n');
    return s;
}

/// for printing json, we have to escape double quotes
static inline std::string escape_quotes(const std::string& before) {
    std::string after;

    for (auto c: before) {
        switch (c) {
        case '"':
        case '\\':
            after += '\\';
            /// don't break here as we want to append actual character

        default:
            after += c;
        }
    }

    return after;
}

/// Spilt string with given delimiter and returns vector
static inline std::vector<std::string> split_string(const std::string& text, char delimiter) {
    std::vector<std::string> elements;
    std::stringstream ss(text);
    std::string item;

    while (std::getline(ss, item, delimiter)) {
        elements.push_back(item);
    }

    return elements;
}

/// Left/Right/Center-aligns string within a field of width "width"
static inline std::string align_text(std::string text, int width, text_alignment type) {
    /// left and right spacing
    std::string left, right;

    /// count excess room to pad
    int padding = width - text.size();

    if (padding > 0) {
        if (type == text_alignment::left) {
            right = std::string(padding, ' ');
        } else if (type == text_alignment::right) {
            left = std::string(padding, ' ');
        } else {
            left = std::string(padding / 2, ' ');
            right = std::string(padding / 2, ' ');
            /// if odd #, add one more space
            if (padding > 0 && padding % 2 != 0) {
                right += " ";
            }
        }
    }
    return left + text + right;
}

/// To lower case
static inline std::string tolower(std::string text) {
    std::transform(text.begin(), text.end(), text.begin(), ::tolower);
    return text;
}

/**
 * Convert double value to string without trailing zeros
 *
 * When we use std::to_string with double value 1 then it gets
 * printed as `1.000000`. This is not convenient for testing
 * and testing/validation. To avoid this issue, we use to_string
 * for integer values and stringstream for the rest.
 */
static inline std::string to_string(double value, const std::string& format_spec = "{:.16g}") {
    // double containing integer value
    if (std::ceil(value) == value &&
        value < static_cast<double>(std::numeric_limits<long long>::max()) &&
        value > static_cast<double>(std::numeric_limits<long long>::min())) {
        return std::to_string(static_cast<long long>(value));
    }

    // actual float value
    return fmt::format(format_spec, value);
}

/** @} */  // end of utils

}  // namespace stringutils
}  // namespace nmodl
