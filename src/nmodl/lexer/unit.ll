/**********************************************************************************
 *
 * @brief Flex lexer implementation reading Units of Neuron like MOD2C
 *
 * @brief Parsing unit definition file for use in NMODL
 *        https://github.com/BlueBrain/nmodl/blob/master/share/nrnunits.lib.in
 *****************************************************************************/

%{
    #include <iostream>

    #include "lexer/unit_lexer.hpp"
    #include "parser/unit_driver.hpp"
    #include "parser/unit/unit_parser.hpp"
    #include "utils/string_utils.hpp"

    /**
      * YY_USER_ACTION is called before each of token actions and
      * we update columns by length of the token. Node that position
      * starts at column 1 and increasing by yyleng gives extra position
      * larger than exact columns span of the token. Hence in ModToken
      * class we reduce the length by one column.
      */
    #define YY_USER_ACTION { loc.step(); loc.columns(yyleng); }

   /**
     * By default yylex returns int, we use token_type. Unfortunately
     * yyterminate by default returns 0, which is not of token_type.
     */
   #define yyterminate() return UnitParser::make_END(loc);

   /**
     * Disables inclusion of unistd.h, which is not available under Visual
     * C++ on Win32. The C++ scanner uses STL streams instead.
     */
   #define YY_NO_UNISTD_H

%}

/** regex for digits, exponents and double numbers */
D   [0-9]
E   [Ee]*[-+]?{D}+
DBL ([-+]?{D})|([-+]?{D}+"."{D}*({E})?)|([-+]?{D}*"."{D}+({E})?)|([-+]?{D}+{E})

/**
  * regex for valid base units names and invalid base units names
  * (maximum number of base units should be 10 and named from a to j)
  */
VALIDBASEUNIT      [a-j]
INVALIDBASEUNIT    [k-zA-Z]

/** regex for valid characters of units */
CHAR    [a-zA-Z$\%]

/** regex for name of new defined unit */
NEWUNIT ({CHAR}+{D}*)|({CHAR}+{D}*"_"{CHAR}+{D}*)|({CHAR}+{D}*"/"{CHAR}+{D}*)|({CHAR}+{D}*"-"{CHAR}*{D}+)

/** we do use yymore feature in copy modes */
%option yymore

/** name of the lexer header file */
%option header-file="unit_base_lexer.hpp"

/** name of the lexer implementation file */
%option outfile="unit_base_lexer.cpp"

/** change the name of the scanner class (to "UnitFlexLexer") */
%option prefix="Unit"

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

/**
  * instructs flex to generate an 8-bit scanner, i.e.,
  * one which can recognize 8-bit characters.
  */
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

"*"{VALIDBASEUNIT}"*"     {
                    return UnitParser::make_BASE_UNIT(yytext, loc);
                }

"*"{INVALIDBASEUNIT}"*"    {
                    return UnitParser::make_INVALID_BASE_UNIT(yytext, loc);
                }

^{NEWUNIT}      {
                    return UnitParser::make_NEW_UNIT(yytext, loc);
                }

{CHAR}+         {
                    return UnitParser::make_UNIT(yytext, loc);
                }

^[a-zA-Z]+"-"   {
                    return UnitParser::make_PREFIX(yytext, loc);
                }

[a-zA-Z]+[2-9]+ {
                    return UnitParser::make_UNIT_POWER(yytext, loc);
                }

{DBL}           {
                    return UnitParser::make_DOUBLE(yytext, loc);
                }

"|"             {
                    return UnitParser::make_FRACTION(yytext, loc);
                }

"-"             {
                    return UnitParser::make_UNIT_PROD(yytext, loc);
                }

^"/".*          {
                    return UnitParser::make_COMMENT(yytext, loc);
                }

"/"|"1/"        {
                    return UnitParser::make_DIVISION(yytext, loc);
                }

[ \t]           {
                    // do nothing
                }

\n              {
                    return UnitParser::make_NEWLINE(yytext, loc);
                }

.               {
                    return UnitParser::make_INVALID_TOKEN(loc);
                }

%%


int UnitFlexLexer::yylex() {
  throw std::runtime_error("next_token() should be used instead of yylex()");
}

