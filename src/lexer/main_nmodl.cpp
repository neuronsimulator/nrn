/*************************************************************************
 * Copyright (C) 2018-2019 Blue Brain Project
 *
 * This file is part of NMODL distributed under the terms of the GNU
 * Lesser General Public License. See top-level LICENSE file for details.
 *************************************************************************/

#include <fstream>
#include <streambuf>

#include "CLI/CLI.hpp"
#include "fmt/format.h"

#include "ast/ast.hpp"
#include "lexer/nmodl_lexer.hpp"
#include "parser/nmodl_driver.hpp"
#include "utils/logger.hpp"
#include "version/version.h"

/**
 * Stand alone lexer program for NMODL. This demonstrate basic
 * usage of scanner and driver classes. We parse user provided
 * nmodl file and print individual token with it's value and
 * location.
 */

using namespace fmt::literals;
using namespace nmodl;

using parser::NmodlDriver;
using parser::NmodlLexer;
using SymbolType = parser::NmodlParser::symbol_type;
using Token = parser::NmodlParser::token;
using TokenType = parser::NmodlParser::token_type;

void tokenize(const std::string& mod_text) {
    std::istringstream in(mod_text);

    /// lexer instance use driver object for error reporting
    NmodlDriver driver;

    /// lexer instance with stream to read-in tokens
    NmodlLexer scanner(driver, &in);

    /// parse nmodl text and print token until EOF
    while (true) {
        SymbolType sym = scanner.next_token();
        TokenType token = sym.token();

        if (token == Token::END) {
            break;
        }

        /** Lexer returns different ast types base on token type. We
         * retrieve token object from each instance and print it.
         * Note that value is of ast type i.e. ast::Name* etc. */
        switch (token) {
        /// token with name ast class
        case Token::NAME:
        case Token::METHOD:
        case Token::SUFFIX:
        case Token::VALENCE:
        case Token::DEL:
        case Token::DEL2: {
            auto value = sym.value.as<ast::Name>();
            std::cout << *(value.get_token()) << std::endl;
            break;
        }

            /// token with prime ast class
        case Token::PRIME: {
            auto value = sym.value.as<ast::PrimeName>();
            std::cout << *(value.get_token()) << std::endl;
            break;
        }

            /// token with integer ast class
        case Token::INTEGER: {
            auto value = sym.value.as<ast::Integer>();
            std::cout << *(value.get_token()) << std::endl;
            break;
        }

            /// token with double/float ast class
        case Token::REAL: {
            auto value = sym.value.as<ast::Double>();
            std::cout << *(value.get_token()) << std::endl;
            break;
        }

            /// token with string ast class
        case Token::STRING: {
            auto value = sym.value.as<ast::String>();
            std::cout << *(value.get_token()) << std::endl;
            break;
        }

            /// token with string data type
        case Token::VERBATIM:
        case Token::BLOCK_COMMENT:
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
}


int main(int argc, const char* argv[]) {
    CLI::App app{"NMODL-Lexer : Standalone Lexer for NMODL Code({})"_format(version::to_string())};

    std::vector<std::string> mod_files;
    std::vector<std::string> mod_texts;

    app.add_option("file", mod_files, "One or more NMODL files")->check(CLI::ExistingFile);
    app.add_option("--text", mod_texts, "One or more NMODL constructs as text");

    CLI11_PARSE(app, argc, argv);

    for (const auto& file: mod_files) {
        logger->info("Processing file : {}", file);
        std::ifstream f(file);
        std::string mod{std::istreambuf_iterator<char>{f}, {}};
        tokenize(mod);
    }

    for (const auto& text: mod_texts) {
        logger->info("Processing text : {}", text);
        tokenize(text);
    }

    return 0;
}
