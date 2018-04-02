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
    #define YY_USER_ACTION { loc.step(); loc.columns(yyleng); driver.process(yytext); }

   /** By default yylex returns int, we use token_type. Unfortunately
     * yyterminate by default returns 0, which is not of token_type. */
   #define yyterminate() return c11::Parser::make_END(loc);

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

/** change the name of the scanner class (to "C11FlexLexer") */
%option prefix="C11"

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


^"#"                                    {
                                            BEGIN(P_P_DIRECTIVE);
                                        }

<P_P_DIRECTIVE>.                        {
                                        }

<P_P_DIRECTIVE>\n                       {
                                            BEGIN(INITIAL);
                                        }

"/*"                                    {
                                            BEGIN(COMMENT);
                                        }

<COMMENT>"*/"                           {
                                            BEGIN(INITIAL);
                                        }

<COMMENT>.                              {
                                        }

<COMMENT>\n                             {
                                            loc.lines(1);
                                        }

"//".*                                  {
                                            /* consume //-comment */
                                        }

"auto"                                  {
                                            return c11::Parser::make_AUTO(yytext, loc);
                                        }

"break"                                 {
                                            return c11::Parser::make_BREAK(yytext, loc);
                                        }

"case"                                  {
                                            return c11::Parser::make_CASE(yytext, loc);
                                        }

"char"                                  {
                                            return c11::Parser::make_CHAR(yytext, loc);
                                        }

"const"                                 {
                                            return c11::Parser::make_CONST(yytext, loc);
                                        }

"continue"                              {
                                            return c11::Parser::make_CONTINUE(yytext, loc);
                                        }

"default"                               {
                                            return c11::Parser::make_DEFAULT(yytext, loc);
                                        }

"do"                                    {
                                            return c11::Parser::make_DO(yytext, loc);
                                        }

"double"                                {
                                            return c11::Parser::make_DOUBLE(yytext, loc);
                                        }

"else"                                  {
                                            return c11::Parser::make_ELSE(yytext, loc);
                                        }

"enum"                                  {
                                            return c11::Parser::make_ENUM(yytext, loc);
                                        }

"extern"                                {
                                            return c11::Parser::make_EXTERN(yytext, loc);
                                        }

"float"                                 {
                                            return c11::Parser::make_FLOAT(yytext, loc);
                                        }

"for"                                   {
                                            return c11::Parser::make_FOR(yytext, loc);
                                        }

"goto"                                  {
                                            return c11::Parser::make_GOTO(yytext, loc);
                                        }

"if"                                    {
                                            return c11::Parser::make_IF(yytext, loc);
                                        }

"inline"                                {
                                            return c11::Parser::make_INLINE(yytext, loc);
                                        }

"int"                                   {
                                            return c11::Parser::make_INT(yytext, loc);
                                        }

"long"                                  {
                                            return c11::Parser::make_LONG(yytext, loc);
                                        }

"register"                              {
                                            return c11::Parser::make_REGISTER(yytext, loc);
                                        }

"restrict"                              {
                                            return c11::Parser::make_RESTRICT(yytext, loc);
                                        }

"return"                                {
                                            return c11::Parser::make_RETURN(yytext, loc);
                                        }

"short"                                 {
                                            return c11::Parser::make_SHORT(yytext, loc);
                                        }

"signed"                                {
                                            return c11::Parser::make_SIGNED(yytext, loc);
                                        }

"sizeof"                                {
                                            return c11::Parser::make_SIZEOF(yytext, loc);
                                        }

"static"                                {
                                            return c11::Parser::make_STATIC(yytext, loc);
                                        }

"struct"                                {
                                            return c11::Parser::make_STRUCT(yytext, loc);
                                        }

"switch"                                {
                                            return c11::Parser::make_SWITCH(yytext, loc);
                                        }

"typedef"                               {
                                            return c11::Parser::make_TYPEDEF(yytext, loc);
                                        }

"union"                                 {
                                            return c11::Parser::make_UNION(yytext, loc);
                                        }

"unsigned"                              {
                                            return c11::Parser::make_UNSIGNED(yytext, loc);
                                        }

"void"                                  {
                                            return c11::Parser::make_VOID(yytext, loc);
                                        }

"volatile"                              {
                                            return c11::Parser::make_VOLATILE(yytext, loc);
                                        }

"while"                                 {
                                            return c11::Parser::make_WHILE(yytext, loc);
                                        }

"_Alignas"                              {
                                            return c11::Parser::make_ALIGNAS(yytext, loc);
                                        }

"_Alignof"                              {
                                            return c11::Parser::make_ALIGNOF(yytext, loc);
                                        }

"_Atomic"                               {
                                            return c11::Parser::make_ATOMIC(yytext, loc);
                                        }

"_Bool"                                 {
                                            return c11::Parser::make_BOOL(yytext, loc);
                                        }

"_Complex"                              {
                                            return c11::Parser::make_COMPLEX(yytext, loc);
                                        }

"_Generic"                              {
                                            return c11::Parser::make_GENERIC(yytext, loc);
                                        }

"_Imaginary"                            {
                                            return c11::Parser::make_IMAGINARY(yytext, loc);
                                        }

"_Noreturn"                             {
                                            return c11::Parser::make_NORETURN(yytext, loc);
                                        }

"_Static_assert"                        {
                                            return c11::Parser::make_STATIC_ASSERT(yytext, loc);
                                        }

"_Thread_local"                         {
                                            return c11::Parser::make_THREAD_LOCAL(yytext, loc);
                                        }

"__func__"                              {
                                            return c11::Parser::make_FUNC_NAME(yytext, loc);
                                        }

{L}{A}*                                 {
                                            return check_type();
                                        }

{HP}{H}+{IS}?                           {
                                            return c11::Parser::make_I_CONSTANT(yytext, loc);
                                        }

{NZ}{D}*{IS}?                           {
                                            return c11::Parser::make_I_CONSTANT(yytext, loc);
                                        }

"0"{O}*{IS}?                            {
                                            return c11::Parser::make_I_CONSTANT(yytext, loc);
                                        }

{CP}?"'"([^'\\\n]|{ES})+"'"             {
                                            return c11::Parser::make_I_CONSTANT(yytext, loc);
                                        }

{D}+{E}{FS}?                            {
                                            return c11::Parser::make_F_CONSTANT(yytext, loc);
                                        }

{D}*"."{D}+{E}?{FS}?                    {
                                            return c11::Parser::make_F_CONSTANT(yytext, loc);
                                        }

{D}+"."{E}?{FS}?                        {
                                            return c11::Parser::make_F_CONSTANT(yytext, loc);
                                        }

{HP}{H}+{P}{FS}?                        {
                                            return c11::Parser::make_F_CONSTANT(yytext, loc);
                                        }

{HP}{H}*"."{H}+{P}{FS}?                 {
                                            return c11::Parser::make_F_CONSTANT(yytext, loc);
                                        }

{HP}{H}+"."{P}{FS}?                     {
                                            return c11::Parser::make_F_CONSTANT(yytext, loc);
                                        }

({SP}?\"([^"\\\n]|{ES})*\"{WS}*)+       {
                                            return c11::Parser::make_STRING_LITERAL(yytext, loc);
                                        }

"..."                                   {
                                            return c11::Parser::make_ELLIPSIS(yytext, loc);
                                        }

">>="                                   {
                                            return c11::Parser::make_RIGHT_ASSIGN(yytext, loc);
                                        }

"<<="                                   {
                                            return c11::Parser::make_LEFT_ASSIGN(yytext, loc);
                                        }

"+="                                    {
                                            return c11::Parser::make_ADD_ASSIGN(yytext, loc);
                                        }

"-="                                    {
                                            return c11::Parser::make_SUB_ASSIGN(yytext, loc);
                                        }

"*="                                    {
                                            return c11::Parser::make_MUL_ASSIGN(yytext, loc);
                                        }

"/="                                    {
                                            return c11::Parser::make_DIV_ASSIGN(yytext, loc);
                                        }

"%="                                    {
                                            return c11::Parser::make_MOD_ASSIGN(yytext, loc);
                                        }

"&="                                    {
                                            return c11::Parser::make_AND_ASSIGN(yytext, loc);
                                        }

"^="                                    {
                                            return c11::Parser::make_XOR_ASSIGN(yytext, loc);
                                        }

"|="                                    {
                                            return c11::Parser::make_OR_ASSIGN(yytext, loc);
                                        }

">>"                                    {
                                            return c11::Parser::make_RIGHT_OP(yytext, loc);
                                        }

"<<"                                    {
                                            return c11::Parser::make_LEFT_OP(yytext, loc);
                                        }

"++"                                    {
                                            return c11::Parser::make_INC_OP(yytext, loc);
                                        }

"--"                                    {
                                            return c11::Parser::make_DEC_OP(yytext, loc);
                                        }

"->"                                    {
                                            return c11::Parser::make_PTR_OP(yytext, loc);
                                        }

"&&"                                    {
                                            return c11::Parser::make_AND_OP(yytext, loc);
                                        }

"||"                                    {
                                            return c11::Parser::make_OR_OP(yytext, loc);
                                        }

"<="                                    {
                                            return c11::Parser::make_LE_OP(yytext, loc);
                                        }

">="                                    {
                                            return c11::Parser::make_GE_OP(yytext, loc);
                                        }

"=="                                    {
                                            return c11::Parser::make_EQ_OP(yytext, loc);
                                        }

"!="                                    {
                                            return c11::Parser::make_NE_OP(yytext, loc);
                                        }

";"                                     {
                                            return c11::Parser::make_SEMICOLON(yytext, loc);
                                        }

("{"|"<%")                              {
                                            return c11::Parser::make_OPEN_BRACE(yytext, loc);
                                        }

("}"|"%>")                              {
                                            return c11::Parser::make_CLOSE_BRACE(yytext, loc);
                                        }

","                                     {
                                            return c11::Parser::make_COMMA(yytext, loc);
                                        }

":"                                     {
                                            return c11::Parser::make_COLON(yytext, loc);
                                        }

"="                                     {
                                            return c11::Parser::make_ASSIGN(yytext, loc);
                                        }

"("                                     {
                                            return c11::Parser::make_OPEN_PARENTHESIS(yytext, loc);
                                        }

")"                                     {
                                            return c11::Parser::make_CLOSE_PARENTHESIS(yytext, loc);
                                        }

("["|"<:")                              {
                                            return c11::Parser::make_OPEN_BRACKET(yytext, loc);
                                        }

("]"|":>")                              {
                                            return c11::Parser::make_CLOSE_BRACKET(yytext, loc);
                                        }

"."                                     {
                                            return c11::Parser::make_PERIOD(yytext, loc);
                                        }

"&"                                     {
                                            return c11::Parser::make_AMPERSTAND(yytext, loc);
                                        }

"!"                                     {
                                            return c11::Parser::make_NEGATION(yytext, loc);
                                        }

"~"                                     {
                                            return c11::Parser::make_NEGATION(yytext, loc);
                                        }

"-"                                     {
                                            return c11::Parser::make_MINUS(yytext, loc);
                                        }

"+"                                     {
                                            return c11::Parser::make_ADD(yytext, loc);
                                        }

"*"                                     {
                                            return c11::Parser::make_MULTIPLY(yytext, loc);
                                        }

"/"                                     {
                                            return c11::Parser::make_DIVIDE(yytext, loc);
                                        }

"%"                                     {
                                            return c11::Parser::make_PERCENT(yytext, loc);
                                        }

"<"                                     {
                                            return c11::Parser::make_LT(yytext, loc);
                                        }

">"                                     {
                                            return c11::Parser::make_GT(yytext, loc);
                                        }

"^"                                     {
                                            return c11::Parser::make_CARET(yytext, loc);
                                        }

"|"                                     {
                                            return c11::Parser::make_OR(yytext, loc);
                                        }

"?"                                     {
                                            return c11::Parser::make_QUESTION(yytext, loc);
                                        }

{WS}+                                   {
                                            std::string str(yytext);
                                            stringutils::remove_character(str, ' ');
                                            if (str == "\n") {
                                                loc.lines(1);
                                            } else {
                                                loc.step();
                                            }
                                        }

.                                       {
                                            /* discard bad characters */
                                            yymore();
                                        }

%%


int C11FlexLexer::yylex() {
  throw std::runtime_error("next_token() should be used instead of yylex()");
}


c11::Parser::symbol_type c11::Lexer::check_type() {
    if (driver.is_typedef(yytext)) {
        return c11::Parser::make_TYPEDEF_NAME(yytext, loc);
    }

    if (driver.is_enum_constant(yytext)) {
        return c11::Parser::make_ENUMERATION_CONSTANT(yytext, loc);
    }

    return c11::Parser::make_IDENTIFIER(yytext, loc);
}
