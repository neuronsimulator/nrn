#define CATCH_CONFIG_MAIN

#include <string>

#include "catch.hpp"
#include "lexer/modtoken.hpp"
#include "lexer/nmodl_lexer.hpp"
#include "parser/nmodl_driver.hpp"

using Token = nmodl::Parser::token;

/// just retrieve token type from lexer
nmodl::Parser::token_type token_type(std::string name) {
    std::istringstream ss(name);
    std::istream& in = ss;

    nmodl::Driver driver;
    nmodl::Lexer scanner(driver, &in);

    nmodl::Parser::symbol_type sym = scanner.next_token();
    return sym.token();
}

TEST_CASE("Lexer tests for valid tokens", "[Lexer]") {
    SECTION("Tests for some keywords") {
        REQUIRE(token_type("VERBATIM Hello ENDVERBATIM") == Token::VERBATIM);
        REQUIRE(token_type("INITIAL") == Token::INITIAL1);
        REQUIRE(token_type("SOLVE") == Token::SOLVE);
    }

    SECTION("Tests for language constructs") {
        REQUIRE(token_type(" h' = (hInf-h)/hTau\n") == Token::PRIME);
        REQUIRE(token_type("while") == Token::WHILE);
        REQUIRE(token_type("if") == Token::IF);
        REQUIRE(token_type("else") == Token::ELSE);
        REQUIRE(token_type("WHILE") == Token::WHILE);
        REQUIRE(token_type("IF") == Token::IF);
        REQUIRE(token_type("ELSE") == Token::ELSE);
    }

    SECTION("Tests for valid numbers") {
        REQUIRE(token_type("123") == Token::INTEGER);
        REQUIRE(token_type("123.32") == Token::REAL);
        REQUIRE(token_type("1.32E+3") == Token::REAL);
        REQUIRE(token_type("1.32e-3") == Token::REAL);
        REQUIRE(token_type("32e-3") == Token::REAL);
    }

    SECTION("Tests for Name/Strings") {
        REQUIRE(token_type("neuron") == Token::NAME);
        REQUIRE(token_type("\"Quoted String\"") == Token::STRING);
    }

    SECTION("Tests for (math) operators") {
        REQUIRE(token_type(">") == Token::GT);
        REQUIRE(token_type(">=") == Token::GE);
        REQUIRE(token_type("<") == Token::LT);
        REQUIRE(token_type("==") == Token::EQ);
        REQUIRE(token_type("!=") == Token::NE);
        REQUIRE(token_type("<->") == Token::REACT1);
        REQUIRE(token_type("~+") == Token::NONLIN1);
        // REQUIRE( token_type("~") == Token::REACTION);
    }

    SECTION("Tests for braces") {
        REQUIRE(token_type("{") == Token::OPEN_BRACE);
        REQUIRE(token_type("}") == Token::CLOSE_BRACE);
        REQUIRE(token_type("(") == Token::OPEN_PARENTHESIS);
        REQUIRE(token_type(")") != Token::OPEN_PARENTHESIS);
    }
}
