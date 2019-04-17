/*************************************************************************
 * Copyright (C) 2018-2019 Blue Brain Project
 *
 * This file is part of NMODL distributed under the terms of the GNU
 * Lesser General Public License. See top-level LICENSE file for details.
 *************************************************************************/

#pragma once

#include "parser/diffeq/diffeq_parser.hpp"

/**
 * Flex expects the declaration of yylex to be defined in the macro YY_DECL
 * and C++ parser class expects it to be declared.
 */
#ifndef YY_DECL
#define YY_DECL nmodl::parser::DiffeqParser::symbol_type nmodl::parser::DiffeqLexer::next_token()
#endif

/**
 * For creating multiple (different) lexer classes, we can use `-P` flag
 * (or prefix option) to rename each `yyFlexLexer` to some other name like
 * `xxFlexLexer`. And then include <FlexLexer.h> in other sources once per
 * lexer class, first renaming `yyFlexLexer `as shown below.
 */
#ifndef __FLEX_LEXER_H
#define yyFlexLexer DiffEqFlexLexer
#include "FlexLexer.h"
#endif

namespace nmodl {
namespace parser {

/**
 * @addtogroup lexer
 * @{
 */

/**
 * \class DiffeqLexer
 * \brief Represent Lexer/Scanner class for differential equation parsing
 *
 * Lexer defined to add some extra function to the scanner class from flex.
 * At the moment we are using basic functionality but it could be easily
 * extended for further development.
 */
class DiffeqLexer: public DiffEqFlexLexer {
  public:
    /// location of the parsed token
    location loc;

    /// \name Ctor & dtor
    /// \{

    /*
     * \brief DiffeqLexer constructor
     *
     * @param in Input stream from where tokens will be read
     * @param out Output stream where output will be sent
     */
    DiffeqLexer(std::istream* in = nullptr, std::ostream* out = nullptr)
        : DiffEqFlexLexer(in, out) {}

    ~DiffeqLexer() override = default;

    /// \}

    /**
     * \brief Function for lexer to scan token (replacement for \c yylex())
     *
     * This is main lexing function generated by `flex` according to the macro
     * declaration \c YY_DECL. The generated bison parser then calls this virtual
     * function to fetch new tokens. Note that \c yylex() has different declaration
     * and hence can't be used for new lexer.
     *
     * @return Symbol encapsulating parsed token
     */
    virtual DiffeqParser::symbol_type next_token();
};

/** @} */  // end of lexer

}  // namespace parser
}  // namespace nmodl
