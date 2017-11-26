#pragma once

#include "parser/location.hh"
#include "parser/nmodl_parser.hpp"

/**
 * \brief Utility functions for nmodl lexer
 *
 * From nmodl lexer we return different symbols to parser.
 * Instead of writing those functions in the flex implementation
 * file, those commonly used routines are defined here. Some of
 * these tasks were implemented in list.c file in the oiginal mod2c
 * implementation.
 */

namespace nmodl {
    using PositionType = nmodl::location;
    using SymbolType = nmodl::Parser::symbol_type;
    using Token = nmodl::Parser::token;
    using TokenType = nmodl::Parser::token_type;

    SymbolType double_symbol(double value, PositionType& pos);
    SymbolType integer_symbol(int value, PositionType& pos, const char* name = nullptr);
    SymbolType name_symbol(std::string text, PositionType& pos, TokenType token = Token::NAME);
    SymbolType prime_symbol(std::string text, PositionType& pos);
    SymbolType string_symbol(std::string text, PositionType& pos);
    SymbolType token_symbol(std::string text, PositionType& pos, TokenType token = Token::UNKNOWN);

}  // namespace nmodl