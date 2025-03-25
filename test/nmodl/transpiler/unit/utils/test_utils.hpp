/*
 * Copyright 2023 Blue Brain Project, EPFL.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <string>

namespace nmodl {
namespace test_utils {

std::string reindent_text(const std::string& text, int indent_level = 0);

}  // namespace test_utils
}  // namespace nmodl
