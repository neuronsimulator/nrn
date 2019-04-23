/*************************************************************************
 * Copyright (C) 2018-2019 Blue Brain Project
 *
 * This file is part of NMODL distributed under the terms of the GNU
 * Lesser General Public License. See top-level LICENSE file for details.
 *************************************************************************/

#include <sstream>

#include "CLI/CLI.hpp"
#include "fmt/format.h"
#include "pybind11/embed.h"

#include "config/config.h"
#include "parser/nmodl_driver.hpp"
#include "utils/logger.hpp"
#include "visitors/ast_visitor.hpp"
#include "visitors/constant_folder_visitor.hpp"
#include "visitors/inline_visitor.hpp"
#include "visitors/json_visitor.hpp"
#include "visitors/kinetic_block_visitor.hpp"
#include "visitors/local_var_rename_visitor.hpp"
#include "visitors/localize_visitor.hpp"
#include "visitors/neuron_solve_visitor.hpp"
#include "visitors/nmodl_visitor.hpp"
#include "visitors/perf_visitor.hpp"
#include "visitors/sympy_conductance_visitor.hpp"
#include "visitors/sympy_solver_visitor.hpp"
#include "visitors/symtab_visitor.hpp"
#include "visitors/verbatim_var_rename_visitor.hpp"
#include "visitors/verbatim_visitor.hpp"


using namespace nmodl;
using namespace visitor;
using namespace fmt::literals;

/**
 * \file
 * \brief Standalone program demonstrating usage of different visitors and driver classes.
 **/

int main(int argc, const char* argv[]) {
    CLI::App app{
        "NMODL Visitor : Runs standalone visitor classes({})"_format(Version::to_string())};

    bool verbose = false;
    std::vector<std::string> files;

    app.add_flag("-v,--verbose", verbose, "Enable debug log level");
    app.add_option("-f,--file,file", files, "One or more MOD files to process")
        ->required()
        ->check(CLI::ExistingFile);

    CLI11_PARSE(app, argc, argv);

    if (verbose) {
        logger->set_level(spdlog::level::debug);
    }

    struct VisitorInfo {
        std::shared_ptr<Visitor> v;
        std::string id;
        std::string description;
    };

    std::vector<VisitorInfo> visitors = {
        {std::make_shared<AstVisitor>(), "astvis", "AstVisitor"},
        {std::make_shared<SymtabVisitor>(), "symtab", "SymtabVisitor"},
        {std::make_shared<JSONVisitor>(), "json", "JSONVisitor"},
        {std::make_shared<VerbatimVisitor>(), "verbatim", "VerbatimVisitor"},
        {std::make_shared<VerbatimVarRenameVisitor>(),
         "verbatim-rename",
         "VerbatimVarRenameVisitor"},
        {std::make_shared<KineticBlockVisitor>(), "kinetic-rewrite", "KineticBlockVisitor"},
        {std::make_shared<ConstantFolderVisitor>(), "const-fold", "ConstantFolderVisitor"},
        {std::make_shared<InlineVisitor>(), "inline", "InlineVisitor"},
        {std::make_shared<LocalVarRenameVisitor>(), "local-rename", "LocalVarRenameVisitor"},
        {std::make_shared<SymtabVisitor>(), "symtab", "SymtabVisitor"},
        {std::make_shared<SympyConductanceVisitor>(),
         "sympy-conductance",
         "SympyConductanceVisitor"},
        {std::make_shared<SympySolverVisitor>(), "sympy-solve", "SympySolverVisitor"},
        {std::make_shared<NeuronSolveVisitor>(), "neuron-solve", "NeuronSolveVisitor"},
        {std::make_shared<LocalizeVisitor>(), "localize", "LocalizeVisitor"},
        {std::make_shared<PerfVisitor>(), "perf", "PerfVisitor"},
    };

    pybind11::initialize_interpreter();

    for (const auto& filename: files) {
        logger->info("Processing {}", filename);

        std::string mod_file = utils::remove_extension(utils::base_name(filename));

        /// driver object that creates lexer and parser
        parser::NmodlDriver driver;

        /// shared_ptr to ast constructed from parsing nmodl file
        auto ast = driver.parse_file(filename);

        /// run all visitors and generate mod file after each run
        for (const auto& visitor: visitors) {
            logger->info("Running {}", visitor.description);
            visitor.v->visit_program(ast.get());
            std::string file = mod_file + "." + visitor.id + ".mod";
            NmodlPrintVisitor(file).visit_program(ast.get());
            logger->info("NMODL visitor generated {}", file);
        }
    }

    pybind11::finalize_interpreter();

    return 0;
}
