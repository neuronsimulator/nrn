/*
 * Copyright 2023 Blue Brain Project, EPFL.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <string>

#include "nmodl/utils/logger.hpp"

namespace nmodl {
namespace test_utils {

std::string reindent_text(const std::string& text, int indent_level = 0);

// RAII state guard for the actual log
class LoggerCapture {
  public:
    LoggerCapture();
    ~LoggerCapture();
    std::string output() const;

  private:
    std::shared_ptr<std::ostringstream> capture_stream;
    nmodl::logger_type original_logger;
};
}  // namespace test_utils
}  // namespace nmodl
