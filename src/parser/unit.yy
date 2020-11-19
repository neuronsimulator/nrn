/**********************************************************************************
 *
 * @brief Bison grammar and parser implementation for Units of Neuron like MOD2C
 *
 * Parsing Unit definition file for use in NMODL.
 *
 * The parser is designed to parse the nrnunits.lib file which is also used by MOD2C
 * https://github.com/BlueBrain/nmodl/blob/master/share/nrnunits.lib.in
 *****************************************************************************/

%code requires
{
    #include "parser/nmodl/location.hh"
    #include "parser/unit_driver.hpp"
    #include "units/units.hpp"
}

/** use C++ parser interface of bison */
%skeleton "lalr1.cc"

/** require modern bison version */
%require  "3.0.2"

/** print verbose error messages instead of just message 'syntax error' */
%define parse.error verbose

/** enable tracing parser for debugging */
%define parse.trace

/** add extra arguments to yyparse() and yylex() methods */
%parse-param {class UnitLexer& scanner}
%parse-param {class UnitDriver& driver}
%lex-param   {nmodl::UnitScanner &scanner}
%lex-param   {nmodl::UnitDriver &driver}

/** use variant based implementation of semantic values */
%define api.value.type variant

/** assert correct cleanup of semantic value objects */
%define parse.assert

/** handle symbol to be handled as a whole (type, value, and possibly location) in scanner */
%define api.token.constructor

/** specify the namespace for the parser class */
%define api.namespace {nmodl::parser}

/** set the parser's class identifier */
%define parser_class_name {UnitParser}

/** keep track of the current position within the input */
%locations

/** specify own location class */
%define api.location.type {nmodl::parser::location}

%token                  END    0     "end of file"
%token                  INVALID_TOKEN

%token <std::string>    BASE_UNIT
%token <std::string>    INVALID_BASE_UNIT
%token <std::string>    UNIT
%token <std::string>    NEW_UNIT
%token <std::string>    UNIT_POWER
%token <std::string>    PREFIX
%token <std::string>    DOUBLE
%token <std::string>    FRACTION
%token <std::string>    COMMENT
%token <std::string>    NEWLINE
%token <std::string>    UNIT_PROD
%token <std::string>    DIVISION

%type <std::shared_ptr<std::vector<std::string>>>    units_nom
%type <std::shared_ptr<std::vector<std::string>>>    units_denom
%type <std::shared_ptr<nmodl::units::Unit>>          nominator
%type <std::shared_ptr<nmodl::units::Unit>>          item
%type <std::shared_ptr<nmodl::units::Prefix>>        prefix
%type <std::shared_ptr<nmodl::units::UnitTable>>     table_insertion

%{
    #include "lexer/unit_lexer.hpp"
    #include "parser/unit_driver.hpp"

    using nmodl::parser::UnitParser;
    using nmodl::parser::UnitLexer;
    using nmodl::parser::UnitDriver;

    /// yylex takes scanner as well as driver reference
    /// \todo Check if driver argument is required
    static UnitParser::symbol_type yylex(UnitLexer &scanner, UnitDriver &/*driver*/) {
        return scanner.next_token();
    }
%}


%start unit_table

%%

unit_table
    : END
    | table_insertion END

table_insertion
    : {
        $$ = driver.table;
      }
    | table_insertion item  {
        $1->insert($2);
        $$ = $1;
      }
    | table_insertion prefix  {
        $1->insert_prefix($2);
        $$ = $1;
      }
    | table_insertion no_insert {
        $$ = $1;
      }
    ;

units_nom
    : {
        $$ = std::make_shared<std::vector<std::string>>();
      }
    | UNIT units_nom {
        $2->push_back($1);
        $$ = $2;
      }
    | UNIT_POWER units_nom {
        $2->push_back($1);
        $$ = $2;
      }
    | UNIT_PROD units_nom {
        $$ = $2;
      }
    ;

units_denom
    : {
        $$ = std::make_shared<std::vector<std::string>>();
      }
    | UNIT units_denom {
        $2->push_back($1);
        $$ = $2;
      }
    | UNIT_POWER units_denom {
        $2->push_back($1);
        $$ = $2;
      }
    | UNIT_PROD units_denom {
        $$ = $2;
      }
    ;

nominator
    : units_nom {
        auto newunit = std::make_shared<nmodl::units::Unit>();
        newunit->add_nominator_unit($1);
        $$ = newunit;
      }
    | DOUBLE units_nom {
        auto newunit = std::make_shared<nmodl::units::Unit>();
        newunit->add_nominator_unit($2);
        newunit->add_nominator_double($1);
        $$ = newunit;
      }
    | FRACTION units_nom {
        auto newunit = std::make_shared<nmodl::units::Unit>();
        newunit->add_nominator_unit($2);
        newunit->add_fraction($1);
        $$ = newunit;
      }
    ;

prefix
    : PREFIX DOUBLE {
        $$ = std::make_shared<nmodl::units::Prefix>($1,$2);
      }
    | PREFIX UNIT {
        $$ = std::make_shared<nmodl::units::Prefix>($1,$2);
      }

no_insert
    : COMMENT
    | NEWLINE
    | INVALID_TOKEN {
        error(scanner.loc, "item");
      }
item
    : NEW_UNIT BASE_UNIT {
        auto newunit = std::make_shared<nmodl::units::Unit>($1);
        newunit->add_base_unit($2);
        $$ = newunit;
      }
    | NEW_UNIT nominator {
        $2->add_unit($1);
        $$ = $2;
      }
    | NEW_UNIT nominator DIVISION units_denom {
        $2->add_unit($1);
        $2->add_denominator_unit($4);
        $$ = $2;
      }
    | NEW_UNIT nominator DIVISION DOUBLE {
          $2->add_unit($1);
          $2->mul_factor(1/std::stod($4));
          $2->add_denominator_unit($4);
          $$ = $2;
        }
    | NEW_UNIT INVALID_BASE_UNIT {
            error(scanner.loc, "Base units should be named by characters a-j");
          }
    ;

%%


/** Bison expects error handler for parser */

void UnitParser::error(const location &loc , const std::string &msg) {
    std::stringstream ss;
    ss << "Unit Parser Error : " << msg << " [Location : " << loc << "]";
    throw std::runtime_error(ss.str());
}
