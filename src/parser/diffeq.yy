/*************************************************************************
 * Copyright (C) 2018-2022 Blue Brain Project
 * Copyright (C) 2018-2022 Michael Hines
 *
 * This file is part of NMODL distributed under the terms of the GNU
 * Lesser General Public License. See top-level LICENSE file for details.
 *************************************************************************/

/**********************************************************************************
 *
 * @brief Bison grammar and parser implementation for ODEs
 *
 * ODEs specified in DERIVATIVE block needs to be solved during code generation.
 * This parser implementation is based on original NEURON's nmodl implementation
 * and solves ODEs using helper routines provided.
 *****************************************************************************/

 %code requires
 {
    #include <string>
    #include "parser/diffeq_context.hpp"
    #include "parser/diffeq_driver.hpp"
    #include "parser/diffeq_helper.hpp"
    #include "parser/nmodl/location.hh"
}

/** use C++ parser interface of bison */
%skeleton "lalr1.cc"

/** require modern bison version */
%require  "3.0.2"

/** print verbose error messages instead of just message 'syntax error' */
%define parse.error verbose

/** enable tracing parser for debugging */
%define parse.trace

/** add extra arguments to yyparse() and yylexe() methods */
%parse-param {class DiffeqLexer& scanner}
%parse-param {diffeq::DiffEqContext& eq_context}
%lex-param   {diffeq::DiffEqScanner &scanner }

/** use variant based implementation of semantic values */
%define api.value.type variant

/** assert correct cleanup of semantic value objects */
%define parse.assert

/** handle symbol to be handled as a whole (type, value, and possibly location) in scanner */
%define api.token.constructor

/** specify the namespace for the parser class */
%define api.namespace {nmodl::parser}

/** set the parser's class identifier */
%define parser_class_name {DiffeqParser}

/** keep track of the current position within the input */
%locations

/** specify own location class */
%define api.location.type {nmodl::parser::location}

%token  <std::string>   ATOM
%token  <std::string>   NEWLINE
%token  <std::string>   ADD                 "+"
%token  <std::string>   MUL                 "*"
%token  <std::string>   SUB                 "-"
%token  <std::string>   DIV                 "/"
%token  <std::string>   OPEN_PARENTHESIS    "("
%token  <std::string>   CLOSE_PARENTHESIS   ")"
%token  <std::string>   COMMA               ","
%token  END             0


%type   <std::string>   top
%type   <diffeq::Term>          expression e arglist arg


/* operator associativity */
%left "+" "-"
%left "*" "/"
%left UNARYMINUS


%{
    #include <sstream>
    #include "lexer/diffeq_lexer.hpp"

    using nmodl::parser::DiffeqParser;
    using nmodl::parser::DiffeqLexer;
    using nmodl::parser::DiffeqDriver;
    using nmodl::parser::diffeq::eval_derivative;
    using nmodl::parser::diffeq::MathOp;
    using nmodl::parser::diffeq::Term;

    static DiffeqParser::symbol_type yylex(DiffeqLexer &scanner) {
        return scanner.next_token();
    }
%}


%start top


%%

/**
 *
 *  \todo We pass the differential equation context to the parser to keep track of current
 *  solution and and also some global states like equation/derivative valid or not. We could
 *  add extra properties in diffeq::Term class and avoid passing context to parser class. Need to
 *  check the implementation of other solvers in neuron before making changes.
 */

top             : expression                {
                                                eq_context.solution.b = "-(" + eq_context.solution.b + ")/(" + eq_context.solution.a + ")";
                                            }
                | error                     {
                                                throw std::runtime_error("Invalid differential equation");
                                            }
                ;


expression      : e                         { $$ = $1; }

                | expression NEWLINE        { $$ = $1; }
                ;



e               : ATOM                      {
                                                eq_context.solution = Term($1, eq_context.state_variable());
                                                $$ = eq_context.solution;
                                            }

                | "(" e ")"                 {
                                                eq_context.solution = Term();
                                                eq_context.solution.expr = "(" + $2.expr + ")";

                                                if($2.deriv_nonzero())
                                                    eq_context.solution.deriv = "(" + $2.deriv + ")";
                                                if($2.a_nonzero())
                                                    eq_context.solution.a = "(" + $2.a + ")";
                                                if($2.b_nonzero())
                                                    eq_context.solution.b = "(" + $2.b + ")";

                                                $$ = eq_context.solution;
                                            }

                | ATOM "(" arglist ")"      {
                                                eq_context.solution = Term();
                                                eq_context.solution.expr = $1 +  "(" + $3.expr + ")";
                                                eq_context.solution.b = $1 + "(" + $3.expr + ")";
                                                $$ = eq_context.solution;
                                            }

                | "-" e %prec UNARYMINUS    {
                                                eq_context.solution = Term();
                                                eq_context.solution.expr = "-" + $2.expr;

                                                if($2.deriv_nonzero())
                                                    eq_context.solution.deriv = "-" + $2.deriv;
                                                if($2.a_nonzero())
                                                    eq_context.solution.a = "-" + $2.a;
                                                if($2.b_nonzero())
                                                    eq_context.solution.b = "-" +$2.b;

                                                $$ = eq_context.solution;
                                            }

                | e "+" e                   {
                                                eq_context.solution = eval_derivative<MathOp::add>($1, $3, eq_context.deriv_invalid, eq_context.eqn_invalid);
                                                $$ = eq_context.solution;
                                            }

                | e "-" e                   {
                                                eq_context.solution = eval_derivative<MathOp::sub>($1, $3, eq_context.deriv_invalid, eq_context.eqn_invalid);
                                                $$ = eq_context.solution;
                                            }

                | e "*" e                   {
                                                eq_context.solution = eval_derivative<MathOp::mul>($1, $3, eq_context.deriv_invalid, eq_context.eqn_invalid);
                                                $$ = eq_context.solution;
                }

                | e "/" e                   {
                                                eq_context.solution = eval_derivative<MathOp::div>($1, $3, eq_context.deriv_invalid, eq_context.eqn_invalid);
                                                $$ = eq_context.solution;

                                            }
                ;



arglist         : /*nothing*/               { $$ = Term("", "0.0", "0.0", ""); }

                | arg                       { $$ = $1; }

                | arglist arg               {
                                                eq_context.solution.expr = $1.expr + " " + $2.expr;
                                                eq_context.solution.b    = $1.expr + " " + $2.expr;
                                                $$ = eq_context.solution;
                                            }

                | arglist "," arg           {
                                                eq_context.solution.expr = $1.expr + "," + $3.expr;
                                                eq_context.solution.b    = $1.expr + "," + $3.expr;
                                                $$ = eq_context.solution;
                                            }
                ;



arg             : e                         {
                                                eq_context.solution = Term();
                                                eq_context.solution.expr = $1.expr;
                                                eq_context.solution.b = $1.expr;
                                                if($1.deriv_nonzero()) {
                                                    eq_context.deriv_invalid = true;
                                                    eq_context.eqn_invalid = true;
                                                }
                                                $$ = eq_context.solution;
                                            }
                ;

%%


void DiffeqParser::error(const location &loc , const std::string &msg) {
    std::stringstream ss;
    ss << "DiffEq Parser Error : " << msg << " [Location : " << loc << "]";
    throw std::runtime_error(ss.str());
}
