/*************************************************************************
 * Copyright (C) 2018-2022 Blue Brain Project
 *
 * This file is part of NMODL distributed under the terms of the GNU
 * Lesser General Public License. See top-level LICENSE file for details.
 *************************************************************************/


#include <CLI/CLI.hpp>

#include "ast/program.hpp"
#include "config/config.h"
#include "parser/nmodl_driver.hpp"
#include "utils/logger.hpp"

/**
 * Standalone parser program for NMODL. This demonstrate
 * basic usage of parser and driver classes.
 */

using namespace nmodl;

int main(int argc, const char* argv[]) {
    CLI::App app{
        fmt::format("NMODL-Parser : Standalone Parser for NMODL({})", Version::to_string())};

    std::vector<std::string> mod_files;
    std::vector<std::string> mod_texts;

    app.add_option("file", mod_files, "One or more NMODL files")->check(CLI::ExistingFile);
    app.add_option("--text", mod_texts, "One or more NMODL constructs as text");

    CLI11_PARSE(app, argc, argv);

    parser::NmodlDriver driver;
    driver.set_verbose(true);

    for (const auto& f: mod_files) {
        logger->info("Processing file : {}", f);
        driver.parse_file(f);
    }

    for (const auto& text: mod_texts) {
        logger->info("Processing text : {}", text);
        driver.parse_string(text);
    }

    return 0;
}
