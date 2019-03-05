/*************************************************************************
 * Copyright (C) 2018-2019 Blue Brain Project
 *
 * This file is part of NMODL distributed under the terms of the GNU
 * Lesser General Public License. See top-level LICENSE file for details.
 *************************************************************************/

#include <fstream>
#include <iostream>
#include <sstream>

#include "parser/nmodl_driver.hpp"
#include "visitors/ast_visitor.hpp"
#include "visitors/cnexp_solve_visitor.hpp"
#include "visitors/inline_visitor.hpp"
#include "visitors/json_visitor.hpp"
#include "visitors/local_var_rename_visitor.hpp"
#include "visitors/localize_visitor.hpp"
#include "visitors/nmodl_visitor.hpp"
#include "visitors/perf_visitor.hpp"
#include "visitors/symtab_visitor.hpp"
#include "visitors/verbatim_var_rename_visitor.hpp"
#include "visitors/verbatim_visitor.hpp"

#include "tclap/CmdLine.h"

using namespace nmodl;
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

        std::string mod_filename = remove_extension(base_name(filename));

        /// driver object creates lexer and parser, just call parser method
        nmodl::parser::NmodlDriver driver;
        driver.parse_file(filename);

        /// shared_ptr to ast constructed from parsing nmodl file
        auto ast = driver.ast();

        {
            AstVisitor v;
            v.visit_program(ast.get());
            std::cout << "----AST VISITOR FINISHED----" << std::endl;
        }

        {
            /// constructor takes true/false argument for printing blocks
            VerbatimVisitor v;
            v.visit_program(ast.get());
            std::cout << "----VERBATIM VISITOR FINISHED----" << std::endl;
        }

        {
            std::stringstream ss;
            JSONVisitor v(ss);
            v.visit_program(ast.get());
            if (verbose) {
                std::cout << "RESULT OF JSON VISITOR : " << std::endl << ss.str();
            }
            std::cout << "----JSON VISITOR FINISHED----" << std::endl;
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
            NmodlPrintVisitor v(mod_filename + ".nmodl.verbrename.mod");
            v.visit_program(ast.get());
        }

        {
            NmodlPrintVisitor v(mod_filename + ".nmodl.mod");
            v.visit_program(ast.get());
        }

        {
            CnexpSolveVisitor v;
            v.visit_program(ast.get());
            std::cout << "----CNEXP SOLVE VISITOR FINISHED----" << std::endl;
        }

        {
            NmodlPrintVisitor v(mod_filename + ".nmodl.cnexp.mod");
            v.visit_program(ast.get());
        }


        {
            InlineVisitor v;
            v.visit_program(ast.get());
        }


        {
            NmodlPrintVisitor v(mod_filename + ".nmodl.cnexp.in.mod");
            v.visit_program(ast.get());
        }

        {
            LocalVarRenameVisitor v;
            v.visit_program(ast.get());
            std::cout << "----LOCAL VAR RENAME VISITOR FINISHED----" << std::endl;
        }

        {
            NmodlPrintVisitor v(mod_filename + ".nmodl.cnexp.in.ren.mod");
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
            NmodlPrintVisitor v(mod_filename + ".nmodl.cnexp.in.ren.loc.mod");
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
            NmodlPrintVisitor v(mod_filename + ".nmodl.cnexp.in.ren.loc.ren.mod");
            v.visit_program(ast.get());
        }

        {
            PerfVisitor v(mod_filename + ".perf.json");
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


    } catch (TCLAP::ArgException& e) {
        std::cout << "Argument Error: " << e.error() << " for arg " << e.argId() << std::endl;
        return 1;
    }

    return 0;
}
