#include <fstream>
#include <iostream>
#include <sstream>

#include "tclap/CmdLine.h"
#include "src/version/version.h"
#include "parser/nmodl_driver.hpp"
#include "version/version.h"
#include "visitors/ast_visitor.hpp"
#include "visitors/inline_visitor.hpp"
#include "visitors/json_visitor.hpp"
#include "visitors/local_var_rename_visitor.hpp"
#include "visitors/perf_visitor.hpp"
#include "visitors/symtab_visitor.hpp"
#include "visitors/verbatim_visitor.hpp"
#include "visitors/nmodl_visitor.hpp"
#include "visitors/localize_visitor.hpp"
#include "visitors/cnexp_solve_visitor.hpp"
#include "visitors/verbatim_var_rename_visitor.hpp"
#include "codegen/c/codegen_c_visitor.hpp"
#include "codegen/c-openmp/codegen_c_omp_visitor.hpp"
#include "codegen/c-openacc/codegen_c_acc_visitor.hpp"
#include "codegen/c-cuda/codegen_c_cuda_visitor.hpp"
#include "utils/common_utils.hpp"

/**
 * Standalone visitor program for NMODL. This demonstrate basic
 * usage of different visitors classes and driver class.
 **/

void ast_to_nmodl(ast::Program* ast, std::string filename) {
    NmodlPrintVisitor v(filename);
    v.visit_program(ast);
}

int main(int argc, const char* argv[]) {
    // version string
    auto version = nmodl::version::NMODL_VERSION + " [" + nmodl::version::GIT_REVISION + "]";

    try {
        using string_vector_t = std::vector<std::string>;
        using value_constraint_t = TCLAP::ValuesConstraint<std::string>;
        using value_arg_t = TCLAP::ValueArg<std::string>;
        using switch_arg_t = TCLAP::SwitchArg;
        using unlabel_arg_t = TCLAP::UnlabeledMultiArg<std::string>;

        TCLAP::CmdLine cmd("NMODL Code Generator for NMODL", ' ', version);

        // clang-format off
        string_vector_t host_val = {"SERIAL", "OPENMP", "OPENACC"};
        value_constraint_t host_constr(host_val);
        value_arg_t host_arg(       "",
                                    "host",
                                    "Code generation backend for host [" + host_val[0] +"]",
                                    false,
                                    host_val[0],
                                    &host_constr,
                                    cmd);

        string_vector_t accel_val = {"CUDA"};
        value_constraint_t accel_constr(accel_val);
        value_arg_t accel_arg(      "",
                                    "accelerator",
                                    "Accelerator backend [" + accel_val[0] + "]",
                                    false,
                                    "",
                                    &accel_constr,
                                    cmd);

        string_vector_t dtype_val = {"DOUBLE", "FLOAT"};
        value_constraint_t dtype_constr(dtype_val);
        value_arg_t dtype_arg(      "",
                                    "datatype",
                                    "Floating point data type [" + dtype_val[0] + "]",
                                    false,
                                    dtype_val[0],
                                    &dtype_constr,
                                    cmd);

        string_vector_t layout_val = {"SOA", "AOS"};
        value_constraint_t layout_constr(layout_val);
        value_arg_t layout_arg(     "",
                                    "layout",
                                    "Memory layout for channel data [" + layout_val[0] +"]",
                                    false,
                                    layout_val[0],
                                    &layout_constr,
                                    cmd);

        value_arg_t output_dir_arg( "",
                                    "output-dir",
                                    "Output directory for code generator output [.]",
                                    false,
                                    ".",
                                    "string",
                                    cmd);

        value_arg_t scratch_dir_arg( "",
                                    "scratch-dir",
                                    "Output directory for intermediate output [tmp]",
                                    false,
                                    "tmp",
                                    "string",
                                    cmd);

        switch_arg_t  verbose_arg(  "",
                                    "verbose",
                                    "Enable verbose output",
                                    cmd,
                                    false);

        switch_arg_t inline_arg(    "",
                                    "nmodl-inline",
                                    "Enable NMODL inlining",
                                    cmd,
                                    false);

        switch_arg_t localize_arg(  "",
                                    "localize",
                                    "Localize RANGE, GLOBAL, ASSIGNED variables",
                                    cmd,
                                    false);

        switch_arg_t localize_verbatim_arg(
                                    "",
                                    "localize-verbatim",
                                    "Localize even with VERBATIM blocks (unsafe optimizations)",
                                    cmd,
                                    false);

        switch_arg_t local_rename_arg(
                                    "",
                                    "local-rename",
                                    "Rename LOCAL variables if necessary",
                                    cmd,
                                    false);

        switch_arg_t perf_stats_arg(
                                    "",
                                    "dump-perf-stats",
                                    "Run performance visitor and dump stats into JSON file",
                                    cmd,
                                    false);

        switch_arg_t nmodl_state_arg(
                                    "",
                                    "dump-nmodl-state",
                                    "Dump intermediate AST states into NMODL",
                                    cmd,
                                    false);

        switch_arg_t no_verbatim_rename_arg(
                                    "",
                                    "no-verbatim-rename",
                                    "Disable renaming variables in VERBATIM block",
                                    cmd,
                                    false);

        switch_arg_t show_symtab_arg(
                                    "",
                                    "show-symtab",
                                    "Show symbol table to stdout",
                                    cmd,
                                    false);

        unlabel_arg_t nmodl_arg(    "files",
                                    "NMODL input models path",
                                    true,
                                    "string",
                                    cmd);
        // clang-format on

        cmd.parse(argc, argv);

        auto nmodl_files = nmodl_arg.getValue();
        auto host_backend = host_arg.getValue();
        auto accel_backend = accel_arg.getValue();
        auto data_type = stringutils::tolower(dtype_arg.getValue());
        auto mem_layout = layout_arg.getValue();
        auto verbose = verbose_arg.getValue();
        auto enable_inline = inline_arg.getValue();
        auto ignore_verbatim = localize_verbatim_arg.getValue();
        auto local_rename = local_rename_arg.getValue();
        auto verbatim_rename = !no_verbatim_rename_arg.getValue();
        auto enable_localize = localize_arg.getValue();
        auto dump_perf_stats = perf_stats_arg.getValue();
        auto show_symtab = show_symtab_arg.getValue();
        auto output_dir = output_dir_arg.getValue();
        auto scratch_dir = scratch_dir_arg.getValue();
        auto dump_nmodl = nmodl_state_arg.getValue();

        make_path(output_dir);
        make_path(scratch_dir);

        for (auto& nmodl_file : nmodl_files) {
            std::ifstream file(nmodl_file);

            if (!file.good()) {
                throw std::runtime_error("Could not open file " + nmodl_file);
            }

            std::string mod_file = remove_extension(base_name(nmodl_file));

            /// driver object creates lexer and parser, just call parser method
            nmodl::Driver driver;
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

            if (dump_nmodl) {
                ast_to_nmodl(ast.get(), scratch_dir + "/" + mod_file + ".nmodl.mod");
            }

            if (verbatim_rename) {
                VerbatimVarRenameVisitor v;
                v.visit_program(ast.get());
                if (dump_nmodl) {
                    ast_to_nmodl(ast.get(), scratch_dir + "/" + mod_file + ".nmodl.verbrename.mod");
                }
            }

            {
                CnexpSolveVisitor v;
                v.visit_program(ast.get());
            }

            if (dump_nmodl) {
                ast_to_nmodl(ast.get(), scratch_dir + "/" + mod_file + ".nmodl.cnexp.mod");
            }

            if (enable_inline) {
                InlineVisitor v;
                v.visit_program(ast.get());
                if (dump_nmodl) {
                    ast_to_nmodl(ast.get(), scratch_dir + "/" + mod_file + ".nmodl.in.mod");
                }
            }

            if (local_rename) {
                LocalVarRenameVisitor v;
                v.visit_program(ast.get());
                if (dump_nmodl) {
                    ast_to_nmodl(ast.get(), scratch_dir + "/" + mod_file + ".nmodl.locrename.mod");
                }
            }

            {
                SymtabVisitor v(true);
                v.visit_program(ast.get());
            }

            if (enable_localize) {
                // localize pass must be followed by renaming in order to avoid conflict
                // with global scope variables
                LocalizeVisitor v1(ignore_verbatim);
                v1.visit_program(ast.get());
                LocalVarRenameVisitor v2;
                v2.visit_program(ast.get());
                if (dump_nmodl) {
                    ast_to_nmodl(ast.get(), scratch_dir + "/" + mod_file + ".nmodl.localize.mod");
                }
            }

            {
                SymtabVisitor v(true);
                v.visit_program(ast.get());
            }

            if (dump_perf_stats) {
                PerfVisitor v(scratch_dir + "/" + mod_file + ".perf.json");
                v.visit_program(ast.get());
            }

            if (show_symtab) {
                std::cout << "----SYMBOL TABLE ----" << std::endl;
                std::stringstream stream;
                auto symtab = ast->get_model_symbol_table();
                symtab->print(stream);
                std::cout << stream.str();
                std::cout << "---------------------" << std::endl;
            }

            {
                // make sure to run perf visitor because code generator
                // looks for read/write counts const/non-const declaration
                PerfVisitor v;
                v.visit_program(ast.get());

                bool aos_layout = (mem_layout == "AOS");

                if (host_backend == "SERIAL") {
                    CodegenCVisitor visitor(mod_file, output_dir, aos_layout, data_type);
                    visitor.visit_program(ast.get());
                } else if (host_backend == "OPENMP") {
                    CodegenCOmpVisitor visitor(mod_file, output_dir, aos_layout, data_type);
                    visitor.visit_program(ast.get());
                } else if (host_backend == "OPENACC") {
                    CodegenCAccVisitor visitor(mod_file, output_dir, aos_layout, data_type);
                    visitor.visit_program(ast.get());
                }

                if (accel_backend == "CUDA") {
                    CodegenCCudaVisitor visitor(mod_file, output_dir, aos_layout, data_type);
                    visitor.visit_program(ast.get());
                }
            }
        }

    } catch (TCLAP::ArgException& e) {
        std::cout << "Argument Error: " << e.error() << " for arg " << e.argId() << std::endl;
        return 1;
    }

    return 0;
}
