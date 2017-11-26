#include <fstream>
#include <iostream>
#include <sstream>

#include "parser/nmodl_driver.hpp"
#include "visitors/ast_visitor.hpp"
#include "visitors/json_visitor.hpp"
#include "visitors/verbatim_visitor.hpp"

#include "tclap/CmdLine.h"

/**
 * Standlone visitor program for NMODL. This demonstrate basic
 * usage of different visitors classes and driver class.
 **/

int main(int argc, char* argv[]) {
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

        /// driver object creates lexer and parser, just call parser method
        nmodl::Driver driver;
        driver.parse_file(filename);

        /// shared_ptr to ast constructed from parsing nmodl file
        auto ast = driver.ast();

        {
            /// run basic AST visitor
            AstVisitor v;
            v.visitProgram(ast.get());

            std::cout << "----AST VISITOR FINISHED----" << std::endl;
        }

        {
            /// run basic Verbatim visitor
            /// constructor takes true/false argument for printing blocks
            VerbatimVisitor v;
            v.visitProgram(ast.get());

            std::cout << "----VERBATIM VISITOR FINISHED----" << std::endl;
        }

        {
            std::stringstream ss;
            JSONVisitor v(ss);

            /// to get compact json we can set compact mode
            /// v.compact_json(true);

            v.visitProgram(ast.get());

            std::cout << "----JSON VISITOR FINISHED----" << std::endl;

            if (verbose) {
                std::cout << "RESULT OF JSON VISITOR : " << std::endl;
                std::cout << ss.str();
            }
        }
    } catch (TCLAP::ArgException& e) {
        std::cout << "Argument Error: " << e.error() << " for arg " << e.argId() << std::endl;
        return 1;
    }

    return 0;
}
