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
        c11::Driver driver;
        c11::Lexer scanner(driver, &in);

        using Token = c11::Parser::token;
        using TokenType = c11::Parser::token_type;
        using SymbolType = c11::Parser::symbol_type;

        /// parse C file untile EOF, print each token
        while (true) {
            SymbolType sym = scanner.next_token();
            TokenType token = sym.token();

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
