/*************************************************************************
 * Copyright (C) 2018-2019 Blue Brain Project
 * Copyright (C) 2018-2019 Michael Hines
 *
 * This file is part of NMODL distributed under the terms of the GNU
 * Lesser General Public License. See top-level LICENSE file for details.
 *************************************************************************/

/**********************************************************************************
 *
 * @brief Bison grammar and parser implementation for NMODL
 *
 * This implementation is based NEURON's nmodl program. The grammar rules,
 * symbols, terminals and non-terminals closely resember to NEURON
 * implementation. This is to support entire NMODL language for NEURON as well
 * as CoreNEURON. As opposed to non-reentrant C parser, this implementation
 * uses C++ interface of flex and bison. This makes lexer/parser fully
 * reentrant.
 * One of the implementation decision was whether to construct/return smart
 * pointers from parser or let ast class constructor to handle creation of
 * smart pointers from raw one. After trying shared_ptr and unique_ptr
 * implementations within bison parset, the later approach seems more
 * convenient and flexible while working with large number of grammar rules.
 * But this could be change once ast specification and compiler passes will
 * be implemented.
 *****************************************************************************/

/** to include parser's header file, one has to include ast definitions */
 %code requires
 {
    #include "ast/ast.hpp"
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
%parse-param {class NmodlLexer& scanner}
%parse-param {class NmodlDriver& driver}
%lex-param { nmodl::NmodlScanner &scanner }
%lex-param { nmodl::NmodlDriver &driver }

/** use variant based implementation of semantic values */
%define api.value.type variant

/** assert correct cleanup of semantic value objects */
%define parse.assert

/** handle symbol to be handled as a whole (type, value, and possibly location) in scanner */
%define api.token.constructor

/** specify the namespace for the parser class */
%define api.namespace {nmodl::parser}

/** set the parser's class identifier */
%define parser_class_name {NmodlParser}

/** keep track of the current position within the input */
%locations

/** Initializations before parsing : Use filename from driver to initialize location object */
%initial-action
{
    @$.begin.filename = @$.end.filename = &driver.stream_name;
};

/** Tokens for lexer : we return ModToken object from lexer. This is useful when
 *  we want to access original string and location. With C++ interface and other
 *  location related information, this is now less useful in parser. But when we
 *  lexer executable or tests, it's useful to return ModToken. Note that UNKNOWN
 *  token is added for convenience (with default arguments). */

%token  <ModToken>              MODEL
%token  <ModToken>              CONSTANT
%token  <ModToken>              INDEPENDENT
%token  <ModToken>              DEPENDENT
%token  <ModToken>              STATE
%token  <ModToken>              INITIAL1
%token  <ModToken>              DERIVATIVE
%token  <ModToken>              SOLVE
%token  <ModToken>              USING
%token  <ModToken>              STEADYSTATE
%token  <ModToken>              WITH
%token  <ModToken>              STEPPED
%token  <ModToken>              DISCRETE
%token  <ModToken>              FROM
%token  <ModToken>              FORALL1
%token  <ModToken>              TO
%token  <ModToken>              BY
%token  <ModToken>              WHILE
%token  <ModToken>              IF
%token  <ModToken>              ELSE
%token  <ModToken>              START1
%token  <ModToken>              STEP
%token  <ModToken>              SENS
%token  <ModToken>              SOLVEFOR
%token  <ModToken>              PROCEDURE
%token  <ModToken>              PARTIAL
%token  <ModToken>              DEFINE1
%token  <ModToken>              IFERROR
%token  <ModToken>              PARAMETER
%token  <ModToken>              DERFUNC
%token  <ModToken>              EQUATION
%token  <ModToken>              TERMINAL
%token  <ModToken>              LINEAR
%token  <ModToken>              NONLINEAR
%token  <ModToken>              FUNCTION1
%token  <ModToken>              LOCAL
%token  <ModToken>              LIN1
%token  <ModToken>              NONLIN1
%token  <ModToken>              PUTQ
%token  <ModToken>              GETQ
%token  <ModToken>              TABLE
%token  <ModToken>              DEPEND
%token  <ModToken>              BREAKPOINT
%token  <ModToken>              INCLUDE1
%token  <ModToken>              FUNCTION_TABLE
%token  <ModToken>              PROTECT
%token  <ModToken>              NRNMUTEXLOCK
%token  <ModToken>              NRNMUTEXUNLOCK
%token  <ModToken>              OR
%token  <ModToken>              AND
%token  <ModToken>              GT
%token  <ModToken>              LT
%token  <ModToken>              LE
%token  <ModToken>              EQ
%token  <ModToken>              NE
%token  <ModToken>              NOT
%token  <ModToken>              GE
%token  <ModToken>              PLOT
%token  <ModToken>              VS
%token  <ModToken>              LAG
%token  <ModToken>              RESET
%token  <ModToken>              MATCH
%token  <ModToken>              MODEL_LEVEL
%token  <ModToken>              SWEEP
%token  <ModToken>              FIRST
%token  <ModToken>              LAST
%token  <ModToken>              KINETIC
%token  <ModToken>              CONSERVE
%token  <ModToken>              REACTION
%token  <ModToken>              REACT1
%token  <ModToken>              COMPARTMENT
%token  <ModToken>              UNITS
%token  <ModToken>              UNITSON
%token  <ModToken>              UNITSOFF
%token  <ModToken>              LONGDIFUS
%token  <ModToken>              NEURON
%token  <ModToken>              NONSPECIFIC
%token  <ModToken>              READ
%token  <ModToken>              WRITE
%token  <ModToken>              USEION
%token  <ModToken>              THREADSAFE
%token  <ModToken>              GLOBAL
%token  <ModToken>              SECTION
%token  <ModToken>              RANGE POINTER
%token  <ModToken>              BBCOREPOINTER
%token  <ModToken>              EXTERNAL
%token  <ModToken>              BEFORE
%token  <ModToken>              AFTER
%token  <ModToken>              WATCH
%token  <ModToken>              ELECTRODE_CURRENT
%token  <ModToken>              CONSTRUCTOR
%token  <ModToken>              DESTRUCTOR
%token  <ModToken>              NETRECEIVE
%token  <ModToken>              FOR_NETCONS
%token  <ModToken>              CONDUCTANCE
%token  <ast::Double>           REAL
%token  <ast::Integer>          INTEGER
%token  <ast::Integer>          DEFINEDVAR
%token  <ast::Name>             NAME
%token  <ast::Name>             METHOD
%token  <ast::Name>             SUFFIX
%token  <ast::Name>             VALENCE
%token  <ast::Name>             DEL
%token  <ast::Name>             DEL2
%token  <ast::PrimeName>        PRIME
%token  <std::string>           VERBATIM
%token  <std::string>           BLOCK_COMMENT
%token  <std::string>           LINE_COMMENT
%token  <std::string>           LINE_PART
%token  <ast::String>           STRING
%token  <ast::Name>             FLUX_VAR
%token  <ModToken>              OPEN_BRACE          "{"
%token  <ModToken>              CLOSE_BRACE         "}"
%token  <ModToken>              OPEN_PARENTHESIS    "("
%token  <ModToken>              CLOSE_PARENTHESIS   ")"
%token  <ModToken>              OPEN_BRACKET        "["
%token  <ModToken>              CLOSE_BRACKET       "]"
%token  <ModToken>              AT                  "@"
%token  <ModToken>              ADD                 "+"
%token  <ModToken>              MULTIPLY            "*"
%token  <ModToken>              MINUS               "-"
%token  <ModToken>              DIVIDE              "/"
%token  <ModToken>              EQUAL               "="
%token  <ModToken>              CARET               "^"
%token  <ModToken>              COLON               ":"
%token  <ModToken>              COMMA               ","
%token  <ModToken>              TILDE               "~"
%token  <ModToken>              PERIOD              "."
%token  END                     0                   "End of file"
%token                          UNKNOWN
%token                          INVALID_TOKEN

/** Define terminal and nonterminal symbols : Instead of using AST classes
 *  directly, we are using typedefs like program_ptr. This is useful when we
 *  want to transparently change naming/type scheme. For example, raw pointer
 *  to smakrt pointers. Manually changing all types in bison specification is
 *  time consuming. Also, naming of termina/non-terminal symbols is kept same
 *  as NEURON. This helps during development and debugging. This could be
 *  once all implementation details get ported. */

%type   <ast::Program*>                  all
%type   <ast::Name*>                     Name
%type   <ast::Number*>                   NUMBER
%type   <ast::Double*>                   real
%type   <ast::Expression*>               intexpr
%type   <ast::Integer*>                  integer
%type   <ast::Model*>                    model
%type   <ast::Unit*>                     units
%type   <ast::Integer*>                  optindex
%type   <ast::Unit*>                     unit
%type   <ast::Block*>                    proc
%type   <ast::Limits*>                   limits
%type   <ast::Double*>                   abstol
%type   <ast::Identifier*>               name
%type   <ast::Number*>                   number
%type   <ast::Expression*>               primary
%type   <ast::Expression*>               term
%type   <ast::Expression*>               leftlinexpr
%type   <ast::Expression*>               linexpr
%type   <ast::NumberVector>              numlist
%type   <ast::Expression*>               expr
%type   <ast::Expression*>               aexpr
%type   <ast::Statement*>                ostmt
%type   <ast::Statement*>                astmt
%type   <ast::StatementBlock*>           stmtlist
%type   <ast::LocalListStatement*>       locallist
%type   <ast::LocalVarVector>            locallist1
%type   <ast::VarName*>                  varname
%type   <ast::ExpressionVector>          exprlist
%type   <ast::Define*>                   define1
%type   <ast::QueueStatement*>           queuestmt
%type   <ast::Expression*>               asgn
%type   <ast::FromStatement*>            fromstmt
%type   <ast::WhileStatement*>           whilestmt
%type   <ast::IfStatement*>              ifstmt
%type   <ast::ElseIfStatementVector>     optelseif
%type   <ast::ElseStatement*>            optelse
%type   <ast::SolveBlock*>               solveblk
%type   <ast::WrappedExpression*>        funccall
%type   <ast::StatementBlock*>           ifsolerr
%type   <ast::Expression*>               opinc
%type   <ast::Number*>                   opstart
%type   <ast::VarNameVector>             senslist
%type   <ast::Sens*>                     sens
%type   <ast::LagStatement*>             lagstmt
%type   <ast::ForAllStatement*>          forallstmt
%type   <ast::ParamAssign*>              parmasgn
%type   <ast::Stepped*>                  stepped
%type   <ast::IndependentDef*>           indepdef
%type   <ast::DependentDef*>             depdef
%type   <ast::Block*>                    declare
%type   <ast::ParamBlock*>               parmblk
%type   <ast::ParamAssignVector>         parmbody
%type   <ast::IndependentBlock*>         indepblk
%type   <ast::IndependentDefVector>      indepbody
%type   <ast::DependentBlock*>           depblk
%type   <ast::DependentDefVector>        depbody
%type   <ast::StateBlock*>               stateblk
%type   <ast::StepBlock*>                stepblk
%type   <ast::SteppedVector>             stepbdy
%type   <ast::WatchStatement*>           watchstmt
%type   <ast::BinaryOperator>            watchdir
%type   <ast::Watch*>                    watch1
%type   <ast::ForNetcon*>                fornetcon
%type   <ast::PlotDeclaration*>          plotdecl
%type   <ast::PlotVarVector>             pvlist
%type   <ast::ConstantBlock*>            constblk
%type   <ast::ConstantStatementVector>   conststmt
%type   <ast::MatchBlock*>               matchblk
%type   <ast::MatchVector>               matchlist
%type   <ast::Match*>                    match
%type   <ast::Identifier*>               matchname
%type   <ast::PartialBoundary*>          pareqn
%type   <ast::FirstLastTypeIndex*>       firstlast
%type   <ast::ReactionStatement*>        reaction
%type   <ast::Conserve*>                 conserve
%type   <ast::Expression*>               react
%type   <ast::Compartment*>              compart
%type   <ast::LonDifuse*>                ldifus
%type   <ast::NameVector>                namelist
%type   <ast::UnitBlock*>                unitblk
%type   <ast::ExpressionVector>          unitbody
%type   <ast::UnitDef*>                  unitdef
%type   <ast::FactorDef*>                factordef
%type   <ast::NameVector>                solvefor
%type   <ast::NameVector>                solvefor1
%type   <ast::UnitState*>                uniton
%type   <ast::NameVector>                tablst
%type   <ast::NameVector>                tablst1
%type   <ast::TableStatement*>           tablestmt
%type   <ast::NameVector>                dependlst
%type   <ast::ArgumentVector>            arglist
%type   <ast::ArgumentVector>            arglist1
%type   <ast::Integer*>                  locoptarray
%type   <ast::NeuronBlock*>              neuronblk
%type   <ast::Useion*>                   nrnuse
%type   <ast::StatementVector>           nrnstmt
%type   <ast::ReadIonVarVector>          nrnionrlist
%type   <ast::WriteIonVarVector>         nrnionwlist
%type   <ast::NonspecificCurVarVector>   nrnonspeclist
%type   <ast::ElectrodeCurVarVector>     nrneclist
%type   <ast::SectionVarVector>          nrnseclist
%type   <ast::RangeVarVector>            nrnrangelist
%type   <ast::GlobalVarVector>           nrnglobalist
%type   <ast::PointerVarVector>          nrnptrlist
%type   <ast::BbcorePointerVarVector>    nrnbbptrlist
%type   <ast::ExternVarVector>           nrnextlist
%type   <ast::ThreadsafeVarVector>       opthsafelist
%type   <ast::ThreadsafeVarVector>       threadsafelist
%type   <ast::Valence*>                  valence
%type   <ast::ExpressionStatement*>      initstmt
%type   <ast::BABlock*>                  bablk
%type   <ast::ConductanceHint*>          conducthint
%type   <ast::StatementVector>           stmtlist1
%type   <ast::InitialBlock*>             initblk
%type   <ast::ConstructorBlock*>         constructblk
%type   <ast::DestructorBlock*>          destructblk
%type   <ast::FunctionBlock*>            funcblk
%type   <ast::KineticBlock*>             kineticblk
%type   <ast::BreakpointBlock*>          brkptblk
%type   <ast::DerivativeBlock*>          derivblk
%type   <ast::LinearBlock*>              linblk
%type   <ast::NonLinearBlock*>           nonlinblk
%type   <ast::ProcedureBlock*>           procedblk
%type   <ast::NetReceiveBlock*>          netrecblk
%type   <ast::TerminalBlock*>            terminalblk
%type   <ast::DiscreteBlock*>            discretblk
%type   <ast::PartialBlock*>             partialblk
%type   <ast::FunctionTableBlock*>       functableblk
%type   <ast::Integer*>                  INTEGER_PTR
%type   <ast::Name*>                     NAME_PTR
%type   <ast::String*>                   STRING_PTR
%type   <ast::WrappedExpression*>        flux_variable

/** Precedence and Associativity : specify operator precedency and
 *  associativity (from lower to higher. Note that '^' represent
 *  exponentiation. */

%left   OR
%left   AND
%left   GT GE LT LE EQ NE
%left   "+" "-"
%left   "*" "/" "%"
%left   UNARYMINUS NOT
%right  "^"

%{
    #include "lexer/nmodl_lexer.hpp"
    #include "parser/nmodl_driver.hpp"
    #include "parser/verbatim_driver.hpp"

    using nmodl::parser::NmodlParser;
    using nmodl::parser::NmodlLexer;
    using nmodl::parser::NmodlDriver;
    using nmodl::parser::VerbatimDriver;

    /// yylex takes scanner as well as driver reference
    /// \todo: check if driver argument is required
    static NmodlParser::symbol_type yylex(NmodlLexer &scanner, NmodlDriver &/*driver*/) {
        return scanner.next_token();
    }

    /// forward declaration for the function which handles interface with other parsers
    std::string parse_with_verbatim_parser(std::string);
%}

/** start symbol */
%start top


%%

/** Grammar rules : specification of all grammar rules for NMODL. The core
 *  grammar rules are based on NEURON implementation but all other
 *  implementation details are changed to support generate AST. Some rules
 *  are adapted to support better error handling and location tracking.
 *  Note that YYLLOC_DEFAULT is not sufficient for all rules and hence
 *  we need to update @$ (especially @$.begin). Consider below example:
 *
 *  nrnstmt RANGE nrnrangelist
 *
 *  In this case we have to do : @$.begin = $1.begin (@$.end is already
 *  set to $3.end and hence don't need to update).
 *
 *  \todo ModToken is set for the symbols returned by Lexer. But we need to
 *  set accurate location for each production. We need to add method in AST
 *  classes to handle this.
 *
 *  \todo LINE_COMMENT adds comment as separate statement and hence they
 *   go into separate line in nmodl printer. Need to update grammar to distinguish
 *   standalone single line comment vs. inline comment.
 */

top             :   all
                    {
                        driver.set_ast($1);
                    }
                |   error
                    {
                        error(scanner.loc, "top");
                    }
                ;


all             :   {
                        $$ = new ast::Program();
                    }
                |   all model
                    {
                        $1->addNode($2);
                        $$ = $1;
                    }
                |   all locallist
                    {
                        $1->addNode($2);
                        $$ = $1;
                    }
                |   all define1
                    {
                        $1->addNode($2);
                        $$ = $1;
                    }
                |   all declare
                    {
                        $1->addNode($2);
                        $$ = $1;
                    }
                |   all MODEL_LEVEL INTEGER_PTR declare
                    {
                        /** todo : This is discussed with Michael Hines. Model level was inserted
                         * by merge program which is no longer exist. This was to avoid the name
                         * collision in case of include. Idea was to have some kind of namespace!
                         */
                    }
                |   all proc
                    {
                        $1->addNode($2);
                        $$ = $1;
                    }
                |   all VERBATIM
                    {
                        auto text = parse_with_verbatim_parser($2);
                        auto statement = new ast::Verbatim(new ast::String(text));
                        $1->addNode(statement);
                        $$ = $1;
                    }
                |   all BLOCK_COMMENT
                    {
                        auto text = parse_with_verbatim_parser($2);
                        auto statement = new ast::BlockComment(new ast::String(text));
                        $1->addNode(statement);
                        $$ = $1;
                    }
                |   all LINE_COMMENT
                    {
                        auto statement = new ast::LineComment(new ast::String($2));
                        $1->addNode(statement);
                        $$ = $1;
                    }
                |   all uniton
                    {
                        $1->addNode($2);
                        $$ = $1;
                    }
                |   all INCLUDE1 STRING_PTR
                    {
                        auto statement = new ast::Include($3);
                        $1->addNode(statement);
                        $$ = $1;
                    }
                ;


model           :   MODEL LINE_PART
                    {
                        $$ = new ast::Model(new ast::String($2));
                    }
                ;


define1         :   DEFINE1 NAME_PTR INTEGER_PTR
                    {
                        $$ = new ast::Define($2, $3);
                        driver.add_defined_var($2->get_node_name(), $3->eval());
                    }
                |   DEFINE1 error
                    {
                        error(scanner.loc, "define1");
                    }
                ;


Name            :   NAME_PTR    {   $$ = $1;    }
                ;


declare         :   parmblk     {   $$ = $1;    }
                |   indepblk    {   $$ = $1;    }
                |   depblk      {   $$ = $1;    }
                |   stateblk    {   $$ = $1;    }
                |   stepblk     {   $$ = $1;    }
                |   plotdecl
                    {
                        $$ = new ast::PlotBlock($1);
                    }
                |   neuronblk   {   $$ = $1;    }
                |   unitblk     {   $$ = $1;    }
                |   constblk    {   $$ = $1;    }
                ;


parmblk         :   PARAMETER "{" parmbody"}"
                    {
                        $$ = new ast::ParamBlock($3);
                    }
                ;


parmbody        :   {
                        $$ = ast::ParamAssignVector();
                    }
                |   parmbody parmasgn
                    {
                        $1.emplace_back($2);
                        $$ = $1;
                    }
                ;


parmasgn        :   NAME_PTR "=" number units limits
                    {
                        $$ = new ast::ParamAssign($1, $3, $4, $5);
                    }
                |   NAME_PTR units limits
                    {
                        $$ = new ast::ParamAssign($1, NULL, $2, $3);
                    }
                |   NAME_PTR "[" integer "]" units limits
                    {
                        $$ = new ast::ParamAssign(new ast::IndexedName($1, $3), NULL, $5, $6);
                    }
                |   error
                    {
                        error(scanner.loc, "parmasgn");
                    }
                ;


units           :           {   $$ = nullptr;  }
                |   unit    {   $$ = $1;    }
                ;


unit            :   "(" { scanner.scan_unit(); } ")"
                    {
                        // @todo : empty units should be handled in semantic analysis
                        auto unit = scanner.get_unit();
                        auto text = unit->eval();
                        $$ = new ast::Unit(unit);
                    }
                ;


uniton          :   UNITSON
                    {
                        $$ = new ast::UnitState(ast::UNIT_ON);
                    }
                |   UNITSOFF
                    {
                        $$ = new ast::UnitState(ast::UNIT_OFF);
                    }
                ;


limits          :   {   $$ = nullptr;  }
                |   LT real "," real GT
                    {
                        $$ = new ast::Limits($2, $4);
                    }
                ;


stepblk         :   STEPPED "{" stepbdy "}"
                    {
                        $$ = new ast::StepBlock($3);
                    }
                ;


stepbdy         :   {   $$ = ast::SteppedVector();    }
                |   stepbdy stepped
                    {
                            $1.emplace_back($2);
                            $$ = $1;
                    }
                ;


stepped         :   NAME_PTR "=" numlist units
                    {
                        $$ = new ast::Stepped($1, $3, $4);
                    }
                ;


numlist         :   number "," number
                    {
                        $$ = ast::NumberVector();
                        $$.emplace_back($1);
                        $$.emplace_back($3);
                    }
                |   numlist "," number
                    {
                        $1.emplace_back($3);
                        $$ = $1;
                    }
                ;


name            :   Name    {   $$ = $1;            }
                |   PRIME   {   $$ = $1.clone();    }
                ;


number          :   NUMBER  {   $$ = $1;    }
                |   "-" NUMBER
                    {
                        $2->negate();
                        $$ = $2;
                    }
                ;


NUMBER          :   integer     {   $$ = $1;            }
                |   REAL        {   $$ = $1.clone();    }
                ;


integer         :   INTEGER_PTR     {   $$ = $1;        }
                |   DEFINEDVAR  {   $$ = $1.clone();    }
                ;


real            :   REAL        {   $$ = $1.clone();    }
                |   integer
                    {
                        $$ = new ast::Double(double($1->eval()));
                        delete($1);
                    }
                ;


indepblk        :   INDEPENDENT "{" indepbody "}"
                    {
                        $$ = new ast::IndependentBlock($3);
                    }
                ;


indepbody       :   {
                        $$ = ast::IndependentDefVector();
                    }
                |   indepbody indepdef
                    {
                        $1.emplace_back($2);
                        $$ = $1;
                    }
                |   indepbody SWEEP indepdef
                    {
                        $1.emplace_back($3);
                        $3->set_sweep(std::make_shared<ast::Boolean>(1));
                        $$ = $1;
                    }
                ;


indepdef        :   NAME_PTR FROM number TO number withby integer opstart units
                    {
                        $$ = new ast::IndependentDef(NULL, $1, $3, $5, $7, $8, $9);
                    }
                |   error
                    {
                        error(scanner.loc, "indepdef");
                    }
                ;


withby          :   WITH
                ;


depblk          :   DEPENDENT "{" depbody"}"
                    {
                        $$ = new ast::DependentBlock($3);
                    }
                ;


depbody         :   {
                        $$ = ast::DependentDefVector();
                    }
                |   depbody depdef
                    {
                        $1.emplace_back($2);
                        $$ = $1;
                    }
                ;


depdef          :   name opstart units abstol
                    {
                        $$ = new ast::DependentDef($1, NULL, NULL, NULL, $2, $3, $4);
                    }
                |   name "[" integer "]" opstart units abstol
                    {
                        $$ = new ast::DependentDef($1, $3, NULL, NULL, $5, $6, $7);
                    }
                |   name FROM number TO number opstart units abstol
                    {
                        $$ = new ast::DependentDef($1, NULL, $3, $5, $6, $7, $8);
                    }
                |   name "[" integer "]" FROM number TO number opstart units abstol
                    {
                        $$ = new ast::DependentDef($1, $3, $6, $8, $9, $10, $11);
                    }
                |   error
                    {
                        error(scanner.loc, "depdef");
                    }
                ;


opstart         :                   {   $$ = nullptr;  }
                |   START1 number   {   $$ = $2;    }
                ;


abstol          :                   {   $$ = nullptr;  }
                |   LT real GT      {   $$ = $2;    }
                ;


stateblk        :   STATE  "{" depbody "}"
                    {
                        $$ = new ast::StateBlock($3);
                    }
                ;


plotdecl        :   PLOT pvlist VS name optindex
                    {
                        $$ = new ast::PlotDeclaration($2, new ast::PlotVar($4,$5));
                    }
                |   PLOT error
                    {
                        error(scanner.loc, "plotdecl");
                    }
                ;


pvlist          :   name optindex
                    {
                        $$ = ast::PlotVarVector();
                        auto variable = new ast::PlotVar($1, $2);
                        $$.emplace_back(variable);
                    }
                |   pvlist "," name optindex
                    {
                        $$ = $1;
                        auto variable = new ast::PlotVar($3, $4);
                        $$.emplace_back(variable);
                    }
                ;


optindex        :                       {   $$ = nullptr;   }
                |   "[" INTEGER_PTR "]" {   $$ = $2;        }
                ;


proc            :   initblk             {   $$ = $1;    }
                |   derivblk            {   $$ = $1;    }
                |   brkptblk            {   $$ = $1;    }
                |   linblk              {   $$ = $1;    }
                |   nonlinblk           {   $$ = $1;    }
                |   funcblk             {   $$ = $1;    }
                |   procedblk           {   $$ = $1;    }
                |   netrecblk           {   $$ = $1;    }
                |   terminalblk         {   $$ = $1;    }
                |   discretblk          {   $$ = $1;    }
                |   partialblk          {   $$ = $1;    }
                |   kineticblk          {   $$ = $1;    }
                |   constructblk        {   $$ = $1;    }
                |   destructblk         {   $$ = $1;    }
                |   functableblk        {   $$ = $1;    }
                |   BEFORE bablk
                    {
                        $$ = new ast::BeforeBlock($2);
                    }
                |   AFTER bablk
                    {
                        $$ = new ast::AfterBlock($2);
                    }
                ;


initblk         :   INITIAL1 stmtlist "}"
                    {
                        $$ = new ast::InitialBlock($2);
                    }
                ;


constructblk    :   CONSTRUCTOR stmtlist "}"
                    {
                        $$ = new ast::ConstructorBlock($2);
                    }
                ;


destructblk     :   DESTRUCTOR stmtlist "}"
                    {
                        $$ = new ast::DestructorBlock($2);
                    }
                ;


stmtlist        :   "{" stmtlist1
                    {
                        $$ = new ast::StatementBlock($2);
                        $$->set_token($1);
                    }
                |   "{" locallist stmtlist1
                    {
                        $3.insert($3.begin(), std::shared_ptr<ast::LocalListStatement>($2));
                        $$ = new ast::StatementBlock($3);
                        $$->set_token($1);
                    }
                ;


conducthint     :   CONDUCTANCE Name
                    {
                        $$ = new ast::ConductanceHint($2, NULL);
                    }
                |   CONDUCTANCE Name USEION NAME_PTR
                    {
                        $$ = new ast::ConductanceHint($2, $4);
                    }
                ;


locallist       :   LOCAL locallist1
                    {
                        $$ = new ast::LocalListStatement($2);
                    }
                |   LOCAL error
                    {
                        error(scanner.loc, "locallist");
                    }
                ;


locallist1      :   NAME_PTR locoptarray
                    {
                        $$ = ast::LocalVarVector();
                        if($2) {
                            auto variable = new ast::LocalVar(new ast::IndexedName($1, $2));
                            $$.emplace_back(variable);
                        } else {
                            auto variable = new ast::LocalVar($1);
                            $$.emplace_back(variable);
                        }
                    }
                |   locallist1 "," NAME_PTR locoptarray
                    {
                        if($4) {
                            auto variable = new ast::LocalVar(new ast::IndexedName($3, $4));
                            $1.emplace_back(variable);
                        } else {
                            auto variable = new ast::LocalVar($3);
                            $1.emplace_back(variable);
                        }
                        $$ = $1;
                    }
                ;


locoptarray     :                       {   $$ = nullptr;  }
                |   "[" integer "]"     {   $$ = $2;    }
                ;


stmtlist1       :   {
                        $$ = ast::StatementVector();
                    }
                |   stmtlist1 ostmt
                    {
                        $1.emplace_back($2);
                        $$ = $1;
                    }
                |   stmtlist1 astmt
                    {
                        $1.emplace_back($2);
                        $$ = $1;
                    }
                |   stmtlist1 LINE_COMMENT
                    {
                        auto statement = new ast::LineComment(new ast::String($2));
                        $1.emplace_back(statement);
                        $$ = $1;
                    }
                ;


ostmt           :   fromstmt        {   $$ = $1;    }
                |   forallstmt      {   $$ = $1;    }
                |   whilestmt       {   $$ = $1;    }
                |   ifstmt          {   $$ = $1;    }
                |   stmtlist "}"
                    {
                        $$ = new ast::ExpressionStatement($1);
                    }
                |   solveblk
                    {
                        $$ = new ast::ExpressionStatement($1);
                    }
                |   conducthint     {   $$ = $1;    }
                |   VERBATIM
                    {   auto text = parse_with_verbatim_parser($1);
                        $$ = new ast::Verbatim(new ast::String(text));
                    }
                |   BLOCK_COMMENT
                    {   auto text = parse_with_verbatim_parser($1);
                        $$ = new ast::BlockComment(new ast::String(text));
                    }
                |   sens            { $$ = $1; }
                |   compart         { $$ = $1; }
                |   ldifus          { $$ = $1; }
                |   conserve        { $$ = $1; }
                |   lagstmt         { $$ = $1; }
                |   queuestmt       { $$ = $1; }
                |   RESET
                    {
                        $$ = new ast::Reset();
                    }
                |   matchblk
                    {
                        $$ = new ast::ExpressionStatement($1);
                    }
                |   pareqn          { $$ = $1; }
                |   tablestmt       { $$ = $1; }
                |   uniton          { $$ = $1; }
                |   initstmt        { $$ = $1; }
                |   watchstmt       { $$ = $1; }
                |   fornetcon
                    {
                        $$ = new ast::ExpressionStatement($1);
                    }
                |   NRNMUTEXLOCK
                    {
                        $$ = new ast::MutexLock();
                    }
                |   NRNMUTEXUNLOCK
                    {
                        $$ = new ast::MutexUnlock();
                    }
                |   error
                    {
                        error(scanner.loc, "ostmt");
                    }
                ;


astmt           :   asgn
                    {
                        $$ = new ast::ExpressionStatement($1);
                    }
                |   PROTECT asgn
                    {
                        $$ = new ast::ProtectStatement($2);
                    }
                |   reaction      { $$ = $1; }
                |   funccall
                    {
                        $$ = new ast::ExpressionStatement($1);
                    }
                ;


asgn            :   varname "=" expr
                    {
                        auto expression = new ast::BinaryExpression($1, ast::BinaryOperator(ast::BOP_ASSIGN), $3);
                        auto name = $1->get_name();
                        if ((name->is_prime_name()) ||
                            (name->is_indexed_name() &&
                            std::dynamic_pointer_cast<ast::IndexedName>(name)->get_name()->is_prime_name()))
                        {
                            $$ = new ast::DiffEqExpression(expression);
                        } else {
                            $$ = expression;
                        }
                    }
                |   nonlineqn expr "=" expr
                    {
                        $$ = new ast::NonLinEquation($2, $4);
                    }
                |   lineqn leftlinexpr "=" linexpr
                    {
                        $$ = new ast::LinEquation($2, $4);
                    }
                ;


varname         :   name
                    {
                        $$ = new ast::VarName($1, nullptr, nullptr);
                    }
                |   name "[" intexpr "]"
                    {
                        $$ = new ast::VarName(new ast::IndexedName($1, $3), nullptr, nullptr);
                    }
                |   NAME_PTR "@" integer
                    {
                        $$ = new ast::VarName($1, $3, nullptr);
                    }
                |   NAME_PTR "@" integer "[" intexpr "]"
                    {
                        $$ = new ast::VarName($1, $3, $5);
                    }
                ;


intexpr         :   Name                    { $$ = $1; }
                |   integer                 { $$ = $1; }
                |   "(" intexpr ")"
                    {
                        auto expr = new ast::ParenExpression($2);
                        $$ = new ast::WrappedExpression(expr);
                    }
                |   intexpr "+" intexpr
                    {
                        auto expr = new ast::BinaryExpression($1, ast::BinaryOperator(ast::BOP_ADDITION), $3);
                        $$ = new ast::WrappedExpression(expr);
                    }
                |   intexpr "-" intexpr
                    {
                        auto expr = new ast::BinaryExpression($1, ast::BinaryOperator(ast::BOP_SUBTRACTION), $3);
                        $$ = new ast::WrappedExpression(expr);
                    }
                |   intexpr "*" intexpr
                    {
                        auto expr = new ast::BinaryExpression($1, ast::BinaryOperator(ast::BOP_MULTIPLICATION), $3);
                        $$ = new ast::WrappedExpression(expr);
                    }
                |   intexpr "/" intexpr
                    {
                        auto expr = new ast::BinaryExpression($1, ast::BinaryOperator(ast::BOP_DIVISION), $3);
                        $$ = new ast::WrappedExpression(expr);
                    }
                |   error
                    {
                        error(scanner.loc, "intexpr");
                    }
                ;


expr            :   varname             { $$ = $1; }
                |   flux_variable       { $$ = $1; }
                |   real units
                    {
                        if($2)
                            $$ = new ast::DoubleUnit($1, $2);
                        else
                            $$ = $1;
                    }
                |   funccall            { $$ = $1; }
                |   "(" expr ")"
                    {
                        auto expr = new ast::ParenExpression($2);
                        $$ = new ast::WrappedExpression(expr);
                    }
                |   expr "+" expr
                    {
                        auto expr  = new ast::BinaryExpression($1, ast::BinaryOperator(ast::BOP_ADDITION), $3);
                        $$ = new ast::WrappedExpression(expr);
                    }
                |   expr "-" expr
                    {
                        auto expr  = new ast::BinaryExpression($1, ast::BinaryOperator(ast::BOP_SUBTRACTION), $3);
                        $$ = new ast::WrappedExpression(expr);
                    }
                |   expr "*" expr
                    {
                        auto expr  = new ast::BinaryExpression($1, ast::BinaryOperator(ast::BOP_MULTIPLICATION), $3);
                        $$ = new ast::WrappedExpression(expr);
                    }
                |   expr "/" expr
                    {
                        auto expr  = new ast::BinaryExpression($1, ast::BinaryOperator(ast::BOP_DIVISION), $3);
                        $$ = new ast::WrappedExpression(expr);
                    }
                |   expr "^" expr
                    {
                        auto expr  = new ast::BinaryExpression($1, ast::BinaryOperator(ast::BOP_POWER), $3);
                        $$ = new ast::WrappedExpression(expr);
                    }
                |   expr OR expr
                    {
                        $$ = new ast::BinaryExpression($1, ast::BinaryOperator(ast::BOP_OR), $3);
                    }
                |   expr AND expr
                    {
                        $$ = new ast::BinaryExpression($1, ast::BinaryOperator(ast::BOP_AND), $3);
                    }
                |   expr GT expr
                    {
                        $$ = new ast::BinaryExpression($1, ast::BinaryOperator(ast::BOP_GREATER), $3);
                    }
                |   expr LT expr
                    {
                        $$ = new ast::BinaryExpression($1, ast::BinaryOperator(ast::BOP_LESS), $3);
                    }
                |   expr GE expr
                    {
                        $$ = new ast::BinaryExpression($1, ast::BinaryOperator(ast::BOP_GREATER_EQUAL), $3);
                    }
                |   expr LE expr
                    {
                        $$ = new ast::BinaryExpression($1, ast::BinaryOperator(ast::BOP_LESS_EQUAL), $3);
                    }
                |   expr EQ expr
                    {
                        $$ = new ast::BinaryExpression($1, ast::BinaryOperator(ast::BOP_EXACT_EQUAL), $3);
                    }
                |   expr NE expr
                    {
                        $$ = new ast::BinaryExpression($1, ast::BinaryOperator(ast::BOP_NOT_EQUAL), $3);
                    }
                |   NOT expr
                    {
                        $$ = new ast::UnaryExpression(ast::UnaryOperator(ast::UOP_NOT), $2);
                    }
                |   "-" expr %prec UNARYMINUS
                    {
                        $$ = new ast::UnaryExpression(ast::UnaryOperator(ast::UOP_NEGATION), $2);
                    }
                |   error
                    {
                        error(scanner.loc, "expr");
                    }
                ;

                /** \todo Add extra rules for better error reporting :
                | "(" expr      { yyerror("Unbalanced left parenthesis followed by valid expressions"); }
                | "(" error     { yyerror("Unbalanced left parenthesis followed by non parseable"); }
                |  expr ")"     { yyerror("Unbalanced right parenthesis"); }
                */


nonlineqn       : NONLIN1
                ;


lineqn          : LIN1
                ;


leftlinexpr     : linexpr       { $$ = $1; }
                ;


linexpr         :   primary     { $$ = $1; }
                |   "-" primary
                    {
                        $$ = new ast::UnaryExpression(ast::UnaryOperator(ast::UOP_NEGATION), $2);
                    }
                |   linexpr "+" primary
                    {
                        $$ = new ast::BinaryExpression($1, ast::BinaryOperator(ast::BOP_ADDITION), $3);
                    }
                |   linexpr "-" primary
                    {
                        $$ = new ast::BinaryExpression($1, ast::BinaryOperator(ast::BOP_SUBTRACTION), $3);
                    }
                ;


primary         :   term        { $$ = $1; }
                |   primary "*" term
                    {
                        $$ = new ast::BinaryExpression($1, ast::BinaryOperator(ast::BOP_MULTIPLICATION), $3);
                    }
                |   primary "/" term
                    {
                        $$ = new ast::BinaryExpression($1, ast::BinaryOperator(ast::BOP_DIVISION), $3);
                    }
                ;


term            :   varname         { $$ = $1; }
                |   real            { $$ = $1; }
                |   funccall        { $$ = $1; }
                |   "(" expr ")"    { $$ = new ast::ParenExpression($2); }
                |   error
                    {
                        error(scanner.loc, "term");
                    }
                ;


funccall        :   NAME_PTR "(" exprlist ")"
                    {
                        auto expression = new ast::FunctionCall($1, $3);
                        $$ = new ast::WrappedExpression(expression);
                    }
                ;


exprlist        :   {
                        $$ = ast::ExpressionVector();
                    }
                |   expr
                    {
                        $$ = ast::ExpressionVector();
                        $$.emplace_back($1);
                    }
                |   STRING_PTR
                    {
                        $$ = ast::ExpressionVector();
                        $$.emplace_back($1);
                    }
                |   exprlist "," expr
                    {
                        $1.emplace_back($3);
                        $$ = $1;
                    }
                |   exprlist "," STRING_PTR
                    {
                        $1.emplace_back($3);
                        $$ = $1;
                    }
                ;


fromstmt        :   FROM NAME_PTR "=" intexpr TO intexpr opinc stmtlist "}"
                    {
                        $$ = new ast::FromStatement($2, $4, $6, $7, $8);
                    }
                |   FROM error
                    {
                        error(scanner.loc, "fromstmt");
                    }
                ;


opinc           :               { $$ = nullptr; }
                | BY intexpr    { $$ = $2; }
                ;


forallstmt      :   FORALL1 NAME_PTR stmtlist "}"
                    {
                        $$ = new ast::ForAllStatement($2, $3);
                    }
                |   FORALL1 error
                    {
                        error(scanner.loc, "forallstmt");
                    }
                ;


whilestmt       :   WHILE "(" expr ")" stmtlist "}"
                    {
                        $$ = new ast::WhileStatement($3, $5);
                    }
                ;


ifstmt          :   IF "(" expr ")" stmtlist "}" optelseif optelse
                    {
                        $$ = new ast::IfStatement($3, $5, $7, $8);
                    }
                ;


optelseif       :   {
                        $$ = ast::ElseIfStatementVector();
                    }
                |   optelseif ELSE IF "(" expr ")" stmtlist "}"
                    {
                        $1.emplace_back(new ast::ElseIfStatement($5, $7));
                        $$ = $1;
                    }
                ;


optelse         :   { $$ = nullptr; }
                |   ELSE stmtlist "}"
                    {
                        $$ = new ast::ElseStatement($2);
                    }
                ;


derivblk        :   DERIVATIVE NAME_PTR stmtlist "}"
                    {
                        $$ = new ast::DerivativeBlock($2, $3);
                        $$->set_token($1);
                    }
                ;


linblk          :   LINEAR NAME_PTR solvefor stmtlist "}"
                    {
                        $$ = new ast::LinearBlock($2, $3, $4);
                        $$->set_token($1);
                    }
                ;


nonlinblk       :   NONLINEAR NAME_PTR solvefor stmtlist "}"
                    {
                        $$ = new ast::NonLinearBlock($2, $3, $4);
                        $$->set_token($1);
                    }
                ;


discretblk      :   DISCRETE NAME_PTR stmtlist "}"
                    {
                        $$ = new ast::DiscreteBlock($2, $3);
                        // todo : disabled symbol table, remove this
                        //$$->set_token($1);
                    }
                ;


partialblk      :   PARTIAL NAME_PTR stmtlist "}"
                    {
                        $$ = new ast::PartialBlock($2, $3);
                        $$->set_token($1);
                    }
                |   PARTIAL error
                    {
                        error(scanner.loc, "partialblk");
                    }
                ;


pareqn          :   "~" PRIME "=" NAME_PTR "*" DEL2 "(" NAME_PTR ")" "+" NAME_PTR
                    {
                        $$ = new ast::PartialBoundary(NULL, $2.clone(), NULL, NULL, $4, $6.clone(), $8, $11);
                    }
                |   "~" DEL NAME_PTR "[" firstlast "]" "=" expr
                    {
                        $$ = new ast::PartialBoundary($2.clone(), $3, $5, $8, NULL, NULL, NULL, NULL);
                    }
                |   "~" NAME_PTR "[" firstlast "]" "=" expr
                    {
                        $$ = new ast::PartialBoundary(NULL, $2, $4, $7, NULL, NULL, NULL, NULL);
                    }
                ;


firstlast       :   FIRST
                    {
                        $$ = new ast::FirstLastTypeIndex(ast::PEQ_FIRST);
                    }
                |   LAST
                    {
                        $$ = new ast::FirstLastTypeIndex(ast::PEQ_LAST);
                    }
                ;


functableblk    :   FUNCTION_TABLE NAME_PTR "(" arglist ")" units
                    {
                        $$ = new ast::FunctionTableBlock($2, $4, $6);
                        $$->set_token($1);
                    }
                ;


funcblk         :   FUNCTION1 NAME_PTR "(" arglist ")" units stmtlist "}"
                    {
                        $$ = new ast::FunctionBlock($2, $4, $6, $7);
                        $$->set_token($1);
                    }
                ;


arglist         :   {
                        $$ = ast::ArgumentVector();
                    }
                |   arglist1 { $$ = $1; }
                ;


arglist1        :   name units
                    {
                        $$ = ast::ArgumentVector();
                        $$.emplace_back(new ast::Argument($1, $2));
                    }
                |   arglist1 "," name units
                    {
                        $1.emplace_back(new ast::Argument($3, $4));
                        $$ = $1;
                    }
                ;


procedblk       :   PROCEDURE NAME_PTR "(" arglist ")" units stmtlist "}"
                    {
                            $$ = new ast::ProcedureBlock($2, $4, $6, $7); $$->set_token($1);
                    }
                ;


netrecblk       :   NETRECEIVE "(" arglist ")" stmtlist "}"
                    {
                        $$ = new ast::NetReceiveBlock($3, $5);
                    }
                |   NETRECEIVE error
                    {
                        error(scanner.loc, "netrecblk");
                    }
                ;


initstmt        :   INITIAL1 stmtlist "}"
                    {
                        $$ = new ast::ExpressionStatement(new ast::InitialBlock($2));
                    }
                ;


solveblk        :   SOLVE NAME_PTR ifsolerr
                    {
                        $$ = new ast::SolveBlock($2, NULL, NULL, $3);
                    }
                |   SOLVE NAME_PTR USING METHOD ifsolerr
                    {
                        $$ = new ast::SolveBlock($2, $4.clone(), NULL, $5);
                    }
                |
                    SOLVE NAME_PTR STEADYSTATE METHOD ifsolerr
                    {
                        $$ = new ast::SolveBlock($2, NULL, $4.clone(), $5);
                    }
                |   SOLVE error
                    {
                        error(scanner.loc, "solveblk");
                    }
                ;


ifsolerr        :                           { $$ = nullptr; }
                |   IFERROR stmtlist "}"    { $$ = $2; }
                ;


solvefor        :                           { $$ = ast::NameVector(); }
                | solvefor1                 { $$ = $1; }
                ;


solvefor1       :   SOLVEFOR NAME_PTR
                    {
                        $$ = ast::NameVector();
                        $$.emplace_back($2);
                    }
                |   solvefor1 "," NAME_PTR
                    {
                        $1.emplace_back($3);
                        $$ = $1;
                    }
                |   SOLVEFOR error
                    {
                        error(scanner.loc, "solvefor1");
                    }
                ;


brkptblk        :   BREAKPOINT stmtlist "}"
                    {
                        $$ = new ast::BreakpointBlock($2);
                    }
                ;


terminalblk     :   TERMINAL stmtlist "}"
                    {
                        $$ = new ast::TerminalBlock($2);
                    }
                ;


bablk           :   BREAKPOINT stmtlist "}"
                    {
                        $$ = new ast::BABlock(new ast::BABlockType(ast::BATYPE_BREAKPOINT), $2);
                    }
                |   SOLVE stmtlist "}"
                    {
                        $$ = new ast::BABlock(new ast::BABlockType(ast::BATYPE_SOLVE), $2);
                    }
                |   INITIAL1 stmtlist "}"
                    {
                        $$ = new ast::BABlock(new ast::BABlockType(ast::BATYPE_INITIAL), $2);
                    }
                |   STEP stmtlist "}"
                    {
                        $$ = new ast::BABlock(new ast::BABlockType(ast::BATYPE_STEP), $2);
                    }
                |   error
                    {
                        error(scanner.loc, "bablk");
                    }
                ;


watchstmt       :   WATCH watch1
                    {
                        $$ = new ast::WatchStatement(ast::WatchVector());
                        $$->addWatch($2);
                    }
                |   watchstmt "," watch1
                    {
                        $1->addWatch($3); $$ = $1;
                    }
                |   WATCH error
                    {
                        error(scanner.loc, "watchstmt");
                    }
                ;


watch1          :   "(" aexpr watchdir aexpr ")" real
                    {
                        $$ = new ast::Watch( new ast::BinaryExpression($2, $3, $4), $6);
                    }
                ;


watchdir        :   GT
                    {
                        $$ = ast::BinaryOperator(ast::BOP_GREATER);
                    }
                |   LT
                    {
                        $$ = ast::BinaryOperator(ast::BOP_LESS);
                    }
                ;


fornetcon       :   FOR_NETCONS "(" arglist ")" stmtlist "}"
                    {
                        $$ = new ast::ForNetcon($3, $5);
                    }
                |   FOR_NETCONS error
                    {
                        error(scanner.loc, "fornetcon");
                    }
                ;


aexpr           :   varname                 { $$ = $1; }
                |   real units
                    {
                        $$ = new ast::DoubleUnit($1, $2);
                    }
                |   funccall                { $$ = $1; }
                |   "(" aexpr ")"           { $$ = new ast::ParenExpression($2); }
                |   aexpr "+" aexpr
                    {
                        $$ = new ast::BinaryExpression($1, ast::BinaryOperator(ast::BOP_ADDITION), $3);
                    }
                |   aexpr "-" aexpr
                    {
                        $$ = new ast::BinaryExpression($1, ast::BinaryOperator(ast::BOP_SUBTRACTION), $3);
                    }
                |   aexpr "*" aexpr
                    {
                        $$ = new ast::BinaryExpression($1, ast::BinaryOperator(ast::BOP_MULTIPLICATION), $3);
                    }
                |   aexpr "/" aexpr
                    {
                        $$ = new ast::BinaryExpression($1, ast::BinaryOperator(ast::BOP_DIVISION), $3);
                    }
                |   aexpr "^" aexpr
                    {
                        $$ = new ast::BinaryExpression($1, ast::BinaryOperator(ast::BOP_POWER), $3);
                    }
                |   "-" aexpr %prec UNARYMINUS
                    {
                        $$ = new ast::UnaryExpression(ast::UnaryOperator(ast::UOP_NEGATION), $2);
                    }
                |   error
                    {
                        error(scanner.loc, "aexpr");
                    }
                ;


sens            :   SENS senslist
                    {
                        $$ = new ast::Sens($2);
                    }
                |   SENS error
                    {
                        error(scanner.loc, "sens");
                    }
                ;


senslist        :   varname
                    {
                        $$ = ast::VarNameVector();
                        $$.emplace_back($1);
                    }
                |   senslist "," varname
                    {
                        $1.emplace_back($3);
                        $$ = $1;
                    }
                ;


conserve        :   CONSERVE react "=" expr
                    {
                        $$ = new ast::Conserve($2, $4);
                    }
                |   CONSERVE error
                    {
                        error(scanner.loc, "conserve");
                    }
                ;


compart         :   COMPARTMENT NAME_PTR "," expr "{" namelist "}"
                    {
                        $$ = new ast::Compartment($2, $4, $6);
                    }
                |   COMPARTMENT expr "{" namelist "}"
                    {
                        $$ = new ast::Compartment(NULL, $2, $4);
                    }
                ;


ldifus          :   LONGDIFUS NAME_PTR "," expr "{" namelist "}"
                    {
                        $$ = new ast::LonDifuse($2, $4, $6);
                    }
                |   LONGDIFUS expr "{" namelist "}"
                    {
                        $$ = new ast::LonDifuse(NULL, $2, $4);
                    }
                ;


namelist        :   NAME_PTR
                    {
                        $$ = ast::NameVector();
                        $$.emplace_back($1);
                    }
                |   namelist NAME_PTR
                    {
                        $1.emplace_back($2);
                        $$ = $1;
                    }
                ;


kineticblk      :   KINETIC NAME_PTR solvefor stmtlist "}"
                    {
                        $$ = new ast::KineticBlock($2, $3, $4);
                        $$->set_token($1);
                    }
                ;


reaction        :   REACTION react REACT1 react "(" expr "," expr ")"
                    {
                        auto op = ast::ReactionOperator(ast::LTMINUSGT);
                        $$ = new ast::ReactionStatement($2, op, $4, $6, $8);
                    }
                |   REACTION react LT LT  "(" expr ")"
                    {
                        auto op = ast::ReactionOperator(ast::LTLT);
                        $$ = new ast::ReactionStatement($2, op, NULL, $6, NULL);
                    }
                |   REACTION react "-" GT "(" expr ")"
                    {
                        auto op = ast::ReactionOperator(ast::MINUSGT);
                        $$ = new ast::ReactionStatement($2, op, NULL, $6, NULL);
                    }
                |   REACTION error
                    {
                        /** \todo Need to revisit reaction implementation */
                    }
                ;


react           :   varname
                    {
                        $$ = new ast::ReactVarName(nullptr, $1);
                    }
                |   integer varname
                    {
                        $$ = new ast::ReactVarName($1, $2);
                    }
                |   react "+" varname
                    {
                        auto op = ast::BinaryOperator(ast::BOP_ADDITION);
                        auto variable = new ast::ReactVarName(nullptr, $3);
                        $$ = new ast::BinaryExpression($1, op, variable);
                    }
                |   react "+" integer varname
                    {
                        auto op = ast::BinaryOperator(ast::BOP_ADDITION);
                        auto variable = new ast::ReactVarName($3, $4);
                        $$ = new ast::BinaryExpression($1, op, variable);
                    }
                ;


lagstmt         :   LAG name BY NAME_PTR
                    {
                        $$ = new ast::LagStatement($2, $4);
                    }
                |   LAG error
                    {
                        error(scanner.loc, "lagstmt");
                    }
                ;


queuestmt       :   PUTQ name
                    {
                        $$ = new ast::QueueStatement(new ast::QueueExpressionType(ast::PUT_QUEUE), $2);
                    }
                |   GETQ name
                    {
                        $$ = new ast::QueueStatement(new ast::QueueExpressionType(ast::GET_QUEUE), $2);
                    }
                ;


matchblk        :   MATCH "{" matchlist "}"
                    {
                        $$ = new ast::MatchBlock($3);
                    }
                ;


matchlist       :   match
                    {
                        $$ = ast::MatchVector();
                        $$.emplace_back($1);
                    }
                |   matchlist match
                    {
                        $1.emplace_back($2);
                        $$ = $1;
                    }
                ;


match           :   name
                    {
                        $$ = new ast::Match($1, NULL);
                    }
                |   matchname "(" expr ")" "=" expr
                    {
                        auto op = ast::BinaryOperator(ast::BOP_ASSIGN);
                        auto lhs = new ast::ParenExpression($3);
                        auto rhs = $6;
                        auto expr = new ast::BinaryExpression(lhs, op, rhs);
                        $$ = new ast::Match($1, expr);
                    }
                |   error
                    {
                        error(scanner.loc, "match ");
                    }
                ;


matchname       :   name    { $$ = $1; }
                |   name "[" NAME_PTR "]"
                    {
                        $$ = new ast::IndexedName($1, $3);
                    }
                ;


unitblk         :   UNITS "{" unitbody "}"
                    {
                        $$ = new ast::UnitBlock($3);
                    }
                ;


unitbody        :   {
                        $$ = ast::ExpressionVector();
                    }
                |   unitbody unitdef
                    {
                        $1.emplace_back($2);
                        $$ = $1;
                    }
                |   unitbody factordef
                    {
                        $1.emplace_back($2);
                        $$ = $1;
                    }
                ;


unitdef         :   unit "=" unit
                    {
                        $$ = new ast::UnitDef($1, $3);
                    }
                |   unit error
                    {
                        error(scanner.loc, "unitdef ");
                    }
                ;


factordef       :   NAME_PTR "=" real unit
                    {
                        $$ = new ast::FactorDef($1, $3, $4, NULL, NULL);
                        $$->set_token(*($1->get_token()));
                    }
                |   NAME_PTR "=" unit unit
                    {
                        $$ = new ast::FactorDef($1, NULL, $3, NULL, $4);
                        $$->set_token(*($1->get_token()));
                    }
                |   NAME_PTR "=" unit "-" GT unit
                    {
                        $$ = new ast::FactorDef($1, NULL, $3, new ast::Boolean(1), $6);
                        $$->set_token(*($1->get_token()));
                    }
                |   error
                    {

                    }
                ;


constblk        :   CONSTANT "{" conststmt "}"
                    {
                        $$ = new ast::ConstantBlock($3);
                    }
                ;


conststmt       :   {
                        $$ = ast::ConstantStatementVector();
                    }
                |   conststmt NAME_PTR "=" number units
                    {
                        auto constant = new ast::ConstantVar($2, $4, $5);
                        $1.emplace_back(new ast::ConstantStatement(constant));
                        $$ = $1;
                    }
                ;


tablestmt       :   TABLE tablst dependlst FROM expr TO expr WITH INTEGER_PTR
                    {
                        $$ = new ast::TableStatement($2, $3, $5, $7, $9);
                    }
                |   TABLE error
                    {
                        error(scanner.loc, "tablestmt");
                    }
                ;


tablst          :               {   $$ = ast::NameVector(); }
                |   tablst1     {   $$ = $1; }
                ;


tablst1         :   Name
                    {
                        $$ = ast::NameVector();
                        $$.emplace_back($1);
                    }
                |   tablst1 "," Name
                    {
                        $1.emplace_back($3);
                        $$ = $1;
                    }
                ;


dependlst       :                       { $$ = ast::NameVector(); }
                |   DEPEND tablst1      { $$ = $2; }
                ;


neuronblk       :   NEURON OPEN_BRACE nrnstmt CLOSE_BRACE
                    {
                        auto block = new ast::StatementBlock($3);
                        block->set_token($2);
                        $$ = new ast::NeuronBlock(block);
                    }
                ;


nrnstmt         :   { $$ = ast::StatementVector(); }
                |   nrnstmt SUFFIX NAME_PTR
                    {
                        $1.emplace_back(new ast::Suffix($2.clone(), $3));
                        $$ = $1;
                    }
                |   nrnstmt nrnuse
                    {
                        $1.emplace_back($2);
                        $$ = $1;
                    }
                |   nrnstmt NONSPECIFIC nrnonspeclist
                    {
                        $1.emplace_back(new ast::Nonspecific($3));
                        $$ = $1;
                    }
                |   nrnstmt ELECTRODE_CURRENT nrneclist
                    {
                        $1.emplace_back(new ast::ElctrodeCurrent($3));
                        $$ = $1;
                    }
                |   nrnstmt SECTION nrnseclist
                    {
                        $1.emplace_back(new ast::Section($3));
                        $$ = $1;
                    }
                |   nrnstmt RANGE nrnrangelist
                    {
                        $1.emplace_back(new ast::Range($3));
                        $$ = $1;
                    }
                |   nrnstmt GLOBAL nrnglobalist
                    {
                        $1.emplace_back(new ast::Global($3));
                        $$ = $1;
                    }
                |   nrnstmt POINTER nrnptrlist
                    {
                        $1.emplace_back(new ast::Pointer($3));
                        $$ = $1;
                    }
                |   nrnstmt BBCOREPOINTER nrnbbptrlist
                    {
                        $1.emplace_back(new ast::BbcorePtr($3));
                        $$ = $1;
                    }
                |   nrnstmt EXTERNAL nrnextlist
                    {
                        $1.emplace_back(new ast::External($3));
                        $$ = $1;
                    }
                |   nrnstmt THREADSAFE opthsafelist
                    {
                        $1.emplace_back(new ast::ThreadSafe($3));
                        $$ = $1;
                    }
                ;


nrnuse          :   USEION NAME_PTR READ nrnionrlist valence
                    {
                        $$ = new ast::Useion($2, $4, ast::WriteIonVarVector(), $5);
                    }
                |   USEION NAME_PTR WRITE nrnionwlist valence
                    {
                        $$ = new ast::Useion($2, ast::ReadIonVarVector(), $4, $5);
                    }
                |   USEION NAME_PTR READ nrnionrlist WRITE nrnionwlist valence
                    {
                        $$ = new ast::Useion($2, $4, $6, $7);
                    }
                |   USEION error
                    {
                        error(scanner.loc, "nrnuse");
                    }
                ;


nrnionrlist     :   NAME_PTR
                    {
                        $$ = ast::ReadIonVarVector();
                        $$.emplace_back(new ast::ReadIonVar($1));
                    }
                |   nrnionrlist "," NAME_PTR
                    {
                        $1.emplace_back(new ast::ReadIonVar($3));
                        $$ = $1;
                    }
                |   error
                    {
                        error(scanner.loc, "nrnionrlist");
                    }
                ;


nrnionwlist     :   NAME_PTR
                    {
                        $$ = ast::WriteIonVarVector();
                        $$.emplace_back(new ast::WriteIonVar($1));
                    }
                |   nrnionwlist "," NAME_PTR
                    {
                        $1.emplace_back(new ast::WriteIonVar($3));
                        $$ = $1;
                    }
                |   error
                    {
                        error(scanner.loc, "nrnionwlist");
                    }
                ;


nrnonspeclist   :   NAME_PTR
                    {
                        $$ = ast::NonspecificCurVarVector();
                        $$.emplace_back(new ast::NonspecificCurVar($1));
                    }
                |   nrnonspeclist "," NAME_PTR
                    {
                        $1.emplace_back(new ast::NonspecificCurVar($3));
                        $$ = $1;
                    }
                |   error
                    {
                        error(scanner.loc, "nrnonspeclist");
                    }
                ;


nrneclist       :   NAME_PTR
                    {
                        $$ = ast::ElectrodeCurVarVector();
                        $$.emplace_back(new ast::ElectrodeCurVar($1));
                    }
                |   nrneclist "," NAME_PTR
                    {
                        $1.emplace_back(new ast::ElectrodeCurVar($3));
                        $$ = $1;
                    }
                |   error
                    {
                        error(scanner.loc, "nrneclist");
                    }
                ;


nrnseclist      :   NAME_PTR
                    {
                        $$ = ast::SectionVarVector();
                        $$.emplace_back(new ast::SectionVar($1));
                    }
                |   nrnseclist "," NAME_PTR
                    {
                        $1.emplace_back(new ast::SectionVar($3));
                        $$ = $1;
                    }
                |   error
                    {
                        error(scanner.loc, "nrnseclist");
                    }
                ;


nrnrangelist    :   NAME_PTR
                    {
                        $$ = ast::RangeVarVector();
                        $$.emplace_back(new ast::RangeVar($1));
                    }
                |   nrnrangelist "," NAME_PTR
                    {
                        $1.emplace_back(new ast::RangeVar($3));
                        $$ = $1;
                    }
                |   error
                    {
                        error(scanner.loc, "nrnrangelist");
                    }
                ;


nrnglobalist    :   NAME_PTR
                    {
                        $$ = ast::GlobalVarVector();
                        $$.emplace_back(new ast::GlobalVar($1));
                    }
                |   nrnglobalist "," NAME_PTR
                    {
                        $1.emplace_back(new ast::GlobalVar($3));
                        $$ = $1;
                    }
                |   error
                    {
                        error(scanner.loc, "nrnglobalist");
                    }
                ;


nrnptrlist      :   NAME_PTR
                    {
                        $$ = ast::PointerVarVector();
                        $$.emplace_back(new ast::PointerVar($1));
                    }
                |   nrnptrlist "," NAME_PTR
                    {
                        $1.emplace_back(new ast::PointerVar($3));
                        $$ = $1;
                    }
                |   error
                    {
                        error(scanner.loc, "nrnptrlist");
                    }
                ;


nrnbbptrlist    :   NAME_PTR
                    {
                        $$ = ast::BbcorePointerVarVector();
                        $$.emplace_back(new ast::BbcorePointerVar($1));
                    }
                |   nrnbbptrlist "," NAME_PTR
                    {
                        $1.emplace_back(new ast::BbcorePointerVar($3));
                        $$ = $1;
                    }
                |   error
                    {
                        error(scanner.loc, "nrnbbptrlist");
                    }
                ;


nrnextlist      :   NAME_PTR
                    {
                        $$ = ast::ExternVarVector();
                        $$.emplace_back(new ast::ExternVar($1));
                    }
                |   nrnextlist "," NAME_PTR
                    {
                        $1.emplace_back(new ast::ExternVar($3));
                        $$ = $1;
                    }
                |   error
                    {
                        error(scanner.loc, "nrnextlist");
                    }
                ;


opthsafelist    :                       { $$ = ast::ThreadsafeVarVector(); }
                |   threadsafelist      { $$ = $1; }
                ;


threadsafelist  :   NAME_PTR
                    {
                        $$ = ast::ThreadsafeVarVector();
                        $$.emplace_back(new ast::ThreadsafeVar($1));
                    }
                |   threadsafelist "," NAME_PTR
                    {
                        $1.emplace_back(new ast::ThreadsafeVar($3));
                        $$ = $1;
                    }
                ;


valence         :   { $$ = nullptr; }
                |   VALENCE real
                    {
                        $$ = new ast::Valence($1.clone(), $2);
                    }
                |   VALENCE "-" real
                    {
                        $3->negate();
                        $$ = new ast::Valence($1.clone(), $3);
                    }
                ;


 INTEGER_PTR    :   INTEGER
                    {
                        $$ = $1.clone();
                    }
                 ;


 NAME_PTR        :  NAME
                    {
                        $$ = $1.clone();
                    }
                 ;


 STRING_PTR      :  STRING
                    {
                        $$ = $1.clone();
                    }
                 ;

 flux_variable   :  FLUX_VAR
                    {
                        $$ = new ast::WrappedExpression($1.clone());
                    }
                 ;
%%


/** Parse verbatim and commnet blocks : In the future we will use C parser to
 *  analyze verbatim blocks for better code generation. For GPU code generation
 *  we need to disable printf like statements. We could add new pragma
 *  annotations to enable certain optimizations. Idea of having separate parser
 *  is to support this type of analysis in separate parser. Currently we have
 *  "empty" Verbatim parser which scan and return same string. */

std::string parse_with_verbatim_parser(std::string str) {
    auto is = new std::istringstream(str.c_str());

    VerbatimDriver extcontext(is);
    Verbatim_parse(&extcontext);

    std::string ss(*(extcontext.result));

    delete is;
    return ss;
}

/** Bison expects error handler for parser.
 *  \todo Need to implement error codes and driver should accumulate
 *  and report all errors. For now simply abort.
 */

void NmodlParser::error(const location &loc , const std::string &msg) {
    std::stringstream ss;
    ss << "NMODL Parser Error : " << msg << " [Location : " << loc << "]";
    throw std::runtime_error(ss.str());
}
