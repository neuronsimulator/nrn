/*
 * Copyright 2023 Blue Brain Project, EPFL.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "config/config.h"
#include "lexer/unit_lexer.hpp"
#include "parser/unit_driver.hpp"
#include "utils/logger.hpp"

#include <CLI/CLI.hpp>

#include <fstream>

/**
 * \file
 * Example of standalone lexer program for Units that
 * demonstrate use of UnitLexer and UnitDriver classes.
 */

using namespace nmodl;
using Token = parser::UnitParser::token;

int main(int argc, const char* argv[]) {
    CLI::App app{fmt::format("Unit-Lexer : Standalone Lexer for Units({})", Version::to_string())};

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
            auto token_type = sym.type_get();
            if (token_type == parser::UnitParser::by_type(Token::END).type_get()) {
                break;
            }
            std::cout << sym.value.as<std::string>() << std::endl;
        }
    }
    return 0;
}
