/*
 * Copyright 2023 Blue Brain Project, EPFL.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

/**
 * \file token_mapping.hpp
 * \brief Map different tokens from lexer to token types
 */

#include <string>

#include "parser/nmodl/nmodl_parser.hpp"

namespace nmodl {

bool is_keyword(const std::string& name);
bool is_method(const std::string& name);

parser::NmodlParser::token_type token_type(const std::string& name);
std::vector<std::string> get_external_variables();
std::vector<std::string> get_external_functions();

namespace details {

bool needs_neuron_thread_first_arg(const std::string& token);

}  // namespace details

}  // namespace nmodl
