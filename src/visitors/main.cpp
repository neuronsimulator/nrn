#include <fstream>
#include <iostream>
#include <sstream>

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

#include "tclap/CmdLine.h"

using namespace symtab;

/**
 * Standalone visitor program for NMODL. This demonstrate basic
 * usage of different visitors classes and driver class.
 **/

int main(int argc, const char* argv[]) {
    try {
        TCLAP::CmdLine cmd("NMODL Visitor: Standalone visitor program for NMODL");
        TCLAP::ValueArg<std::string> filearg("", "file", "NMODL input file path", false,
                                             "../test/input/channel.mod", "string");
        TCLAP::SwitchArg verbosearg("", "verbose", "Enable verbose output", false);

        cmd.add(filearg);
        cmd.add(verbosearg);

        cmd.parse(argc, argv);

        std::string filename = filearg.getValue();
        bool verbose = verbosearg.getValue();

        std::ifstream file(filename);

        if (!file.good()) {
            throw std::runtime_error("Could not open file " + filename);
        }

        std::string channel_name = remove_extension(base_name(filename));

        /// driver object creates lexer and parser, just call parser method
        nmodl::Driver driver;
        driver.parse_file(filename);

        /// shared_ptr to ast constructed from parsing nmodl file
        auto ast = driver.ast();

        {
            /// run basic AST visitor
            AstVisitor v;
            v.visit_program(ast.get());

            std::cout << "----AST VISITOR FINISHED----" << std::endl;
        }

        {
            /// run basic Verbatim visitor
            /// constructor takes true/false argument for printing blocks
            VerbatimVisitor v;
            v.visit_program(ast.get());

            std::cout << "----VERBATIM VISITOR FINISHED----" << std::endl;
        }

        {
            std::stringstream ss;
            JSONVisitor v(ss);

            /// to get compact json we can set compact mode
            /// v.compact_json(true);

            v.visit_program(ast.get());

            std::cout << "----JSON VISITOR FINISHED----" << std::endl;

            if (verbose) {
                std::cout << "RESULT OF JSON VISITOR : " << std::endl;
                std::cout << ss.str();
            }
        }

        {
            // todo : we should pass this or use get method to retrieve?
            std::stringstream ss1;

            SymtabVisitor v(ss1);
            v.visit_program(ast.get());

            // std::cout << ss1.str();

            std::stringstream ss2;
            auto symtab = ast->get_model_symbol_table();
            symtab->print(ss2);
            std::cout << ss2.str();

            std::cout << "----SYMTAB VISITOR FINISHED----" << std::endl;
        }

        {
            CnexpSolveVisitor v;
            v.visit_program(ast.get());
            std::cout << "----CNEXP SOLVE VISITOR FINISHED----" << std::endl;
        }

        {
            PerfVisitor v(channel_name + ".perf.json");
            v.visit_program(ast.get());

            auto symtab = ast->get_symbol_table();
            std::stringstream ss;
            symtab->print(ss, 0);
            std::cout << ss.str();

            ss.str("");
            v.print(ss);
            std::cout << ss.str() << std::endl;
            std::cout << "----PERF VISITOR FINISHED----" << std::endl;
        }

        {
            NmodlPrintVisitor v(channel_name + ".nocmodl.cnexp.mod");
            v.visit_program(ast.get());
        }

        {
            LocalVarRenameVisitor v;
            v.visit_program(ast.get());
        }

        {
            InlineVisitor v;
            v.visit_program(ast.get());
        }

        {
            NmodlPrintVisitor v(channel_name + ".nocmodl.in.mod");
            v.visit_program(ast.get());
        }

        {
            LocalizeVisitor v;
            v.visit_program(ast.get());
        }

        {
            NmodlPrintVisitor v(channel_name + ".nocmodl.loc.mod");
            v.visit_program(ast.get());
        }

        {
            LocalVarRenameVisitor v;
            v.visit_program(ast.get());
        }

        {
            NmodlPrintVisitor v(channel_name + ".nocmodl.loc.ren.mod");
            v.visit_program(ast.get());
        }
    } catch (TCLAP::ArgException& e) {
        std::cout << "Argument Error: " << e.error() << " for arg " << e.argId() << std::endl;
        return 1;
    }

    return 0;
}
