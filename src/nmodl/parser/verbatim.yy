/*************************************************************************
 * Copyright (C) 2018-2019 Blue Brain Project
 *
 * This file is part of NMODL distributed under the terms of the GNU
 * Lesser General Public License. See top-level LICENSE file for details.
 *************************************************************************/

/*
 * Bison specification for NMODL Extensions which includes
 * VERBATIM and COMMENT blocks
 */

%{
    #include <cstdio>
    #include <cstdlib>
    #include <iostream>
    #include <cstring>
    #include "parser/verbatim_context.hpp"
%}

/* print out verbose error instead of just message 'syntax error' */
%error-verbose

/* make a reentrant parser */
%pure-parser

/* parser prefix */
%name-prefix "Verbatim_"

/* enable location tracking */
%locations

/* generate header file */
%defines

/* yyparse() takes an extra argument context */
%parse-param {nmodl::parser::VerbatimContext* context}

/* reentrant lexer needs an extra argument for yylex() */
%lex-param {void * scanner}

/* token types */
%union 
{
    char str;
    std::string *string_ptr;
}

/* define our terminal symbols (tokens)
 */
%token  <str>           CHAR
%token  <str>           NEWLINE
%token  <str>           VERBATIM
%token  <str>           COMMENT
%token  <str>           ENDVERBATIM
%token  <str>           ENDCOMMENT

%type   <string_ptr>    top
%type   <string_ptr>    charlist
%type   <string_ptr>    verbatimblock
%type   <string_ptr>    commentblock

%{

    using nmodl::parser::VerbatimContext;

    /* a macro that extracts the scanner state from the parser state for yylex */
    #define scanner context->scanner

    extern int yylex(YYSTYPE*, YYLTYPE*, void*);
    extern int yyparse(VerbatimContext*);
    extern void yyerror(YYLTYPE*, VerbatimContext*, const char *);
%}


/* start symbol is named "top" */
%start top

%%


top             : verbatimblock { $$ = $1; context->result = $1; }
                | commentblock  { $$ = $1; context->result = $1; }
                | error                 { printf("\n _ERROR_"); }
                ;

verbatimblock   : VERBATIM charlist ENDVERBATIM     { $$ = $2; }

commentblock    : COMMENT charlist ENDCOMMENT       { $$ = $2; }


charlist        :                       { $$ = new std::string(""); }
                | charlist CHAR         { *($1) += $2; $$ = $1; }
                | charlist NEWLINE      { *($1) += $2; $$ = $1; }
                ;

%%

/** \todo : better error handling required */
void yyerror(YYLTYPE* /*locp*/, VerbatimContext* /*context*/, const char *s) {
    std::printf("\n Error in verbatim parser :  %s \n", s);
    std::exit(1); 
}
