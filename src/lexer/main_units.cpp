/*************************************************************************
 * Copyright (C) 2018-2019 Blue Brain Project
 *
 * This file is part of NMODL distributed under the terms of the GNU
 * Lesser General Public License. See top-level LICENSE file for details.
 *************************************************************************/

#include <fstream>

#include "CLI/CLI.hpp"
#include "fmt/format.h"

#include "config/config.h"
#include "lexer/unit_lexer.hpp"
#include "parser/unit_driver.hpp"
#include "utils/logger.hpp"

/**
 * \file
 * Example of standalone lexer program for Units that
 * demonstrate use of UnitLexer and UnitDriver classes.
 */

using namespace fmt::literals;
using namespace nmodl;

int main(int argc, const char* argv[]) {
    CLI::App app{"Unit-Lexer : Standalone Lexer for Units({})"_format(version::to_string())};

    std::vector<std::string> files;
    app.add_option("file", files, "One or more units files to process")
        ->required()
        ->check(CLI::ExistingFile);

    CLI11_PARSE(app, argc, argv);

    for (const auto& f: files) {
        nmodl::logger->info("Processing {}", f);
        std::ifstream file(f);
        nmodl::parser::UnitDriver driver;
        nmodl::parser::UnitLexer scanner(driver, &file);

        /// parse Units file and print token until EOF
        while (true) {
            auto sym = scanner.next_token();
            auto token = sym.token();
            if (token == nmodl::parser::UnitParser::token::END) {
                break;
            }
            std::cout << sym.value.as<std::string>() << std::endl;
        }
    }
    return 0;
}
