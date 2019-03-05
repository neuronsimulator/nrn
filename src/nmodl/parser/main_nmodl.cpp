/*************************************************************************
 * Copyright (C) 2018-2019 Blue Brain Project
 *
 * This file is part of NMODL distributed under the terms of the GNU
 * Lesser General Public License. See top-level LICENSE file for details.
 *************************************************************************/


#include "CLI/CLI.hpp"
#include "parser/nmodl_driver.hpp"
#include "utils/logger.hpp"

/**
 * Standalone parser program for NMODL. This demonstrate
 * basic usage of parser and driver classes.
 */

int main(int argc, const char* argv[]) {
    CLI::App app{"NMODL-Parser : Standalone Parser for NMODL"};

    std::vector<std::string> files;
    app.add_option("file", files, "One or more MOD files to process")
        ->required()
        ->check(CLI::ExistingFile);

    CLI11_PARSE(app, argc, argv);

    for (const auto& f: files) {
        nmodl::logger->info("Processing {}", f);
        nmodl::parser::NmodlDriver driver;
        driver.set_verbose(true);
        driver.parse_file(f);
    }
    return 0;
}