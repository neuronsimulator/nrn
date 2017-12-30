#include <fstream>
#include <iostream>

#include "ast/ast.hpp"
#include "lexer/nmodl_lexer.hpp"
#include "parser/nmodl_driver.hpp"
#include "tclap/CmdLine.h"

/**
 * Standlone lexer program for NMODL. This demonstrate basic
 * usage of scanner and driver class. We parse user provided
 * nmodl file and print individual token with it's value and
 * location.
 */

int main(int argc, char* argv[]) {
    try {
        TCLAP::CmdLine cmd("NMODL Lexer: Standalone lexer program for NMODL");
        TCLAP::ValueArg<std::string> filearg("", "file", "NMODL input file path", false,
                                             "../test/input/channel.mod", "string");

        cmd.add(filearg);
        cmd.parse(argc, argv);

        std::string filename = filearg.getValue();
        std::ifstream file(filename);

        if (!file) {
            throw std::runtime_error("Could not open file " + filename);
        }

        std::cout << "\n NMODL Lexer : Processing file : " << filename << std::endl;

        std::istream& in(file);

        /// lexer instace use driver object for error reporting
        nmodl::Driver driver;

        /// lexer instace with stream to read-in tokens
        nmodl::Lexer scanner(driver, &in);

        using Token = nmodl::Parser::token;
        using TokenType = nmodl::Parser::token_type;
        using SymbolType = nmodl::Parser::symbol_type;

        /// parse nmodl file untile EOF, print each token
        while (true) {
            SymbolType sym = scanner.next_token();
            TokenType token = sym.token();

            /// end of file
            if (token == Token::END) {
                break;
            }

            /** Lexer returns different ast types base on token type. We
             * retrieve token object from each instance and print it.
             * Note that value is of ast type i.e. ast::name_ptr etc. */
            switch (token) {
                /// token with name ast class
                case Token::NAME:
                case Token::METHOD:
                case Token::SUFFIX:
                case Token::VALENCE:
                case Token::DEL:
                case Token::DEL2: {
                    auto value = sym.value.as<ast::name_ptr>();
                    std::cout << *(value->get_token()) << std::endl;
                    delete value;
                    break;
                }

                /// token with prime ast class
                case Token::PRIME: {
                    auto value = sym.value.as<ast::primename_ptr>();
                    std::cout << *(value->get_token()) << std::endl;
                    delete value;
                    break;
                }

                /// token with integer ast class
                case Token::INTEGER: {
                    auto value = sym.value.as<ast::integer_ptr>();
                    std::cout << *(value->get_token()) << std::endl;
                    delete value;
                    break;
                }

                /// token with double/float ast class
                case Token::REAL: {
                    auto value = sym.value.as<ast::double_ptr>();
                    std::cout << *(value->get_token()) << std::endl;
                    delete value;
                    break;
                }

                /// token with string ast class
                case Token::STRING: {
                    auto value = sym.value.as<ast::string_ptr>();
                    std::cout << *(value->get_token()) << std::endl;
                    delete value;
                    break;
                }

                /// token with string data type
                case Token::VERBATIM:
                case Token::COMMENT:
                case Token::LINE_PART: {
                    auto str = sym.value.as<std::string>();
                    std::cout << str << std::endl;
                    break;
                }

                /// all remaining tokens has ModToken* as a vaue
                default: {
                    auto token = sym.value.as<ModToken>();
                    std::cout << token << std::endl;
                }
            }
        }
    } catch (TCLAP::ArgException& e) {
        std::cout << std::endl << "Argument Error: " << e.error() << " for arg " << e.argId();
        return 1;
    }

    return 0;
}
