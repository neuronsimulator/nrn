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
#include "pybind/pyembed.hpp"
#include "utils/logger.hpp"
#include "visitors/ast_visitor.hpp"
#include "visitors/checkparent_visitor.hpp"
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
#include "visitors/units_visitor.hpp"
#include "visitors/verbatim_var_rename_visitor.hpp"
#include "visitors/verbatim_visitor.hpp"


using namespace nmodl;
using namespace visitor;

/**
 * \file
 * \brief Standalone program demonstrating usage of different visitors and driver classes.
 **/

template <typename T>
struct ClassInfo {
    std::shared_ptr<T> v;
    std::string id;
    std::string description;
};
using VisitorInfo = ClassInfo<Visitor>;
using ConstVisitorInfo = ClassInfo<ConstVisitor>;

template <typename Visitor>
void visit_program(const std::string& mod_file,
                   const ClassInfo<Visitor>& visitor,
                   ast::Program& ast) {
    logger->info("Running {}", visitor.description);
    visitor.v->visit_program(ast);
    const std::string file = fmt::format("{}.{}.mod", mod_file, visitor.id);
    NmodlPrintVisitor(file).visit_program(ast);
    logger->info("NMODL visitor generated {}", file);
};

int main(int argc, const char* argv[]) {
    CLI::App app{
        fmt::format("NMODL Visitor : Runs standalone visitor classes({})", Version::to_string())};

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

    const std::vector<VisitorInfo> visitors = {
        {std::make_shared<AstVisitor>(), "astvis", "AstVisitor"},
        {std::make_shared<SymtabVisitor>(), "symtab", "SymtabVisitor"},
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
        {std::make_shared<UnitsVisitor>(NrnUnitsLib::get_path()), "units", "UnitsVisitor"},
    };

    const std::vector<ConstVisitorInfo> const_visitors = {
        {std::make_shared<JSONVisitor>(), "json", "JSONVisitor"},
        {std::make_shared<test::CheckParentVisitor>(), "check-parent", "CheckParentVisitor"},
        {std::make_shared<PerfVisitor>(), "perf", "PerfVisitor"},
        {std::make_shared<LocalizeVisitor>(), "localize", "LocalizeVisitor"},
        {std::make_shared<VerbatimVisitor>(), "verbatim", "VerbatimVisitor"},
    };

    nmodl::pybind_wrappers::EmbeddedPythonLoader::get_instance().api()->initialize_interpreter();

    for (const auto& filename: files) {
        logger->info("Processing {}", filename);

        const std::string mod_file(utils::remove_extension(utils::base_name(filename)));

        /// driver object that creates lexer and parser
        parser::NmodlDriver driver;

        /// shared_ptr to ast constructed from parsing nmodl file
        const auto& ast = driver.parse_file(filename);

        /// run all visitors and generate mod file after each run
        for (const auto& visitor: visitors) {
            visit_program(mod_file, visitor, *ast);
        }
        for (const auto& visitor: const_visitors) {
            visit_program(mod_file, visitor, *ast);
        }
    }

    nmodl::pybind_wrappers::EmbeddedPythonLoader::get_instance().api()->finalize_interpreter();

    return 0;
}
