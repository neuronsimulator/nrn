/*************************************************************************
 * Copyright (C) 2018-2022 Blue Brain Project
 *
 * This file is part of NMODL distributed under the terms of the GNU
 * Lesser General Public License. See top-level LICENSE file for details.
 *************************************************************************/

#include <sstream>

#include <CLI/CLI.hpp>

#include "config/config.h"
#include "lexer/c11_lexer.hpp"
#include "parser/c11_driver.hpp"
#include "utils/logger.hpp"

/**
 * \file
 * \brief Example of standalone lexer program for C code
 *
 * This example demonstrate use of CLexer and CDriver classes
 * to scan arbitrary C code.
 */

using namespace nmodl;
using Token = parser::CParser::token;

void scan_c_code(std::istream& in) {
    nmodl::parser::CDriver driver;
    nmodl::parser::CLexer scanner(driver, &in);

    /// parse C file and print token until EOF
    while (true) {
        auto sym = scanner.next_token();
        auto token_type = sym.type_get();
        if (token_type == parser::CParser::by_type(Token::END).type_get()) {
            break;
        }
        std::cout << sym.value.as<std::string>() << std::endl;
    }
}


int main(int argc, const char* argv[]) {
    CLI::App app{fmt::format("C-Lexer : Standalone Lexer for C Code({})", Version::to_string())};

    std::vector<std::string> c_files;
    std::vector<std::string> c_codes;

    app.add_option("file", c_files, "One or more C files to process")->check(CLI::ExistingFile);
    app.add_option("--text", c_codes, "One or more C code as text");

    CLI11_PARSE(app, argc, argv);

    for (const auto& file: c_files) {
        nmodl::logger->info("Processing {}", file);
        std::ifstream in(file);
        scan_c_code(in);
    }

    for (const auto& code: c_codes) {
        nmodl::logger->info("Processing {}", code);
        std::istringstream in(code);
        scan_c_code(in);
    }

    return 0;
}
