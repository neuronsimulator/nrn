/*************************************************************************
 * Copyright (C) 2018-2022 Blue Brain Project
 *
 * This file is part of NMODL distributed under the terms of the GNU
 * Lesser General Public License. See top-level LICENSE file for details.
 *************************************************************************/

#define CATCH_CONFIG_MAIN

#include <string>

#include <catch/catch.hpp>

#include "lexer/unit_lexer.hpp"
#include "parser/unit_driver.hpp"

using namespace nmodl;

using nmodl::parser::UnitDriver;
using nmodl::parser::UnitLexer;
using parser::UnitParser;
using Token = UnitParser::token;
using TokenType = UnitParser::token_type;
using SymbolType = UnitParser::symbol_type;

/// retrieve token type from lexer and check if it's of given type
bool check_token_type(const std::string& name, TokenType type) {
    std::istringstream ss(name);
    std::istream& in = ss;

    UnitDriver driver;
    UnitLexer scanner(driver, &in);
    auto symbol = scanner.next_token();
    return symbol.type_get() == UnitParser::by_type(type).type_get();
}

TEST_CASE("Unit Lexer tests for valid tokens", "[lexer][unit]") {
    SECTION("Tests for comments") {
        REQUIRE(check_token_type("/ comment", Token::COMMENT));
        REQUIRE(check_token_type("/comment", Token::COMMENT));
    }

    SECTION("Tests for doubles") {
        REQUIRE(check_token_type("27.52", Token::DOUBLE));
        REQUIRE(check_token_type("1.01325+5", Token::DOUBLE));
        REQUIRE(check_token_type("1", Token::DOUBLE));
        REQUIRE(check_token_type("-1.324e+10", Token::DOUBLE));
        REQUIRE(check_token_type("1-1", Token::DOUBLE));
        REQUIRE(check_token_type("1|100", Token::FRACTION));
        REQUIRE(check_token_type(".03", Token::DOUBLE));
        REQUIRE(check_token_type("12345e-2", Token::DOUBLE));
        REQUIRE(check_token_type("1|8.988e9", Token::FRACTION));
    }

    SECTION("Tests for units") {
        REQUIRE(check_token_type("*a*", Token::BASE_UNIT));
        REQUIRE(check_token_type("*k*", Token::INVALID_BASE_UNIT));
        REQUIRE(check_token_type("planck", Token::NEW_UNIT));
        REQUIRE(check_token_type("mse-1", Token::NEW_UNIT));
        REQUIRE(check_token_type("mA/cm2", Token::NEW_UNIT));
        REQUIRE(check_token_type(" m2", Token::UNIT_POWER));
        REQUIRE(check_token_type(" m", Token::UNIT));
        REQUIRE(check_token_type(" m_2", Token::UNIT));
        REQUIRE(check_token_type(" m_unit2", Token::UNIT));
        REQUIRE(check_token_type("yotta-", Token::PREFIX));
    }

    SECTION("Tests for special characters") {
        REQUIRE(check_token_type("-", Token::UNIT_PROD));
        REQUIRE(check_token_type(" / ", Token::DIVISION));
        REQUIRE(check_token_type("\n", Token::NEWLINE));
    }
}
