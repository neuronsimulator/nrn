/*************************************************************************
 * Copyright (C) 2018-2019 Blue Brain Project
 *
 * This file is part of NMODL distributed under the terms of the GNU
 * Lesser General Public License. See top-level LICENSE file for details.
 *************************************************************************/

#include <fstream>
#include <sstream>

#include "lexer/c11_lexer.hpp"
#include "parser/c11_driver.hpp"

namespace nmodl {
namespace parser {

CDriver::CDriver(bool strace, bool ptrace)
    : trace_scanner(strace)
    , trace_parser(ptrace) {}

/// parse c file provided as istream
bool CDriver::parse_stream(std::istream& in) {
    CLexer scanner(*this, &in);
    CParser parser(scanner, *this);

    this->lexer = &scanner;
    this->parser = &parser;

    scanner.set_debug(trace_scanner);
    parser.set_debug_level(trace_parser);
    return (parser.parse() == 0);
}

//// parse c file
bool CDriver::parse_file(const std::string& filename) {
    std::ifstream in(filename.c_str());
    streamname = filename;

    if (!in.good()) {
        return false;
    }
    return parse_stream(in);
}

/// parser c provided as string (used for testing)
bool CDriver::parse_string(const std::string& input) {
    std::istringstream iss(input);
    return parse_stream(iss);
}

void CDriver::error(const std::string& m) {
    std::cerr << m << std::endl;
}

void CDriver::add_token(const std::string& text) {
    tokens.push_back(text);
    // here we will query and look into symbol table or register callback
}

void CDriver::error(const std::string& m, const location& l) {
    std::cerr << l << " : " << m << std::endl;
}

void CDriver::scan_string(std::string& text) {
    std::istringstream in(text);
    CLexer scanner(*this, &in);
    CParser parser(scanner, *this);
    this->lexer = &scanner;
    this->parser = &parser;
    while (true) {
        auto sym = lexer->next_token();
        auto token = sym.token();
        if (token == CParser::token::END) {
            break;
        }
    }
}

}  // namespace parser
}  // namespace nmodl
