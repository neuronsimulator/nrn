/*
 * Copyright 2023 Blue Brain Project, EPFL.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <filesystem>
#include <string>

namespace nmodl {
namespace test_utils {

std::string reindent_text(const std::string& text);

/**
 * \brief Create an empty file which is then removed when the C++ object is destructed
 */
struct TempFile {
    TempFile(std::filesystem::path path, const std::string& content);
    ~TempFile();

  private:
    std::filesystem::path path_;
};

}  // namespace test_utils
}  // namespace nmodl
