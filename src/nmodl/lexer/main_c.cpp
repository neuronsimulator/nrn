/*************************************************************************
 * Copyright (C) 2018-2019 Blue Brain Project
 *
 * This file is part of NMODL distributed under the terms of the GNU
 * Lesser General Public License. See top-level LICENSE file for details.
 *************************************************************************/

#include <fstream>

#include "CLI/CLI.hpp"
#include "fmt/format.h"

#include "lexer/c11_lexer.hpp"
#include "parser/c11_driver.hpp"
#include "utils/logger.hpp"
#include "version/version.h"

/**
 * Example of standalone lexer program for C codes that
 * demonstrate use of CLexer and CDriver classes.
 */

using namespace fmt::literals;
using namespace nmodl;

int main(int argc, const char* argv[]) {
    CLI::App app{"C-Lexer : Standalone Lexer for C Code({})"_format(version::to_string())};

    std::vector<std::string> files;
    app.add_option("file", files, "One or more C files to process")
        ->required()
        ->check(CLI::ExistingFile);

    CLI11_PARSE(app, argc, argv);

    for (const auto& f: files) {
        nmodl::logger->info("Processing {}", f);
        std::ifstream file(f);
        nmodl::parser::CDriver driver;
        nmodl::parser::CLexer scanner(driver, &file);

        /// parse C file and print token until EOF
        while (true) {
            auto sym = scanner.next_token();
            auto token = sym.token();
            if (token == nmodl::parser::CParser::token::END) {
                break;
            }
            std::cout << sym.value.as<std::string>() << std::endl;
        }
    }
    return 0;
}
