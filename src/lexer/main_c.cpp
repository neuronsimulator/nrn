/*************************************************************************
 * Copyright (C) 2018-2019 Blue Brain Project
 *
 * This file is part of NMODL distributed under the terms of the GNU
 * Lesser General Public License. See top-level LICENSE file for details.
 *************************************************************************/

#include <fstream>
#include <iostream>

#include "lexer/c11_lexer.hpp"
#include "parser/c11_driver.hpp"
#include "tclap/CmdLine.h"

/**
 * Standlone lexer program for C. This demonstrate basic
 * usage of scanner and driver class.
 */


int main(int argc, const char* argv[]) {
    try {
        TCLAP::CmdLine cmd("C Lexer: Standalone lexer program for C");
        TCLAP::ValueArg<std::string> filearg("", "file", "C input file path", false, "", "string");

        cmd.add(filearg);
        cmd.parse(argc, argv);

        std::string filename = filearg.getValue();

        if (filename.empty()) {
            std::cerr << "Error : Pass input C file, see --help" << std::endl;
            return 1;
        }


        std::ifstream file(filename);

        if (!file) {
            throw std::runtime_error("Could not open file " + filename);
        }

        std::cout << "\n C Lexer : Processing file : " << filename << std::endl;

        std::istream& in(file);
        nmodl::parser::CDriver driver;
        nmodl::parser::CLexer scanner(driver, &in);

        using Token = nmodl::parser::CParser::token;

        /// parse C file untile EOF, print each token
        while (true) {
            auto sym = scanner.next_token();
            auto token = sym.token();

            /// end of file
            if (token == Token::END) {
                break;
            }
            std::cout << sym.value.as<std::string>();
        }
    } catch (TCLAP::ArgException& e) {
        std::cout << std::endl << "Argument Error: " << e.error() << " for arg " << e.argId();
        return 1;
    }

    return 0;
}
