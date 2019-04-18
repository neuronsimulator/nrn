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
#include "parser/unit_driver.hpp"
#include "utils/logger.hpp"

/**
 * Standalone parser program for Units. This demonstrate basic
 * usage of parser and driver class.
 *
 * \todo This is a placeholder and needs to be changed to parse
 *       NMODL file and then show corresponding units.
 */

using namespace fmt::literals;
using namespace nmodl;

void parse_units(std::vector<std::string> files) {
    for (const auto& f: files) {
        logger->info("Processing {}", f);
        std::ifstream file(f);

        /// driver object creates lexer and parser
        parser::UnitDriver driver;
        driver.set_verbose(true);

        /// just call parser method
        driver.parse_stream(file);
    }
}

int main(int argc, const char* argv[]) {
    CLI::App app{"Unit-Parser : Standalone Parser for Units({})"_format(version::to_string())};

    std::vector<std::string> files;
    files.push_back(NrnUnitsLib::get_path());
    app.add_option("file", files, "One or more Units files to process");

    CLI11_PARSE(app, argc, argv);

    parse_units(files);

    return 0;
}
