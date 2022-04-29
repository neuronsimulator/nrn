/*************************************************************************
 * Copyright (C) 2018-2022 Blue Brain Project
 *
 * This file is part of NMODL distributed under the terms of the GNU
 * Lesser General Public License. See top-level LICENSE file for details.
 *************************************************************************/

#pragma once

/**
 * \file nmodl_utils.hpp
 * \brief Utility functions for NMODL lexer
 *
 * From nmodl lexer we return different symbols to parser. Instead of writing
 * those functions in the flex implementation file, those commonly used routines
 * are defined here. Some of these tasks were implemented in list.c file in the
 * original mod2c implementation.
 */

#include "parser/nmodl/location.hh"
#include "parser/nmodl/nmodl_parser.hpp"

namespace nmodl {

using PositionType = parser::location;
using SymbolType = parser::NmodlParser::symbol_type;
using Token = parser::NmodlParser::token;
using TokenType = parser::NmodlParser::token_type;

SymbolType double_symbol(const std::string& value, PositionType& pos);
SymbolType integer_symbol(int value, PositionType& pos, const char* text = nullptr);
SymbolType name_symbol(const std::string& text, PositionType& pos, TokenType type = Token::NAME);
SymbolType prime_symbol(std::string text, PositionType& pos);
SymbolType string_symbol(const std::string& text, PositionType& pos);
SymbolType token_symbol(const std::string& key, PositionType& pos, TokenType type = Token::UNKNOWN);

}  // namespace nmodl
