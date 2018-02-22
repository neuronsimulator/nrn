#include <fstream>
#include <iostream>
#include <sstream>

#include <tclap/CmdLine.h>
#include "parser/nmodl_driver.hpp"
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

using namespace symtab;

/**
 * Standalone visitor program for NMODL. This demonstrate basic
 * usage of different visitors classes and driver class.
 **/

int main(int argc, const char* argv[]) {
    try {
        TCLAP::CmdLine cmd("NMODL Code Generator for NMODL");

        // clang-format off
        TCLAP::ValueArg<std::string> modfile_arg(   "",
                                                    "file",
                                                    "NMODL input file path",
                                                    false,
                                                    "../test/input/channel.mod",
                                                    "string");
        TCLAP::ValueArg<std::string> host_arg(      "",
                                                    "host",
                                                    "Host backend [c, c-openmp, c-openacc]",
                                                    false,
                                                    "c",
                                                    "string");
        TCLAP::ValueArg<std::string> accel_arg(     "",
                                                    "accelerator",
                                                    "Accelerator backend [cuda]",
                                                    false,
                                                    "cuda",
                                                    "string");
        TCLAP::SwitchArg verbose_arg(               "",
                                                    "verbose",
                                                    "Enable verbose output",
                                                    false);
        TCLAP::SwitchArg aos_layout_arg(            "",
                                                    "aos",
                                                    "Enable AoS layout (default SoA)",
                                                    false);
        TCLAP::SwitchArg inline_arg(                "",
                                                    "inline",
                                                    "Enable inlining in NMODL implementation",
                                                    false);
        // clang-format on

        cmd.add(modfile_arg);
        cmd.add(verbose_arg);
        cmd.add(aos_layout_arg);
        cmd.add(inline_arg);
        cmd.add(host_arg);
        cmd.add(accel_arg);
        cmd.parse(argc, argv);

        std::string filename = modfile_arg.getValue();
        std::string host_backend = host_arg.getValue();
        bool verbose = verbose_arg.getValue();
        bool aos_code = aos_layout_arg.getValue();
        bool mod_inline = inline_arg.getValue();

        std::ifstream file(filename);

        if (!file.good()) {
            throw std::runtime_error("Could not open file " + filename);
        }

        std::string mod_filename = remove_extension(base_name(filename));

        /// driver object creates lexer and parser, just call parser method
        nmodl::Driver driver;
        driver.parse_file(filename);

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

        {
            std::stringstream stream;
            auto symtab = ast->get_model_symbol_table();
            symtab->print(stream);
            std::cout << stream.str();
            std::cout << "----SYMTAB VISITOR FINISHED----" << std::endl;
        }

        {
            VerbatimVarRenameVisitor v;
            v.visit_program(ast.get());
        }

        {
            NmodlPrintVisitor v(mod_filename + ".nocmodl.verbrename.mod");
            v.visit_program(ast.get());
        }

        {
            NmodlPrintVisitor v(mod_filename + ".nocmodl.mod");
            v.visit_program(ast.get());
        }

        {
            CnexpSolveVisitor v;
            v.visit_program(ast.get());
            std::cout << "----CNEXP SOLVE VISITOR FINISHED----" << std::endl;
        }

        {
            NmodlPrintVisitor v(mod_filename + ".nocmodl.cnexp.mod");
            v.visit_program(ast.get());
        }


        {
            if (mod_inline) {
                InlineVisitor v;
                v.visit_program(ast.get());
            }
        }


        {
            NmodlPrintVisitor v(mod_filename + ".nocmodl.cnexp.in.mod");
            v.visit_program(ast.get());
        }

        {
            LocalVarRenameVisitor v;
            v.visit_program(ast.get());
            std::cout << "----LOCAL VAR RENAME VISITOR FINISHED----" << std::endl;
        }

        {
            NmodlPrintVisitor v(mod_filename + ".nocmodl.cnexp.in.ren.mod");
            v.visit_program(ast.get());
        }

        {
            SymtabVisitor v(true);
            v.visit_program(ast.get());
        }

        {
            /// for benchmarking/plotting purpose we want to enable unsafe mode
            bool ignore_verbatim = true;
            LocalizeVisitor v(ignore_verbatim);
            v.visit_program(ast.get());
        }

        {
            NmodlPrintVisitor v(mod_filename + ".nocmodl.cnexp.in.ren.loc.mod");
            v.visit_program(ast.get());
        }

        {
            LocalVarRenameVisitor v;
            v.visit_program(ast.get());
        }

        {
            SymtabVisitor v(true);
            v.visit_program(ast.get());
        }

        {
            std::stringstream stream;
            auto symtab = ast->get_model_symbol_table();
            symtab->print(stream);
            std::cout << stream.str();
            std::cout << "----SYMTAB VISITOR FINISHED----" << std::endl;
        }

        {
            NmodlPrintVisitor v(mod_filename + ".nocmodl.cnexp.in.ren.loc.ren.mod");
            v.visit_program(ast.get());
        }

        {
            if (host_backend == "c") {
                CodegenCVisitor visitor(mod_filename, aos_code);
                visitor.visit_program(ast.get());
            } else if (host_backend == "c-openmp") {
                CodegenCOmpVisitor visitor(mod_filename, aos_code);
                visitor.visit_program(ast.get());
            } else if (host_backend == "c-openacc") {
                CodegenCAccVisitor visitor(mod_filename, aos_code);
                visitor.visit_program(ast.get());
            } else {
                std::cerr << "Argument Error: Unknown host backend " << host_backend << std::endl;
                return 1;
            }
        }

    } catch (TCLAP::ArgException& e) {
        std::cout << "Argument Error: " << e.error() << " for arg " << e.argId() << std::endl;
        return 1;
    }

    return 0;
}
