/******************************************************************************
 *
 * @brief Bison grammar and parser implementation for NMODL
 *
 * This implementation is based NEURON's nocmodl program. The grammar rules,
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

/** enable location tracking */
%locations

/** add extra arguments to yyparse() and yylexe() methods */
%parse-param {class Lexer& scanner}
%parse-param {class Driver& driver}
%lex-param { nmodl::Scanner &scanner }
%lex-param { nmodl::Driver &driver }

/** use variant based implementation of semantic values */
%define api.value.type variant

/** assert correct cleanup of semantic value objects */
%define parse.assert

/** handle symbol to be handled as a whole (type, value, and possibly location) in scanner */
%define api.token.constructor

/** namespace to enclose parser */
%name-prefix "nmodl"

/** set the parser's class identifier */
%define parser_class_name {Parser}

/** keep track of the current position within the input */
%locations

/** Initializations before parsing : Use filename from driver to initialize location object */
%initial-action
{
    @$.begin.filename = @$.end.filename = &driver.streamname;
};

/** Tokens for lexer : we return ModToken object from lexer. This is useful when
 *  we want to access original string and location. With C++ interface and other
 *  location related information, this is now less useful in parser. But when we
 *  lexer executable or tests, it's useful to return ModToken. Note that UNKNOWN
 *  token is added for convenience (with default argumebts). */

%token  <ModToken>              MODEL
%token  <ModToken>              CONSTANT
%token  <ModToken>              INDEPENDENT
%token  <ModToken>              DEPENDENT
%token  <ModToken>              STATE
%token  <ModToken>              INITIAL1
%token  <ModToken>              DERIVATIVE
%token  <ModToken>              SOLVE
%token  <ModToken>              USING
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
%token  <ModToken>              LAG RESET
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
%token  <ast::double_ptr>       REAL
%token  <ast::integer_ptr>      INTEGER
%token  <ast::integer_ptr>      DEFINEDVAR
%token  <ast::name_ptr>         NAME
%token  <ast::name_ptr>         METHOD
%token  <ast::name_ptr>         SUFFIX
%token  <ast::name_ptr>         VALENCE
%token  <ast::name_ptr>         DEL
%token  <ast::name_ptr>         DEL2
%token  <ast::primename_ptr>    PRIME
%token  <std::string>           VERBATIM
%token  <std::string>           COMMENT
%token  <std::string>           LINE_PART
%token  <ast::string_ptr>       STRING
%token  <ast::string_ptr>       UNIT_STR
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

/** Define terminal and nonterminal symbols : Instead of using AST classes
 *  directly, we are using typedefs like program_ptr. This is useful when we
 *  want to transparently change naming/type scheme. For example, raw pointer
 *  to smakrt pointers. Manually changing all types in bison specification is
 *  time consuming. Also, naming of termina/non-terminal symbols is kept same
 *  as NEURON. This helps during development and debugging. This could be
 *  once all implementation details get ported. */

%type   <ast::program_ptr>                  all
%type   <ast::name_ptr>                     Name
%type   <ast::number_ptr>                   NUMBER
%type   <ast::double_ptr>                   real
%type   <ast::expression_ptr>               intexpr
%type   <ast::integer_ptr>                  integer
%type   <ast::model_ptr>                    model
%type   <ast::unit_ptr>                     units
%type   <ast::integer_ptr>                  optindex
%type   <ast::unit_ptr>                     unit
%type   <ast::block_ptr>                    proc
%type   <ast::limits_ptr>                   limits
%type   <ast::double_ptr>                   abstol
%type   <ast::identifier_ptr>               name
%type   <ast::number_ptr>                   number
%type   <ast::expression_ptr>               primary
%type   <ast::expression_ptr>               term
%type   <ast::expression_ptr>               leftlinexpr
%type   <ast::expression_ptr>               linexpr
%type   <ast::number_list>                  numlist
%type   <ast::expression_ptr>               expr
%type   <ast::expression_ptr>               aexpr
%type   <ast::statement_ptr>                ostmt
%type   <ast::statement_ptr>                astmt
%type   <ast::statementblock_ptr>           stmtlist
%type   <ast::localliststatement_ptr>       locallist
%type   <ast::localvar_list>                locallist1
%type   <ast::varname_ptr>                  varname
%type   <ast::expression_list>              exprlist
%type   <ast::define_ptr>                   define1
%type   <ast::queuestatement_ptr>           queuestmt
%type   <ast::expression_ptr>               asgn
%type   <ast::fromstatement_ptr>            fromstmt
%type   <ast::whilestatement_ptr>           whilestmt
%type   <ast::ifstatement_ptr>              ifstmt
%type   <ast::elseifstatement_list>         optelseif
%type   <ast::elsestatement_ptr>            optelse
%type   <ast::solveblock_ptr>               solveblk
%type   <ast::functioncall_ptr>             funccall
%type   <ast::statementblock_ptr>           ifsolerr
%type   <ast::expression_ptr>               opinc
%type   <ast::number_ptr>                   opstart
%type   <ast::varname_list>                 senslist
%type   <ast::sens_ptr>                     sens
%type   <ast::lagstatement_ptr>             lagstmt
%type   <ast::forallstatement_ptr>          forallstmt
%type   <ast::paramassign_ptr>              parmasgn
%type   <ast::stepped_ptr>                  stepped
%type   <ast::independentdef_ptr>           indepdef
%type   <ast::dependentdef_ptr>             depdef
%type   <ast::block_ptr>                    declare
%type   <ast::paramblock_ptr>               parmblk
%type   <ast::paramassign_list>             parmbody
%type   <ast::independentblock_ptr>         indepblk
%type   <ast::independentdef_list>          indepbody
%type   <ast::dependentblock_ptr>           depblk
%type   <ast::dependentdef_list>            depbody
%type   <ast::stateblock_ptr>               stateblk
%type   <ast::stepblock_ptr>                stepblk
%type   <ast::stepped_list>                 stepbdy
%type   <ast::watchstatement_ptr>           watchstmt
%type   <ast::binaryoperator_ptr>           watchdir
%type   <ast::watch_ptr>                    watch1
%type   <ast::fornetcon_ptr>                fornetcon
%type   <ast::plotdeclaration_ptr>          plotdecl
%type   <ast::plotvar_list>                 pvlist
%type   <ast::constantblock_ptr>            constblk
%type   <ast::constantstatement_list>       conststmt
%type   <ast::matchblock_ptr>               matchblk
%type   <ast::match_list>                   matchlist
%type   <ast::match_ptr>                    match
%type   <ast::identifier_ptr>               matchname
%type   <ast::partialboundary_ptr>          pareqn
%type   <ast::firstlasttypeindex_ptr>       firstlast
%type   <ast::reactionstatement_ptr>        reaction
%type   <ast::conserve_ptr>                 conserve
%type   <ast::expression_ptr>               react
%type   <ast::compartment_ptr>              compart
%type   <ast::ldifuse_ptr>                  ldifus
%type   <ast::name_list>                    namelist
%type   <ast::unitblock_ptr>                unitblk
%type   <ast::expression_list>              unitbody
%type   <ast::unitdef_ptr>                  unitdef
%type   <ast::factordef_ptr>                factordef
%type   <ast::name_list>                    solvefor
%type   <ast::name_list>                    solvefor1
%type   <ast::unitstate_ptr>                uniton
%type   <ast::name_list>                    tablst
%type   <ast::name_list>                    tablst1
%type   <ast::tablestatement_ptr>           tablestmt
%type   <ast::name_list>                    dependlst
%type   <ast::argument_list>                arglist
%type   <ast::argument_list>                arglist1
%type   <ast::integer_ptr>                  locoptarray
%type   <ast::neuronblock_ptr>              neuronblk
%type   <ast::useion_ptr>                   nrnuse
%type   <ast::statement_list>               nrnstmt
%type   <ast::readionvar_list>              nrnionrlist
%type   <ast::writeionvar_list>             nrnionwlist
%type   <ast::nonspecurvar_list>            nrnonspeclist
%type   <ast::electrodecurvar_list>         nrneclist
%type   <ast::sectionvar_list>              nrnseclist
%type   <ast::rangevar_list>                nrnrangelist
%type   <ast::globalvar_list>               nrnglobalist
%type   <ast::pointervar_list>              nrnptrlist
%type   <ast::bbcorepointervar_list>        nrnbbptrlist
%type   <ast::externvar_list>               nrnextlist
%type   <ast::threadsafevar_list>           opthsafelist
%type   <ast::threadsafevar_list>           threadsafelist
%type   <ast::valence_ptr>                  valence
%type   <ast::expressionstatement_ptr>      initstmt
%type   <ast::bablock_ptr>                  bablk
%type   <ast::conductancehint_ptr>          conducthint
%type   <ast::statement_list>               stmtlist1
%type   <ast::initialblock_ptr>             initblk
%type   <ast::constructorblock_ptr>         constructblk
%type   <ast::destructorblock_ptr>          destructblk
%type   <ast::functionblock_ptr>            funcblk
%type   <ast::kineticblock_ptr>             kineticblk
%type   <ast::breakpointblock_ptr>          brkptblk
%type   <ast::derivativeblock_ptr>          derivblk
%type   <ast::linearblock_ptr>              linblk
%type   <ast::nonlinearblock_ptr>           nonlinblk
%type   <ast::procedureblock_ptr>           procedblk
%type   <ast::netreceiveblock_ptr>          netrecblk
%type   <ast::terminalblock_ptr>            terminalblk
%type   <ast::discreteblock_ptr>            discretblk
%type   <ast::partialblock_ptr>             partialblk
%type   <ast::functiontableblock_ptr>       functableblk

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
    #include "parser/verbatim_context.hpp"

    /// yylex takes scanner as well as driver reference
    static nmodl::Parser::symbol_type yylex(nmodl::Lexer &scanner, nmodl::Driver &driver) {
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
 *  classes to handle this. */

top             :   all
                    {
                        driver.astRoot = std::shared_ptr<ast::Program>($1);
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
                        $1->addStatement($2);
                        $$ = $1;
                    }
                |   all locallist
                    {
                        $1->addStatement($2);
                        $$ = $1;
                    }
                |   all define1
                    {
                        $1->addStatement($2);
                        $$ = $1;
                    }
                |   all declare
                    {
                        $1->addBlock($2);
                        $$ = $1;
                    }
                |   all MODEL_LEVEL INTEGER declare
                    {
                        /** todo : This is discussed with Michael Hines. Model level was inserted
                         * by merge program which is no longer exist. This was to avoid the name
                         * collision in case of include. Idea was to have some kind of namespace!
                         */
                    }
                |   all proc
                    {
                        $1->addBlock($2);
                        $$ = $1;
                    }
                |   all VERBATIM
                    {
                        auto text = parse_with_verbatim_parser($2);
                        auto statement = new ast::Verbatim(new ast::String(text));
                        $1->addStatement(statement);
                        $$ = $1;
                    }
                |   all COMMENT
                    {
                        auto text = parse_with_verbatim_parser($2);
                        auto statement = new ast::Comment(new ast::String(text));
                        $1->addStatement(statement);
                        $$ = $1;
                    }
                |   all uniton
                    {
                        $1->addStatement($2);
                        $$ = $1;
                    }
                |   all INCLUDE1 STRING
                    {
                        $1->addStatement(new ast::Include($3));
                        $$ = $1;
                    }
                ;


model           :   MODEL LINE_PART
                    {
                        $$ = new ast::Model(new ast::String($2));
                    }
                ;


define1         :   DEFINE1 NAME INTEGER
                    {
                        $$ = new ast::Define($2, $3);
                        driver.add_defined_var($2->getTypeName(), $3->eval());
                    }
                |   DEFINE1 error
                    {
                        error(scanner.loc, "define1");
                    }
                ;


Name            :   NAME        {   $$ = $1;    }
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
                        $1.push_back(std::shared_ptr<ast::ParamAssign>($2));
                        $$ = $1;
                    }
                ;


parmasgn        :   NAME "=" number units limits
                    {
                        $$ = new ast::ParamAssign($1, $3, $4, $5);
                    }
                |   NAME units limits
                    {
                        $$ = new ast::ParamAssign($1, NULL, $2, $3);
                    }
                |   NAME "[" integer "]" units limits
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
                        $$ = new ast::Unit(scanner.get_unit());
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
                            $1.push_back(std::shared_ptr<ast::Stepped>($2));
                            $$ = $1;
                    }
                ;


stepped         :   NAME "=" numlist units
                    {
                        $$ = new ast::Stepped($1, $3, $4);
                    }
                ;


numlist         :   number "," number
                    {
                        $$ = ast::NumberVector();
                        $$.push_back(std::shared_ptr<ast::Number>($1));
                        $$.push_back(std::shared_ptr<ast::Number>($3));
                    }
                |   numlist "," number
                    {
                        $1.push_back(std::shared_ptr<ast::Number>($3));
                        $$ = $1;
                    }
                ;


name            :   Name    {   $$ = $1;    }
                |   PRIME   {   $$ = $1;    }
                ;


number          :   NUMBER  {   $$ = $1;    }
                |   "-" NUMBER
                    {
                        $2->negate();
                        $$ = $2;
                    }
                ;


NUMBER          :   integer     {   $$ = $1;    }
                |   REAL        {   $$ = $1;    }
                ;


integer         :   INTEGER     {   $$ = $1;    }
                |   DEFINEDVAR  {   $$ = $1;    }
                ;


real            :   REAL        {   $$ = $1;    }
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
                        $1.push_back(std::shared_ptr<ast::IndependentDef>($2));
                        $$ = $1;
                    }
                |   indepbody SWEEP indepdef
                    {
                        $1.push_back(std::shared_ptr<ast::IndependentDef>($3));
                        $3->sweep = std::shared_ptr<ast::Boolean>(new ast::Boolean(1));
                        $$ = $1;
                    }
                ;


indepdef        :   NAME FROM number TO number withby integer opstart units
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
                        $1.push_back(std::shared_ptr<ast::DependentDef>($2));
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
                        $$.push_back(std::shared_ptr<ast::PlotVar>(variable));
                    }
                |   pvlist "," name optindex
                    {
                        $$ = $1;
                        auto variable = new ast::PlotVar($3, $4);
                        $$.push_back(std::shared_ptr<ast::PlotVar>(variable));
                    }
                ;


optindex        :                       {   $$ = nullptr;  }
                |   "[" INTEGER "]"     {   $$ = $2;    }
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
                        $$->setToken($1);
                    }
                |   "{" locallist stmtlist1
                    {
                        $3.insert($3.begin(), std::shared_ptr<ast::LocalListStatement>($2));
                        $$ = new ast::StatementBlock($3);
                        $$->setToken($1);
                    }
                ;


conducthint     :   CONDUCTANCE Name
                    {
                        $$ = new ast::ConductanceHint($2, NULL);
                    }
                |   CONDUCTANCE Name USEION NAME
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


locallist1      :   NAME locoptarray
                    {
                        $$ = ast::LocalVarVector();
                        if($2) {
                            auto variable = new ast::LocalVar(new ast::IndexedName($1, $2));
                            $$.push_back(std::shared_ptr<ast::LocalVar>(variable));
                        } else {
                            auto variable = new ast::LocalVar($1);
                            $$.push_back(std::shared_ptr<ast::LocalVar>(variable));
                        }
                    }
                |   locallist1 "," NAME locoptarray
                    {
                        if($4) {
                            auto variable = new ast::LocalVar(new ast::IndexedName($3, $4));
                            $1.push_back(std::shared_ptr<ast::LocalVar>(variable));
                        } else {
                            auto variable = new ast::LocalVar($3);
                            $1.push_back(std::shared_ptr<ast::LocalVar>(variable));
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
                        $1.push_back(std::shared_ptr<ast::Statement>($2));
                        $$ = $1;
                    }
                |   stmtlist1 astmt
                    {
                        $1.push_back(std::shared_ptr<ast::Statement>($2));
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
                |   COMMENT
                    {   auto text = parse_with_verbatim_parser($1);
                        $$ = new ast::Comment(new ast::String(text));
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
                        $$ = new ast::BinaryExpression($1, ast::BinaryOperator(ast::BOP_ASSIGN), $3);
                    }
                |   nonlineqn expr "=" expr
                    {
                        $$ = new ast::NonLinEuation($2, $4);
                    }
                |   lineqn leftlinexpr "=" linexpr
                    {
                        $$ = new ast::LinEquation($2, $4);
                    }
                ;


varname         :   name
                    {
                        $$ = new ast::VarName($1, NULL);
                    }
                |   name "[" intexpr "]"
                    {
                        $$ = new ast::VarName(new ast::IndexedName($1, $3), NULL);
                    }
                |   NAME "@" integer
                    {
                        $$ = new ast::VarName($1, $3);
                    }
                |   NAME "@" integer "[" intexpr "]"
                    {
                        $$ = new ast::VarName(new ast::IndexedName($1, $5), $3);
                    }
                ;


intexpr         :   Name                    { $$ = $1; }
                |   integer                 { $$ = $1; }
                |   "(" intexpr ")"         { $$ = $2; }
                |   intexpr "+" intexpr
                    {
                        $$ = new ast::BinaryExpression($1, ast::BinaryOperator(ast::BOP_ADDITION), $3);
                    }
                |   intexpr "-" intexpr
                    {
                        $$ = new ast::BinaryExpression($1, ast::BinaryOperator(ast::BOP_SUBTRACTION), $3);
                    }
                |   intexpr "*" intexpr
                    {
                        $$ = new ast::BinaryExpression($1, ast::BinaryOperator(ast::BOP_MULTIPLICATION), $3);
                    }
                |   intexpr "/" intexpr
                    {
                        $$ = new ast::BinaryExpression($1, ast::BinaryOperator(ast::BOP_DIVISION), $3);
                    }
                |   error
                    {

                    }
                ;


expr            :   varname             { $$ = $1; }
                |   real units
                    {
                        if($2)
                            $$ = new ast::DoubleUnit($1, $2);
                        else
                            $$ = $1;
                    }
                |   funccall            { $$ = $1; }
                |   "(" expr ")"        { $$ = $2; }
                |   expr "+" expr
                    {
                        $$ = new ast::BinaryExpression($1, ast::BinaryOperator(ast::BOP_ADDITION), $3);
                    }
                |   expr "-" expr
                    {
                        $$ = new ast::BinaryExpression($1, ast::BinaryOperator(ast::BOP_SUBTRACTION), $3);
                    }
                |   expr "*" expr
                    {
                        $$ = new ast::BinaryExpression($1, ast::BinaryOperator(ast::BOP_MULTIPLICATION), $3);
                    }
                |   expr "/" expr
                    {
                        $$ = new ast::BinaryExpression($1, ast::BinaryOperator(ast::BOP_DIVISION), $3);
                    }
                |   expr "^" expr
                    {
                        $$ = new ast::BinaryExpression($1, ast::BinaryOperator(ast::BOP_POWER), $3);
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
                |   "(" expr ")"    { $$ = $2; }
                |   error
                    {
                        error(scanner.loc, "term");
                    }
                ;


funccall        :   NAME "(" exprlist ")"
                    {
                        $$ = new ast::FunctionCall($1, $3);
                    }
                ;


exprlist        :   {
                        $$ = ast::ExpressionVector();
                    }
                |   expr
                    {
                        $$ = ast::ExpressionVector();
                        $$.push_back(std::shared_ptr<ast::Expression>($1));
                    }
                |   STRING
                    {
                        $$ = ast::ExpressionVector();
                        $$.push_back(std::shared_ptr<ast::Expression>($1));
                    }
                |   exprlist "," expr
                    {
                        $1.push_back(std::shared_ptr<ast::Expression>($3));
                        $$ = $1;
                    }
                |   exprlist "," STRING
                    {
                        $1.push_back(std::shared_ptr<ast::Expression>($3));
                        $$ = $1;
                    }
                ;


fromstmt        :   FROM NAME "=" intexpr TO intexpr opinc stmtlist "}"
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


forallstmt      :   FORALL1 NAME stmtlist "}"
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
                        auto statement = new ast::ElseIfStatement($5, $7);
                        $1.push_back(std::shared_ptr<ast::ElseIfStatement>(statement));
                        $$ = $1;
                    }
                ;


optelse         :   { $$ = nullptr; }
                |   ELSE stmtlist "}"
                    {
                        $$ = new ast::ElseStatement($2);
                    }
                ;


derivblk        :   DERIVATIVE NAME stmtlist "}"
                    {
                        $$ = new ast::DerivativeBlock($2, $3);
                        $$->setToken($1);
                    }
                ;


linblk          :   LINEAR NAME solvefor stmtlist "}"
                    {
                        $$ = new ast::LinearBlock($2, $3, $4);
                        $$->setToken($1);
                    }
                ;


nonlinblk       :   NONLINEAR NAME solvefor stmtlist "}"
                    {
                        $$ = new ast::NonLinearBlock($2, $3, $4);
                        $$->setToken($1);
                    }
                ;


discretblk      :   DISCRETE NAME stmtlist "}"
                    {
                        $$ = new ast::DiscreteBlock($2, $3);
                        $$->setToken($1);
                    }
                ;


partialblk      :   PARTIAL NAME stmtlist "}"
                    {
                        $$ = new ast::PartialBlock($2, $3);
                        $$->setToken($1);
                    }
                |   PARTIAL error
                    {
                        error(scanner.loc, "partialblk");
                    }
                ;


pareqn          :   "~" PRIME "=" NAME "*" DEL2 "(" NAME ")" "+" NAME
                    {
                        $$ = new ast::PartialBoundary(NULL, $2, NULL, NULL, $4, $6, $8, $11);
                    }
                |   "~" DEL NAME "[" firstlast "]" "=" expr
                    {
                        $$ = new ast::PartialBoundary($2, $3, $5, $8, NULL, NULL, NULL, NULL);
                    }
                |   "~" NAME "[" firstlast "]" "=" expr
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


functableblk    :   FUNCTION_TABLE NAME "(" arglist ")" units
                    {
                        $$ = new ast::FunctionTableBlock($2, $4, $6);
                        $$->setToken($1);
                    }
                ;


funcblk         :   FUNCTION1 NAME "(" arglist ")" units stmtlist "}"
                    {
                        $$ = new ast::FunctionBlock($2, $4, $6, $7);
                        $$->setToken($1);
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
                        $$.push_back(std::shared_ptr<ast::Argument>(new ast::Argument($1, $2)));
                    }
                |   arglist1 "," name units
                    {
                        $1.push_back(std::shared_ptr<ast::Argument>(new ast::Argument($3, $4)));
                        $$ = $1;
                    }
                ;


procedblk       :   PROCEDURE NAME "(" arglist ")" units stmtlist "}"
                    {
                            $$ = new ast::ProcedureBlock($2, $4, $6, $7); $$->setToken($1);
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


solveblk        :   SOLVE NAME ifsolerr
                    {
                        $$ = new ast::SolveBlock($2, NULL, $3);
                    }
                |   SOLVE NAME USING METHOD ifsolerr
                    {
                        $$ = new ast::SolveBlock($2, $4, $5);
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


solvefor1       :   SOLVEFOR NAME
                    {
                        $$ = ast::NameVector();
                        $$.push_back(std::shared_ptr<ast::Name>($2));
                    }
                |   solvefor1 "," NAME
                    {
                        $1.push_back(std::shared_ptr<ast::Name>($3));
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
                |   "(" aexpr ")"           { $$ = $2; }
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
                        $$.push_back(std::shared_ptr<ast::VarName>($1));
                    }
                |   senslist "," varname
                    {
                        $1.push_back(std::shared_ptr<ast::VarName>($3));
                        $$ = $1;
                    }
                ;


conserve        :   CONSERVE react "=" expr
                    {
                        $$ = new ast::Conserve($2, $4);
                    }
                |   CONSERVE error
                    {

                    }
                ;


compart         :   COMPARTMENT NAME "," expr "{" namelist "}"
                    {
                        $$ = new ast::Compartment($2, $4, $6);
                    }
                |   COMPARTMENT expr "{" namelist "}"
                    {
                        $$ = new ast::Compartment(NULL, $2, $4);
                    }
                ;


ldifus          :   LONGDIFUS NAME "," expr "{" namelist "}"
                    {
                        $$ = new ast::LDifuse($2, $4, $6);
                    }
                |   LONGDIFUS expr "{" namelist "}"
                    {
                        $$ = new ast::LDifuse(NULL, $2, $4);
                    }
                ;


namelist        :   NAME
                    {
                        $$ = ast::NameVector();
                        $$.push_back(std::shared_ptr<ast::Name>($1));
                    }
                |   namelist NAME
                    {
                        $1.push_back(std::shared_ptr<ast::Name>($2));
                        $$ = $1;
                    }
                ;


kineticblk      :   KINETIC NAME solvefor stmtlist "}"
                    {
                        $$ = new ast::KineticBlock($2, $3, $4);
                        $$->setToken($1);
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


react           :   varname     { $$ = $1; }
                |   integer varname
                    {
                        $$ = new ast::ReactVarName($1, $2);
                    }
                |   react "+" varname
                    {
                        auto op = ast::BinaryOperator(ast::BOP_ADDITION);
                        $$ = new ast::BinaryExpression($1, op, $3);
                    }
                |   react "+" integer varname
                    {
                        auto op = ast::BinaryOperator(ast::BOP_ADDITION);
                        auto variable = new ast::ReactVarName($3, $4);
                        $$ = new ast::BinaryExpression($1, op, variable);
                    }
                ;


lagstmt         :   LAG name BY NAME
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
                        $$.push_back(std::shared_ptr<ast::Match>($1));
                    }
                |   matchlist match
                    {
                        $1.push_back(std::shared_ptr<ast::Match>($2));
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
                        auto expr = new ast::BinaryExpression($3, op, $6);
                        $$ = new ast::Match($1, expr);
                    }
                |   error
                    {
                        error(scanner.loc, "match ");
                    }
                ;


matchname       :   name    { $$ = $1; }
                |   name "[" NAME "]"
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
                        $1.push_back(std::shared_ptr<ast::Expression>($2));
                        $$ = $1;
                    }
                |   unitbody factordef
                    {
                        $1.push_back(std::shared_ptr<ast::Expression>($2));
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


factordef       :   NAME "=" real unit
                    {
                        $$ = new ast::FactorDef($1, $3, $4, NULL, NULL);
                        $$->setToken(*($1->getToken()));
                    }
                |   NAME "=" unit unit
                    {
                        $$ = new ast::FactorDef($1, NULL, $3, NULL, $4);
                        $$->setToken(*($1->getToken()));
                    }
                |   NAME "=" unit "-" GT unit
                    {
                        $$ = new ast::FactorDef($1, NULL, $3, new ast::Boolean(1), $6);
                        $$->setToken(*($1->getToken()));
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
                |   conststmt NAME "=" number units
                    {
                        auto statement = new ast::ConstantStatement($2, $4, $5);
                        $1.push_back(std::shared_ptr<ast::ConstantStatement>(statement));
                        $$ = $1;
                    }
                ;


tablestmt       :   TABLE tablst dependlst FROM expr TO expr WITH INTEGER
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
                        $$.push_back(std::shared_ptr<ast::Name>($1));
                    }
                |   tablst1 "," Name
                    {
                        $1.push_back(std::shared_ptr<ast::Name>($3));
                        $$ = $1;
                    }
                ;


dependlst       :                       { $$ = ast::NameVector(); }
                |   DEPEND tablst1      { $$ = $2; }
                ;


neuronblk       :   NEURON OPEN_BRACE nrnstmt CLOSE_BRACE
                    {
                        auto block = new ast::StatementBlock($3);
                        block->setToken($2);
                        $$ = new ast::NeuronBlock(block);
                    }
                ;


nrnstmt         :   { $$ = ast::StatementVector(); }
                |   nrnstmt SUFFIX NAME
                    {
                        auto statement = new ast::Suffix($2, $3);
                        $1.push_back(std::shared_ptr<ast::Statement>(statement));
                        $$ = $1;
                    }
                |   nrnstmt nrnuse
                    {
                        $1.push_back(std::shared_ptr<ast::Statement>($2));
                        $$ = $1;
                    }
                |   nrnstmt NONSPECIFIC nrnonspeclist
                    {
                        auto statement = new ast::Nonspecific($3);
                        $1.push_back(std::shared_ptr<ast::Statement>(statement));
                        $$ = $1;
                    }
                |   nrnstmt ELECTRODE_CURRENT nrneclist
                    {
                        auto statement = new ast::ElctrodeCurrent($3);
                        $1.push_back(std::shared_ptr<ast::Statement>(statement));
                        $$ = $1;
                    }
                |   nrnstmt SECTION nrnseclist
                    {
                        auto statement = new ast::Section($3);
                        $1.push_back(std::shared_ptr<ast::Statement>(statement));
                        $$ = $1;
                    }
                |   nrnstmt RANGE nrnrangelist
                    {
                        auto statement = new ast::Range($3);
                        $1.push_back(std::shared_ptr<ast::Statement>(statement));
                        $$ = $1;
                    }
                |   nrnstmt GLOBAL nrnglobalist
                    {
                        auto statement = new ast::Global($3);
                        $1.push_back(std::shared_ptr<ast::Statement>(statement));
                        $$ = $1;
                    }
                |   nrnstmt POINTER nrnptrlist
                    {
                        auto statement = new ast::Pointer($3);
                        $1.push_back(std::shared_ptr<ast::Statement>(statement));
                        $$ = $1;
                    }
                |   nrnstmt BBCOREPOINTER nrnbbptrlist
                    {
                        auto statement = new ast::BbcorePtr($3);
                        $1.push_back(std::shared_ptr<ast::Statement>(statement));
                        $$ = $1;
                    }
                |   nrnstmt EXTERNAL nrnextlist
                    {
                        auto statement = new ast::External($3);
                        $1.push_back(std::shared_ptr<ast::Statement>(statement));
                        $$ = $1;
                    }
                |   nrnstmt THREADSAFE opthsafelist
                    {
                        auto statement = new ast::ThreadSafe($3);
                        $1.push_back(std::shared_ptr<ast::Statement>(statement));
                        $$ = $1;
                    }
                ;


nrnuse          :   USEION NAME READ nrnionrlist valence
                    {
                        $$ = new ast::Useion($2, $4, ast::WriteIonVarVector(), $5);
                    }
                |   USEION NAME WRITE nrnionwlist valence
                    {
                        $$ = new ast::Useion($2, ast::ReadIonVarVector(), $4, $5);
                    }
                |   USEION NAME READ nrnionrlist WRITE nrnionwlist valence
                    {
                        $$ = new ast::Useion($2, $4, $6, $7);
                    }
                |   USEION error
                    {
                        error(scanner.loc, "nrnuse");
                    }
                ;


nrnionrlist     :   NAME
                    {
                        $$ = ast::ReadIonVarVector();
                        $$.push_back(std::shared_ptr<ast::ReadIonVar>(new ast::ReadIonVar($1)));
                    }
                |   nrnionrlist "," NAME
                    {
                        $1.push_back(std::shared_ptr<ast::ReadIonVar>(new ast::ReadIonVar($3)));
                        $$ = $1;
                    }
                |   error
                    {
                        error(scanner.loc, "nrnionrlist");
                    }
                ;


nrnionwlist     :   NAME
                    {
                        $$ = ast::WriteIonVarVector();
                        $$.push_back(std::shared_ptr<ast::WriteIonVar>(new ast::WriteIonVar($1)));
                    }
                |   nrnionwlist "," NAME
                    {
                        $1.push_back(std::shared_ptr<ast::WriteIonVar>(new ast::WriteIonVar($3)));
                        $$ = $1;
                    }
                |   error
                    {
                        error(scanner.loc, "nrnionwlist");
                    }
                ;


nrnonspeclist   :   NAME
                    {
                        $$ = ast::NonspeCurVarVector();
                        auto var = new ast::NonspeCurVar($1);
                        $$.push_back(std::shared_ptr<ast::NonspeCurVar>(var));
                    }
                |   nrnonspeclist "," NAME
                    {
                        auto var = new ast::NonspeCurVar($3);
                        $1.push_back(std::shared_ptr<ast::NonspeCurVar>(var));
                        $$ = $1;
                    }
                |   error
                    {
                        error(scanner.loc, "nrnonspeclist");
                    }
                ;


nrneclist       :   NAME
                    {
                        $$ = ast::ElectrodeCurVarVector();
                        auto var = new ast::ElectrodeCurVar($1);
                        $$.push_back(std::shared_ptr<ast::ElectrodeCurVar>(var));
                    }
                |   nrneclist "," NAME
                    {
                        auto var = new ast::ElectrodeCurVar($3);
                        $1.push_back(std::shared_ptr<ast::ElectrodeCurVar>(var));
                        $$ = $1;
                    }
                |   error
                    {
                        error(scanner.loc, "nrneclist");
                    }
                ;


nrnseclist      :   NAME
                    {
                        $$ = ast::SectionVarVector();
                        auto var = new ast::SectionVar($1);
                        $$.push_back(std::shared_ptr<ast::SectionVar>(var));
                    }
                |   nrnseclist "," NAME
                    {
                        auto var = new ast::SectionVar($3);
                        $1.push_back(std::shared_ptr<ast::SectionVar>(var));
                        $$ = $1;
                    }
                |   error
                    {
                        error(scanner.loc, "nrnseclist");
                    }
                ;


nrnrangelist    :   NAME
                    {
                        $$ = ast::RangeVarVector();
                        $$.push_back(std::shared_ptr<ast::RangeVar>(new ast::RangeVar($1)));
                    }
                |   nrnrangelist "," NAME
                    {
                        $1.push_back(std::shared_ptr<ast::RangeVar>(new ast::RangeVar($3)));
                        $$ = $1;
                    }
                |   error
                    {
                        error(scanner.loc, "nrnrangelist");
                    }
                ;


nrnglobalist    :   NAME
                    {
                        $$ = ast::GlobalVarVector();
                        $$.push_back(std::shared_ptr<ast::GlobalVar>(new ast::GlobalVar($1)));
                    }
                |   nrnglobalist "," NAME
                    {
                        $1.push_back(std::shared_ptr<ast::GlobalVar>(new ast::GlobalVar($3)));
                        $$ = $1;
                    }
                |   error
                    {
                        error(scanner.loc, "nrnglobalist");
                    }
                ;


nrnptrlist      :   NAME
                    {
                        $$ = ast::PointerVarVector();
                        auto var = new ast::PointerVar($1);
                        $$.push_back(std::shared_ptr<ast::PointerVar>(var));
                    }
                |   nrnptrlist "," NAME
                    {
                        auto var = new ast::PointerVar($3);
                        $1.push_back(std::shared_ptr<ast::PointerVar>(var));
                        $$ = $1;
                    }
                |   error
                    {
                        error(scanner.loc, "nrnptrlist");
                    }
                ;


nrnbbptrlist    :   NAME
                    {
                        $$ = ast::BbcorePointerVarVector();
                        auto var = new ast::BbcorePointerVar($1);
                        $$.push_back(std::shared_ptr<ast::BbcorePointerVar>(var));
                    }
                |   nrnbbptrlist "," NAME
                    {
                        auto var = new ast::BbcorePointerVar($3);
                        $1.push_back(std::shared_ptr<ast::BbcorePointerVar>(var));
                        $$ = $1;
                    }
                |   error
                    {
                        error(scanner.loc, "nrnbbptrlist");
                    }
                ;


nrnextlist      :   NAME
                    {
                        $$ = ast::ExternVarVector();
                        auto var = new ast::ExternVar($1);
                        $$.push_back(std::shared_ptr<ast::ExternVar>(var));
                    }
                |   nrnextlist "," NAME
                    {
                        auto var = new ast::ExternVar($3);
                        $1.push_back(std::shared_ptr<ast::ExternVar>(var));
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


threadsafelist  :   NAME
                    {
                        $$ = ast::ThreadsafeVarVector();
                        auto var = new ast::ThreadsafeVar($1);
                        $$.push_back(std::shared_ptr<ast::ThreadsafeVar>(var));
                    }
                |   threadsafelist "," NAME
                    {
                        auto var = new ast::ThreadsafeVar($3);
                        $1.push_back(std::shared_ptr<ast::ThreadsafeVar>(var));
                        $$ = $1;
                    }
                ;


valence         :   { $$ = nullptr; }
                |   VALENCE real
                    {
                        $$ = new ast::Valence($1, $2);
                    }
                |   VALENCE "-" real
                    {
                        $3->negate();
                        $$ = new ast::Valence($1, $3);
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

    VerbatimContext extcontext(is);
    Verbatim_parse(&extcontext);

    std::string ss(*(extcontext.result));

    delete is;
    return ss;
}

/** Bison expects error handler for parser.
 *  \todo Need to implement error codes and driver should accumulate
 *  and report all errors. For now simply abort.
 */

void nmodl::Parser::error(const location &loc , const std::string &message) {
    std::stringstream ss;
    ss << "NMODL Parser Error : " << message << " [Location : " << loc << "]";
    throw std::runtime_error(ss.str());
}
