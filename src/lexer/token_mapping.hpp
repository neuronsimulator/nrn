/*************************************************************************
 * Copyright (C) 2018-2019 Blue Brain Project
 *
 * This file is part of NMODL distributed under the terms of the GNU
 * Lesser General Public License. See top-level LICENSE file for details.
 *************************************************************************/

#pragma once

#include "parser/nmodl/nmodl_parser.hpp"
#include <string>

namespace nmodl {

bool is_keyword(const std::string& name);
bool is_method(const std::string& name);
nmodl::parser::NmodlParser::token_type token_type(const std::string& name);
std::vector<std::string> get_external_variables();
std::vector<std::string> get_external_functions();

}  // namespace nmodl
