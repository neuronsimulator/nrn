#pragma once

#include <string>
#include "parser/nmodl_parser.hpp"

namespace nmodl {
    bool is_keyword(std::string name);
    bool is_method(std::string name);
    nmodl::Parser::token_type token_type(std::string name);
    std::vector<std::string> get_external_variables();
    std::vector<std::string> get_external_functions();
}  // namespace nmodl