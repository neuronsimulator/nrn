/*
 * Copyright 2025 EPFL.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <filesystem>

#include <CLI/CLI.hpp>

#include "ast/program.hpp"
#include "config/config.h"
#include "parser/nmodl_driver.hpp"
#include "utils/logger.hpp"
#include "visitors/nmodl_visitor.hpp"
#include "visitors/threadsafe_visitor.hpp"

/**
 * Standalone mkthreadsafe program for NMODL.
 */

using namespace nmodl;

int main(int argc, const char* argv[]) {
    CLI::App app{fmt::format("NMODL-mkthreadsafe : Standalone mkthreadsafe for NMODL({})",
                             Version::to_string())};

    std::vector<std::string> mod_files;

    bool convert_globals = false;

    bool convert_verbatim = false;

    app.add_option("file", mod_files, "One or more NMODL files")
        ->required()
        ->check(CLI::ExistingFile);

    app.add_flag("--global",
                 convert_globals,
                 "Automatically mark threadsafe despite the use of GLOBAL");

    app.add_flag("--verbatim",
                 convert_verbatim,
                 "Automatically mark threadsafe despite the use of VERBATIM");

    CLI11_PARSE(app, argc, argv);


    for (const auto& f: mod_files) {
        parser::NmodlDriver driver;
        const auto& ast = driver.parse_file(f);

        logger->info("Running Threadsafe visitor on file {}", f);
        visitor::ThreadsafeVisitor(convert_globals, convert_verbatim).visit_program(*ast);
        logger->info("Writing AST to NMODL transformation to {}", f);
        visitor::NmodlPrintVisitor(f).visit_program(*ast);
    }

    return 0;
}
