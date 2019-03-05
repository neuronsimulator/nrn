/*************************************************************************
 * Copyright (C) 2018-2019 Blue Brain Project
 *
 * This file is part of NMODL distributed under the terms of the GNU
 * Lesser General Public License. See top-level LICENSE file for details.
 *************************************************************************/

#include <fstream>
#include <iostream>
#include <pybind11/embed.h>
#include <sstream>

#include "arg_handler.hpp"
#include "codegen/codegen_acc_visitor.hpp"
#include "codegen/codegen_c_visitor.hpp"
#include "codegen/codegen_cuda_visitor.hpp"
#include "codegen/codegen_omp_visitor.hpp"
#include "parser/nmodl_driver.hpp"
#include "utils/common_utils.hpp"
#include "utils/logger.hpp"
#include "visitors/ast_visitor.hpp"
#include "visitors/cnexp_solve_visitor.hpp"
#include "visitors/inline_visitor.hpp"
#include "visitors/json_visitor.hpp"
#include "visitors/local_var_rename_visitor.hpp"
#include "visitors/localize_visitor.hpp"
#include "visitors/nmodl_visitor.hpp"
#include "visitors/perf_visitor.hpp"
#include "visitors/sympy_conductance_visitor.hpp"
#include "visitors/sympy_solver_visitor.hpp"
#include "visitors/symtab_visitor.hpp"
#include "visitors/verbatim_var_rename_visitor.hpp"
#include "visitors/verbatim_visitor.hpp"

using namespace nmodl;
using namespace codegen;

using nmodl::codegen::LayoutType;


void ast_to_nmodl(ast::Program* ast, const std::string& filename) {
    NmodlPrintVisitor v(filename);
    v.visit_program(ast);
    logger->info("AST to NMODL transformation written to {}", filename);
}

int main(int argc, const char* argv[]) {
    ArgumentHandler arg(argc, argv);

    make_path(arg.output_dir);
    make_path(arg.scratch_dir);

    int error_count = 0;

    if (arg.verbose) {
        logger->set_level(spdlog::level::debug);
    }

    if (arg.sympy) {
        pybind11::initialize_interpreter();
    }

    for (auto& nmodl_file: arg.nmodl_files) {
        std::ifstream file(nmodl_file);

        if (!file.good()) {
            logger->warn("Could not open file : {}", nmodl_file);
            error_count++;
            continue;
        }

        logger->info("Processing {}", nmodl_file);

        std::string mod_file = remove_extension(base_name(nmodl_file));

        /// driver object creates lexer and parser, just call parser method
        nmodl::parser::NmodlDriver driver;
        driver.parse_file(nmodl_file);

        /// shared_ptr to ast constructed from parsing nmodl file
        auto ast = driver.ast();

        {
            AstVisitor v;
            v.visit_program(ast.get());
        }

        {
            SymtabVisitor v(false);
            v.visit_program(ast.get());
        }

        if (arg.ast_to_nmodl) {
            ast_to_nmodl(ast.get(), arg.scratch_dir + "/" + mod_file + ".nmodl.mod");
        }

        if (arg.verbatim_rename) {
            VerbatimVarRenameVisitor v;
            v.visit_program(ast.get());
            if (arg.ast_to_nmodl) {
                ast_to_nmodl(ast.get(), arg.scratch_dir + "/" + mod_file + ".nmodl.verbrename.mod");
            }
        }

        if (arg.sympy) {
            SympyConductanceVisitor v;
            v.visit_program(ast.get());
            {
                SymtabVisitor v(false);
                v.visit_program(ast.get());
            }
            if (arg.ast_to_nmodl) {
                ast_to_nmodl(ast.get(),
                             arg.scratch_dir + "/" + mod_file + ".nmodl.conductance.mod");
            }
        }

        if (arg.sympy) {
            SympySolverVisitor v(arg.pade_approx);
            v.visit_program(ast.get());
        }

        {
            CnexpSolveVisitor v;
            v.visit_program(ast.get());
        }

        if (arg.ast_to_nmodl) {
            ast_to_nmodl(ast.get(), arg.scratch_dir + "/" + mod_file + ".nmodl.cnexp.mod");
        }

        if (arg.inlining) {
            InlineVisitor v;
            v.visit_program(ast.get());
            if (arg.ast_to_nmodl) {
                ast_to_nmodl(ast.get(), arg.scratch_dir + "/" + mod_file + ".nmodl.in.mod");
            }
        }

        if (arg.local_rename) {
            LocalVarRenameVisitor v;
            v.visit_program(ast.get());
            if (arg.ast_to_nmodl) {
                ast_to_nmodl(ast.get(), arg.scratch_dir + "/" + mod_file + ".nmodl.locrename.mod");
            }
        }

        {
            SymtabVisitor v(true);
            v.visit_program(ast.get());
        }

        if (arg.localize) {
            // localize pass must be followed by renaming in order to avoid conflict
            // with global scope variables
            LocalizeVisitor v1(arg.localize_with_verbatim);
            v1.visit_program(ast.get());
            LocalVarRenameVisitor v2;
            v2.visit_program(ast.get());
            if (arg.ast_to_nmodl) {
                ast_to_nmodl(ast.get(), arg.scratch_dir + "/" + mod_file + ".nmodl.localize.mod");
            }
        }

        {
            SymtabVisitor v(true);
            v.visit_program(ast.get());
        }

        if (arg.perf_stats) {
            PerfVisitor v(arg.scratch_dir + "/" + mod_file + ".perf.json");
            logger->info("Dumping performance statistics into JSON format");
            v.visit_program(ast.get());
        }

        if (arg.ast_to_json) {
            JSONVisitor v(arg.scratch_dir + "/" + mod_file + ".ast.json");
            logger->info("Dumping AST state into JSON format");
            v.visit_program(ast.get());
        }

        if (arg.show_symtab) {
            logger->info("Printing symbol table");
            std::stringstream stream;
            auto symtab = ast->get_model_symbol_table();
            symtab->print(stream);
            std::cout << stream.str();
        }

        {
            // make sure to run perf visitor because code generator
            // looks for read/write counts const/non-const declaration
            PerfVisitor v;
            v.visit_program(ast.get());

            auto layout = arg.aos_memory_layout() ? LayoutType::aos : LayoutType::soa;

            logger->info("Generating host code with {} backend", arg.host_backend);

            if (arg.host_c_backend()) {
                CodegenCVisitor visitor(mod_file, arg.output_dir, layout, arg.dtype);
                visitor.visit_program(ast.get());
            } else if (arg.host_omp_backend()) {
                nmodl::codegen::CodegenOmpVisitor visitor(mod_file, arg.output_dir, layout,
                                                          arg.dtype);
                visitor.visit_program(ast.get());
            } else if (arg.host_acc_backend()) {
                CodegenAccVisitor visitor(mod_file, arg.output_dir, layout, arg.dtype);
                visitor.visit_program(ast.get());
            }

            if (arg.device_cuda_backend()) {
                logger->info("Generating device code with {} backend", arg.accel_backend);
                CodegenCudaVisitor visitor(mod_file, arg.output_dir, layout, arg.dtype);
                visitor.visit_program(ast.get());
            }
        }
    }

    if (arg.sympy) {
        pybind11::finalize_interpreter();
    }

    if (error_count != 0) {
        logger->error("Code generation encountered {} errors", error_count);
    } else {
        logger->info("Code generation finished successfully");
    }
    return error_count;
}
