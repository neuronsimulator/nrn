#include <fstream>
#include <iostream>
#include <sstream>

#include "arg_handler.hpp"
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

void ast_to_nmodl(ast::Program* ast, std::string filename) {
    NmodlPrintVisitor v(filename);
    v.visit_program(ast);
}

int main(int argc, const char* argv[]) {
    // version string
    auto version = nmodl::version::NMODL_VERSION + " [" + nmodl::version::GIT_REVISION + "]";

    ArgumentHandler arg(argc, argv);

    make_path(arg.output_dir);
    make_path(arg.scratch_dir);

    for (auto& nmodl_file : arg.nmodl_files) {
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
            v.visit_program(ast.get());
        }

        if (arg.show_symtab) {
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

            bool aos_layout = arg.aos_memory_layout();

            if (arg.host_c_backend()) {
                CodegenCVisitor visitor(mod_file, arg.output_dir, aos_layout, arg.dtype);
                visitor.visit_program(ast.get());
            } else if (arg.host_omp_backend()) {
                CodegenCOmpVisitor visitor(mod_file, arg.output_dir, aos_layout, arg.dtype);
                visitor.visit_program(ast.get());
            } else if (arg.host_acc_backend()) {
                CodegenCAccVisitor visitor(mod_file, arg.output_dir, aos_layout, arg.dtype);
                visitor.visit_program(ast.get());
            }

            if (arg.device_cuda_backend()) {
                CodegenCCudaVisitor visitor(mod_file, arg.output_dir, aos_layout, arg.dtype);
                visitor.visit_program(ast.get());
            }
        }
    }

    return 0;
}
