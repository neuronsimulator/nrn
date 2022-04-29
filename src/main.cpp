/*************************************************************************
 * Copyright (C) 2018-2022 Blue Brain Project
 *
 * This file is part of NMODL distributed under the terms of the GNU
 * Lesser General Public License. See top-level LICENSE file for details.
 *************************************************************************/

#include <string>
#include <vector>

#include <CLI/CLI.hpp>

#include "ast/program.hpp"
#include "codegen/codegen_acc_visitor.hpp"
#include "codegen/codegen_c_visitor.hpp"
#include "codegen/codegen_compatibility_visitor.hpp"
#include "codegen/codegen_cuda_visitor.hpp"
#include "codegen/codegen_ispc_visitor.hpp"
#include "codegen/codegen_omp_visitor.hpp"
#include "config/config.h"
#include "parser/nmodl_driver.hpp"
#include "pybind/pyembed.hpp"
#include "utils/common_utils.hpp"
#include "utils/logger.hpp"
#include "visitors/after_cvode_to_cnexp_visitor.hpp"
#include "visitors/ast_visitor.hpp"
#include "visitors/constant_folder_visitor.hpp"
#include "visitors/global_var_visitor.hpp"
#include "visitors/indexedname_visitor.hpp"
#include "visitors/inline_visitor.hpp"
#include "visitors/ispc_rename_visitor.hpp"
#include "visitors/json_visitor.hpp"
#include "visitors/kinetic_block_visitor.hpp"
#include "visitors/local_to_assigned_visitor.hpp"
#include "visitors/local_var_rename_visitor.hpp"
#include "visitors/localize_visitor.hpp"
#include "visitors/loop_unroll_visitor.hpp"
#include "visitors/neuron_solve_visitor.hpp"
#include "visitors/nmodl_visitor.hpp"
#include "visitors/perf_visitor.hpp"
#include "visitors/semantic_analysis_visitor.hpp"
#include "visitors/solve_block_visitor.hpp"
#include "visitors/steadystate_visitor.hpp"
#include "visitors/sympy_conductance_visitor.hpp"
#include "visitors/sympy_solver_visitor.hpp"
#include "visitors/symtab_visitor.hpp"
#include "visitors/units_visitor.hpp"
#include "visitors/verbatim_var_rename_visitor.hpp"
#include "visitors/verbatim_visitor.hpp"

/**
 * \dir
 * \brief Main NMODL code generation program
 */

using namespace nmodl;
using namespace codegen;
using namespace visitor;
using nmodl::parser::NmodlDriver;

int main(int argc, const char* argv[]) {
    CLI::App app{fmt::format("NMODL : Source-to-Source Code Generation Framework [{}]",
                             Version::to_string())};

    /// list of mod files to process
    std::vector<std::string> mod_files;

    /// true if debug logger statements should be shown
    std::string verbose("info");

    /// true if serial c code to be generated
    bool c_backend(true);

    /// true if c code with openmp to be generated
    bool omp_backend(false);

    /// true if ispc code to be generated
    bool ispc_backend(false);

    /// true if c code with openacc to be generated
    bool oacc_backend(false);

    /// true if cuda code to be generated
    bool cuda_backend(false);

    /// true if sympy should be used for solving ODEs analytically
    bool sympy_analytic(false);

    /// true if Pade approximation to be used
    bool sympy_pade(false);

    /// true if CSE (temp variables) to be used
    bool sympy_cse(false);

    /// true if conductance keyword can be added to breakpoint
    bool sympy_conductance(false);

    /// true if inlining at nmodl level to be done
    bool nmodl_inline(false);

    /// true if unroll at nmodl level to be done
    bool nmodl_unroll(false);

    /// true if perform constant folding at nmodl level to be done
    bool nmodl_const_folding(false);

    /// true if range variables to be converted to local
    bool nmodl_localize(false);

    /// true if global variables to be converted to range
    bool nmodl_global_to_range(false);

    /// true if top level local variables to be converted to range
    bool nmodl_local_to_range(false);

    /// true if localize variables even if verbatim block is used
    bool localize_verbatim(false);

    /// true if local variables to be renamed
    bool local_rename(true);

    /// true if inline even if verbatim block exist
    bool verbatim_inline(false);

    /// true if verbatim blocks
    bool verbatim_rename(true);

    /// true if code generation is forced to happen even if there
    /// is any incompatibility
    bool force_codegen(false);

    /// true if we want to only check compatibility without generating code
    bool only_check_compatibility(false);

    /// true if ion variable copies should be avoided
    bool optimize_ionvar_copies_codegen(false);

    /// directory where code will be generated
    std::string output_dir(".");

    /// directory where intermediate file will be generated
    std::string scratch_dir("tmp");

    /// directory where units lib file is located
    std::string units_dir(NrnUnitsLib::get_path());

    /// true if ast should be converted to json
    bool json_ast(false);

    /// true if ast should be converted to nmodl
    bool nmodl_ast(false);

    /// true if performance stats should be converted to json
    bool json_perfstat(false);

    /// true if symbol table should be printed
    bool show_symtab(false);

    /// floating point data type
    std::string data_type("double");

    app.get_formatter()->column_width(40);
    app.set_help_all_flag("-H,--help-all", "Print this help message including all sub-commands");

    app.add_option("--verbose", verbose, "Verbosity of logger output")
        ->capture_default_str()
        ->ignore_case()
        ->check(CLI::IsMember({"trace", "debug", "info", "warning", "error", "critical", "off"}));

    app.add_option("file", mod_files, "One or more MOD files to process")
        ->ignore_case()
        ->required()
        ->check(CLI::ExistingFile);

    app.add_option("-o,--output", output_dir, "Directory for backend code output")
        ->capture_default_str()
        ->ignore_case();
    app.add_option("--scratch", scratch_dir, "Directory for intermediate code output")
        ->capture_default_str()
        ->ignore_case();
    app.add_option("--units", units_dir, "Directory of units lib file")
        ->capture_default_str()
        ->ignore_case();

    auto host_opt = app.add_subcommand("host", "HOST/CPU code backends")->ignore_case();
    host_opt->add_flag("--c", c_backend, fmt::format("C/C++ backend ({})", c_backend))
        ->ignore_case();
    host_opt
        ->add_flag("--omp", omp_backend, fmt::format("C/C++ backend with OpenMP ({})", omp_backend))
        ->ignore_case();
    host_opt
        ->add_flag("--ispc",
                   ispc_backend,
                   fmt::format("C/C++ backend with ISPC ({})", ispc_backend))
        ->ignore_case();

    auto acc_opt = app.add_subcommand("acc", "Accelerator code backends")->ignore_case();
    acc_opt
        ->add_flag("--oacc",
                   oacc_backend,
                   fmt::format("C/C++ backend with OpenACC ({})", oacc_backend))
        ->ignore_case();
    acc_opt
        ->add_flag("--cuda",
                   cuda_backend,
                   fmt::format("C/C++ backend with CUDA ({})", cuda_backend))
        ->ignore_case();

    // clang-format off
    auto sympy_opt = app.add_subcommand("sympy", "SymPy based analysis and optimizations")->ignore_case();
    sympy_opt->add_flag("--analytic",
        sympy_analytic,
        fmt::format("Solve ODEs using SymPy analytic integration ({})", sympy_analytic))->ignore_case();
    sympy_opt->add_flag("--pade",
        sympy_pade,
        fmt::format("Pade approximation in SymPy analytic integration ({})", sympy_pade))->ignore_case();
    sympy_opt->add_flag("--cse",
        sympy_cse,
        fmt::format("CSE (Common Subexpression Elimination) in SymPy analytic integration ({})", sympy_cse))->ignore_case();
    sympy_opt->add_flag("--conductance",
        sympy_conductance,
        fmt::format("Add CONDUCTANCE keyword in BREAKPOINT ({})", sympy_conductance))->ignore_case();

    auto passes_opt = app.add_subcommand("passes", "Analyse/Optimization passes")->ignore_case();
    passes_opt->add_flag("--inline",
        nmodl_inline,
        fmt::format("Perform inlining at NMODL level ({})", nmodl_inline))->ignore_case();
    passes_opt->add_flag("--unroll",
        nmodl_unroll,
        fmt::format("Perform loop unroll at NMODL level ({})", nmodl_unroll))->ignore_case();
    passes_opt->add_flag("--const-folding",
        nmodl_const_folding,
        fmt::format("Perform constant folding at NMODL level ({})", nmodl_const_folding))->ignore_case();
    passes_opt->add_flag("--localize",
        nmodl_localize,
        fmt::format("Convert RANGE variables to LOCAL ({})", nmodl_localize))->ignore_case();
    passes_opt->add_flag("--global-to-range",
         nmodl_global_to_range,
         fmt::format("Convert GLOBAL variables to RANGE ({})", nmodl_global_to_range))->ignore_case();
    passes_opt->add_flag("--local-to-range",
         nmodl_local_to_range,
         fmt::format("Convert top level LOCAL variables to RANGE ({})", nmodl_local_to_range))->ignore_case();
    passes_opt->add_flag("--localize-verbatim",
        localize_verbatim,
        fmt::format("Convert RANGE variables to LOCAL even if verbatim block exist ({})", localize_verbatim))->ignore_case();
    passes_opt->add_flag("--local-rename",
        local_rename,
        fmt::format("Rename LOCAL variable if variable of same name exist in global scope ({})", local_rename))->ignore_case();
    passes_opt->add_flag("--verbatim-inline",
        verbatim_inline,
        fmt::format("Inline even if verbatim block exist ({})", verbatim_inline))->ignore_case();
    passes_opt->add_flag("--verbatim-rename",
        verbatim_rename,
        fmt::format("Rename variables in verbatim block ({})", verbatim_rename))->ignore_case();
    passes_opt->add_flag("--json-ast",
        json_ast,
        fmt::format("Write AST to JSON file ({})", json_ast))->ignore_case();
    passes_opt->add_flag("--nmodl-ast",
        nmodl_ast,
        fmt::format("Write AST to NMODL file ({})", nmodl_ast))->ignore_case();
    passes_opt->add_flag("--json-perf",
        json_perfstat,
        fmt::format("Write performance statistics to JSON file ({})", json_perfstat))->ignore_case();
    passes_opt->add_flag("--show-symtab",
        show_symtab,
        fmt::format("Write symbol table to stdout ({})", show_symtab))->ignore_case();

    auto codegen_opt = app.add_subcommand("codegen", "Code generation options")->ignore_case();
    codegen_opt->add_option("--datatype",
        data_type,
        "Data type for floating point variables")->capture_default_str()->ignore_case()->check(CLI::IsMember({"float", "double"}));
    codegen_opt->add_flag("--force",
        force_codegen,
        "Force code generation even if there is any incompatibility");
    codegen_opt->add_flag("--only-check-compatibility",
                          only_check_compatibility,
                          "Check compatibility and return without generating code");
    codegen_opt->add_flag("--opt-ionvar-copy",
        optimize_ionvar_copies_codegen,
        fmt::format("Optimize copies of ion variables ({})", optimize_ionvar_copies_codegen))->ignore_case();

    // clang-format on

    CLI11_PARSE(app, argc, argv);

    // if any of the other backends is used we force the C backend to be off.
    if (omp_backend || ispc_backend) {
        c_backend = false;
    }

    utils::make_path(output_dir);
    utils::make_path(scratch_dir);

    if (sympy_opt) {
        nmodl::pybind_wrappers::EmbeddedPythonLoader::get_instance()
            .api()
            ->initialize_interpreter();
    }

    logger->set_level(spdlog::level::from_str(verbose));

    /// write ast to nmodl
    const auto ast_to_nmodl = [nmodl_ast](ast::Program& ast, const std::string& filepath) {
        if (nmodl_ast) {
            NmodlPrintVisitor(filepath).visit_program(ast);
            logger->info("AST to NMODL transformation written to {}", filepath);
        }
    };

    for (const auto& file: mod_files) {
        logger->info("Processing {}", file);

        const auto modfile = utils::remove_extension(utils::base_name(file));

        /// create file path for nmodl file
        auto filepath = [scratch_dir, modfile](const std::string& suffix) {
            static int count = 0;
            return fmt::format(
                "{}/{}.{}.{}.mod", scratch_dir, modfile, std::to_string(count++), suffix);
        };

        /// driver object creates lexer and parser, just call parser method
        NmodlDriver driver;

        /// parse mod file and construct ast
        const auto& ast = driver.parse_file(file);

        /// whether to update existing symbol table or create new
        /// one whenever we run symtab visitor.
        bool update_symtab = false;

        /// just visit the ast
        AstVisitor().visit_program(*ast);

        /// Check some rules that ast should follow
        {
            logger->info("Running semantic analysis visitor");
            if (SemanticAnalysisVisitor().check(*ast)) {
                return 1;
            }
        }

        /// construct symbol table
        {
            logger->info("Running symtab visitor");
            SymtabVisitor(update_symtab).visit_program(*ast);
        }

        /// use cnexp instead of after_cvode solve method
        {
            logger->info("Running CVode to cnexp visitor");
            AfterCVodeToCnexpVisitor().visit_program(*ast);
            ast_to_nmodl(*ast, filepath("after_cvode_to_cnexp"));
        }

        /// Rename variables that match ISPC compiler double constants
        if (ispc_backend) {
            logger->info("Running ISPC variables rename visitor");
            IspcRenameVisitor(ast).visit_program(*ast);
            SymtabVisitor(update_symtab).visit_program(*ast);
            ast_to_nmodl(*ast, filepath("ispc_double_rename"));
        }

        /// GLOBAL to RANGE rename visitor
        if (nmodl_global_to_range) {
            // make sure to run perf visitor because code generator
            // looks for read/write counts const/non-const declaration
            PerfVisitor().visit_program(*ast);
            // make sure to run the GlobalToRange visitor after all the
            // reinitializations of Symtab
            logger->info("Running GlobalToRange visitor");
            GlobalToRangeVisitor(*ast).visit_program(*ast);
            SymtabVisitor(update_symtab).visit_program(*ast);
            ast_to_nmodl(*ast, filepath("global_to_range"));
        }

        /// LOCAL to ASSIGNED visitor
        if (nmodl_local_to_range) {
            logger->info("Running LOCAL to ASSIGNED visitor");
            PerfVisitor().visit_program(*ast);
            LocalToAssignedVisitor().visit_program(*ast);
            SymtabVisitor(update_symtab).visit_program(*ast);
            ast_to_nmodl(*ast, filepath("local_to_assigned"));
        }

        {
            // Compatibility Checking
            logger->info("Running code compatibility checker");
            // run perfvisitor to update read/write counts
            PerfVisitor().visit_program(*ast);

            // If we want to just check compatibility we return the result
            if (only_check_compatibility) {
                return CodegenCompatibilityVisitor().find_unhandled_ast_nodes(*ast);
            }

            // If there is an incompatible construct and code generation is not forced exit NMODL
            if (CodegenCompatibilityVisitor().find_unhandled_ast_nodes(*ast) && !force_codegen) {
                return 1;
            }
        }

        if (show_symtab) {
            logger->info("Printing symbol table");
            auto symtab = ast->get_model_symbol_table();
            symtab->print(std::cout);
        }

        ast_to_nmodl(*ast, filepath("ast"));

        if (json_ast) {
            auto file = scratch_dir + "/" + modfile + ".ast.json";
            logger->info("Writing AST into {}", file);
            JSONVisitor(file).write(*ast);
        }

        if (verbatim_rename) {
            logger->info("Running verbatim rename visitor");
            VerbatimVarRenameVisitor().visit_program(*ast);
            ast_to_nmodl(*ast, filepath("verbatim_rename"));
        }

        if (nmodl_const_folding) {
            logger->info("Running nmodl constant folding visitor");
            ConstantFolderVisitor().visit_program(*ast);
            ast_to_nmodl(*ast, filepath("constfold"));
        }

        if (nmodl_unroll) {
            logger->info("Running nmodl loop unroll visitor");
            LoopUnrollVisitor().visit_program(*ast);
            ConstantFolderVisitor().visit_program(*ast);
            ast_to_nmodl(*ast, filepath("unroll"));
            SymtabVisitor(update_symtab).visit_program(*ast);
        }

        /// note that we can not symtab visitor in update mode as we
        /// replace kinetic block with derivative block of same name
        /// in global scope
        {
            logger->info("Running KINETIC block visitor");
            auto kineticBlockVisitor = KineticBlockVisitor();
            kineticBlockVisitor.visit_program(*ast);
            SymtabVisitor(update_symtab).visit_program(*ast);
            const auto filename = filepath("kinetic");
            ast_to_nmodl(*ast, filename);
            if (nmodl_ast && kineticBlockVisitor.get_conserve_statement_count()) {
                logger->warn(
                    fmt::format("{} presents non-standard CONSERVE statements in DERIVATIVE "
                                "blocks. Use it only for debugging/developing",
                                filename));
            }
        }

        {
            logger->info("Running STEADYSTATE visitor");
            SteadystateVisitor().visit_program(*ast);
            SymtabVisitor(update_symtab).visit_program(*ast);
            ast_to_nmodl(*ast, filepath("steadystate"));
        }

        /// Parsing units fron "nrnunits.lib" and mod files
        {
            logger->info("Parsing Units");
            UnitsVisitor(units_dir).visit_program(*ast);
        }

        /// once we start modifying (especially removing) older constructs
        /// from ast then we should run symtab visitor in update mode so
        /// that old symbols (e.g. prime variables) are not lost
        update_symtab = true;

        if (nmodl_inline) {
            logger->info("Running nmodl inline visitor");
            InlineVisitor().visit_program(*ast);
            ast_to_nmodl(*ast, filepath("inline"));
        }

        if (local_rename) {
            logger->info("Running local variable rename visitor");
            LocalVarRenameVisitor().visit_program(*ast);
            SymtabVisitor(update_symtab).visit_program(*ast);
            ast_to_nmodl(*ast, filepath("local_rename"));
        }

        if (nmodl_localize) {
            // localize pass must follow rename pass to avoid conflict
            logger->info("Running localize visitor");
            LocalizeVisitor(localize_verbatim).visit_program(*ast);
            LocalVarRenameVisitor().visit_program(*ast);
            SymtabVisitor(update_symtab).visit_program(*ast);
            ast_to_nmodl(*ast, filepath("localize"));
        }

        if (sympy_conductance) {
            logger->info("Running sympy conductance visitor");
            SympyConductanceVisitor().visit_program(*ast);
            SymtabVisitor(update_symtab).visit_program(*ast);
            ast_to_nmodl(*ast, filepath("sympy_conductance"));
        }

        if (sympy_analytic || sparse_solver_exists(*ast)) {
            if (!sympy_analytic) {
                logger->info(
                    "Automatically enable sympy_analytic because it exists solver of type sparse");
            }
            logger->info("Running sympy solve visitor");
            SympySolverVisitor(sympy_pade, sympy_cse).visit_program(*ast);
            SymtabVisitor(update_symtab).visit_program(*ast);
            ast_to_nmodl(*ast, filepath("sympy_solve"));
        }

        {
            logger->info("Running cnexp visitor");
            NeuronSolveVisitor().visit_program(*ast);
            ast_to_nmodl(*ast, filepath("cnexp"));
        }

        {
            SolveBlockVisitor().visit_program(*ast);
            SymtabVisitor(update_symtab).visit_program(*ast);
            ast_to_nmodl(*ast, filepath("solveblock"));
        }

        if (json_perfstat) {
            auto file = scratch_dir + "/" + modfile + ".perf.json";
            logger->info("Writing performance statistics to {}", file);
            PerfVisitor(file).visit_program(*ast);
        }

        {
            // make sure to run perf visitor because code generator
            // looks for read/write counts const/non-const declaration
            PerfVisitor().visit_program(*ast);
        }

        {
            if (ispc_backend) {
                logger->info("Running ISPC backend code generator");
                CodegenIspcVisitor visitor(modfile,
                                           output_dir,
                                           data_type,
                                           optimize_ionvar_copies_codegen);
                visitor.visit_program(*ast);
            }

            else if (oacc_backend) {
                logger->info("Running OpenACC backend code generator");
                CodegenAccVisitor visitor(modfile,
                                          output_dir,
                                          data_type,
                                          optimize_ionvar_copies_codegen);
                visitor.visit_program(*ast);
            }

            else if (omp_backend) {
                logger->info("Running OpenMP backend code generator");
                CodegenOmpVisitor visitor(modfile,
                                          output_dir,
                                          data_type,
                                          optimize_ionvar_copies_codegen);
                visitor.visit_program(*ast);
            }

            else if (c_backend) {
                logger->info("Running C backend code generator");
                CodegenCVisitor visitor(modfile,
                                        output_dir,
                                        data_type,
                                        optimize_ionvar_copies_codegen);
                visitor.visit_program(*ast);
            }

            if (cuda_backend) {
                logger->info("Running CUDA backend code generator");
                CodegenCudaVisitor visitor(modfile,
                                           output_dir,
                                           data_type,
                                           optimize_ionvar_copies_codegen);
                visitor.visit_program(*ast);
            }
        }
    }

    if (sympy_opt) {
        nmodl::pybind_wrappers::EmbeddedPythonLoader::get_instance().api()->finalize_interpreter();
    }
}
