/*
 * Copyright 2023 Blue Brain Project, EPFL.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <fstream>
#include <streambuf>

#include <CLI/CLI.hpp>

#include "ast/ast.hpp"
#include "config/config.h"
#include "lexer/nmodl_lexer.hpp"
#include "parser/nmodl_driver.hpp"
#include "utils/logger.hpp"

/**
 * \file
 *
 * \brief Example of standalone lexer program for NMODL
 *
 * This example demonstrate basic usage of scanner and driver classes.
 * We parse user provided nmodl file and print individual token with
 * it's value and location.
 */

namespace nmodl {

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

    auto get_token_type = [](TokenType token) {
        return parser::NmodlParser::by_type(token).type_get();
    };

    /// parse nmodl text and print token until EOF
    while (true) {
        SymbolType sym = scanner.next_token();
        auto token_type = sym.type_get();

        if (token_type == get_token_type(Token::END)) {
            break;
        }

        /**
         * Lexer returns different ast types base on token type. We
         * retrieve token object from each instance and print it.
         * Note that value is of ast type i.e. ast::Name* etc.
         */
        /// token with name ast class
        if (token_type == get_token_type(Token::NAME) ||
            token_type == get_token_type(Token::METHOD) ||
            token_type == get_token_type(Token::SUFFIX) ||
            token_type == get_token_type(Token::VALENCE) ||
            token_type == get_token_type(Token::DEL) || token_type == get_token_type(Token::DEL2)) {
            auto value = sym.value.as<ast::Name>();
            std::cout << *(value.get_token()) << std::endl;
        }
        /// token with prime ast class
        else if (token_type == get_token_type(Token::PRIME)) {
            auto value = sym.value.as<ast::PrimeName>();
            std::cout << *(value.get_token()) << std::endl;
        }
        /// token with integer ast class
        else if (token_type == get_token_type(Token::INTEGER)) {
            auto value = sym.value.as<ast::Integer>();
            std::cout << *(value.get_token()) << std::endl;
        }
        /// token with double/float ast class
        else if (token_type == get_token_type(Token::REAL)) {
            auto value = sym.value.as<ast::Double>();
            std::cout << *(value.get_token()) << std::endl;
        }
        /// token with string ast class
        else if (token_type == get_token_type(Token::STRING)) {
            auto value = sym.value.as<ast::String>();
            std::cout << *(value.get_token()) << std::endl;
        }
        /// token with string data type
        else if (token_type == get_token_type(Token::VERBATIM) ||
                 token_type == get_token_type(Token::BLOCK_COMMENT) ||
                 token_type == get_token_type(Token::LINE_PART)) {
            auto str = sym.value.as<std::string>();
            std::cout << str << std::endl;
        }
        /// all remaining tokens has ModToken* as a vaue
        else {
            auto token = sym.value.as<ModToken>();
            std::cout << token << std::endl;
        }
    }
}

}  // namespace nmodl


int main(int argc, const char* argv[]) {
    CLI::App app{fmt::format("NMODL-Lexer : Standalone Lexer for NMODL Code({})",
                             nmodl::Version::to_string())};

    std::vector<std::string> mod_files;
    std::vector<std::string> mod_texts;

    app.add_option("file", mod_files, "One or more NMODL files")->check(CLI::ExistingFile);
    app.add_option("--text", mod_texts, "One or more NMODL constructs as text");

    CLI11_PARSE(app, argc, argv);

    for (const auto& file: mod_files) {
        nmodl::logger->info("Processing file : {}", file);
        std::ifstream f(file);
        std::string mod{std::istreambuf_iterator<char>{f}, {}};
        nmodl::tokenize(mod);
    }

    for (const auto& text: mod_texts) {
        nmodl::logger->info("Processing text : {}", text);
        nmodl::tokenize(text);
    }

    return 0;
}