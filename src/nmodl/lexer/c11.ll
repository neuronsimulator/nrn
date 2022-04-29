/**********************************************************************************
 * Copyright (C) 2018-2022 Blue Brain Project
 *
 * This file is part of NMODL distributed under the terms of the GNU
 * Lesser General Public License. See top-level LICENSE file for details.
 *
 * CREDIT : This is based on flex specification available at
 *          http://www.quut.com/c/ANSI-C-grammar-l-2011.html
 *
 * @brief C (11) lexer based
 *****************************************************************************/

%{
    #include <iostream>

    #include "lexer/c11_lexer.hpp"
    #include "parser/c11_driver.hpp"
    #include "parser/c/c11_parser.hpp"
    #include "utils/string_utils.hpp"

    /** YY_USER_ACTION is called before each of token actions and
      * we update columns by length of the token. Node that position
      * starts at column 1 and increasing by yyleng gives extra position
      * larger than exact columns span of the token. Hence in ModToken
      * class we reduce the length by one column. */
    #define YY_USER_ACTION { loc.step(); loc.columns(yyleng); }

   /** By default yylex returns int, we use token_type. Unfortunately
     * yyterminate by default returns 0, which is not of token_type. */
   #define yyterminate() return CParser::make_END(loc);

   /** Disables inclusion of unistd.h, which is not available under Visual
     * C++ on Win32. The C++ scanner uses STL streams instead. */
   #define YY_NO_UNISTD_H

%}

%e  1019
%p  2807
%n  371
%k  284
%a  1213
%o  1117

O   [0-7]
D   [0-9]
NZ  [1-9]
L   [a-zA-Z_]
A   [a-zA-Z_0-9]
H   [a-fA-F0-9]
HP  (0[xX])
E   ([Ee][+-]?{D}+)
P   ([Pp][+-]?{D}+)
FS  (f|F|l|L)
IS  (((u|U)(l|L|ll|LL)?)|((l|L|ll|LL)(u|U)?))
CP  (u|U|L)
SP  (u8|u|U|L)
ES  (\\(['"\?\\abfnrtv]|[0-7]{1,3}|x[a-fA-F0-9]+))
WS  [ \t\v\n\f]

/** we do use yymore feature in copy modes */
%option yymore

/** name of the lexer header file */
%option header-file="c11_base_lexer.hpp"

/** name of the lexer implementation file */
%option outfile="c11_base_lexer.cpp"

/** change the name of the scanner class (to "CFlexLexer") */
%option prefix="C"

/** enable C++ scanner which is also reentrant */
%option c++

/** no plan for include files for now */
%option noyywrap

/** need to unput characters back to buffer for custom routines */
%option unput

/** need to put in buffer for custom routines */
%option input

/** not an interactive lexer, takes a file instead */
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

/* keep line information */
%option yylineno

/* enable use of start condition stacks */
%option stack

/* mode for preprocessor directive */
%x P_P_DIRECTIVE

/* mode for multi-line comment */
%x COMMENT


%%

[ \t]*"#define"                         {
                                            /** macros also need to be processed / re-defined */
                                            driver.add_token(yytext);
                                        }

[ \t]*"#"                               {
                                            BEGIN(P_P_DIRECTIVE);
                                            yymore();
                                        }

<P_P_DIRECTIVE>.                        {
                                            yymore();
                                        }

<P_P_DIRECTIVE>\n                       {
                                            driver.add_token(yytext);
                                            BEGIN(INITIAL);
                                        }

"/*"                                    {
                                            BEGIN(COMMENT);
                                            yymore();
                                        }

<COMMENT>"*/"                           {
                                            driver.add_token(yytext);
                                            BEGIN(INITIAL);
                                        }

<COMMENT>\n                             {
                                            loc.lines(1);
                                            yymore();
                                        }

<COMMENT>.                              {
                                            yymore();
                                        }

"//".*                                  {
                                            /* consume single liine comment */
                                        }

"auto"                                  {
                                            driver.add_token(yytext);
                                            return CParser::make_AUTO(yytext, loc);
                                        }

"break"                                 {
                                            driver.add_token(yytext);
                                            return CParser::make_BREAK(yytext, loc);
                                        }

"case"                                  {
                                            driver.add_token(yytext);
                                            return CParser::make_CASE(yytext, loc);
                                        }

"char"                                  {
                                            driver.add_token(yytext);
                                            return CParser::make_CHAR(yytext, loc);
                                        }

"const"                                 {
                                            driver.add_token(yytext);
                                            return CParser::make_CONST(yytext, loc);
                                        }

"continue"                              {
                                            driver.add_token(yytext);
                                            return CParser::make_CONTINUE(yytext, loc);
                                        }

"default"                               {
                                            driver.add_token(yytext);
                                            return CParser::make_DEFAULT(yytext, loc);
                                        }

"do"                                    {
                                            driver.add_token(yytext);
                                            return CParser::make_DO(yytext, loc);
                                        }

"double"                                {
                                            driver.add_token(yytext);
                                            return CParser::make_DOUBLE(yytext, loc);
                                        }

"else"                                  {
                                            driver.add_token(yytext);
                                            return CParser::make_ELSE(yytext, loc);
                                        }

"enum"                                  {
                                            driver.add_token(yytext);
                                            return CParser::make_ENUM(yytext, loc);
                                        }

"extern"                                {
                                            driver.add_token(yytext);
                                            return CParser::make_EXTERN(yytext, loc);
                                        }

"float"                                 {
                                            driver.add_token(yytext);
                                            return CParser::make_FLOAT(yytext, loc);
                                        }

"for"                                   {
                                            driver.add_token(yytext);
                                            return CParser::make_FOR(yytext, loc);
                                        }

"goto"                                  {
                                            driver.add_token(yytext);
                                            return CParser::make_GOTO(yytext, loc);
                                        }

"if"                                    {
                                            driver.add_token(yytext);
                                            return CParser::make_IF(yytext, loc);
                                        }

"inline"                                {
                                            driver.add_token(yytext);
                                            return CParser::make_INLINE(yytext, loc);
                                        }

"int"                                   {
                                            driver.add_token(yytext);
                                            return CParser::make_INT(yytext, loc);
                                        }

"long"                                  {
                                            driver.add_token(yytext);
                                            return CParser::make_LONG(yytext, loc);
                                        }

"register"                              {
                                            driver.add_token(yytext);
                                            return CParser::make_REGISTER(yytext, loc);
                                        }

"restrict"                              {
                                            driver.add_token(yytext);
                                            return CParser::make_RESTRICT(yytext, loc);
                                        }

"return"                                {
                                            driver.add_token(yytext);
                                            return CParser::make_RETURN(yytext, loc);
                                        }

"short"                                 {
                                            driver.add_token(yytext);
                                            return CParser::make_SHORT(yytext, loc);
                                        }

"signed"                                {
                                            driver.add_token(yytext);
                                            return CParser::make_SIGNED(yytext, loc);
                                        }

"sizeof"                                {
                                            driver.add_token(yytext);
                                            return CParser::make_SIZEOF(yytext, loc);
                                        }

"static"                                {
                                            driver.add_token(yytext);
                                            return CParser::make_STATIC(yytext, loc);
                                        }

"struct"                                {
                                            driver.add_token(yytext);
                                            return CParser::make_STRUCT(yytext, loc);
                                        }

"switch"                                {
                                            driver.add_token(yytext);
                                            return CParser::make_SWITCH(yytext, loc);
                                        }

"typedef"                               {
                                            driver.add_token(yytext);
                                            return CParser::make_TYPEDEF(yytext, loc);
                                        }

"union"                                 {
                                            driver.add_token(yytext);
                                            return CParser::make_UNION(yytext, loc);
                                        }

"unsigned"                              {
                                            driver.add_token(yytext);
                                            return CParser::make_UNSIGNED(yytext, loc);
                                        }

"void"                                  {
                                            driver.add_token(yytext);
                                            return CParser::make_VOID(yytext, loc);
                                        }

"volatile"                              {
                                            driver.add_token(yytext);
                                            return CParser::make_VOLATILE(yytext, loc);
                                        }

"while"                                 {
                                            driver.add_token(yytext);
                                            return CParser::make_WHILE(yytext, loc);
                                        }

"_Alignas"                              {
                                            driver.add_token(yytext);
                                            return CParser::make_ALIGNAS(yytext, loc);
                                        }

"_Alignof"                              {
                                            driver.add_token(yytext);
                                            return CParser::make_ALIGNOF(yytext, loc);
                                        }

"_Atomic"                               {
                                            driver.add_token(yytext);
                                            return CParser::make_ATOMIC(yytext, loc);
                                        }

"_Bool"                                 {
                                            driver.add_token(yytext);
                                            return CParser::make_BOOL(yytext, loc);
                                        }

"_Complex"                              {
                                            driver.add_token(yytext);
                                            return CParser::make_COMPLEX(yytext, loc);
                                        }

"_Generic"                              {
                                            driver.add_token(yytext);
                                            return CParser::make_GENERIC(yytext, loc);
                                        }

"_Imaginary"                            {
                                            driver.add_token(yytext);
                                            return CParser::make_IMAGINARY(yytext, loc);
                                        }

"_Noreturn"                             {
                                            driver.add_token(yytext);
                                            return CParser::make_NORETURN(yytext, loc);
                                        }

"_Static_assert"                        {
                                            driver.add_token(yytext);
                                            return CParser::make_STATIC_ASSERT(yytext, loc);
                                        }

"_Thread_local"                         {
                                            driver.add_token(yytext);
                                            return CParser::make_THREAD_LOCAL(yytext, loc);
                                        }

"__func__"                              {
                                            driver.add_token(yytext);
                                            return CParser::make_FUNC_NAME(yytext, loc);
                                        }

{L}{A}*                                 {
                                            driver.add_token(yytext);
                                            return get_token_type();
                                        }

{HP}{H}+{IS}?                           {
                                            driver.add_token(yytext);
                                            return CParser::make_I_CONSTANT(yytext, loc);
                                        }

{NZ}{D}*{IS}?                           {
                                            driver.add_token(yytext);
                                            return CParser::make_I_CONSTANT(yytext, loc);
                                        }

"0"{O}*{IS}?                            {
                                            driver.add_token(yytext);
                                            return CParser::make_I_CONSTANT(yytext, loc);
                                        }

{CP}?"'"([^'\\\n]|{ES})+"'"             {
                                            driver.add_token(yytext);
                                            return CParser::make_I_CONSTANT(yytext, loc);
                                        }

{D}+{E}{FS}?                            {
                                            driver.add_token(yytext);
                                            return CParser::make_F_CONSTANT(yytext, loc);
                                        }

{D}*"."{D}+{E}?{FS}?                    {
                                            driver.add_token(yytext);
                                            return CParser::make_F_CONSTANT(yytext, loc);
                                        }

{D}+"."{E}?{FS}?                        {
                                            driver.add_token(yytext);
                                            return CParser::make_F_CONSTANT(yytext, loc);
                                        }

{HP}{H}+{P}{FS}?                        {
                                            driver.add_token(yytext);
                                            return CParser::make_F_CONSTANT(yytext, loc);
                                        }

{HP}{H}*"."{H}+{P}{FS}?                 {
                                            driver.add_token(yytext);
                                            return CParser::make_F_CONSTANT(yytext, loc);
                                        }

{HP}{H}+"."{P}{FS}?                     {
                                            driver.add_token(yytext);
                                            return CParser::make_F_CONSTANT(yytext, loc);
                                        }

({SP}?\"([^"\\\n]|{ES})*\"{WS}*)+       {
                                            driver.add_token(yytext);
                                            return CParser::make_STRING_LITERAL(yytext, loc);
                                        }

"..."                                   {
                                            driver.add_token(yytext);
                                            return CParser::make_ELLIPSIS(yytext, loc);
                                        }

">>="                                   {
                                            driver.add_token(yytext);
                                            return CParser::make_RIGHT_ASSIGN(yytext, loc);
                                        }

"<<="                                   {
                                            driver.add_token(yytext);
                                            return CParser::make_LEFT_ASSIGN(yytext, loc);
                                        }

"+="                                    {
                                            driver.add_token(yytext);
                                            return CParser::make_ADD_ASSIGN(yytext, loc);
                                        }

"-="                                    {
                                            driver.add_token(yytext);
                                            return CParser::make_SUB_ASSIGN(yytext, loc);
                                        }

"*="                                    {
                                            driver.add_token(yytext);
                                            return CParser::make_MUL_ASSIGN(yytext, loc);
                                        }

"/="                                    {
                                            driver.add_token(yytext);
                                            return CParser::make_DIV_ASSIGN(yytext, loc);
                                        }

"%="                                    {
                                            driver.add_token(yytext);
                                            return CParser::make_MOD_ASSIGN(yytext, loc);
                                        }

"&="                                    {
                                            driver.add_token(yytext);
                                            return CParser::make_AND_ASSIGN(yytext, loc);
                                        }

"^="                                    {
                                            driver.add_token(yytext);
                                            return CParser::make_XOR_ASSIGN(yytext, loc);
                                        }

"|="                                    {
                                            driver.add_token(yytext);
                                            return CParser::make_OR_ASSIGN(yytext, loc);
                                        }

">>"                                    {
                                            driver.add_token(yytext);
                                            return CParser::make_RIGHT_OP(yytext, loc);
                                        }

"<<"                                    {
                                            driver.add_token(yytext);
                                            return CParser::make_LEFT_OP(yytext, loc);
                                        }

"++"                                    {
                                            driver.add_token(yytext);
                                            return CParser::make_INC_OP(yytext, loc);
                                        }

"--"                                    {
                                            driver.add_token(yytext);
                                            return CParser::make_DEC_OP(yytext, loc);
                                        }

"->"                                    {
                                            driver.add_token(yytext);
                                            return CParser::make_PTR_OP(yytext, loc);
                                        }

"&&"                                    {
                                            driver.add_token(yytext);
                                            return CParser::make_AND_OP(yytext, loc);
                                        }

"||"                                    {
                                            driver.add_token(yytext);
                                            return CParser::make_OR_OP(yytext, loc);
                                        }

"<="                                    {
                                            driver.add_token(yytext);
                                            return CParser::make_LE_OP(yytext, loc);
                                        }

">="                                    {
                                            driver.add_token(yytext);
                                            return CParser::make_GE_OP(yytext, loc);
                                        }

"=="                                    {
                                            driver.add_token(yytext);
                                            return CParser::make_EQ_OP(yytext, loc);
                                        }

"!="                                    {
                                            driver.add_token(yytext);
                                            return CParser::make_NE_OP(yytext, loc);
                                        }

";"                                     {
                                            driver.add_token(yytext);
                                            return CParser::make_SEMICOLON(yytext, loc);
                                        }

("{"|"<%")                              {
                                            driver.add_token(yytext);
                                            return CParser::make_OPEN_BRACE(yytext, loc);
                                        }

("}"|"%>")                              {
                                            driver.add_token(yytext);
                                            return CParser::make_CLOSE_BRACE(yytext, loc);
                                        }

","                                     {
                                            driver.add_token(yytext);
                                            return CParser::make_COMMA(yytext, loc);
                                        }

":"                                     {
                                            driver.add_token(yytext);
                                            return CParser::make_COLON(yytext, loc);
                                        }

"="                                     {
                                            driver.add_token(yytext);
                                            return CParser::make_ASSIGN(yytext, loc);
                                        }

"("                                     {
                                            driver.add_token(yytext);
                                            return CParser::make_OPEN_PARENTHESIS(yytext, loc);
                                        }

")"                                     {
                                            driver.add_token(yytext);
                                            return CParser::make_CLOSE_PARENTHESIS(yytext, loc);
                                        }

("["|"<:")                              {
                                            driver.add_token(yytext);
                                            return CParser::make_OPEN_BRACKET(yytext, loc);
                                        }

("]"|":>")                              {
                                            driver.add_token(yytext);
                                            return CParser::make_CLOSE_BRACKET(yytext, loc);
                                        }

"."                                     {
                                            driver.add_token(yytext);
                                            return CParser::make_PERIOD(yytext, loc);
                                        }

"&"                                     {
                                            driver.add_token(yytext);
                                            return CParser::make_AMPERSTAND(yytext, loc);
                                        }

"!"                                     {
                                            driver.add_token(yytext);
                                            return CParser::make_NEGATION(yytext, loc);
                                        }

"~"                                     {
                                            driver.add_token(yytext);
                                            return CParser::make_NEGATION(yytext, loc);
                                        }

"-"                                     {
                                            driver.add_token(yytext);
                                            return CParser::make_MINUS(yytext, loc);
                                        }

"+"                                     {
                                            driver.add_token(yytext);
                                            return CParser::make_ADD(yytext, loc);
                                        }

"*"                                     {
                                            driver.add_token(yytext);
                                            return CParser::make_MULTIPLY(yytext, loc);
                                        }

"/"                                     {
                                            driver.add_token(yytext);
                                            return CParser::make_DIVIDE(yytext, loc);
                                        }

"%"                                     {
                                            driver.add_token(yytext);
                                            return CParser::make_PERCENT(yytext, loc);
                                        }

"<"                                     {
                                            driver.add_token(yytext);
                                            return CParser::make_LT(yytext, loc);
                                        }

">"                                     {
                                            driver.add_token(yytext);
                                            return CParser::make_GT(yytext, loc);
                                        }

"^"                                     {
                                            driver.add_token(yytext);
                                            return CParser::make_CARET(yytext, loc);
                                        }

"|"                                     {
                                            driver.add_token(yytext);
                                            return CParser::make_OR(yytext, loc);
                                        }

"?"                                     {
                                            driver.add_token(yytext);
                                            return CParser::make_QUESTION(yytext, loc);
                                        }

{WS}+                                   {
                                            driver.add_token(yytext);
                                            std::string str(yytext);
                                            stringutils::remove_character(str, ' ');
                                            if (str == "\n") {
                                                loc.lines(1);
                                            } else {
                                                loc.step();
                                            }
                                        }

.                                       {
                                            /* if nothing match, scan more characters ? */
                                            yymore();
                                        }

%%


int CFlexLexer::yylex() {
  throw std::runtime_error("next_token() should be used instead of yylex()");
}


nmodl::parser::CParser::symbol_type nmodl::parser::CLexer::get_token_type() {
    if (driver.is_typedef(yytext)) {
        return CParser::make_TYPEDEF_NAME(yytext, loc);
    }

    if (driver.is_enum_constant(yytext)) {
        return CParser::make_ENUMERATION_CONSTANT(yytext, loc);
    }

    return CParser::make_IDENTIFIER(yytext, loc);
}
