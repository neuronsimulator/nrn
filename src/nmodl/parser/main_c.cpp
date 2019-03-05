/*************************************************************************
 * Copyright (C) 2018-2019 Blue Brain Project
 *
 * This file is part of NMODL distributed under the terms of the GNU
 * Lesser General Public License. See top-level LICENSE file for details.
 *************************************************************************/

#include <fstream>

#include "CLI/CLI.hpp"
#include "parser/c11_driver.hpp"
#include "utils/logger.hpp"

/**
 * Standalone parser program for C. This demonstrate basic
 * usage of parser and driver class.
 */

int main(int argc, const char* argv[]) {
    CLI::App app{"C-Parser : Standalone Parser for C Code"};

    std::vector<std::string> files;
    app.add_option("file", files, "One or more C files to process")
        ->required()
        ->check(CLI::ExistingFile);

    CLI11_PARSE(app, argc, argv);

    for (const auto& f: files) {
        nmodl::logger->info("Processing {}", f);
        std::ifstream file(f);

        /// driver object creates lexer and parser
        nmodl::parser::CDriver driver;
        driver.set_verbose(true);

        /// just call parser method
        driver.parse_stream(file);
    }
    return 0;
}
