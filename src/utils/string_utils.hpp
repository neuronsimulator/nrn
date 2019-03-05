/*************************************************************************
 * Copyright (C) 2018-2019 Blue Brain Project
 *
 * This file is part of NMODL distributed under the terms of the GNU
 * Lesser General Public License. See top-level LICENSE file for details.
 *************************************************************************/

#pragma once

#include <algorithm>
#include <sstream>
#include <vector>


namespace nmodl {

/**
 * \brief String manipulation functions
 *
 * String trimming and manipulation functions based on
 * stackoverflow.com/questions/216823/whats-the-best-way-to-trim-stdstring
 */

enum class text_alignment { left, right, center };

namespace stringutils {
/// Trim from start
static inline std::string& ltrim(std::string& s) {
    s.erase(s.begin(),
            std::find_if(s.begin(), s.end(), std::not1(std::ptr_fun<int, int>(std::isspace))));
    return s;
}

/// Trim from end
static inline std::string& rtrim(std::string& s) {
    s.erase(
        std::find_if(s.rbegin(), s.rend(), std::not1(std::ptr_fun<int, int>(std::isspace))).base(),
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

}  // namespace stringutils

}  // namespace nmodl
