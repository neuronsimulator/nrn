/*************************************************************************
 * Copyright (C) 2018-2022 Blue Brain Project
 *
 * This file is part of NMODL distributed under the terms of the GNU
 * Lesser General Public License. See top-level LICENSE file for details.
 *
 * @brief Lexer for verbatim constructs from NMODL (e.g. VERBATIM, COMMENT)
 *************************************************************************/

%{
    #include <cstdio>
    #include <cstdlib>
    #include <iostream>
    #include <stdlib.h>
    #include "parser/verbatim_driver.hpp"
    #include "parser/verbatim_parser.hpp"

    /**
     * The scanner state will include a field called yyextra
     * that can be used for user-defined state
     */
    #define YY_EXTRA_TYPE nmodl::parser::VerbatimDriver*


    /**
     * We need to parse strings as well as files, we redefine YY_INPUT
     * and give it a C++ flavour. It will use the istream from the parser
     * context to read the next character. The parser context defaults is
     * to cin, but we will set it to an istringstream later.
     *
     * \todo Not able recognize NEWLINE character
     */
    #define YY_INPUT(buf,result,max_size)       \
    {                                           \
        char c;                                 \
        (*yyextra->is) >> std::noskipws >> c;   \
        if(yyextra->is->eof())                  \
            result = YY_NULL;                   \
        else {                                  \
            buf[0] = c;                         \
            result = 1;                         \
        }                                       \
    }

%}

/**
 * need to provide this option to flex otherwise complain:
 * "error: 'yymore_used_but_not_detected' was not declared in
 * this scope
 */
%option yymore

/** lexer header file */
%option header-file="verbatim_lexer.hpp"

/** lexer implementation file */
%option outfile="verbatim_lexer.cpp"

/** lexer prefix */
%option prefix="Verbatim_"

/** need to be reentrant */
%option reentrant

/** no need for includes */
%option noyywrap

/**
 * need to unput in buffer for custom routines
 * \todo Check if unused function warning appears
 */
%option nounput

/**
 * need to input in buffer for custom routines
 * \todo Check unused function warning
 */
%option noinput

/** not an interactive lexer, takes a file */
%option batch

/** debug, disable for production */
%option debug

/** bison compatible lexer */
%option bison-bridge

/** bison location information */
%option bison-locations

/** keep line information */
%option yylineno

/** mode for verbatim or comment */
%x COPY_MODE

%%

    /**
     * Currently we do simple thing for VERBATIM and COMMENT
     * just pass them as it is!
     * This will be revisited in the future once we have
     * everything in place working with clang!
     */

"VERBATIM"                  {
                                BEGIN(COPY_MODE);
                                return VERBATIM;
                            }

"COMMENT"                   {
                                BEGIN(COPY_MODE);
                                return COMMENT;
                            }

<COPY_MODE>"ENDVERBATIM"   {
                                BEGIN(INITIAL);
                                return ENDVERBATIM;
                            }


<COPY_MODE>"ENDCOMMENT"    {
                                BEGIN(INITIAL);
                                return ENDCOMMENT;
                            }


<COPY_MODE>"\n"            {
                                yylval->str = yytext[0];
                                return NEWLINE;
                            }

<COPY_MODE>.               {
                                yylval->str = yytext[0];
                                return CHAR;
                            }

%%


/** initialize nmodlext lexer context */
void nmodl::parser::VerbatimDriver::init_scanner() {
    yylex_init(&scanner);
    yyset_extra(this, scanner);
}


/** delete nmodlext lexer context */
void nmodl::parser::VerbatimDriver::destroy_scanner() {
    yylex_destroy(scanner);
}

