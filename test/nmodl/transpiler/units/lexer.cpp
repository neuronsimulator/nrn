/*************************************************************************
 * Copyright (C) 2018-2019 Blue Brain Project
 *
 * This file is part of NMODL distributed under the terms of the GNU
 * Lesser General Public License. See top-level LICENSE file for details.
 *************************************************************************/

#define CATCH_CONFIG_MAIN

#include <string>

#include "catch/catch.hpp"
#include "lexer/unit_lexer.hpp"
#include "parser/unit_driver.hpp"

using namespace nmodl;

using nmodl::parser::UnitDriver;
using nmodl::parser::UnitLexer;
using parser::UnitParser;
using Token = UnitParser::token;
using TokenType = UnitParser::token_type;
using SymbolType = UnitParser::symbol_type;

/// just retrieve token type from lexer
TokenType token_type(const std::string& name) {
    std::istringstream ss(name);
    std::istream& in = ss;

    UnitDriver driver;
    UnitLexer scanner(driver, &in);
    return scanner.next_token().token();
}

TEST_CASE("Lexer tests for valid tokens", "[Lexer]") {
    SECTION("Tests for comments") {
        REQUIRE(token_type("/ comment") == Token::COMMENT);
        REQUIRE(token_type("/comment") == Token::COMMENT);
    }

    SECTION("Tests for doubles") {
        REQUIRE(token_type("27.52") == Token::DOUBLE);
        REQUIRE(token_type("1.01325+5") == Token::DOUBLE);
        REQUIRE(token_type("1") == Token::DOUBLE);
        REQUIRE(token_type("-1.324e+10") == Token::DOUBLE);
        REQUIRE(token_type("1-1") == Token::DOUBLE);
        REQUIRE(token_type("1|100") == Token::FRACTION);
        REQUIRE(token_type(".03") == Token::DOUBLE);
        REQUIRE(token_type("1|8.988e9") == Token::FRACTION);
    }

    SECTION("Tests for units") {
        REQUIRE(token_type("*a*") == Token::BASE_UNIT);
        REQUIRE(token_type("*k*") == Token::INVALID_BASE_UNIT);
        REQUIRE(token_type("planck") == Token::NEW_UNIT);
        REQUIRE(token_type(" m2") == Token::UNIT_POWER);
        REQUIRE(token_type(" m") == Token::UNIT);
        REQUIRE(token_type("yotta-") == Token::PREFIX);
    }

    SECTION("Tests for special characters") {
        REQUIRE(token_type("-") == Token::UNIT_PROD);
        REQUIRE(token_type(" / ") == Token::DIVISION);
        REQUIRE(token_type("\n") == Token::NEWLINE);
    }
}
