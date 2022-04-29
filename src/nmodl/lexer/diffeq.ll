/*************************************************************************
 * Copyright (C) 2018-2022 Blue Brain Project
 * Copyright (C) 2018-2022 Michael Hines
 *
 * This file is part of NMODL distributed under the terms of the GNU
 * Lesser General Public License. See top-level LICENSE file for details.
 *
 * @brief Lexer for ODEs from NMODL
 *************************************************************************/

%{
    #include <cstdio>
    #include <cstdlib>
    #include <iostream>
    #include <stdlib.h>

    #include "lexer/diffeq_lexer.hpp"
    #include "parser/diffeq_driver.hpp"

    #define yyterminate() return DiffeqParser::make_END(loc);

    #define YY_USER_ACTION { loc.step(); loc.columns(yyleng); }

    #define YY_NO_UNISTD_H

%}

D   [0-9]
E   [Ee][-+]?{D}+
N   [a-zA-Z][a-zA-Z0-9_]*

/** if want to use yymore feature in copy modes */
%option yymore

/** name of the lexer header file */
%option header-file="diffeq_base_lexer.hpp"

/** name of the lexer implementation file */
%option outfile="diffeq_base_lexer.cpp"

/** change the name of the scanner class (to "DiffEqFlexLexer") */
%option prefix="DiffEq"

/** enable C++ scanner which is also reentrant */
%option c++

/* no need for includes */
%option noyywrap

/** need to unput characters back to buffer for custom routines */
%option unput

/** need to put in buffer for custom routines */
%option input

/* not an interactive lexer, takes a file or stream */
%option batch

/** enable debug mode (disable for production) */
%option debug

/** instructs flex to generate an 8-bit scanner, i.e.,
  * one which can recognize 8-bit characters. */
%option 8bit

/** show warning messages */
%option warn

/* to insure there are no holes in scanner rules */
%option nodefault


%%

"+"                     { return DiffeqParser::make_ADD(yytext, loc); }

"-"                     { return DiffeqParser::make_SUB(yytext, loc); }

"*"                     { return DiffeqParser::make_MUL(yytext, loc); }

"/"                     { return DiffeqParser::make_DIV(yytext, loc); }

"("                     { return DiffeqParser::make_OPEN_PARENTHESIS(yytext, loc); }

")"                     { return DiffeqParser::make_CLOSE_PARENTHESIS(yytext, loc); }

","                     { return DiffeqParser::make_COMMA(yytext, loc); }

:.*                     { /* ignore inline comments */ }

[ \t]                   { /* ignore white spaces */ }

{D}+                    |
{D}+"."{D}*({E})?       |
{D}*"."{D}+({E})?       |
{D}+{E}                 |
{N}\[(({D}+)|({N}))\]   |
{N}                     { return DiffeqParser::make_ATOM(yytext, loc); }

"\n"                    { return DiffeqParser::make_NEWLINE(yytext, loc); }


%%

int DiffEqFlexLexer::yylex() {
  throw std::runtime_error("next_token() should be used instead of yylex()");
}

