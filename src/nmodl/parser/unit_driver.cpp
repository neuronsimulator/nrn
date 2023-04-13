/*************************************************************************
 * Copyright (C) 2018-2022 Blue Brain Project
 *
 * This file is part of NMODL distributed under the terms of the GNU
 * Lesser General Public License. See top-level LICENSE file for details.
 *************************************************************************/

#include <fstream>
#include <sstream>

#include "lexer/unit_lexer.hpp"
#include "parser/unit_driver.hpp"

namespace nmodl {
namespace parser {

/// parse Units file provided as istream
bool UnitDriver::parse_stream(std::istream& in) {
    UnitLexer scanner(*this, &in);
    UnitParser parser(scanner, *this);

    this->lexer = &scanner;
    this->parser = &parser;

    return (parser.parse() == 0);
}

/// parse Units file
bool UnitDriver::parse_file(const std::string& filename) {
    std::ifstream in(filename.c_str());
    stream_name = filename;

    if (!in.good()) {
        return false;
    }
    return parse_stream(in);
}

/// parser Units provided as string (used for testing)
bool UnitDriver::parse_string(const std::string& input) {
    std::istringstream iss(input);
    return parse_stream(iss);
}

}  // namespace parser
}  // namespace nmodl
