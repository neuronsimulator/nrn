/*
 * Copyright 2023 Blue Brain Project, EPFL.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

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

namespace nmodl {
/// string utility functions
namespace stringutils {

/**
 * \addtogroup utils
 * \{
 */

/// text alignment when printing in the tabular form
enum class text_alignment { left, right, center };

/**
 * \param text the string to manipulate
 * \return a copy of the given string with leading ASCII space characters removed
 */
[[nodiscard]] static inline std::string ltrim(std::string text) {
    text.erase(text.begin(),
               std::find_if(text.begin(), text.end(), [](int c) { return !std::isspace(c); }));
    return text;
}

/**
 * \param text the string to manipulate
 * \return a copy of the given string with trailing characters removed.
 */
[[nodiscard]] static inline std::string rtrim(std::string text) {
    text.erase(
        std::find_if(text.rbegin(), text.rend(), [](int c) { return !std::isspace(c); }).base(),
        text.end());
    return text;
}

/**
 *
 * \param text the string to manipulate
 * \return a copy of the given string with both leading and trailing ASCII space characters removed
 */
[[nodiscard]] static inline std::string trim(std::string text) {
    return ltrim(rtrim(std::move(text)));
}

/**
 * Remove all occurrences of a given character in a text
 * \param text the string to manipulate
 * \param c the character to remove
 * @return a copy the modified text
 */
[[nodiscard]] static inline std::string remove_character(std::string text, const char c) {
    text.erase(std::remove(text.begin(), text.end(), c), text.end());
    return text;
}

/**
 *
 * \param text the string to manipulate
 * \return a copy of the given string with all occurrences of the ASCII newline character removed
 */
[[nodiscard]] static inline std::string trim_newline(std::string text) {
    return remove_character(std::move(text), '\n');
}

/**
 * Escape double-quote in a text, useful for JSON pretty printer.
 * \param text the string to manipulate
 * \return a copy of the given string with every " and \ characters prefixed with an extra \
 */
[[nodiscard]] static inline std::string escape_quotes(const std::string& text) {
    std::ostringstream oss;
    std::string result;

    for (auto c: text) {
        switch (c) {
        case '"':
        case '\\':
            result += '\\';
            /// don't break here as we want to append actual character

        default:
            result += c;
        }
    }

    return result;
}

/**
 * Split a text in a list of words, using a given delimiter character
 * \param text the string to manipulate
 * \param delimiter the delimiter character
 * \return a container holding copies of the substrings
 */
[[nodiscard]] static inline std::vector<std::string> split_string(const std::string& text,
                                                                  char delimiter) {
    std::vector<std::string> elements;
    std::stringstream ss(text);
    std::string item;

    while (std::getline(ss, item, delimiter)) {
        elements.push_back(item);
    }

    return elements;
}

///
/**
 * Aligns a text within a field of width \a width
 * \param text the string to manipulate
 * \param width the width of the field
 * \param type the kind of alignment Left, Right, or Center
 * \return a copy of the string aligned
 */
[[nodiscard]] static inline std::string align_text(const std::string& text,
                                                   int width,
                                                   text_alignment type) {
    /// left and right spacing
    std::string left, right;

    /// count excess room to pad
    const int padding = width - static_cast<int>(text.size());

    if (padding > 0) {
        if (type == text_alignment::left) {
            right = std::string(padding, ' ');
        } else if (type == text_alignment::right) {
            left = std::string(padding, ' ');
        } else {
            left = std::string(padding / 2, ' ');
            right = std::string(padding / 2, ' ');
            /// if odd #, add one more space
            if (padding % 2 != 0) {
                right += " ";
            }
        }
    }
    return left + text + right;
}

/// To lower case
/**
 * \param text the string to manipulate
 * \return a copy of string converted to lowercase
 */
[[nodiscard]] static inline std::string tolower(std::string text) {
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
std::string to_string(double value, const std::string& format_spec = "{:.16g}");

/** \} */  // end of utils

}  // namespace stringutils
}  // namespace nmodl
