/*************************************************************************
 * Copyright (C) 2018-2022 Blue Brain Project
 *
 * This file is part of NMODL distributed under the terms of the GNU
 * Lesser General Public License. See top-level LICENSE file for details.
 *************************************************************************/

#define CATCH_CONFIG_MAIN

#include <string>

#include <catch/catch.hpp>

#include "lexer/modtoken.hpp"
#include "lexer/nmodl_lexer.hpp"
#include "parser/nmodl_driver.hpp"

using namespace nmodl;

using nmodl::parser::NmodlDriver;
using nmodl::parser::NmodlLexer;
using parser::NmodlParser;
using Token = NmodlParser::token;
using TokenType = NmodlParser::token_type;
using SymbolType = NmodlParser::symbol_type;


/// retrieve token type from lexer and check if it's of given type
bool check_token_type(const std::string& name, TokenType type) {
    std::istringstream ss(name);
    std::istream& in = ss;

    NmodlDriver driver;
    NmodlLexer scanner(driver, &in);

    SymbolType sym = scanner.next_token();
    int token_type = sym.type_get();

    auto get_token_type = [](TokenType token) {
        return parser::NmodlParser::by_type(token).type_get();
    };

    /**
     * Lexer returns raw pointers for some AST types
     * and we need to clean-up memory for those.
     * Todo: add tests later for checking values
     */
    // clang-format off
    if (token_type == get_token_type(Token::NAME) ||
        token_type == get_token_type(Token::METHOD) ||
        token_type == get_token_type(Token::SUFFIX) ||
        token_type == get_token_type(Token::VALENCE) ||
        token_type == get_token_type(Token::DEL) ||
        token_type == get_token_type(Token::DEL2)) {
        auto value = sym.value.as<ast::Name>();
        REQUIRE(value.get_node_name() != "");
    }
    // clang-format on
    // prime variable
    else if (token_type == get_token_type(Token::PRIME)) {
        auto value = sym.value.as<ast::PrimeName>();
        REQUIRE(value.get_node_name() != "");
    }
    // integer constant
    else if (token_type == get_token_type(Token::INTEGER)) {
        auto value = sym.value.as<ast::Integer>();
        REQUIRE(value.get_value() != 0);
    }
    // float constant
    else if (token_type == get_token_type(Token::REAL)) {
        auto value = sym.value.as<ast::Double>();
        REQUIRE(value.to_double() != 0);
    }
    // const char*
    else if (token_type == get_token_type(Token::STRING)) {
        auto value = sym.value.as<ast::String>();
        REQUIRE(value.get_value() != "");
    }
    // string block representation verbatim or block comment
    else if (token_type == get_token_type(Token::VERBATIM) ||
             token_type == get_token_type(Token::BLOCK_COMMENT) ||
             token_type == get_token_type(Token::LINE_PART)) {
        auto value = sym.value.as<std::string>();
        REQUIRE(value != "");
    }
    // rest of the tokens
    else {
        auto value = sym.value.as<ModToken>();
        REQUIRE(value.text() != "");
    }

    return sym.type_get() == get_token_type(type);
}

TEST_CASE("NMODL Lexer returning valid token types", "[Lexer]") {
    SECTION("Some keywords") {
        REQUIRE(check_token_type("VERBATIM Hello ENDVERBATIM", Token::VERBATIM));
        REQUIRE(check_token_type("INITIAL", Token::INITIAL1));
        REQUIRE(check_token_type("SOLVE", Token::SOLVE));
    }

    SECTION("NMODL language keywords and constructs") {
        REQUIRE(check_token_type(" h' = (hInf-h)/hTau\n", Token::PRIME));
        REQUIRE(check_token_type("while", Token::WHILE));
        REQUIRE(check_token_type("if", Token::IF));
        REQUIRE(check_token_type("else", Token::ELSE));
        REQUIRE(check_token_type("WHILE", Token::WHILE));
        REQUIRE(check_token_type("IF", Token::IF));
        REQUIRE(check_token_type("ELSE", Token::ELSE));
        REQUIRE(check_token_type("REPRESENTS NCIT:C17145", Token::REPRESENTS));
    }

    SECTION("Different number types") {
        REQUIRE(check_token_type("123", Token::INTEGER));
        REQUIRE(check_token_type("123.32", Token::REAL));
        REQUIRE(check_token_type("1.32E+3", Token::REAL));
        REQUIRE(check_token_type("1.32e-3", Token::REAL));
        REQUIRE(check_token_type("32e-3", Token::REAL));
        REQUIRE(check_token_type("1e+23", Token::REAL));
        REQUIRE(check_token_type("1e-23", Token::REAL));
        REQUIRE_FALSE(check_token_type("124.11", Token::INTEGER));
    }

    SECTION("Name/Strings types") {
        REQUIRE(check_token_type("neuron", Token::NAME));
        REQUIRE(check_token_type("\"Quoted String\"", Token::STRING));
    }

    SECTION("Logical operator types") {
        REQUIRE(check_token_type(">", Token::GT));
        REQUIRE(check_token_type(">=", Token::GE));
        REQUIRE(check_token_type("<", Token::LT));
        REQUIRE(check_token_type("==", Token::EQ));
        REQUIRE(check_token_type("!=", Token::NE));
        REQUIRE(check_token_type("<->", Token::REACT1));
        REQUIRE(check_token_type("~+", Token::NONLIN1));
    }

    SECTION("Brace types") {
        REQUIRE(check_token_type("{", Token::OPEN_BRACE));
        REQUIRE(check_token_type("}", Token::CLOSE_BRACE));
        REQUIRE(check_token_type("(", Token::OPEN_PARENTHESIS));
        REQUIRE_FALSE(check_token_type(")", Token::OPEN_PARENTHESIS));
    }
}
