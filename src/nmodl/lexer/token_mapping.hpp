/*************************************************************************
 * Copyright (C) 2018-2022 Blue Brain Project
 *
 * This file is part of NMODL distributed under the terms of the GNU
 * Lesser General Public License. See top-level LICENSE file for details.
 *************************************************************************/

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
