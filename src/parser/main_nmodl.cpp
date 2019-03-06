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

    std::vector<std::string> mod_files;
    std::vector<std::string> mod_texts;

    app.add_option("file", mod_files, "One or more NMODL files")->check(CLI::ExistingFile);
    app.add_option("--text", mod_texts, "One or more NMODL constructs as text");

    CLI11_PARSE(app, argc, argv);

    nmodl::parser::NmodlDriver driver;
    driver.set_verbose(true);

    for (const auto& f: mod_files) {
        nmodl::logger->info("Processing file : {}", f);
        driver.parse_file(f);
    }

    for (const auto& text: mod_texts) {
        nmodl::logger->info("Processing text : {}", text);
        driver.parse_string(text);
    }

    return 0;
}