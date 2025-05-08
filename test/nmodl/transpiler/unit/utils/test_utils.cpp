/*
 * Copyright 2023 Blue Brain Project, EPFL.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "test_utils.hpp"
#include "nmodl/utils/logger.hpp"
#include "utils/string_utils.hpp"

#include <cassert>

#include <spdlog/sinks/ostream_sink.h>

namespace nmodl {
namespace test_utils {

int count_leading_spaces(std::string text) {
    auto const length = text.size();
    text = nmodl::stringutils::ltrim(text);
    auto const num_whitespaces = length - text.size();
    assert(num_whitespaces <= std::numeric_limits<int>::max());
    return static_cast<int>(num_whitespaces);
}

/// check if string has only whitespaces
bool is_empty(const std::string& text) {
    return nmodl::stringutils::trim(text).empty();
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
 * Then for every sub-sequent line, we remove first X characters (assuming those
 * all are whitespaces). This is done because when ast is transformed back to
 * nmodl, the nmodl output is without "extra" whitespaces in the provided input.
 */

std::string reindent_text(const std::string& text, int indent_level) {
    std::string indented_text;
    int num_whitespaces = 0;
    bool flag = false;
    std::string line;
    std::stringstream stream(text);
    std::string indent(4 * indent_level, ' ');

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
            indented_text += indent + line;
        }
        /// discard empty lines at very beginning
        if (!stream.eof() && flag) {
            indented_text += "\n";
        }
    }
    return indented_text;
}

LoggerCapture::LoggerCapture()
    : original_logger(nmodl::logger) {
    capture_stream = std::make_shared<std::ostringstream>();
    auto capture_sink = std::make_shared<spdlog::sinks::ostream_sink_mt>(*capture_stream);
    auto test_logger = std::make_shared<spdlog::logger>("TEST_LOGGER", capture_sink);
    test_logger->set_pattern("[%n] [%l] :: %v");
    nmodl::logger = test_logger;
}

LoggerCapture::~LoggerCapture() {
    nmodl::logger = original_logger;
}

std::string LoggerCapture::output() const {
    return capture_stream->str();
}


}  // namespace test_utils
}  // namespace nmodl
