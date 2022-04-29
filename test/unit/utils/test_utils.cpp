/*************************************************************************
 * Copyright (C) 2018-2022 Blue Brain Project
 *
 * This file is part of NMODL distributed under the terms of the GNU
 * Lesser General Public License. See top-level LICENSE file for details.
 *************************************************************************/

#include "utils/string_utils.hpp"

namespace nmodl {
namespace test_utils {

int count_leading_spaces(std::string text) {
    int length = text.size();
    nmodl::stringutils::ltrim(text);
    int num_whitespaces = length - text.size();
    return num_whitespaces;
}

/// check if string has only whitespaces
bool is_empty(std::string text) {
    nmodl::stringutils::trim(text);
    return text.empty();
}

/** Reindent nmodl text for text-to-text comparison
 *
 * Nmodl constructs defined in test database has extra leading whitespace.
 * This is done for readability reason in nmodl_constructs.cpp. For example,
 * we have following nmodl text with 8 leading whitespaces:


        NEURON {
            RANGE x
        }

 * We convert above paragraph to:

NEURON {
    RANGE x
}

 * i.e. we get first non-empty line and count number of leading whitespaces (X).
 * Then for every sub-sequent line, we remove first X characters (assuing those
 * all are whitespaces). This is done because when ast is transformed back to
 * nmodl, the nmodl output is without "extra" whitespaces in the provided input.
 */

std::string reindent_text(const std::string& text) {
    std::string indented_text;
    int num_whitespaces = 0;
    bool flag = false;
    std::string line;
    std::stringstream stream(text);

    while (std::getline(stream, line)) {
        if (!line.empty()) {
            /// count whitespaces for first non-empty line only
            if (!flag) {
                flag = true;
                num_whitespaces = count_leading_spaces(line);
            }

            /// make sure we don't remove non whitespaces characters
            if (!is_empty(line.substr(0, num_whitespaces))) {
                throw std::runtime_error("Test nmodl input not correctly formatted");
            }

            line.erase(0, num_whitespaces);
            indented_text += line;
        }
        /// discard empty lines at very beginning
        if (!stream.eof() && flag) {
            indented_text += "\n";
        }
    }
    return indented_text;
}

}  // namespace test_utils
}  // namespace nmodl
