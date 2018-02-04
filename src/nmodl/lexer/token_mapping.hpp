#pragma once

#include <string>
#include "parser/nmodl/nmodl_parser.hpp"

namespace nmodl {
    bool is_keyword(const std::string& name);
    bool is_method(const std::string& name);
    nmodl::Parser::token_type token_type(const std::string& name);
    std::vector<std::string> get_external_variables();
    std::vector<std::string> get_external_functions();
}  // namespace nmodl
