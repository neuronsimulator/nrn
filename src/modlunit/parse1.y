%{
/* /local/src/master/nrn/src/modlunit/parse1.y,v 1.11 1999/02/27 21:13:50 hines Exp */

#include <../../nmodlconf.h>
#include <stdlib.h>
#include "model.h"

/* Constructs a parse tree. No translation is done, ie. on exit printing
the intoken list will make an exact copy of the input file.
All tokens and productions are of Item type and consist of STRING, SYMBOL,
and LIST.
SPACE and NEWLINE are in the intoken list but are not yacc tokens.

All explicitly declared names are given their subtype
and a pointer to their declaration. All "used" names are marked with
their usage for later error checking.
*/

extern Symbol *checklocal(Symbol*);
extern int next_intoken(Item**);
extern Item *title;
extern int declare_level;
extern int parse_pass, restart_pass;
extern List *solvelist;

extern int conductance_seen_;
extern int breakpoint_local_seen_;

#define IFP(n)	if (parse_pass == n)
#define IFR(n)	if (restart_pass == n)
#define P1	IFP(1)
#define P2	IFP(2)
#define P3	IFP(3)
#define R0	IFR(0)
#define R1	IFR(1)

static int yylex();
static void yyerror(char *);

#if YYBISON 
#define myerr(arg) static int ierr=0;\
if (!(ierr++))yyerror(arg); --yyssp; --yyvsp; YYERROR
#else
#define myerr(arg) static int ierr=0;\
if (!(ierr++))yyerror(arg); --yyps; --yypv; YYERROR
#endif

extern Item *lastok;	/* last token accepted by expr */
static int blocktype;
static int unitflagsave; /*must turn off units in restartpass0 in kinetic block */
static List* netreceive_arglist;
static List* args;
extern int lexcontext;
%}

%union {
	Item	*qp;
}

%token	<qp>	VERBATIM COMMENT TITLE CONSTANT INDEPENDENT ASSIGNED STATE
%token	<qp>	END_VERBATIM END_COMMENT UNITS BREAKPOINT PARAMETER INT
%token	<qp>	INITIAL1 DERIVATIVE SOLVE USING WITH STEPPED DISCRETE
%token	<qp>	FROM TO BY WHILE IF ELSE START1 STEP SENS SOLVEFOR
%token	<qp>	PROCEDURE PARTIAL DEL DEL2 DEFINE1 IFERROR
%token	<qp>	DERFUNC EQUATION TERMINAL LINEAR NONLINEAR FUNCTION1 LOCAL
%token	<qp>	METHOD LIN1 NONLIN1 PUTQ GETQ FUNCTION_TABLE
%token	<qp>	INCLUDE1 PROTECT
%token	<qp>	'{' '}' '(' ')' '[' ']' '@' '+' '*' '-' '/' '=' '^' ':' ','
%token	<qp>	'~'
%token	<qp>	OR AND GT GE LT LE EQ NE NOT
%token	<qp>	NAME PRIME REAL INTEGER DEFINEDVAR
%token	<qp>	KINETIC CONSERVE REACTION REACT1 COMPARTMENT LONGDIFUS PARTEQN
%token	<qp>	STRING PLOT VS LAG RESET MATCH MODEL_LEVEL SWEEP FIRST LAST
%token	<qp>	SPACE NEWLINE TO_EOL STUFF
%token	<qp>	UNITBLK UNITSON UNITSOFF
%token	<qp>	TABLE DEPEND

%type	<qp>	top all all1 title verbatim comment define1 declare declare1
%type	<qp>	constblk constbody constasgn units limits tolerance
%type	<qp>	stepblk stepbdy stepped
%type	<qp>	Name numlist name number NUMBER integer real
%type	<qp>	indepblk indepbody indepdef withby
%type	<qp>	depblk depbody depdef opstart stateblk statbody discretblk
%type	<qp>	plotdecl pvlist optindex proc initblk
%type	<qp>	stmtlist stmtlist1 locallist locallist1 locoptarray stmt
%type	<qp>	asgn varname intexpr expr funccall exprlist exprlist1
%type	<qp>	fromstmt opinc whilestmt ifstmt optelseif optelse
%type	<qp>	tablestmt tablst dependlst
%type	<qp>	derivblk linblk nonlinblk
%type	<qp>	partialblk pareqn firstlast
%type	<qp>	funcblk arglist arglist1 procedblk solveblk ifsolerr functbl
%type	<qp>	solvefor1 solvefor netrecblk fornetcon
%type	<qp>	watchstmt watch1
%type	<qp>	eqnblk terminalblk sens compartlist ldifuslist longdifus
%type	<qp>	conserve compart namelist optnamelist kineticblk reaction react consreact
%type	<qp>	lagstmt queuestmt matchblk matchlist match matchname
%type	<qp>	unitblk unitflag unitbody unitdef Units factordef include1

/* interface to NEURON */
%token	<qp>	NEURON SUFFIX NONSPECIFIC READ WRITE USEION VALENCE
%token	<qp>	GLOBAL SECTION RANGE POINTER EXTERNAL BEFORE AFTER
%token	<qp>	ELECTRODE_CURRENT CONSTRUCTOR DESTRUCTOR NETRECEIVE
%token	<qp>	FOR_NETCONS WATCH THREADSAFE
%type	<qp>	neuronblk nrnuse nrnlist valence constructblk destructblk
%type	<qp>	initstmt bablk
%token  <qp>    CONDUCTANCE
%type   <qp>    conducthint

/* precedence in expressions--- low to high */
%left   OR
%left   AND
%left   GT GE LT LE EQ NE
%left   '+' '-' /* left associative, same precedence */
%left   '*' '/' '%'/* left assoc., higher precedence */
%left   UNARYMINUS NOT
%right  '^'     /* exponentiation */

%%
top: 	all
	|error {diag("Illegal block", (char *)0);}
	;
all:	/*nothing*/
	{$$ = ITEM0;}
	| all all1
	;
all1:	title | locallist | define1 | declare | proc | verbatim	| comment
	| unitflag
	| include1
	;
title 	:TITLE TO_EOL
		{P1{if (!title) title = $2;}}
	;
verbatim: VERBATIM END_VERBATIM
	;
comment: COMMENT END_COMMENT
	;
unitflag: UNITSON
		{ unitonflag = unitflagsave = 1;}
	| UNITSOFF
		{unitonflag = unitflagsave = 0;}
	;
include1: INCLUDE1 STRING
		{P1{include_file($2);}}
	;
define1: DEFINE1 NAME integer
		{P1{define_value($2, $3);}}
	| DEFINE1 error {myerr("syntax: DEFINE name integer");}
	;
Name:	NAME
		{P1{$1->element = (void *)checklocal(SYM($1));}}
	;
declare: declare1
	| MODEL_LEVEL INTEGER {declare_level = atoi(STR($2));} declare1
		{declare_level = 0;}
	;
declare1: constblk | indepblk | depblk | stateblk | stepblk
	| plotdecl | unitblk | neuronblk
	;
constblk: CONSTANT '{' constbody '}'
	| PARAMETER '{' constbody '}'
	;
constbody: /*nothing*/
		{$$ = ITEM0;}
	| constbody constasgn
	;
constasgn: NAME '=' number units limits
		/* ugh. 7 slots because constant may appear in COMPARTMENT
			and the 7th space will contain the compartment
			size units */
		{P1{$$ = itemarray(7, $1, $4, ITEM0, $3,ITEM0,ITEM0,ITEM0); declare(modlunitCONST, $1, $$);}}
	| NAME units limits
		{P1{$$ = itemarray(7, $1, $2, ITEM0,ITEM0,ITEM0,ITEM0,ITEM0); declare(modlunitCONST, $1, $$);}}
	| NAME '[' integer ']' units limits
		{P1{$$ = itemarray(7, $1, $5, $3, ITEM0, ITEM0,ITEM0,ITEM0); declare(modlunitCONST, $1, $$);}}
	| error {myerr("name = number");}
	;
units:	/*nothing*/
		{$$ = ITEM0;}
	| Units
	;
Units:	'(' {P1{lex_units();}} UNITS ')'
		{$$ = $3; lastok = $4; P2{unitcheck(STR($3));}}
	;
limits: /*nothing*/
		{$$ = ITEM0;}
	| LT number ',' number GT
	;
tolerance: /*nothing*/
		{$$ = ITEM0;}
	| LT number GT
	;
stepblk: STEPPED '{' stepbdy '}'
	;
stepbdy: /*nothing*/
		{$$ = ITEM0;}
	| stepbdy stepped
	;
stepped: NAME '=' numlist units
		{P1{$$ = itemarray(3,$1, $4, $3); declare(STEP1, $1, $$);}}
	;
numlist: number ',' number
	| numlist ',' number
	;
name:	Name	{lastok = $1;}
	| PRIME {lastok = $1;}
	;
number: NUMBER
		{lastok = $1;}
	| '-' NUMBER
		{lastok = $2;}
 	;
NUMBER: integer | REAL
	;
integer: INTEGER
	| DEFINEDVAR
	;
real:	REAL
		{lastok = $1;}
	| integer
		{lastok = $1;}
	;
indepblk: INDEPENDENT '{' indepbody '}'
	;
indepbody: /*nothing*/
		{$$ = ITEM0;}
	| indepbody indepdef
		{P1{declare(INDEP, ITMA($2)[0], $2);}}
	| indepbody SWEEP indepdef
		{P1{ITMA($3)[7] = $2; declare(INDEP, ITMA($3)[0], $3);}}

	;
indepdef: NAME FROM number TO number withby integer opstart units
		{P1{$$ = itemarray(8, $1, $9, $3, $5, $6, $7, $8, ITEM0);}}
	| error {myerr("name FROM number TO number WITH integer");}
	;
withby:	WITH
	;
depblk:	ASSIGNED '{' depbody '}'
	;
depbody: /*nothing*/
		{$$ = ITEM0;}
	| depbody depdef
		{P1{declare(DEP, ITMA($2)[0], $2);}}
	;
depdef: name opstart units tolerance
		{P1{$$ = itemarray(7, $1, $3, ITEM0, ITEM0, ITEM0, $2, ITEM0);}}
	| name '[' integer ']' opstart units tolerance
		{P1{$$ = itemarray(7, $1, $6, $3, ITEM0, ITEM0, $5, ITEM0);}}
	| name FROM number TO number opstart units tolerance
		{P1{$$ = itemarray(7, $1, $7, ITEM0, $3, $5, $6, ITEM0);}}
	| name '[' integer ']' FROM number TO number opstart units tolerance
		{P1{$$ = itemarray(7, $1, $10, $3, $6, $8, $9, ITEM0);}}
	| error {
diag("name FROM number TO number START number\n",
"FROM...TO and START are optional, name can be name[integer]\n");}
	;
opstart: /*nothing*/ {$$ = ITEM0;}
	| START1 number {$$ = $2;}
	;
stateblk: STATE '{' statbody '}'
	;
statbody: /*nothing*/
		{$$ = ITEM0;}
	| statbody depdef
		{P1{declare(STAT, ITMA($2)[0], $2);}}
	;
plotdecl: PLOT pvlist VS name optindex
	| PLOT error { myerr("PLOT namelist VS name");}
	;
pvlist:	name optindex
	| pvlist ',' name optindex
	;
optindex: /*nothing*/
		{ $$ = ITEM0;}
	| '[' integer ']'
		{ $$ = $2;}
	;
unitblk: UNITBLK '{' unitbody '}'
	;
unitbody: /*nothing*/
		{$$ = ITEM0;}
	| unitbody unitdef
	| unitbody factordef
	;
unitdef: Units '=' Units
		{P1{install_units(STR($1), STR($3));}}
	| Units error {myerr("Unit definition syntax: (units) = (units)");}
	;
factordef: NAME '=' real Units
		{P1{$$ = itemarray(3, $1, $4, $3); declare(UFACTOR, $1, $$);}}
	| NAME '=' Units Units 
		{P1{Item *q; double d, unit_mag();
		    Unit_push(STR($3));
			Unit_push(STR($4)); unit_div();
			dimensionless();
			Sprintf(buf, "%g",unit_mag());
			$$ = itemarray(3, $1, $4, lappendstr(misc, buf));
/*printf("%s has value %s and units (%s)\n", SYM($1)->name, buf, STR($5));*/
		    unit_pop();
		    declare(UFACTOR, $1, $$);
		}}
	| NAME '=' Units '-' GT Units
		{P1{ Item *q; double unit_mag();
		    Unit_push(STR($3)); Unit_push(STR($6)); unit_div();
		    q = lappendstr(misc, unit_str());
		    dimensionless();
		    Sprintf(buf, "%g", 1./unit_mag());
		    $$ = itemarray(3, $1, lappendstr(misc, buf), q),
/*printf("%s has value %s and units (%s)\n", SYM($1)->name, STR(q), buf );*/
		    unit_pop();
		    declare(UFACTOR, $1, $$);
		}}
	| error {myerr("Unit factor syntax: examples:\n\
foot2inch = (foot) -> (inch)\n\
F = 96520 (coulombs)\n\
R = (k-mole) (joule/degC)");
		}
	;
proc:	 {blocktype = INITIAL1;} initblk
		{$$ = $2;}
	| {lexcontext = NONLINEAR; blocktype = DERIVATIVE;} derivblk
		{$$ = $2;}
	| {blocktype = BREAKPOINT;} eqnblk
		{$$ = $2;}
	| {lexcontext = blocktype = LINEAR;} linblk
		{$$ = $2;}
	| {lexcontext = blocktype = NONLINEAR;} nonlinblk
		{$$ = $2;}
	| {blocktype = FUNCTION1;} funcblk
		{$$ = $2;}
	| {blocktype = FUNCTION_TABLE;} functbl
		{$$ = $2;}
	| {blocktype = PROCEDURE;} procedblk
		{$$ = $2;}
	| {blocktype = PROCEDURE;} netrecblk
		{$$ = $2;}
	| {blocktype = TERMINAL;} terminalblk
		{$$ = $2;}
	| {blocktype = DISCRETE;} discretblk
		{$$ = $2;}
	| {blocktype = CONSTRUCTOR;} constructblk
		{$$ = $2;}
	| {blocktype = DESTRUCTOR;} destructblk
		{$$ = $2;}
	| {lexcontext = blocktype = PARTIAL;} partialblk
		{$$ = $2;}
	| {lexcontext = blocktype = KINETIC;
		P3 R0{unitflagsave=unitonflag; unitonflag=0;}}
	    kineticblk
		{$$ = $2; P3{ R0{unitonflag=unitflagsave;}
		R1{clear_compartlist();} parse_restart($$, 1);}}
	| {blocktype = BEFORE;} BEFORE bablk {$$ = $2;}
	| {blocktype = AFTER;} AFTER bablk {$$ = $2;}
	;
initblk: INITIAL1 stmtlist
	;
constructblk: CONSTRUCTOR stmtlist
        ;
destructblk: DESTRUCTOR stmtlist
        ;
bablk: BREAKPOINT stmtlist
	| SOLVE stmtlist
	| INITIAL1 stmtlist
	| STEP stmtlist
	;
stmtlist: '{' stmtlist1 '}'
		{$$ = $2;}
	| '{' locallist stmtlist1 '}'
		{P1{poplocal();}}
	;
conducthint: CONDUCTANCE Name
		{$$ = ITEM0; conductance_seen_ = 1;}
        | CONDUCTANCE Name USEION NAME
		{$$ = ITEM0; conductance_seen_ = 1;}
        ; 
locallist: LOCAL locallist1 { if (blocktype == BREAKPOINT) breakpoint_local_seen_ = 1; }
	| LOCAL error {myerr("Illegal LOCAL declaration");}
	;
locallist1: NAME locoptarray
		{P1{pushlocal($1, $2);}}
	| locallist1 ',' NAME locoptarray
		{P1{install_local($3, $4);}}
	;
locoptarray: /*nothing*/
		{ $$ = ITEM0;}
	| '[' integer ']'
		{$$ = $2;}
	;
stmtlist1: /*nothing*/
		{$$ = ITEM0;}
	| stmtlist1 stmt
	;
stmt:	asgn
	| PROTECT asgn
	| fromstmt
	| whilestmt
	| ifstmt
	| stmtlist
	| funccall
	| conducthint
	| solveblk
	| verbatim 
	| comment
	| sens
	| reaction
	| conserve
	| compart
	| longdifus
	| lagstmt
	| tablestmt
	| pareqn
	| queuestmt
	| RESET
	| matchblk
	| unitflag
	| initstmt
	| watchstmt
	| fornetcon
	| error
		{myerr("Illegal statement");}
	;	
asgn:	varname '=' expr
		{
		  P3 {
			if (SYM($1)->subtype & LOCL) {
				SYM($1)->u.str = stralloc(unit_str(), (char *)0);
				unit_pop();
			}else{
			   unit_push($1); unit_swap();
			   unit_cmp($1, $2, lastok); unit_pop();
			}
		  }
		}
	| '~' expr '=' expr
		{
		  P3 {
			unit_cmp($2, $3, lastok); unit_pop();
		  }
		}
	;
varname: name
		{lastok = $1;
		  P1{SYM($1)->usage |= DEP;}
		  P2{ if (SYM($1)->subtype & ARRAY) {
			myerr("variable needs an index");}
		  }
		}
	| name '[' intexpr ']'
		{lastok = $4; 
		  P1{SYM($1)->usage |= DEP;}
		  P2{ if ((SYM($1)->subtype & ARRAY) == 0)
			{myerr("variable is not an array");}
		  }
		}
	| NAME '@' integer
		{lastok = $3;
		  P1{SYM($1)->usage |= DEP;}
		  P2{ if (SYM($1)->subtype & ARRAY) {
			myerr("variable needs an index");}
		  }
		}

	| NAME '@' integer '[' intexpr ']' 
		{lastok = $6;
		  P1{SYM($1)->usage |= DEP;}
		  P2{ if ((SYM($1)->subtype & ARRAY) == 0)
			{myerr("variable is not an array");}
		  }
		}
	;
intexpr: Name
		{lastok = $1; P1{SYM($1)->usage |= DEP;}}
	| integer		{ lastok = $1;}
	| '(' intexpr ')'	{ lastok = $3;}
	| intexpr '+' intexpr
	| intexpr '-' intexpr
	| intexpr '*' intexpr
	| intexpr '/' intexpr
	| INT '(' expr ')' {lastok = $4;}
	| error {myerr("Illegal integer expression");}
	;
expr:	varname {P3{unit_push($1);}}
	| real units	{P3{if ($2) {
				Unit_push(STR($2));
			     }else{		
				Unit_push((char *)0);
			     }
			}}
	| funccall	{P3{unit_push($1);}}
	| '(' expr ')'	{ lastok = $3;
			  P3{ifcnvfac($3);}
			}
	| expr '+' expr	{P3{unit_cmp($1, $2, lastok);}}
	| expr '-' expr	{P3{unit_cmp($1, $2, lastok);}}
	| expr '*' expr	{P3{unit_mul();}}
	| expr '/' expr	{P3{unit_div();}}
	| expr '^' expr {P3{unit_exponent($3, lastok);}}
	| expr OR expr	{P3{unit_logic(1, $1, $2, lastok);}}
	| expr AND expr	{P3{unit_logic(1, $1, $2, lastok);}}
	| expr GT expr	{P3{unit_logic(2, $1, $2, lastok);}}
	| expr LT expr	{P3{unit_logic(2, $1, $2, lastok);}}
	| expr GE expr	{P3{unit_logic(2, $1, $2, lastok);}}
	| expr LE expr	{P3{unit_logic(2, $1, $2, lastok);}}
	| expr EQ expr	{P3{unit_logic(2, $1, $2, lastok);}}
	| expr NE expr	{P3{unit_logic(2, $1, $2, lastok);}}
	| NOT expr	{P3{unit_pop(); Unit_push( "");}}
	| '-' expr %prec UNARYMINUS
	| error {myerr("Illegal expression");}
	;
funccall: NAME  '(' {P3{unit_push_args($1);}} exprlist ')'
		{ lastok = $5; P1{SYM($1)->usage |= FUNCT;}
		  P3{unit_done_args();}
		}
	;
exprlist: /*nothing*/
		{$$ = ITEM0; }
	| exprlist1
	;
exprlist1: expr {P3{unit_chk_arg($1, lastok);}}
	| STRING
	| exprlist ',' expr {P3{unit_chk_arg($3, lastok);}}
	| exprlist ',' STRING
	;
fromstmt: FROM NAME '=' intexpr TO intexpr opinc {P1{pushlocal($2, ITEM0);}} stmtlist
		{P1{$$ = itemarray(6, $1, $2, $4, $6, $7, $9); poplocal();}}
	|FROM error {
myerr("FROM intvar = intexpr TO intexpr BY intexpr { statements }");}
	;
opinc: /*nothing*/ {$$ = ITEM0;}
	| BY intexpr
	;
whilestmt: WHILE '(' expr ')' stmtlist {P3{unit_pop();}}
	;
ifstmt:	IF '(' expr ')' stmtlist optelseif optelse  {P3{unit_pop();}}
	;
optelseif: /*nothing*/
		{$$ = ITEM0;}
	| optelseif ELSE IF '(' expr ')' stmtlist {P3{unit_pop();}}
	;
optelse: /*nothing*/
		{$$ = ITEM0;}
	| ELSE stmtlist
	;
initstmt: INITIAL1 stmtlist
	;
derivblk: DERIVATIVE NAME stmtlist
		{P1{$$ = itemarray(3, $1, $2, $3); declare(DERF, $2, $$);}}
	;
linblk: LINEAR NAME solvefor stmtlist
		{P1{$$ = itemarray(4, $1, $2, $3, $4); declare(LINF, $2, $$);}}
	;
nonlinblk: NONLINEAR NAME solvefor stmtlist
		{P1{$$ = itemarray(4, $1, $2, $3, $4); declare(NLINF, $2, $$);}}
	;
discretblk: DISCRETE NAME stmtlist
		{P1{$$ = itemarray(3, $1, $2, $3); declare(DISCF, $2, $$);}}
	;
partialblk: PARTIAL NAME stmtlist
		{P1{$$ = itemarray(3, $1, $2, $3); declare(PARF, $2, $$);}}
	;
pareqn: PARTEQN PRIME '=' NAME '*' DEL2 '(' NAME ')' '+' NAME
		{lastok = $11;
		 P3{
			unit_push($4); unit_del(2); unit_mul();
			unit_push($8); unit_mul();
			unit_push($11); unit_cmp($4, $10, $11);
			unit_push($2); unit_swap(); unit_cmp($2, $3, $11);
			unit_pop();
		}}
	| PARTEQN DEL NAME '[' firstlast ']' '=' expr
		{P3{
			unit_del(1);
			unit_push($3); unit_mul(); unit_swap();
			unit_cmp($2,$7,lastok); unit_pop();
		}}
	| PARTEQN NAME '[' firstlast ']' '=' expr
		{P3{unit_push($2); unit_swap();
		    unit_cmp($2,$6,lastok); unit_pop();
		}}
	| PARTEQN error {myerr("Illeqal partial diffeq");}
	;
firstlast: FIRST | LAST
	;
funcblk: FUNCTION1 NAME '(' arglist ')'	units
	  	{P1{pushlocal($2, ITEM0); func_unit($2, $6);}}
	  stmtlist
		{P1{
		    declare(FUNCT, $2, itemarray(3, $2, $6, $4));
		    poplocal(); poplocal(); SYM($2)->usage |= FUNCT;
		   }
		}
	;
functbl: FUNCTION_TABLE NAME '(' arglist ')'	units
	  	{P1{pushlocal($2, ITEM0); func_unit($2, $6);}
		 P1{
		    declare(FUNCT, $2, itemarray(3, $2, $6, $4));
		    poplocal(); poplocal(); SYM($2)->usage |= FUNCT;
		   }
		}
	;
arglist: /*nothing*/
		{$$ = ITEM0; P1{pushlocal(ITEM0, ITEM0);}
			if (args) { freelist(&args); }
			args = newlist();
		}
	| arglist1
	;
arglist1: name units
		{P1{
		    if (args) { freelist(&args); }
		    args = newlist();
		    $$ = (Item *)newlist();
		    pushlocal($1, ITEM0);
		    Lappendsym(args, checklocal(SYM($1)));
		    if ($2) {
			checklocal(SYM($1))->u.str = STR($2);
			Lappendstr((List *)$$, STR($2));
		    }else{
			Lappendstr((List *)$$, "");
		    }
		}}
	| arglist1 ',' name units
		{P1{
		    pushlocal($3, ITEM0);
		    Lappendsym(args, checklocal(SYM($3)));
		    if ($4) {
			checklocal(SYM($3))->u.str = STR($4);
			Lappendstr((List *)$1, STR($4));
		    }else{
			Lappendstr((List *)$1, "");
		    }
		}}
	;
procedblk: PROCEDURE NAME '(' arglist ')' stmtlist
		{P1{
		    declare(PROCED, $2, itemarray(3, $2, ITEM0, $4));
		    poplocal(); SYM($2)->usage |= FUNCT;
		   }
		}
	;
netrecblk: NETRECEIVE '(' arglist ')'
		{P1{
			List* l; Item* q;
			if ($3 == ITEM0) {
				diag("NET_RECEIVE must have at least one argument", (char*)0);
			}
			l = newlist();
			q = lappendsym(l, install("flag", NAME));
			pushlocal(q, ITEM0);
			Lappendstr((List *)$3, "");
			netreceive_arglist = args; args = (List*)0;
		}}
	   stmtlist
		{ P1{poplocal();}}
	| NETRECEIVE error { myerr("Illegal NETRECEIVE block");}
	;

watchstmt: WATCH watch1
	| watchstmt ',' watch1
	| WATCH error { myerr("Illegal WATCH statement");}
	;
watch1: '(' expr ')' real
	;
fornetcon: FOR_NETCONS '(' arglist ')'
		{P1{ Item* q1, *q2;
		  q1 = netreceive_arglist->next;
		  q2 = args->next;
		  while (q1 != netreceive_arglist && q2 != args) {
			Symbol* s1 = SYM(q1);
			Symbol* s2 = SYM(q2);
			if (s1->u.str) { /* s2 must be nil or same */
				if (s2->u.str) {
					if (strcmp(s1->u.str, s2->u.str) != 0) {
						diag(s1->name, "in FOR_NETCONS arglist does not have same units as corresponding arg in NET_RECEIVE arglist");
					}
				}else{
					s2->u.str = s1->u.str;
				}
			}else{ /* s2 must be nil */
				if (s2->u.str) {
					diag(s1->name, "in FOR_NETCONS arglist does not have same units as corresponding arg in NET_RECEIVE arglist");
				}
			}
/*printf("|%s|%s|  |%s|%s|\n", s1->name, s1->u.str, s2->name, s2->u.str);*/
			q1 = q1->next;
			q2 = q2->next;
		  }
		  if (q1 != netreceive_arglist || q2 != args) {
			diag("NET_RECEIVE and FOR_NETCONS do not have same number of arguments", (char*)0);
		  }
		}}
	  stmtlist
	| FOR_NETCONS error { myerr("Illegal FOR_NETCONS statement");}
	;

solveblk: SOLVE NAME ifsolerr
		{P1{$$ = itemarray(4, $1, $2, ITEM0, $3); lappenditem(solvelist, $$);}}
	| SOLVE NAME USING METHOD ifsolerr
		{P1{$$ = itemarray(4, $1, $2, $3, $4); lappenditem(solvelist, $$);}}
	| SOLVE error { myerr("Illegal SOLVE statement");}
	;
ifsolerr: /*nothing*/
		{ $$ = ITEM0; }
	| IFERROR stmtlist
		{ $$ = $2; }
	;
solvefor: /*nothing*/
		{$$ = ITEM0;}
	| solvefor1
	;
solvefor1: SOLVEFOR NAME
		{ P2{if(!(SYM($2)->subtype&STAT)){
			myerr("Not a STATE");}
		  }
		}
	| solvefor1 ',' NAME
		{ P2{if(!(SYM($2)->subtype&STAT)){
			myerr("Not a STATE");}
		  }
		}
	| SOLVEFOR error {myerr("Syntax: SOLVEFOR name, name, ...");}
	;
eqnblk: BREAKPOINT stmtlist
	;
terminalblk: TERMINAL stmtlist
	;
sens:	SENS namelist
	|SENS error {myerr("syntax is SENS var1, var2, var3, etc");}
	;

conserve: CONSERVE consreact '=' expr
		{P3{unit_cmp($2, $3, lastok);}}
	| CONSERVE error {myerr("Illegal CONSERVE syntax");}
	;
consreact:varname {P3{consreact_push($1);}}
	|INTEGER varname {P3{consreact_push($2);}}
	|consreact '+' varname {P3{consreact_push($3); unit_cmp($1,$2,lastok);}}
	|consreact '+' INTEGER varname {
		  P3{consreact_push($4); unit_cmp($1,$2,lastok);}
		}
	;
compart: COMPARTMENT NAME {P1{pushlocal($2, ITEM0);}} ',' expr '{' compartlist '}'
		{P1{poplocal();}
		  P3{
		    unit_pop();
		  }
		}
	| COMPARTMENT expr '{' compartlist '}'
		{ P3{
		    unit_pop();
		  }
		}
	;
compartlist: NAME
		{P3 R0{unit_compartlist($1);}}
	| compartlist  NAME
		{P3 R0{unit_compartlist($2);}}
	;
longdifus: LONGDIFUS NAME {P1{pushlocal($2, ITEM0);}} ',' expr '{' ldifuslist '}'
		{P1{poplocal();}
		  P3{
		    unit_pop();
		  }
		}
	| LONGDIFUS expr '{' ldifuslist '}'
		{ P3{
		    unit_pop();
		  }
		}
	;
ldifuslist: NAME
		{P3 R0{unit_ldifuslist($1, unitflagsave);}}
	| ldifuslist  NAME
		{P3 R0{unit_ldifuslist($2, unitflagsave);}}
	;
namelist: Name
	| namelist ',' Name
	;
kineticblk: KINETIC NAME solvefor stmtlist
		{P1{declare(KINF, $2, ITEM0);}}
	;
reaction: REACTION react REACT1 react '(' expr ',' expr ')'
		{P3{kinunits($3, restart_pass);}}
	| REACTION react LT LT  '(' expr ')'
		{P3{kinunits($3, restart_pass);}}
	| REACTION react '-' GT '(' expr ')'
		{P3{kinunits($3, restart_pass);}}
	| REACTION error {myerr("Illegal reaction syntax");}
	;
react:	varname {P3{R1{ureactadd($1);} unit_push($1);}}
	|INTEGER varname {P3{R1{ureactadd($2);} unit_push($2); Unit_push(0); unit_exponent($1,$1);}}
	|react '+' varname {P3{R1{ureactadd($3);}unit_push($3); unit_mul();}}
	|react '+' INTEGER varname {
		  P3{R1{ureactadd($4);}unit_push($4); Unit_push(0); unit_exponent($3,$3); unit_mul();}
		}
	;
lagstmt: LAG name BY NAME
	| LAG error {myerr("Lag syntax is: LAG name BY const");}
	;
tablestmt: TABLE tablst dependlst FROM expr TO expr WITH integer
		{P3{unit_pop(); unit_pop();}}
	;
tablst: /*Nothing*/
		{$$ = ITEM0;}
	| namelist
	;
dependlst: /*Nothing*/
		{$$ = ITEM0;}
	| DEPEND namelist
	;
queuestmt: PUTQ name
	| GETQ name
	;
matchblk: MATCH '{' matchlist '}'
	;
matchlist: match
	| matchlist match
	;
match:	name
	| matchname '(' expr ')' '=' expr
	| error
		{myerr("MATCH syntax is state0 or state(expr)=expr or\
state[i](expr(i)) = expr(i)");}
	;
matchname: name
	| name '[' NAME ']'
	;

neuronblk: NEURON 
			{
			  lastok = $1;
#if NRNUNIT
			  P2{nrn_unit_chk();}
#endif
			}
		'{' nrnstmt '}'
			{lastok = $3;}
	;
nrnstmt: /*nothing*/
	| nrnstmt SUFFIX NAME
		{ P1{nrn_list($2, $3);}}
	| nrnstmt nrnuse
	| nrnstmt NONSPECIFIC nrnlist
		{ P1{nrn_list($2,$3);}}
	| nrnstmt ELECTRODE_CURRENT nrnlist
		{ P1{nrn_list($2,$3);}}
	| nrnstmt RANGE nrnlist
		{ P1{nrn_list($2, $3);}}
	| nrnstmt SECTION nrnlist
		{ P1{nrn_list($2, $3);}}
	| nrnstmt GLOBAL nrnlist
		{ P1{nrn_list($2, $3);}}
        | nrnstmt POINTER nrnlist
                { P1{nrn_list($2, $3);}}
        | nrnstmt EXTERNAL nrnlist
                { P1{nrn_list($2, $3);}}
	| nrnstmt THREADSAFE optnamelist
	;
optnamelist: /* nothing */
		{$$ = NULL;}
	| namelist
	;
nrnuse: USEION NAME READ nrnlist valence
		{P1{nrn_use($2, $4, ITEM0);}}
	|USEION NAME WRITE nrnlist valence
		{P1{nrn_use($2, ITEM0, $4);}}
	|USEION NAME READ nrnlist WRITE nrnlist valence
		{P1{nrn_use($2, $4, $6);}}
	| error
		{myerr("syntax is: USEION ion READ list WRITE list");}
	;
nrnlist: NAME
		{P1{$$ = (Item *)newlist(); Lappendsym((List *)$$, SYM($1));}}
	| nrnlist ',' NAME
		{P1{ Lappendsym((List *)$1, SYM($3));}}
	| error
		{myerr("syntax is: keyword name , name, ..., name");}
	;
valence: /*nothing*/
		{$$ = ITEM0;}
	| VALENCE real
	{$$ = $2;}
	| VALENCE '-' real
		{$$ = $3;}
	;
%%
	/* end of grammar */

static void yyerror(char* s)	/* called for yacc syntax error */
{
	Fprintf(stderr, "%s:\n ", s);
}

static int yylex() {return next_intoken(&(yylval.qp));}

#if !NRNUNIT
void nrn_list(Item *q1, Item* q2)
{
	/*ARGSUSED*/
}
void nrn_use(Item* q1, Item* q2, Item* q3)
{
	/*ARGSUSED*/
}
#endif

