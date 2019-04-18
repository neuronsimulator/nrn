/*************************************************************************
 * Copyright (C) 2018-2019 Blue Brain Project
 *
 * This file is part of NMODL distributed under the terms of the GNU
 * Lesser General Public License. See top-level LICENSE file for details.
 *************************************************************************/

#include <fstream>
#include <sstream>

#include "lexer/nmodl_lexer.hpp"
#include "parser/nmodl_driver.hpp"

namespace nmodl {
namespace parser {

NmodlDriver::NmodlDriver(bool strace, bool ptrace)
    : trace_scanner(strace)
    , trace_parser(ptrace) {}

/// parse nmodl file provided as istream
bool NmodlDriver::parse_stream(std::istream& in) {
    NmodlLexer scanner(*this, &in);
    NmodlParser parser(scanner, *this);

    this->lexer = &scanner;
    this->parser = &parser;

    scanner.set_debug(trace_scanner);
    parser.set_debug_level(trace_parser);
    return (parser.parse() == 0);
}

//// parse nmodl file
bool NmodlDriver::parse_file(const std::string& filename) {
    std::ifstream in(filename.c_str());
    stream_name = filename;

    if (!in.good()) {
        return false;
    }
    return parse_stream(in);
}

/// parser nmodl provided as string (used for testing)
bool NmodlDriver::parse_string(const std::string& input) {
    std::istringstream iss(input);
    return parse_stream(iss);
}

void NmodlDriver::error(const std::string& m, const class location& l) {
    std::cerr << l << " : " << m << std::endl;
}

void NmodlDriver::error(const std::string& m) {
    std::cerr << m << std::endl;
}

/// add macro definition and it's value (DEFINE keyword of nmodl)
void NmodlDriver::add_defined_var(const std::string& name, int value) {
    defined_var[name] = value;
}

/// check if particular text is defined as macro
bool NmodlDriver::is_defined_var(const std::string& name) {
    return !(defined_var.find(name) == defined_var.end());
}

/// return variable's value defined as macro (always an integer)
int NmodlDriver::get_defined_var_value(const std::string& name) {
    if (is_defined_var(name)) {
        return defined_var[name];
    }
    throw std::runtime_error("Trying to get undefined macro / define :" + name);
}

}  // namespace parser
}  // namespace nmodl
