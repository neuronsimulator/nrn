/* Bison specification for NMODL Parser
 *
 * This is based on original NOCMODL implementation and similar
 * to most of the open source tools implementation. There are C++
 * based parser examples (e.g.
 * http://panthema.net/2007/flex-bison-cpp-example/) but we are
 * avoiding it because they are somewhat difficult to understand
 * and we have seen very few projects using flex/bison with C++.
 */

%{

    #include <cstdio>
    #include <cstdlib>
    #include <iostream>
    #include <cstring>
    #include "lexer/modtoken.hpp"
    #include "lexer/lexer_utils.hpp"
    #include "parser/nmodl_context.hpp"
    #include "utils/stringutils.hpp"
    #include "ast/ast.hpp"

    /* root of the ast */
    //ast::ProgramNode *astRoot;

    /* a macro that extracts the scanner state from the parser state for yylex */
    #define scanner context->scanner

%}

/* print out verbose error instead of just message 'syntax error' */
%error-verbose

/* make a reentrant parser */
%pure-parser

/* parser prefix */
%name-prefix "Nmodl_"

/* enable location tracking */
%locations

/* generate header file */
%defines

/* yyparse() takes an extra argument context */
%parse-param {NmodlContext* context}

/* reentrant lexer needs an extra argument for yylex() */
%lex-param {void * scanner}

/* token types */
%union {
    ModToken *qp;
    std::string *str_ptr;

    ast::ProgramNode *program_ptr;
    ast::StatementNodeList *statement_list_ptr;
    ast::ModelNode *model_ptr;
    ast::StringNode *string_ptr;
    ast::DefineNode *define_ptr;
    ast::NameNode *name_ptr;
    ast::IntegerNode *integer_ptr;
    ast::ParamBlockNode *paramblock_ptr;
    ast::ParamAssignNodeList *paramassign_list_ptr;
    ast::ParamAssignNode *paramassign_ptr;
    ast::IdentifierNode *identifier_ptr;
    ast::NumberNode *number_ptr;
    ast::UnitNode *units_ptr;
    ast::LimitsNode *limits_ptr;
    ast::UnitStateNode *unitstate_ptr;
    ast::StepBlockNode *stepblock_ptr;
    ast::SteppedNodeList *stepped_list_ptr;
    ast::SteppedNode *stepped_ptr;
    ast::NumberNodeList *number_list_ptr;
    ast::IndependentBlockNode *independentblock_ptr;
    ast::IndependentDefNodeList *independentdef_list_ptr;
    ast::IndependentDefNode *independentdef_ptr;
    ast::BooleanNode *boolean_ptr;
    ast::DependentDefNode *dependentdef_ptr;
    ast::DependentBlockNode *dependentblock_ptr;
    ast::DependentDefNodeList *dependentdef_list_ptr;
    ast::StateBlockNode *stateblock_ptr;
    ast::PlotDeclarationNode *plotdeclaration_ptr;
    ast::PlotVariableNodeList *plotvariable_list_ptr;
    ast::InitialBlockNode *initialblock_ptr;
    ast::LocalVariableNodeList *localvariable_list_ptr;
    ast::ConstructorBlockNode *constructorblock_ptr;
    ast::DestructorBlockNode *destructorblock_ptr;
    ast::ConductanceHintNode *conductancehint_ptr;
    ast::BinaryExpressionNode *binaryexpression_ptr;
    ast::ExpressionNode *expression_ptr;
    ast::UnaryExpressionNode *unaryexpression_ptr;
    ast::NonLinEuationNode *nonlineuation_ptr;
    ast::LinEquationNode *linequation_ptr;
    ast::FunctionCallNode *functioncall_ptr;
    ast::ExpressionNodeList *expression_list_ptr;
    ast::ExpressionStatementNode *expressionstatement_ptr;
    ast::FromStatementNode *fromstatement_ptr;
    ast::ForAllStatementNode *forallstatement_ptr;
    ast::WhileStatementNode *whilestatement_ptr;
    ast::IfStatementNode *ifstatement_ptr;
    ast::ElseIfStatementNodeList *elseifstatement_list_ptr;
    ast::ElseStatementNode *elsestatement_ptr;
    ast::DerivativeBlockNode *derivativeblock_ptr;
    ast::LinearBlockNode *linearblock_ptr;
    ast::NameNodeList *name_list_ptr;
    ast::NonLinearBlockNode *nonlinearblock_ptr;
    ast::DiscreteBlockNode *discreteblock_ptr;
    ast::PartialBlockNode *partialblock_ptr;
    ast::PartialEquationNode *partialequation_ptr;  /*@TODO: check why this is not used */
    ast::PrimeNameNode *primename_ptr;
    ast::PartialBoundaryNode *partialboundary_ptr;
    ast::FunctionTableBlockNode *functiontableblock_ptr;
    ast::ArgumentNodeList *argument_list_ptr;
    ast::FunctionBlockNode *functionblock_ptr;
    ast::ProcedureBlockNode *procedureblock_ptr;
    ast::NetReceiveBlockNode *netreceiveblock_ptr;
    ast::SolveBlockNode *solveblock_ptr;
    ast::BreakpointBlockNode *breakpointblock_ptr;
    ast::TerminalBlockNode *terminalblock_ptr;
    ast::BABlockNode *bablock_ptr;
    ast::BABlockTypeNode *bablocktype_ptr;
    ast::WatchStatementNode *watchstatement_ptr;
    ast::WatchNode *watch_ptr;
    ast::DoubleNode *double_ptr;
    ast::ForNetconNode *fornetcon_ptr;
    ast::SensNode *sens_ptr;
    ast::VarNameNodeList *varname_list_ptr;
    ast::ConserveNode *conserve_ptr;
    ast::CompartmentNode *compartment_ptr;
    ast::LDifuseNode *ldifuse_ptr;
    ast::KineticBlockNode *kineticblock_ptr;
    ast::VarNameNode *varname_ptr;
    ast::LagStatementNode *lagstatement_ptr;
    ast::QueueStatementNode *queuestatement_ptr;
    ast::QueueExpressionTypeNode *queueexpressiontype_ptr;
    ast::MatchBlockNode *matchblock_ptr;
    ast::MatchNodeList *match_list_ptr;
    ast::MatchNode *match_ptr;
    ast::UnitBlockNode *unitsblock_ptr;
    ast::UnitDefNodeList *unitdef_list_ptr;
    ast::FactordefNodeList *factordef_list_ptr;
    ast::FactordefNode *factordef_ptr;
    ast::UnitDefNode *unitdef_ptr;
    ast::ConstantStatementNode *constatntstatement_ptr;
    ast::ConstantBlockNode *constatntblock_ptr;
    ast::ConstantStatementNodeList *constantstatement_list_ptr;
    ast::TableStatementNode *tablestatement_ptr;
    ast::NeuronBlockNode *neuronblock_ptr;
    ast::NrnSuffixNode *nrnsuffix_ptr;
    ast::NrnUseionNode *nrnuseion_ptr;
    ast::NrnElctrodeCurrentNode *nrnelctrodecurrent_ptr;
    ast::NrnSectionNode *nrnsection_ptr;
    ast::NrnRangeNode *nrnrange_ptr;
    ast::NrnGlobalNode *nrnglobal_ptr;
    ast::NrnPointerNode *nrnpointer_ptr;
    ast::NrnBbcorePtrNode *nrnbbcoreptr_ptr;
    ast::NrnExternalNode *nrnexternal_ptr;
    ast::NrnThreadSafeNode *nrnthreadsafe_ptr;
    ast::ValenceNode *valence_ptr;

    ast::ReactionStatementNode *reactionstatement_ptr;
    ast::ReactionOperatorNode *reactionoperator_ptr;
    ast::ReadIonVarNodeList *readionvar_list_ptr;
    ast::WriteIonVarNodeList *writeionvar_list_ptr;
    ast::NonspeCurVarNodeList *nonspecurvar_list_ptr;
    ast::ElectrodeCurVarNodeList *electrodecurvar_list_ptr;
    ast::SectionVarNodeList *sectionvar_list_ptr;
    ast::RangeVarNodeList *rangevar_list_ptr;
    ast::GlobalVarNodeList *globalvar_list_ptr;
    ast::PointerVarNodeList *pointervar_list_ptr;
    ast::BbcorePointerVarNodeList *bbcorepointervar_list_ptr;
    ast::ExternVarNodeList *externvar_list_ptr;
    ast::ThreadsafeVarNodeList *threadsafevar_list_ptr;

    ast::VerbatimNode *verbatim_ptr;
    ast::CommentNode *comment_ptr;
    ast::FloatNode *float_ptr;
    ast::IndexedNameNode *indexedname_ptr;
    ast::ArgumentNode *argument_ptr;
    ast::LocalVariableNode *localvariable_ptr;
    ast::LocalListStatementNode *localliststatement_ptr;
    ast::NumberRangeNode *numberrange_ptr;
    ast::StatementNode *statement_ptr;
    ast::StatementBlockNode *statementblock_ptr;
    ast::BlockNode *block_ptr;
    ast::FirstLastTypeIndexNode *firstlasttypeindex_ptr;
    ast::BinaryOp binary_op_val;
    ast::BinaryOperatorNode *binaryoperator_ptr;
    ast::DoubleUnitNode *doubleunits_ptr;
}


/* Define our terminal symbols (tokens). This should
 * match our tokens.l lex file.
 */

%token  <qp>    MODEL CONSTANT INDEPENDENT DEPENDENT STATE
%token  <qp>    INITIAL1 DERIVATIVE SOLVE USING WITH STEPPED DISCRETE
%token  <qp>    FROM FORALL1 TO BY WHILE IF ELSE START1 STEP SENS SOLVEFOR
%token  <qp>    PROCEDURE PARTIAL DEFINE1 IFERROR PARAMETER
%token  <qp>    DERFUNC EQUATION TERMINAL LINEAR NONLINEAR FUNCTION1 LOCAL
%token  <qp>    LIN1 NONLIN1 PUTQ GETQ TABLE DEPEND BREAKPOINT
%token  <qp>    INCLUDE1 FUNCTION_TABLE PROTECT NRNMUTEXLOCK NRNMUTEXUNLOCK
%token  <qp>    '{' '}' '(' ')' '[' ']' '@' '+' '*' '-' '/' '=' '^' ':' ','
%token  <qp>    '~'
%token  <qp>    OR AND GT LT LE EQ NE NOT

%token  <qp>    PLOT VS LAG RESET MATCH MODEL_LEVEL SWEEP FIRST LAST
%token  <qp>    KINETIC CONSERVE REACTION REACT1 COMPARTMENT UNITS
%token  <qp>    UNITSON UNITSOFF LONGDIFUS

%token  <qp>    NEURON NONSPECIFIC READ WRITE USEION THREADSAFE
%token  <qp>    GLOBAL SECTION RANGE POINTER BBCOREPOINTER EXTERNAL BEFORE AFTER WATCH
%token  <qp>    ELECTRODE_CURRENT CONSTRUCTOR DESTRUCTOR NETRECEIVE FOR_NETCONS
%token  <qp>    CONDUCTANCE



/* Extra token that we are adding to print more useful errors. These
 * are currently not being used. see wiki page section "Extra Tokens"
 */
%token  <qp>    BEGINBLK ENDBLK             /* this is left and right curly braces */
%token  <qp>    LEFT_PAREN RIGHT_PAREN     /* this is left and right parenthesis */

/*building ast : probably dont do this, just return value!*/
%token  <primename_ptr>             PRIME
%token  <integer_ptr>               INTEGER
%token  <double_ptr>                REAL
%token  <string_ptr>                STRING
%token  <name_ptr>                  NAME
%token  <name_ptr>                  METHOD
%token  <name_ptr>                  SUFFIX
%token  <name_ptr>                  VALENCE
%token  <name_ptr>                  DEL
%token  <name_ptr>                  DEL2
%token  <integer_ptr>               DEFINEDVAR

/*special token returning string */
%token  <str_ptr>                   VERBATIM
%token  <str_ptr>                   COMMENT

/* Define the type of node our nonterminal symbols represent.
   The types refer to the %union declaration above.
*/

/* building AST */
%type   <program_ptr>               all

%type   <name_ptr>                  Name
%type   <number_ptr>                NUMBER
%type   <double_ptr>                real
%type   <expression_ptr>            intexpr
%type   <integer_ptr>               integer

%type   <string_ptr>                line
%type   <model_ptr>                 model
%type   <units_ptr>                 units
%type   <integer_ptr>               optindex
%type   <units_ptr>                 unit

%type   <block_ptr>                 proc

%type   <limits_ptr>                limits
%type   <double_ptr>                abstol
%type   <identifier_ptr>            name
%type   <number_ptr>                number

%type   <expression_ptr>            primary
%type   <expression_ptr>            term
%type   <expression_ptr>            leftlinexpr
%type   <expression_ptr>            linexpr
%type   <number_list_ptr>           numlist
%type   <expression_ptr>            expr
%type   <expression_ptr>            aexpr

%type   <statement_ptr>             ostmt
%type   <statement_ptr>             astmt
%type   <statementblock_ptr>        stmtlist
%type   <localliststatement_ptr>    locallist
%type   <localvariable_list_ptr>    locallist1

%type   <varname_ptr>               varname
%type   <expression_list_ptr>       exprlist
%type   <define_ptr>                define1
%type   <queuestatement_ptr>        queuestmt
%type   <expression_ptr>            asgn

%type   <fromstatement_ptr>         fromstmt
%type   <whilestatement_ptr>        whilestmt
%type   <ifstatement_ptr>           ifstmt
%type   <elseifstatement_list_ptr>  optelseif
%type   <elsestatement_ptr>         optelse
%type   <solveblock_ptr>            solveblk
%type   <functioncall_ptr>          funccall

%type   <statementblock_ptr>        ifsolerr
%type   <expression_ptr>            opinc
%type   <number_ptr>                opstart
/* @todo: though name is sens_pt it has senslist production*/
%type   <varname_list_ptr>          senslist
%type   <sens_ptr>                  sens

%type   <lagstatement_ptr>          lagstmt
%type   <forallstatement_ptr>       forallstmt
%type   <paramassign_ptr>           parmasgn
%type   <stepped_ptr>               stepped
%type   <independentdef_ptr>        indepdef

%type   <dependentdef_ptr>          depdef
/* @todo: productions for which we dont have %type
 * withby : dont need, imo
 */
%type   <block_ptr>                 declare
%type   <paramblock_ptr>            parmblk
%type   <paramassign_list_ptr>      parmbody
%type   <independentblock_ptr>      indepblk
%type   <independentdef_list_ptr>   indepbody
%type   <dependentblock_ptr>        depblk
%type   <dependentdef_list_ptr>     depbody

%type   <stateblock_ptr>            stateblk
%type   <stepblock_ptr>             stepblk

%type   <stepped_list_ptr>          stepbdy

%type   <watchstatement_ptr>        watchstmt
%type   <binaryoperator_ptr>        watchdir
%type   <watch_ptr>                 watch1
%type   <fornetcon_ptr>             fornetcon
%type   <plotdeclaration_ptr>       plotdecl

%type   <plotvariable_list_ptr>     pvlist

%type   <constatntblock_ptr>        constblk
%type   <constantstatement_list_ptr> conststmt
%type   <matchblock_ptr>            matchblk
%type   <match_list_ptr>            matchlist
%type   <match_ptr>                 match
%type   <identifier_ptr>            matchname

%type   <partialboundary_ptr>       pareqn
/* values from ast enum types */
%type   <firstlasttypeindex_ptr>    firstlast
%type   <reactionstatement_ptr>     reaction
%type   <conserve_ptr>              conserve
%type   <expression_ptr>            react

%type   <compartment_ptr>           compart
%type   <ldifuse_ptr>               ldifus
%type   <name_list_ptr>             namelist
%type   <unitsblock_ptr>            unitblk
%type   <expression_list_ptr>       unitbody
%type   <unitdef_ptr>               unitdef
%type   <factordef_ptr>             factordef

%type   <name_list_ptr>             solvefor

%type   <name_list_ptr>             solvefor1
%type   <unitstate_ptr>             uniton
%type   <name_list_ptr>             tablst
%type   <name_list_ptr>             tablst1
%type   <tablestatement_ptr>        tablestmt
%type   <name_list_ptr>             dependlst

%type   <argument_list_ptr>         arglist
%type   <argument_list_ptr>         arglist1
%type   <integer_ptr>               locoptarray
%type   <neuronblock_ptr>           neuronblk
%type   <nrnuseion_ptr>             nrnuse
%type   <statement_list_ptr>        nrnstmt

%type   <readionvar_list_ptr>       nrnionrlist
%type   <writeionvar_list_ptr>      nrnionwlist
%type   <nonspecurvar_list_ptr>     nrnonspeclist
%type   <electrodecurvar_list_ptr>  nrneclist
%type   <sectionvar_list_ptr>       nrnseclist
%type   <rangevar_list_ptr>         nrnrangelist
%type   <globalvar_list_ptr>        nrnglobalist
%type   <pointervar_list_ptr>       nrnptrlist
%type   <bbcorepointervar_list_ptr> nrnbbptrlist
%type   <externvar_list_ptr>        nrnextlist
%type   <threadsafevar_list_ptr>    opthsafelist
%type   <threadsafevar_list_ptr>    threadsafelist
%type   <valence_ptr>               valence
%type   <expressionstatement_ptr>   initstmt
%type   <bablock_ptr>               bablk

%type   <conductancehint_ptr>       conducthint

/*missing types found while constructing ast */
%type   <statement_list_ptr>        stmtlist1
%type   <initialblock_ptr>          initblk
%type   <constructorblock_ptr>      constructblk
%type   <destructorblock_ptr>       destructblk
%type   <functionblock_ptr>         funcblk
%type   <kineticblock_ptr>          kineticblk
%type   <breakpointblock_ptr>       brkptblk
%type   <derivativeblock_ptr>       derivblk
%type   <linearblock_ptr>           linblk
%type   <nonlinearblock_ptr>        nonlinblk
%type   <procedureblock_ptr>        procedblk
%type   <netreceiveblock_ptr>       netrecblk
%type   <terminalblock_ptr>         terminalblk
%type   <discreteblock_ptr>         discretblk
%type   <partialblock_ptr>          partialblk
%type   <functiontableblock_ptr>    functableblk

/* precedence in expressions--- low to high */
%left   OR
%left   AND
%left   GT GE LT LE EQ NE
%left   '+' '-'             /* left associative, same precedence */
%left   '*' '/' '%'         /* left assoc., higher precedence */
%left   UNARYMINUS NOT
%right  '^'                 /* exponentiation */

%{
    extern int yylex(YYSTYPE*, YYLTYPE*, void*);
    extern int yyparse(NmodlContext*);
    extern void yyerror(YYLTYPE*, NmodlContext*, const char *);
    std::string parse_with_verbatim_parser(std::string *);
%}


/* start symbol is named "top" */
%start top

%%


top             : all   { context->astRoot = $1; }
                | error { /*_ER_*/ }
                ;

all             :                               { $$ = new ast::ProgramNode(); }
                | all astmt                     { $1->addStatement($2); }           /* __todo__ : for diffeq */
                | all model                     { $1->addStatement($2); }
                | all locallist                 { $1->addStatement($2); }
                | all define1                   { $1->addStatement($2); }
                | all declare                   { $1->addBlock($2); }
                | all MODEL_LEVEL INTEGER declare   { /*__todo__ @Michael : discussed with Michael, was inserted
                                                            by merge program (which no longer exist). This was to
                                                            avoid the name collision in case of include. Idea was
                                                            to have some kind of namespace!
                                                      */
                                                    }
                | all proc                      { $1->addBlock($2); }
                | all VERBATIM                  { $1->addStatement(new ast::VerbatimNode(new ast::StringNode(parse_with_verbatim_parser($2)))); }
                | all COMMENT                   { $1->addStatement(new ast::CommentNode(new ast::StringNode(parse_with_verbatim_parser($2)))); }
                | all uniton                    { $1->addStatement($2); }
                | all INCLUDE1 STRING           { $1->addStatement(new ast::IncludeNode($3)); }
                ;

model           : MODEL line { $$ = new ast::ModelNode($2); }
                ;

line            : { $$ = new ast::StringNode(LexerUtils::inputline(scanner)); }
                ;

define1         : DEFINE1 NAME INTEGER {
                                            $$ = new ast::DefineNode($2, $3);
                                            context->add_defined_var($2->getName(), $3->eval());
                                       }
                | DEFINE1 error { /*_ER_*/ }
                ;

Name            : NAME { $$ = $1; }
                ;

declare         : parmblk   { $$ = $1; }
                | indepblk  { $$ = $1; }
                | depblk    { $$ = $1; }
                | stateblk  { $$ = $1; }
                | stepblk   { $$ = $1; }
                | plotdecl  { $$ = new ast::PlotBlockNode($1); }
                | neuronblk { $$ = $1; }
                | unitblk   { $$ = $1; }
                | constblk  { $$ = $1; }
                ;

parmblk         : PARAMETER '{' parmbody '}'    { $$ = new ast::ParamBlockNode($3); }
                ;

parmbody        :                   { $$ = new ast::ParamAssignNodeList(); }
                | parmbody parmasgn { $1->push_back($2); }
                ;

parmasgn        : NAME '=' number units limits      { $$ = new ast::ParamAssignNode($1, $3, $4, $5); }
                | NAME units limits                 { $$ = new ast::ParamAssignNode($1, NULL, $2, $3); }
                | NAME '[' integer ']' units limits { $$ = new ast::ParamAssignNode( new ast::IndexedNameNode($1, $3), NULL, $5, $6); }
                | error                             { /*_ER_*/}
                ;

units           :       { $$ = NULL; }
                | unit  { $$ = $1; }
                ;

unit            : '(' { $<string_ptr>$ = LexerUtils::inputtoparpar(scanner); } ')' { $$ = new ast::UnitNode($<string_ptr>2); }
                ;

uniton          : UNITSON   { $$ = new ast::UnitStateNode(ast::UNIT_ON); }
                | UNITSOFF  { $$ = new ast::UnitStateNode(ast::UNIT_OFF); }
                ;

limits          :                           { $$ = NULL; }
                | LT real ',' real GT       { $$ = new ast::LimitsNode($2, $4); }
                ;

stepblk         : STEPPED '{' stepbdy '}'   { $$ = new ast::StepBlockNode($3); }
                ;

stepbdy         :                   { $$ = new ast::SteppedNodeList(); }
                | stepbdy stepped   { $1->push_back($2); }
                ;

stepped         : NAME '=' numlist units    { $$ = new ast::SteppedNode($1, $3, $4); }
                ;

numlist         : number ',' number     { $$ = new ast::NumberNodeList(); $$->push_back($1); $$->push_back($3); }
                | numlist ',' number    { $1->push_back($3); }
                ;

name            : Name  { $$ = $1; }
                | PRIME { $$ = $1; }
                ;

number          : NUMBER        { $$ = $1; }
                | '-' NUMBER    { $2->negate(); $$ = $2; }
                ;

NUMBER          : integer   { $$ = $1; }
                | REAL      { $$ = $1; }
                ;

integer         : INTEGER       { $$ = $1; }
                | DEFINEDVAR    { $$ = $1; }
                ;

real            : REAL      { $$ = $1; }
                | integer   { $$ = new ast::DoubleNode(double($1->eval())); }
                ;

indepblk        : INDEPENDENT '{' indepbody '}' { $$ = new ast::IndependentBlockNode($3); }
                ;

indepbody       :                           { $$ = new ast::IndependentDefNodeList(); }
                | indepbody indepdef        { $1->push_back($2); }
                | indepbody SWEEP indepdef  { $1->push_back($3); $3->sweep = new ast::BooleanNode(1); }
                ;

indepdef        : NAME FROM number TO number withby integer opstart units { $$ = new ast::IndependentDefNode(NULL, $1, $3, $5, $7, $8, $9); }
                | error { /*_ER_*/ }
                ;

withby          : WITH
                ;

depblk          : DEPENDENT '{' depbody '}' { $$ = new ast::DependentBlockNode($3); }
                ;

depbody         :                   { $$ = new ast::DependentDefNodeList(); }
                | depbody depdef    { $1->push_back($2); }
                ;

depdef          : name opstart units abstol { $$ = new ast::DependentDefNode($1, NULL, NULL, NULL, $2, $3, $4); }
                | name '[' integer ']' opstart units abstol { $$ = new ast::DependentDefNode($1, $3, NULL, NULL, $5, $6, $7);}
                | name FROM number TO number opstart units abstol { $$ = new ast::DependentDefNode($1, NULL, $3, $5, $6, $7, $8); }
                | name '[' integer ']' FROM number TO number opstart units abstol { $$ = new ast::DependentDefNode($1, $3, $6, $8, $9, $10, $11);}
                | error { /*_ER_*/ }
                ;

opstart         :               { $$ = NULL; }
                | START1 number { $$ = $2; }
                ;

abstol          :               { $$ = NULL; }
                | LT real GT    { $$ = $2; }
                ;

stateblk        : STATE  '{' depbody '}'    { $$ = new ast::StateBlockNode($3); }
                ;

plotdecl        : PLOT pvlist VS name optindex  { $$ = new ast::PlotDeclarationNode($2, new ast::PlotVariableNode($4,$5)); }
                | PLOT error                    { /*_ER_*/ }
                ;

pvlist          : name optindex             { $$ = new ast::PlotVariableNodeList(); $$->push_back(new ast::PlotVariableNode($1, $2)); }
                | pvlist ',' name optindex  { $$ = $1; $$->push_back(new ast::PlotVariableNode($3, $4));}
                ;

optindex        :                   { $$ = NULL; }
                | '[' INTEGER ']'   { $$ = $2; }
                ;

proc            : initblk       { $$ = $1; }
                | derivblk      { $$ = $1; }
                | brkptblk      { $$ = $1; }
                | linblk        { $$ = $1; }
                | nonlinblk     { $$ = $1; }
                | funcblk       { $$ = $1; }
                | procedblk     { $$ = $1; }
                | netrecblk     { $$ = $1; }
                | terminalblk   { $$ = $1; }
                | discretblk    { $$ = $1; }
                | partialblk    { $$ = $1; }
                | kineticblk    { $$ = $1; }
                | constructblk  { $$ = $1; }
                | destructblk   { $$ = $1; }
                | functableblk  { $$ = $1; }
                | BEFORE bablk  { $$ = new ast::BeforeBlockNode($2); }
                | AFTER bablk   { $$ = new ast::AfterBlockNode($2); }
                ;

initblk         : INITIAL1 stmtlist '}'     { $$ = new ast::InitialBlockNode($2); }
                ;

constructblk    : CONSTRUCTOR stmtlist '}'  { $$ = new ast::ConstructorBlockNode($2); }
                ;

destructblk     : DESTRUCTOR stmtlist '}'   { $$ = new ast::DestructorBlockNode($2); }
                ;

stmtlist        : '{' stmtlist1             { $$ = new ast::StatementBlockNode($2); $$->setToken($1->clone()); }
                | '{' locallist stmtlist1   { $3->insert($3->begin(), $2); $$ = new ast::StatementBlockNode($3); $$->setToken($1->clone()); }
                ;

conducthint     : CONDUCTANCE Name              { $$ = new ast::ConductanceHintNode($2, NULL); }
                | CONDUCTANCE Name USEION NAME  { $$ = new ast::ConductanceHintNode($2, $4); }
                ;

locallist       : LOCAL locallist1  { $$ = new ast::LocalListStatementNode($2); }
                | LOCAL error       { /*_ER_*/ }
                ;

locallist1      : NAME locoptarray
                    {
                        $$ = new ast::LocalVariableNodeList();
                        if($2) {
                            $$->push_back( new ast::LocalVariableNode(new ast::IndexedNameNode($1, $2)));
                        } else {
                            $$->push_back( new ast::LocalVariableNode($1) );
                        }
                    }
                | locallist1 ',' NAME locoptarray
                    {
                        if($4) {
                            $1->push_back( new ast::LocalVariableNode(new ast::IndexedNameNode($3, $4)));
                        } else {
                            $1->push_back( new ast::LocalVariableNode($3) );
                        }
                    }
                ;

locoptarray     :                   { $$ = NULL; }
                | '[' integer ']'   { $$ = $2; }
                ;

stmtlist1       :                   { $$ = new ast::StatementNodeList(); }
                | stmtlist1 ostmt   { $1->push_back($2); }
                | stmtlist1 astmt   { $1->push_back($2); }
                ;

ostmt           : fromstmt      { $$ = $1; }
                | forallstmt    { $$ = $1; }
                | whilestmt     { $$ = $1; }
                | ifstmt        { $$ = $1; }
                | stmtlist '}'  { $$ = new ast::ExpressionStatementNode($1); }
                | solveblk      { $$ = new ast::ExpressionStatementNode($1); }
                | conducthint   { $$ = $1; }
                | VERBATIM      { $$ = new ast::VerbatimNode(new ast::StringNode(parse_with_verbatim_parser($1))); }
                | COMMENT       { $$ = new ast::CommentNode(new ast::StringNode(parse_with_verbatim_parser($1))); }
                | sens          { $$ = $1; }
                | compart       { $$ = $1; }
                | ldifus        { $$ = $1; }
                | conserve      { $$ = $1; }
                | lagstmt       { $$ = $1; }
                | queuestmt     { $$ = $1; }
                | RESET         { $$ = new ast::ResetNode(); }
                | matchblk      { $$ = new ast::ExpressionStatementNode($1); }
                | pareqn        { $$ = $1; }
                | tablestmt     { $$ = $1; }
                | uniton        { $$ = $1; }
                | initstmt      { $$ = $1; }
                | watchstmt     { $$ = $1; }
                | fornetcon     { $$ = new ast::ExpressionStatementNode($1); }
                | NRNMUTEXLOCK  { $$ = new ast::MutexLockNode(); }
                | NRNMUTEXUNLOCK{ $$ = new ast::MutexUnlockNode(); }
                | error         { /*_ER_*/ }
                ;

astmt           : asgn          { $$ = new ast::ExpressionStatementNode($1); }
                | PROTECT asgn  { $$ = new ast::ProtectStatementNode($2); }
                | reaction      { $$ = $1; }
                | funccall      { $$ = new ast::ExpressionStatementNode($1); }
                ;

asgn            : varname '=' expr                  { $$ = new ast::BinaryExpressionNode($1, new ast::BinaryOperatorNode(ast::BOP_ASSIGN), $3); }
                | nonlineqn expr '=' expr           { $$ = new ast::NonLinEuationNode($2, $4); }
                | lineqn leftlinexpr '=' linexpr    { $$ = new ast::LinEquationNode($2, $4); }
                ;

varname         : name                              { $$ = new ast::VarNameNode($1, NULL); }
                | name '[' intexpr ']'              { $$ = new ast::VarNameNode(new ast::IndexedNameNode($1, $3), NULL); }
                | NAME '@' integer                  { $$ = new ast::VarNameNode($1, $3); }
                | NAME '@' integer '[' intexpr ']'  { $$ = new ast::VarNameNode(new ast::IndexedNameNode($1, $5), $3); }
                ;

intexpr         : Name                  { $$ = $1; }
                | integer               { $$ = $1; }
                | '(' intexpr ')'       { $$ = $2; }
                | intexpr '+' intexpr   { $$ = new ast::BinaryExpressionNode($1, new ast::BinaryOperatorNode(ast::BOP_ADDITION), $3); }
                | intexpr '-' intexpr   { $$ = new ast::BinaryExpressionNode($1, new ast::BinaryOperatorNode(ast::BOP_SUBTRACTION), $3); }
                | intexpr '*' intexpr   { $$ = new ast::BinaryExpressionNode($1, new ast::BinaryOperatorNode(ast::BOP_MULTIPLICATION), $3); }
                | intexpr '/' intexpr   { $$ = new ast::BinaryExpressionNode($1, new ast::BinaryOperatorNode(ast::BOP_DIVISION), $3); }
                | error {}
                ;

expr            : varname                       { $$ = $1; }
                | real units                    {
                                                    if($2)
                                                        $$ = new ast::DoubleUnitNode($1, $2);
                                                    else
                                                        $$ = $1;
                                                }
                | funccall                      { $$ = $1; }
                | '(' expr ')'                  { $$ = $2; }
                | expr '+' expr                 { $$ = new ast::BinaryExpressionNode($1, new ast::BinaryOperatorNode(ast::BOP_ADDITION), $3); }
                | expr '-' expr                 { $$ = new ast::BinaryExpressionNode($1, new ast::BinaryOperatorNode(ast::BOP_SUBTRACTION), $3); }
                | expr '*' expr                 { $$ = new ast::BinaryExpressionNode($1, new ast::BinaryOperatorNode(ast::BOP_MULTIPLICATION), $3); }
                | expr '/' expr                 { $$ = new ast::BinaryExpressionNode($1, new ast::BinaryOperatorNode(ast::BOP_DIVISION), $3); }
                | expr '^' expr                 { $$ = new ast::BinaryExpressionNode($1, new ast::BinaryOperatorNode(ast::BOP_POWER), $3); }
                | expr OR expr                  { $$ = new ast::BinaryExpressionNode($1, new ast::BinaryOperatorNode(ast::BOP_OR), $3); }
                | expr AND expr                 { $$ = new ast::BinaryExpressionNode($1, new ast::BinaryOperatorNode(ast::BOP_AND), $3); }
                | expr GT expr                  { $$ = new ast::BinaryExpressionNode($1, new ast::BinaryOperatorNode(ast::BOP_GREATER), $3); }
                | expr LT expr                  { $$ = new ast::BinaryExpressionNode($1, new ast::BinaryOperatorNode(ast::BOP_LESS), $3); }
                | expr GE expr                  { $$ = new ast::BinaryExpressionNode($1, new ast::BinaryOperatorNode(ast::BOP_GREATER_EQUAL), $3); }
                | expr LE expr                  { $$ = new ast::BinaryExpressionNode($1, new ast::BinaryOperatorNode(ast::BOP_LESS_EQUAL), $3); }
                | expr EQ expr                  { $$ = new ast::BinaryExpressionNode($1, new ast::BinaryOperatorNode(ast::BOP_EXACT_EQUAL), $3); }
                | expr NE expr                  { $$ = new ast::BinaryExpressionNode($1, new ast::BinaryOperatorNode(ast::BOP_NOT_EQUAL), $3); }
                | NOT expr                      { $$ = new ast::UnaryExpressionNode(new ast::UnaryOperatorNode(ast::UOP_NOT), $2); }
                | '-' expr %prec UNARYMINUS     { $$ = new ast::UnaryExpressionNode(new ast::UnaryOperatorNode(ast::UOP_NEGATION), $2); }
                | error                         { /*_ER_*/ }
                ;

                /*
                | '(' expr  { yyerror("Unbalanced left parenthesis followed by valid expressions"); }
                | '(' error  { yyerror("Unbalanced left parenthesis followed by non parseable"); }
                |  expr ')' { yyerror("Unbalanced right parenthesis"); }
                */

nonlineqn       : NONLIN1
                ;

lineqn          : LIN1
                ;

leftlinexpr     : linexpr   { $$ = $1; }
                ;

linexpr         : primary               { $$ = $1; }
                | '-' primary           { $$ = new ast::UnaryExpressionNode(new ast::UnaryOperatorNode(ast::UOP_NEGATION), $2); }
                | linexpr '+' primary   { $$ = new ast::BinaryExpressionNode($1, new ast::BinaryOperatorNode(ast::BOP_ADDITION), $3); }
                | linexpr '-' primary   { $$ = new ast::BinaryExpressionNode($1, new ast::BinaryOperatorNode(ast::BOP_SUBTRACTION), $3); }
                ;

primary         : term              { $$ = $1; }
                | primary '*' term  { $$ = new ast::BinaryExpressionNode($1, new ast::BinaryOperatorNode(ast::BOP_MULTIPLICATION), $3); }
                | primary '/' term  { $$ = new ast::BinaryExpressionNode($1, new ast::BinaryOperatorNode(ast::BOP_DIVISION), $3); }
                ;

term            : varname       { $$ = $1; }
                | real          { $$ = $1; }
                | funccall      { $$ = $1; }
                | '(' expr ')'  { $$ = $2; }
                | error         { /*_ER_*/ }
                ;

funccall        : NAME '(' exprlist ')' { $$ = new ast::FunctionCallNode($1, $3); }
                ;

exprlist        :                       { $$ = NULL; }
                | expr                  { $$ = new ast::ExpressionNodeList(); $$->push_back($1); }
                | STRING                { $$ = new ast::ExpressionNodeList(); $$->push_back($1); }
                | exprlist ',' expr     { $1->push_back($3); }
                | exprlist ',' STRING   { $1->push_back($3); }
                ;

fromstmt        : FROM NAME '=' intexpr TO intexpr opinc stmtlist '}'   { $$ = new ast::FromStatementNode($2, $4, $6, $7, $8); }
                | FROM error                                            { /*_ER_*/ }
                ;

opinc           :               { $$ = NULL; }
                | BY intexpr    { $$ = $2; }
                ;

forallstmt      : FORALL1 NAME stmtlist '}' { $$ = new ast::ForAllStatementNode($2, $3); }
                | FORALL1 error             { /*_ER_*/ }
                ;

whilestmt       : WHILE '(' expr ')' stmtlist '}'   { $$ = new ast::WhileStatementNode($3, $5); }
                ;

ifstmt          : IF '(' expr ')' stmtlist '}' optelseif optelse    { $$ = new ast::IfStatementNode($3, $5, $7, $8); }
                ;

optelseif       :                                               { $$ = new ast::ElseIfStatementNodeList(); }
                | optelseif ELSE IF '(' expr ')' stmtlist '}'   { $1->push_back(new ast::ElseIfStatementNode($5, $7)); }
                ;

optelse         :                   { $$ = NULL; }
                | ELSE stmtlist '}' { $$ = new ast::ElseStatementNode($2); }
                ;

derivblk        : DERIVATIVE NAME stmtlist '}'          { $$ = new ast::DerivativeBlockNode($2, $3); $$->setToken($1->clone()); }
                ;

linblk          : LINEAR NAME solvefor stmtlist '}'     { $$ = new ast::LinearBlockNode($2, $3, $4); $$->setToken($1->clone()); }
                ;

nonlinblk       : NONLINEAR NAME solvefor stmtlist '}'  { $$ = new ast::NonLinearBlockNode($2, $3, $4); $$->setToken($1->clone()); }
                ;

discretblk      : DISCRETE NAME stmtlist '}'    { $$ = new ast::DiscreteBlockNode($2, $3); $$->setToken($1->clone()); }
                ;

partialblk      : PARTIAL NAME stmtlist '}' { $$ = new ast::PartialBlockNode($2, $3); $$->setToken($1->clone()); }
                | PARTIAL error             { /*_ER_*/ }
                ;

pareqn          : '~' PRIME '=' NAME '*' DEL2 '(' NAME ')' '+' NAME { $$ = new ast::PartialBoundaryNode(NULL, $2, NULL, NULL, $4, $6, $8, $11); }
                | '~' DEL NAME '[' firstlast ']' '=' expr           { $$ = new ast::PartialBoundaryNode($2, $3, $5, $8, NULL, NULL, NULL, NULL); }
                | '~' NAME '[' firstlast ']' '=' expr               { $$ = new ast::PartialBoundaryNode(NULL, $2, $4, $7, NULL, NULL, NULL, NULL); }
                ;

firstlast       : FIRST { $$ = new ast::FirstLastTypeIndexNode(ast::PEQ_FIRST); }
                | LAST  { $$ = new ast::FirstLastTypeIndexNode(ast::PEQ_LAST); }
                ;

functableblk    : FUNCTION_TABLE NAME '(' arglist ')' units         { $$ = new ast::FunctionTableBlockNode($2, $4, $6); $$->setToken($1->clone()); }
                ;

funcblk         : FUNCTION1 NAME '(' arglist ')' units stmtlist '}' { $$ = new ast::FunctionBlockNode($2, $4, $6, $7); $$->setToken($1->clone()); }
                ;

arglist         : { $$ = NULL; }
                | arglist1 { $$ = $1; }
                ;

arglist1        : name units                { $$ = new ast::ArgumentNodeList(); $$->push_back(new ast::ArgumentNode($1, $2)); }
                | arglist1 ',' name units   { $1->push_back(new ast::ArgumentNode($3, $4)); }
                ;

procedblk       : PROCEDURE NAME '(' arglist ')' units stmtlist '}' { $$ = new ast::ProcedureBlockNode($2, $4, $6, $7); $$->setToken($1->clone()); }
                ;

netrecblk       : NETRECEIVE '(' arglist ')' stmtlist '}'   { $$ = new ast::NetReceiveBlockNode($3, $5); }
                | NETRECEIVE error                          { /*_ER_*/ }
                ;

initstmt        : INITIAL1 stmtlist '}' { $$ = new ast::ExpressionStatementNode(new ast::InitialBlockNode($2)); }
                ;

solveblk        : SOLVE NAME ifsolerr               { $$ = new ast::SolveBlockNode($2, NULL, $3); }
                | SOLVE NAME USING METHOD ifsolerr  { $$ = new ast::SolveBlockNode($2, $4, $5); }
                | SOLVE error                       { /*_ER_*/ }
                ;

ifsolerr        :                       { $$ = NULL; }
                | IFERROR stmtlist '}'  { $$ = $2; }
                ;

solvefor        :           { $$ = NULL; }
                | solvefor1 { $$ = $1; }
                ;

solvefor1       : SOLVEFOR NAME         { $$ = new ast::NameNodeList(); $$->push_back($2); }
                | solvefor1 ',' NAME    { $1->push_back($3); }
                | SOLVEFOR error        { /*_ER_*/ }
                ;

brkptblk        : BREAKPOINT stmtlist '}'   { $$ = new ast::BreakpointBlockNode($2); }
                ;

terminalblk     : TERMINAL stmtlist '}'     { $$ = new ast::TerminalBlockNode($2); }
                ;

bablk           : BREAKPOINT stmtlist '}'   { $$ = new ast::BABlockNode(new ast::BABlockTypeNode(ast::BATYPE_BREAKPOINT), $2); }
                | SOLVE stmtlist '}'        { $$ = new ast::BABlockNode(new ast::BABlockTypeNode(ast::BATYPE_SOLVE), $2); }
                | INITIAL1 stmtlist '}'     { $$ = new ast::BABlockNode(new ast::BABlockTypeNode(ast::BATYPE_INITIAL), $2); }
                | STEP stmtlist '}'         { $$ = new ast::BABlockNode(new ast::BABlockTypeNode(ast::BATYPE_STEP), $2); }
                | error                     { /*_ER_*/ }
                ;

watchstmt       : WATCH watch1          { $$ = new ast::WatchStatementNode(new ast::WatchNodeList()); $$->addWatch($2); }
                | watchstmt ',' watch1  { $1->addWatch($3); }
                | WATCH error           { /*_ER_*/ }
                ;

watch1          : '(' aexpr watchdir aexpr ')' real { $$ = new ast::WatchNode( new ast::BinaryExpressionNode($2, $3, $4), $6); }
                ;

watchdir        : GT    { $$ = new ast::BinaryOperatorNode(ast::BOP_GREATER); }
                | LT    { $$ = new ast::BinaryOperatorNode(ast::BOP_LESS); }
                ;

fornetcon       : FOR_NETCONS '(' arglist ')' stmtlist '}'  { $$ = new ast::ForNetconNode($3, $5); }
                | FOR_NETCONS error                         { /*_ER_*/ }
                ;

aexpr           : varname                       { $$ = $1; }
                | real units                    { $$ = new ast::DoubleUnitNode($1, $2); }
                | funccall                      { $$ = $1; }
                | '(' aexpr ')'                 { $$ = $2; }
                | aexpr '+' aexpr               { $$ = new ast::BinaryExpressionNode($1, new ast::BinaryOperatorNode(ast::BOP_ADDITION), $3); }
                | aexpr '-' aexpr               { $$ = new ast::BinaryExpressionNode($1, new ast::BinaryOperatorNode(ast::BOP_SUBTRACTION), $3); }
                | aexpr '*' aexpr               { $$ = new ast::BinaryExpressionNode($1, new ast::BinaryOperatorNode(ast::BOP_MULTIPLICATION), $3); }
                | aexpr '/' aexpr               { $$ = new ast::BinaryExpressionNode($1, new ast::BinaryOperatorNode(ast::BOP_DIVISION), $3); }
                | aexpr '^' aexpr               { $$ = new ast::BinaryExpressionNode($1, new ast::BinaryOperatorNode(ast::BOP_POWER), $3); }
                | '-' aexpr %prec UNARYMINUS    { $$ = new ast::UnaryExpressionNode(new ast::UnaryOperatorNode(ast::UOP_NEGATION), $2); }
                | error                         { /*_ER_*/ }
                ;

sens            : SENS senslist { $$ = new ast::SensNode($2); }
                | SENS error    { /*_ER_*/ }
                ;

senslist        : varname               { $$ = new ast::VarNameNodeList(); $$->push_back($1); }
                | senslist ',' varname  { $1->push_back($3); }
                ;

conserve        : CONSERVE react '=' expr   { $$ = new ast::ConserveNode($2, $4); }
                | CONSERVE error            {}
                ;

compart         : COMPARTMENT NAME ',' expr '{' namelist '}'    { $$ = new ast::CompartmentNode($2, $4, $6); }
                | COMPARTMENT expr '{' namelist '}'             { $$ = new ast::CompartmentNode(NULL, $2, $4); }
                ;

ldifus          : LONGDIFUS NAME ',' expr '{' namelist '}'  { $$ = new ast::LDifuseNode($2, $4, $6); }
                | LONGDIFUS expr '{' namelist '}'           { $$ = new ast::LDifuseNode(NULL, $2, $4); }
                ;

namelist        : NAME          { $$ = new ast::NameNodeList(); $$->push_back($1); }
                | namelist NAME { $1->push_back($2); }
                ;

kineticblk      : KINETIC NAME solvefor stmtlist '}'    { $$ = new ast::KineticBlockNode($2, $3, $4); $$->setToken($1->clone()); }
                ;

reaction        : REACTION react REACT1 react '(' expr ',' expr ')' { $$ = new ast::ReactionStatementNode($2, new ast::ReactionOperatorNode(ast::LTMINUSGT), $4, $6, $8); }
                | REACTION react LT LT  '(' expr ')'                { $$ = new ast::ReactionStatementNode($2, new ast::ReactionOperatorNode(ast::LTLT), NULL, $6, NULL); }
                | REACTION react '-' GT '(' expr ')'                { $$ = new ast::ReactionStatementNode($2, new ast::ReactionOperatorNode(ast::MINUSGT), NULL, $6, NULL); }
                | REACTION error                                    { /* todo : __REACTION_DISCUSS with Michael__ */ }
                ;

react           : varname                   { $$ = $1; }
                |integer varname            { $$ = new ast::ReactVarNameNode($1, $2); }
                |react '+' varname          { $$ = new ast::BinaryExpressionNode($1, new ast::BinaryOperatorNode(ast::BOP_ADDITION), $3); }
                |react '+' integer varname  { $$ = new ast::BinaryExpressionNode($1, new ast::BinaryOperatorNode(ast::BOP_ADDITION), new ast::ReactVarNameNode($3, $4)); }
                ;

lagstmt         : LAG name BY NAME  { $$ = new ast::LagStatementNode($2, $4); }
                | LAG error         { /*_ER_*/ }
                ;

queuestmt       : PUTQ name { $$ = new ast::QueueStatementNode(new ast::QueueExpressionTypeNode(ast::PUT_QUEUE), $2); }
                | GETQ name { $$ = new ast::QueueStatementNode(new ast::QueueExpressionTypeNode(ast::GET_QUEUE), $2); }
                ;

matchblk        : MATCH '{' matchlist '}' { $$ = new ast::MatchBlockNode($3); }
                ;

matchlist       : match             { $$ = new ast::MatchNodeList(); $$->push_back($1); }
                | matchlist match   { $1->push_back($2); }
                ;

match           : name                              { $$ = new ast::MatchNode($1, NULL); }
                | matchname '(' expr ')' '=' expr   { $$ = new ast::MatchNode($1, new ast::BinaryExpressionNode($3, new ast::BinaryOperatorNode(ast::BOP_ASSIGN), $6)); }
                | error                             { /*_ER_*/ }
                ;

matchname       : name              { $$ = $1; }
                | name '[' NAME ']' { $$ = new ast::IndexedNameNode($1, $3); }
                ;

unitblk         : UNITS '{' unitbody '}' { $$ = new ast::UnitBlockNode($3); }
                ;

unitbody        :                       { $$ = new ast::ExpressionNodeList(); }
                | unitbody unitdef      { $1->push_back($2); }
                | unitbody factordef    { $1->push_back($2); }
                ;

unitdef         : unit '=' unit { $$ = new ast::UnitDefNode($1, $3); }
                | unit error    { /*_ER_*/ }
                ;

factordef       : NAME '=' real unit        { $$ = new ast::FactordefNode($1, $3, $4, NULL, NULL); $$->setToken($1->getToken()->clone()); }
                | NAME '=' unit unit        { $$ = new ast::FactordefNode($1, NULL, $3, NULL, $4); $$->setToken($1->getToken()->clone()); }
                | NAME '=' unit '-' GT unit { $$ = new ast::FactordefNode($1, NULL, $3, new ast::BooleanNode(1), $6); $$->setToken($1->getToken()->clone()); }
                | error {}
                ;

constblk        : CONSTANT '{' conststmt '}'        { $$ = new ast::ConstantBlockNode($3); }
                ;

conststmt       :                                   { $$ = new ast::ConstantStatementNodeList(); }
                | conststmt NAME '=' number units   { $1->push_back( new ast::ConstantStatementNode($2, $4, $5) ); }
                ;

tablestmt       : TABLE tablst dependlst FROM expr TO expr WITH INTEGER { $$ = new ast::TableStatementNode($2, $3, $5, $7, $9); }
                | TABLE error                                           { /*_ER_*/ }
                ;

tablst          :           { $$ = NULL; }
                | tablst1   { $$ = $1; }
                ;

tablst1         : Name              { $$ = new ast::NameNodeList(); $$->push_back($1); }
                | tablst1 ',' Name  { $1->push_back($3); }
                ;

dependlst       :                   { $$ = NULL; }
                | DEPEND tablst1    { $$ = $2; }
                ;

neuronblk       : NEURON '{' nrnstmt '}' { ast::StatementBlockNode *p = new ast::StatementBlockNode($3); p->setToken($2->clone()); $$ = new ast::NeuronBlockNode(p); }
                ;

nrnstmt         :                                   { $$ = new ast::StatementNodeList(); }
                | nrnstmt SUFFIX NAME               { $1->push_back( new ast::NrnSuffixNode($2, $3) ); }
                | nrnstmt nrnuse                    { $1->push_back($2); }
                | nrnstmt NONSPECIFIC nrnonspeclist { $1->push_back(new ast::NrnNonspecificNode($3)); }
                | nrnstmt ELECTRODE_CURRENT nrneclist { $1->push_back(new ast::NrnElctrodeCurrentNode($3)); }
                | nrnstmt SECTION nrnseclist           { $1->push_back(new ast::NrnSectionNode($3)); }
                | nrnstmt RANGE nrnrangelist        { $1->push_back(new ast::NrnRangeNode($3)); }
                | nrnstmt GLOBAL nrnglobalist       { $1->push_back(new ast::NrnGlobalNode($3)); }
                | nrnstmt POINTER nrnptrlist        { $1->push_back(new ast::NrnPointerNode($3)); }
                | nrnstmt BBCOREPOINTER nrnbbptrlist{ $1->push_back(new ast::NrnBbcorePtrNode($3)); }
                | nrnstmt EXTERNAL nrnextlist       { $1->push_back(new ast::NrnExternalNode($3)); }
                | nrnstmt THREADSAFE opthsafelist   { $1->push_back(new ast::NrnThreadSafeNode($3)); }
                ;

nrnuse          : USEION NAME READ nrnionrlist valence                      { $$ = new ast::NrnUseionNode($2, $4, NULL, $5); }
                | USEION NAME WRITE nrnionwlist valence                     { $$ = new ast::NrnUseionNode($2, NULL, $4, $5); }
                | USEION NAME READ nrnionrlist WRITE nrnionwlist valence    { $$ = new ast::NrnUseionNode($2, $4, $6, $7); }
                | USEION error                                      { /*_ER_*/ }
                ;

nrnionrlist     : NAME                  { $$ = new ast::ReadIonVarNodeList(); $$->push_back(new ast::ReadIonVarNode($1)); }
                | nrnionrlist ',' NAME  { $1->push_back(new ast::ReadIonVarNode($3)); }
                | error                 { /*_ER_*/ }
                ;

nrnionwlist     : NAME                  { $$ = new ast::WriteIonVarNodeList(); $$->push_back(new ast::WriteIonVarNode($1)); }
                | nrnionwlist ',' NAME  { $1->push_back(new ast::WriteIonVarNode($3)); }
                | error                 { /*_ER_*/ }
                ;

nrnonspeclist   : NAME                      { $$ = new ast::NonspeCurVarNodeList(); $$->push_back(new ast::NonspeCurVarNode($1)); }
                | nrnonspeclist ',' NAME    { $1->push_back(new ast::NonspeCurVarNode($3)); }
                | error                     { /*_ER_*/ }
                ;

nrneclist       : NAME                  { $$ = new ast::ElectrodeCurVarNodeList(); $$->push_back(new ast::ElectrodeCurVarNode($1)); }
                | nrneclist ',' NAME    { $1->push_back(new ast::ElectrodeCurVarNode($3)); }
                | error                 { /*_ER_*/ }
                ;

nrnseclist      : NAME                  { $$ = new ast::SectionVarNodeList(); $$->push_back(new ast::SectionVarNode($1)); }
                | nrnseclist ',' NAME   { $1->push_back(new ast::SectionVarNode($3)); }
                | error                 { /*_ER_*/ }
                ;

nrnrangelist    : NAME                  { $$ = new ast::RangeVarNodeList(); $$->push_back(new ast::RangeVarNode($1)); }
                | nrnrangelist ',' NAME { $1->push_back(new ast::RangeVarNode($3)); }
                | error                 { /*_ER_*/ }
                ;

nrnglobalist    : NAME                  { $$ = new ast::GlobalVarNodeList(); $$->push_back(new ast::GlobalVarNode($1)); }
                | nrnglobalist ',' NAME { $1->push_back(new ast::GlobalVarNode($3)); }
                | error                 { /*_ER_*/ }
                ;

nrnptrlist      : NAME                  { $$ = new ast::PointerVarNodeList(); $$->push_back(new ast::PointerVarNode($1)); }
                | nrnptrlist ',' NAME   { $1->push_back(new ast::PointerVarNode($3)); }
                | error                 { /*_ER_*/ }
                ;

nrnbbptrlist    : NAME                  { $$ = new ast::BbcorePointerVarNodeList(); $$->push_back(new ast::BbcorePointerVarNode($1)); }
                | nrnbbptrlist ',' NAME { $1->push_back(new ast::BbcorePointerVarNode($3)); }
                | error                 { /*_ER_*/ }
                ;

nrnextlist      : NAME                  { $$ = new ast::ExternVarNodeList(); $$->push_back(new ast::ExternVarNode($1)); }
                | nrnextlist ',' NAME   { $1->push_back(new ast::ExternVarNode($3)); }
                | error                 { /*_ER_*/ }
                ;

opthsafelist    :                   { $$ = NULL; }
                | threadsafelist    { $$ = $1; }
                ;

threadsafelist  : NAME                      { $$ = new ast::ThreadsafeVarNodeList();  $$->push_back(new ast::ThreadsafeVarNode($1)); }
                | threadsafelist ',' NAME   { $1->push_back(new ast::ThreadsafeVarNode($3)); }
                ;

valence         :                   { $$ = NULL; }
                | VALENCE real      { $$ = new ast::ValenceNode($1, $2); }
                | VALENCE '-' real  { $3->negate(); $$ = new ast::ValenceNode($1, $3); }
                ;

%%

std::string parse_with_verbatim_parser(std::string* str) {
   /*
   std::istringstream* is = new std::istringstream(str->c_str());
    VerbatimContext extcontext(is);
    Verbatim_parse(&extcontext);

    std::string ss(*(extcontext.result));
    */
    std::string ss(*str);
    return ss;
}

void yyerror(YYLTYPE* locp, NmodlContext* context, const char *s) {
    std::printf("\n Error:  %s for token %s \n", s, "_for_token_");
    std::exit(1);
}

