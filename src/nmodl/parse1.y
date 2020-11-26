%{
/* /local/src/master/nrn/src/nmodl/parse1.y,v 4.11 1999/03/24 18:34:08 hines Exp */

#include <../../nmodlconf.h>
#include "modl.h"
#include <stdlib.h>

#if defined(__STDC__)
#define sdebug(arg1,arg2) {}
#define qdebug(arg1,arg2) {}
#else
#define sdebug(arg1,arg2) {Fprintf(stderr,"arg1:%s\n", arg2); Fflush(stderr);}
#define qdebug(arg1,arg2) {Item *q; Fprintf(stderr,"arg1:");Fflush(stderr);\
	for (q=arg2; q->type != 0; q=q->next){\
		if (q->type == SYMBOL)\
			Fprintf(stderr,"%s\n", SYM(q)->name);\
		else if (q->type == STRING)\
			Fprintf(stderr,"%s\n", STR(q);\
		else\
			Fprintf(stderr,"Illegal Item type\n");\
		Fflush(stderr);}\
		Fprintf(stderr,"\n");Fflush(stderr);}
#endif

#define ldebug(arg1, arg2) qdebug(arg1, arg2->next)

extern int yylex(), yyparse();
static void yyerror();

#if YYBISON
#define myerr(arg) static int ierr=0;\
if (!(ierr++))yyerror(arg); --yyssp; --yyvsp; YYERROR
#else
#define myerr(arg) static int ierr=0;\
if (!(ierr++))yyerror(arg); --yyps; --yypv; YYERROR
#endif

int brkpnt_exists;
int assert_threadsafe;
int usederivstatearray;
extern int protect_;
extern int vectorize;
extern int in_comment_; /* allow non-ascii in a COMMENT */
extern char *modelline;
extern Item* protect_astmt(Item*, Item*);
extern List* toplocal_;
static List* toplocal1_;
extern List *firstlist; /* NAME symbols in order that they appear in file */
extern int lexcontext; /* ':' can return 3 different tokens */
extern List *solveforlist; /* List of symbols that are actually to be solved
				for in a block. See in_solvefor() */
static int stateblock; /* 0 if dependent, 1 if state */
static int blocktype;
static int saw_verbatim_; /* only print the notice once */
static int inequation; /* inside an equation?*/
static int nstate;	/* number of states seen in an expression */
static int leftside;	/* inside left hand side of equation? */
static int pstate;	/* number of state in a primary expression */
static int tstate;	/* number of states in a term */
static Item *lastok;	/* last token accepted by expr */
static int sensused;	/* a SENS statement occurred in this block */
static Symbol *matchindex; /* local symbol for implied MATCH loop */
static int model_level = 0; /* the model level prepended to declaration
				blocks by merge */
static int scopindep = 0;/* SCoP independent explicitly declared if 1 */
static int extdef2 = 0; /* flag that says we are in an EXTDEF2 function */
static List *table_list = LIST0; /* table information for TABLE statement */
static int forallindex = 0;	/* 0 not in FORALL, -1 just starting, 
					>0 index of arrays used (must all
					be the same */
static Item* astmt_end_;	/* see kinetic.c vectorizing */
static int nr_argcnt_, argcnt_; /* for matching number of args in NET_RECEIVE
				 and FOR_NETCONS */
%}

%union {
	Item	*qp;
	char	*str;
	List	*lp;
	int	i;
}

%token	<qp>	VERBATIM COMMENT MODEL CONSTANT INDEPENDENT DEPENDENT STATE
%token	<qp>	INITIAL1 DERIVATIVE SOLVE USING WITH STEPPED DISCRETE
%token	<qp>	FROM FORALL1 TO BY WHILE IF ELSE START1 STEP SENS SOLVEFOR
%token	<qp>	PROCEDURE PARTIAL DEL DEL2 DEFINE1 IFERROR PARAMETER INT
%token	<qp>	DERFUNC EQUATION TERMINAL LINEAR NONLINEAR FUNCTION1 LOCAL
%token	<qp>	METHOD LIN1 NONLIN1 PUTQ GETQ TABLE DEPEND BREAKPOINT
%token	<qp>	INCLUDE1 FUNCTION_TABLE PROTECT NRNMUTEXLOCK NRNMUTEXUNLOCK
%token	<qp>	'{' '}' '(' ')' '[' ']' '@' '+' '*' '-' '/' '=' '^' ':' ','
%token	<qp>	'~'
%token	<qp>	OR AND GT LT LE EQ NE NOT
%token	<qp>	NAME PRIME REAL INTEGER DEFINEDVAR ONTOLOGY_ID
%type	<qp>	Name NUMBER real intexpr integer
%token	<qp>	STRING PLOT VS LAG RESET MATCH MODEL_LEVEL SWEEP FIRST LAST
%type	<str>	line model units optindex unit limits abstol
%type	<qp>	name number nonlineqn primary term linexpr numlist
%type	<qp>	expr aexpr ostmt astmt stmtlist locallist locallist1
%type	<qp>	varname exprlist define1 queuestmt
%type	<qp>	asgn fromstmt whilestmt ifstmt solveblk funccall ifsolerr
%type	<qp>	opinc opstart senslist sens lagstmt forallstmt
%type	<qp>	parmasgn stepped indepdef depdef withby
%type	<qp>	declare parmblk indepblk depblk stateblk stepblk
%type	<qp>	watchstmt watchdir watch1 fornetcon
%type	<qp>	plotdecl constblk
%type	<qp>	matchblk matchlist match matchname pareqn firstlast

%token	<qp>	KINETIC CONSERVE REACTION REACT1 COMPARTMENT UNITS
%token	<qp>	UNITSON UNITSOFF LONGDIFUS
%type	<qp>	reaction conserve react compart ldifus namelist unitblk
%type	<qp>	solvefor solvefor1 uniton

%type	<lp>	tablst tablst1 dependlst arglist arglist1
%type	<i>	locoptarray
/* interface to NEURON */
%token	<qp>	NEURON SUFFIX NONSPECIFIC READ WRITE USEION VALENCE THREADSAFE REPRESENTS
%token	<qp>	GLOBAL SECTION RANGE POINTER BBCOREPOINTER EXTERNAL BEFORE AFTER WATCH
%token	<qp>	ELECTRODE_CURRENT CONSTRUCTOR DESTRUCTOR NETRECEIVE FOR_NETCONS
%type	<qp>	neuronblk nrnuse nrnlist optnrnlist valence initstmt bablk optontology
%token	<qp>	CONDUCTANCE
%type	<qp>	conducthint

/* precedence in expressions--- low to high */
%left   OR
%left   AND
%left   GT GE LT LE EQ NE
%left   '+' '-' /* left associative, same precedence */
%left   '*' '/' '%'/* left assoc., higher precedence */
%left   UNARYMINUS NOT
%right  '^'     /* exponentiation */

%%
top: 	all {/*ldebug(top, intoken)*/;}
	|error {diag("Illegal block", (char *)0);}
	;
all:	/*nothing*/
	| all model
	| all locallist
		/* move the declarations into firstlist */
		{Item* q; replacstr($2, "static double");
		vectorize_substitute($2, "/*Top LOCAL");
		vectorize_substitute(lastok->next, "*/\n");
		movelist($2, lastok->next, firstlist);
		if (!toplocal_) {toplocal_ = newlist();}
		ITERATE(q, toplocal1_) {
			assert(SYM(q)->name[0] == '_' && SYM(q)->name[1] == 'l');
			SYM(q)->name[1] = 'z';
		}
		movelist(toplocal1_->next, toplocal1_->prev, toplocal_);
		}
	| all define1
	| all declare
	| all MODEL_LEVEL INTEGER {model_level = atoi(STR($3));}
		declare {model_level = 0;}
	| all 
		{if (sensused)
			diag("sensitivity analysis not implemented for",
				" this block type");
		}
	    proc
	| all VERBATIM 
		/* read everything and move as is to end of procfunc */
		{inblock(SYM($2)->name); replacstr($2, "\n/*VERBATIM*/\n");
		if (!assert_threadsafe && !saw_verbatim_) {
 		 fprintf(stderr, "Notice: VERBATIM blocks are not thread safe\n");
		 saw_verbatim_ = 1;
		 vectorize = 0;
		}
		movelist($2,intoken->prev, procfunc);}
	| all COMMENT
		/* read everything and delete */
		{
		in_comment_ = 1;
		inblock(SYM($2)->name); deltokens($2, intoken->prev);
		in_comment_ = 0;
		}
	| all uniton
	| all INCLUDE1 STRING
		{include_file($3);}
	;
model: MODEL line 
		{if (modelline == NULL) modelline = $2;
			deltokens($1, intoken->prev);}
	;
line:	{$$ = inputline();}
	;
define1: DEFINE1 NAME INTEGER
		/* all subsequent occurences of NAME will be replaced
			by integer during parseing.  See 'integer:' */
	{ Symbol *sp = SYM($2);
	 if (sp->subtype)
		diag(sp->name, " used before DEFINEed");
	 sp->u.str = STR($3);
	 sp->type = DEFINEDVAR;
	 deltokens($1, $3);}
	| DEFINE1 error {myerr("syntax: DEFINE name integer");}
	;
Name:	NAME
		{ Symbol *checklocal();
		  SYM($1) = checklocal(SYM($1));  /* it was a bug
			when this was done to the lookahead token in lex */
		}
	;
declare: parmblk | indepblk | depblk | stateblk | stepblk
	| plotdecl | neuronblk | unitblk | constblk
	;
parmblk: PARAMETER '{' parmbody '}' {deltokens($1, $4);}
	;
parmbody: /*nothing*/
	| parmbody parmasgn
		{ explicit_decl(model_level, $2);}
/*	| parmbody stepped */ /*now has its own block*/
	;
parmasgn: NAME '=' number units limits
		/* install in syminorder and put info in Symbol->u.str
			Note that usage is EXPLICIT_DECL */
		{parminstall(SYM($1), STR($3), $4, $5);}
	| NAME units limits
		{parminstall(SYM($1), "0", $2, $3);}
	| NAME '[' integer ']' units limits
		{ int i = atoi(STR($3));
		  if (i < 1) diag("Array index must be > 0", (char*)0);
		  parm_array_install(SYM($1), "0", $5, $6, i);
		}
	| error {diag("name = number", (char *)0);}
	;
units:	/*nothing*/ {$$ = stralloc("", (char *)0);}
	| unit
	;
unit:	'(' {$<str>$ = inputtopar();} /*string*/ ')'
		/*does not include parentheses*/
		 {$$ = $<str>2; delete($1); delete($3);}
	;
uniton: UNITSON {replacstr($1, "");}
	| UNITSOFF {replacstr($1, "");}
	;
limits: /*nothing*/
		{$$ = stralloc("", (char*)0);}
	| LT real ',' real GT
		{
		 Sprintf(buf, "%s %s", STR($2), STR($4));
		 $$ = stralloc(buf, (char*)0);
		}
	;
stepblk: STEPPED '{' stepbdy '}' {deltokens($1, $4);}
	;
stepbdy: /*nothing*/
	| stepbdy stepped
		{ explicit_decl(model_level, $2); }
	;
stepped: NAME '=' numlist units
		/* install */
		{steppedinstall(SYM($1), $3, lastok, $4);}
	;
numlist: number ',' number
	| numlist ',' number
	;
name:	Name
	| PRIME
	;
number: NUMBER	{lastok = $1;}
	| '-' NUMBER
		/* replace the string with -string and discard '-'*/
                { Sprintf(buf, "-%s", STR($2));
		 STR($2) = stralloc(buf, STR($2)); $$ = $2;
		delete($1); lastok = $2;
		}
 	;
NUMBER: integer | REAL
	;
integer: INTEGER
	| DEFINEDVAR {replacstr($1, SYM($1)->u.str);}
	;
real:	REAL {lastok = $1;}
	| integer	/* add a .0 to the string */
		{Sprintf(buf, "%s.0", STR($1));
		STR($1) = stralloc(buf, STR($1));
		lastok = $1;
		}
	;
indepblk: INDEPENDENT '{' indepbody '}' {deltokens($1, $4);}
	;
indepbody: /*nothing*/
	| indepbody indepdef
		{ explicit_decl(model_level, $2); }
	| indepbody SWEEP {scopindep = 1;} indepdef
		{ explicit_decl(model_level, $4);
		  scopindep = 0;
		}
	;
indepdef: NAME FROM number TO number withby integer opstart units
		/*indepinstall()*/
		{indepinstall(SYM($1), STR($3),
		 STR($5), STR($7), $8, $9, scopindep);
		}
	| error {diag("name FROM number TO number WITH integer\n", (char *)0);}
	;
withby:	WITH
	;
depblk:	DEPENDENT {stateblock = 0;} '{' depbody '}' {deltokens($1, $5);}
	;
depbody: /*nothing*/
	| depbody depdef
		{ explicit_decl(model_level, $2);}
	;
depdef: name opstart units abstol
		/*depinstall()*/
		{depinstall(stateblock, SYM($1), 0, "0", "1", $3, $2, 1, $4);
		 }
	| name '[' integer ']' opstart units abstol
		{int i = atoi(STR($3));
		 if (i < 1) diag("Array index must be > 0", (char *)0);
		 depinstall(stateblock, SYM($1), i,
			 "0", "1", $6, $5, 1, $7);
		}
	| name FROM number TO number opstart units abstol
		{depinstall(stateblock, SYM($1), 0, STR($3),
		 STR($5), $7, $6, 1, $8);
		}
	| name '[' integer ']' FROM number TO number opstart units abstol
		{int i = atoi(STR($3));
		 if (i < 1) diag("Array index must be > 0", (char *)0);
		 depinstall(stateblock, SYM($1), i,
		 STR($6), STR($8), $10, $9, 1, $11);
		}
	| error {
diag("name FROM number TO number START number\n",
"FROM...TO and START are optional, name can be name[integer]\n");}
	;
opstart: /*nothing*/ {$$ = ITEM0;}
	| START1 number {$$ = $2;}
	;
abstol: /*nothing*/
		{ $$ = stralloc("", (char*)0);}
	| LT real GT
		{
			$$ = stralloc(STR($2), (char*)0);
		}
	;
stateblk: STATE  {stateblock = 1;} '{' depbody '}' {deltokens($1, $5);}
	;
plotdecl: PLOT pvlist VS name optindex
		/* construct plotlist. Used in parout.c */
		{ Item *q;
		 q = linsertsym(plotlist, SYM($4)); Insertstr(q->next, $5); }
		
	| PLOT error { diag("PLOT namelist VS name", (char *)0);}
	;
pvlist:	name optindex
		{ if (plotlist->next == plotlist) {
			Lappendsym(plotlist, SYM($1));
			Lappendstr(plotlist, $2);
		  }else{
			diag("Only one PLOT declaration allowed", (char *)0);
		  }
		}
	| pvlist ',' name optindex
		{ Lappendsym(plotlist, SYM($3)); Lappendstr(plotlist, $4);}
	;
optindex: /*nothing*/
		{ $$ = "-1";}
	| '[' INTEGER ']'
		{ $$ = STR($2);}
	;
proc:	 {blocktype = INITIAL1;} initblk
		/*blocktype set prior to parsing*/
	| {lexcontext = NONLINEAR; blocktype = DERIVATIVE;} derivblk
	| {blocktype = BREAKPOINT;} brkptblk
	| {lexcontext = blocktype = LINEAR;} linblk
	| {lexcontext = blocktype = NONLINEAR;} nonlinblk
	| {blocktype = FUNCTION1;} funcblk
	| {blocktype = PROCEDURE;} procedblk
	| {blocktype = NETRECEIVE;} netrecblk
	| {blocktype = TERMINAL;} terminalblk
	| {blocktype = DISCRETE;} discretblk
	| {lexcontext = blocktype = PARTIAL;} partialblk
	| {lexcontext = blocktype = KINETIC;ostmt_start();} kineticblk {see_ostmt();}
	| {blocktype = CONSTRUCTOR;} constructblk
	| {blocktype = DESTRUCTOR;} destructblk
	| {blocktype = FUNCTION_TABLE;} functableblk
	| {blocktype = BEFORE;} BEFORE bablk
	| {blocktype = AFTER;} AFTER bablk
	;
initblk: INITIAL1 stmtlist '}'
		{movelist($2, $3, initfunc);}
	;
constructblk: CONSTRUCTOR stmtlist '}'
		{movelist($2, $3, constructorfunc);}
	;
destructblk: DESTRUCTOR stmtlist '}'
		{movelist($2, $3, destructorfunc);}
	;
stmtlist: '{' {pushlocal();} stmtlist1 {poplocal();}
	| '{' locallist stmtlist1
		{poplocal();}
	;
conducthint: CONDUCTANCE Name
		{conductance_hint(blocktype, $1, $2);}
	| CONDUCTANCE Name USEION NAME
		{conductance_hint(blocktype, $1, $4);}
	;
locallist: LOCAL
		{
		  if (toplocal1_) {freelist(&toplocal1_);}
		  toplocal1_ = newlist();
		}
	   locallist1
		{ replacstr($1, "double");
		  Insertstr(lastok->next, ";\n");
		  possible_local_current(blocktype, toplocal1_);
		}
	| LOCAL error {myerr("Illegal LOCAL declaration");}
	;
locallist1: NAME locoptarray
		/* locals are placed in a stack of symbol lists and given
			the prefix _l */
		{int a2; pushlocal();
		 a2 = SYM($1)->assigned_to_; /* in case marked threadsafe */
		 SYM($1) = copylocal(SYM($1));
		 SYM($1)->assigned_to_ = a2;
		 lappendsym(toplocal1_, SYM($1));
		 if ($2) {
			SYM($1)->araydim = $2;
			SYM($1)->subtype |= ARRAY;
		 }else{
			lastok = $1;
		 }
		}
	| locallist1 ',' NAME locoptarray
		{
		 int a2 = SYM($3)->assigned_to_; /* in case marked threadsafe */
		 SYM($3) = copylocal(SYM($3));
		 SYM($3)->assigned_to_ = a2;
		 lappendsym(toplocal1_, SYM($3));
		 if ($4) {
			SYM($3)->araydim = $4;
			SYM($3)->subtype |= ARRAY;
		 }else{
			lastok = $3;
		 }
		}
	;
locoptarray: /*nothing*/
		{$$ = 0;}
	| '[' integer ']'
		{$$ = atoi(STR($2)); lastok = $3;}
	;
stmtlist1: /*nothing*/
	| stmtlist1 {if (blocktype == KINETIC) see_ostmt();} ostmt
		{if (blocktype == KINETIC) see_ostmt();}
	| stmtlist1 astmt {if (blocktype == KINETIC) { see_astmt($2, astmt_end_); }}
	;
ostmt:	fromstmt
	| forallstmt
	| whilestmt
	| ifstmt
	| stmtlist '}'
	| solveblk
	| conducthint
	| VERBATIM 
		{inblock(SYM($1)->name);
		replacstr($1, "\n/*VERBATIM*/\n");
		if (!assert_threadsafe && !saw_verbatim_) {
 		 fprintf(stderr, "Notice: VERBATIM blocks are not thread safe\n");
		 saw_verbatim_ = 1;
		 vectorize = 0;
		}
		}
		
	| COMMENT
		{inblock(SYM($1)->name); deltokens($1, intoken->prev);}
	| sens
	| compart	{check_block(KINETIC, blocktype, "COMPARTMENT");}
	| ldifus	{check_block(KINETIC, blocktype, "LONGDIFUS");}
	| conserve	{check_block(KINETIC, blocktype, "CONSERVE");}
	| lagstmt
	| queuestmt
	| RESET
		{ replacstr($1, " _reset = 1;\n"); }
	| matchblk
	| pareqn	/* 2nd order partial equation and boundary conditions*/
	| tablestmt
		{if (blocktype !=FUNCTION1 && blocktype != PROCEDURE) {
			diag("TABLE valid only for FUNCTION or PROCEDURE", (char *)0);
		}}
	| uniton
	| initstmt
	| watchstmt
	| fornetcon
	| NRNMUTEXLOCK { nrnmutex(1,$1); }
	| NRNMUTEXUNLOCK { nrnmutex(0,$1); }
	| error
		{myerr("Illegal statement");}
	;	
astmt:	asgn
		/* ';' is added when relevant */
		{astmt_end_ = insertsym(lastok->next, semi);}
	| PROTECT {protect_ = 1;} asgn
		{protect_ = 0; astmt_end_ = insertsym(lastok->next, semi);
			astmt_end_ = protect_astmt($1, astmt_end_);
		}
	| {inequation = 1;} reaction {
		$$ = $2; inequation = 0;
		astmt_end_ = insertstr(lastok->next->next->next, "");}
	| funccall
		{astmt_end_ = insertsym(lastok->next, semi);}
	;

asgn:	varname '=' expr
		/* depending on blocktype, varname may get marked as used. */
		{ if (blocktype == DERIVATIVE && SYM($1)->type == PRIME) {
			/* put Dvar in a derivative used list */
			deriv_used(SYM($1), $3, lastok);
			}
		  if (blocktype == DERIVATIVE && (SYM($1)->subtype & STAT)) {
			Fprintf(stderr,
"WARNING: %s (a STATE) is assigned a value\
 in a DERIVATIVE block.\n Multistep integrators (such as Runge) may not\
 work correctly.\n", SYM($1)->name);
		  }
		  if (blocktype == DISCRETE && SYM($1)->type == NAME
			&& (SYM($1)->subtype & STAT)) {
			SYM($1)->used++;
			}
		  if (blocktype == NETRECEIVE) {
			/* STATE discontinuity adjustment */
			netrec_asgn($1, $2, $3, lastok);
		  }
#if NOCMODL
		  nrn_var_assigned(SYM($1));
#endif
		}
	| nonlineqn expr '=' expr
		/* put info in equation list */
		{ inequation = 0;
		Insertstr($2, " -(");
		replacstr($3, ") + ");
		if (nstate == 0)
{yyerror("previous equation contains no state variables"); YYERROR;}
		 eqnqueue($1);
		}
	| lineqn leftlinexpr '=' linexpr
		{ inequation = 0;
		delete($3);
		if (nstate == 0)
{yyerror("previous equation contains no state variables"); YYERROR;}
		}
	;
varname: name
		/* much marking depending on context */
		{lastok = $1;
		if (!extdef2){SYM($1)->usage |= DEP;}
		if (SYM($1)->subtype & ARRAY && !extdef2)
			{myerr("variable needs an index");}
		if (inequation && (SYM($1)->subtype & STAT) && in_solvefor(SYM($1))) {
			SYM($1)->used++;
			nstate++; pstate++; tstate++;
		}
		if (SYM($1)->subtype & INTGER) {
			lastok = insertstr($1->next, ")");
			$1 = insertstr($1, "((double)");
		}
		}
	| name '[' intexpr ']'
		{lastok = $4;
		if (SYM($1)->type == PRIME) {
			usederivstatearray = 1;
		}
		SYM($1)->usage |= DEP;
		if ((SYM($1)->subtype & ARRAY) == 0)
			{myerr("variable is not an array");}
		if (inequation && (SYM($1)->subtype & STAT) && in_solvefor(SYM($1))) {
			SYM($1)->used++;
			nstate++; pstate++; tstate++;
		}
		  if (forallindex) {
			if (forallindex == -1) {
				forallindex = SYM($1)->araydim;
			}
			if (forallindex != SYM($1)->araydim) {
Sprintf(buf, "%s dimension not same as other dimensions used in FORALL statement",
SYM($1)->name);
				diag(buf, (char *)0);
			}
		  }
		}
	| NAME '@' integer
		{lastok = $3;
		SYM($1)->usage |= DEP; disc_var_seen($1, $2, $3, 0);}
	| NAME '@' integer '[' intexpr ']' 
		{lastok = $6;
		SYM($1)->usage |= DEP; disc_var_seen($1, $2, $3, ARRAY);}
	;
intexpr: Name
		{lastok = $1;
		 SYM($1)->usage |= DEP;
		 if (!(SYM($1)->subtype & INTGER)) {
		 	lastok = insertstr($1->next, ")");
			$1 = insertstr($1, "((int)");
		 }
		}
	| integer		{ lastok = $1;}
	| '(' intexpr ')'	{ lastok = $3;}
	| intexpr '+' intexpr
	| intexpr '-' intexpr
	| intexpr '*' intexpr
	| intexpr '/' intexpr
	| INT '(' expr ')' {
		lastok = $4;
		replacstr($1, " (int)");
		}
	| error {myerr("Illegal integer expression");}
	;
expr:	varname
	| real units
	| funccall
	| '(' expr ')'	{lastok = $3;}
	| expr '+' expr
	| expr '-' expr
	| expr '*' expr
	| expr '/' expr
	| expr '^' expr	/* converted to pow() */
		{ $$ = insertstr($1, "pow("); replacstr($2, ",");
			lastok = insertstr(lastok->next, ")"); }
	| expr OR expr {replacstr($2, " ||");}
	| expr AND expr {replacstr($2, " &&");}
	| expr GT expr
	| expr LT expr
	| expr GE expr
	| expr LE expr
	| expr EQ expr {replacstr($2, " ==");}
	| expr NE expr {replacstr($2, " !=");}
	| NOT expr	 {replacstr($1, " !");}
	| '-' expr %prec UNARYMINUS
	| error {myerr("Illegal expression");}
	;
nonlineqn: NONLIN1 {inequation = 1; nstate = 0;}
	;
lineqn: LIN1
		/* initialize a bunch of equation info */
		{inequation = 1; nstate = 0;
		pstate = 0; tstate = 0; init_lineq($1); leftside = -1;}
	;
leftlinexpr: linexpr {leftside = 1;}
	;
linexpr: primary
		/* put terms in a list */
		{linterm($1, lastok, pstate, leftside); pstate = 0;}
	| '-' primary
		{delete($1);
		linterm($2, lastok, pstate, -leftside); pstate = 0;}
	| linexpr '+' primary
		{delete($2);
		linterm($3, lastok, pstate, leftside); pstate = 0;}
	| linexpr '-' primary
		{delete($2);
		linterm($3, lastok, pstate, -leftside); pstate = 0;}
	;
primary: term
		{ if (tstate == 1) {
			lin_state_term($1, lastok);
		  }
		  tstate = 0;
		}
	| primary '*' term
		{ if (tstate == 1) {
			lin_state_term($3, lastok);
		  }
		  tstate = 0;
		}
	| primary '/' term
		{ if (tstate) {
			diag("state ocurs in denominator", (char *)0);
		  }
		}
	;
term:	varname
	| real
	| funccall {if (tstate) diag("states not permitted in function calls",
			(char *)0);}
	| '(' expr ')' { lastok = $3;
			if (tstate) diag("states not permitted between ",
				"parentheses");}
	| error
{diag("Some operators are not permitted in linear\n",
"expressions unless the terms containing them are\nenclosed in parentheses");}
	;
funccall: NAME '('
		{ if (SYM($1)->subtype & EXTDEF2) { extdef2 = 1;}}
	   exprlist ')'
		{lastok = $5; SYM($1)->usage |= FUNCT;
		 if (SYM($1)->subtype & EXTDEF2) { extdef2 = 0;}
		 if (SYM($1)->subtype & EXTDEF3) { add_reset_args($2);}
		 if (SYM($1)->subtype & EXTDEF4) { add_nrnthread_arg($2);}
		 if (SYM($1)->subtype & EXTDEF5) {
			if (!assert_threadsafe) {
fprintf(stderr, "Notice: %s is not thread safe\n", SYM($1)->name);
				vectorize = 0;
			}
		 }
#if VECTORIZE
		 vectorize_use_func($1,$2,$4,$5,blocktype);
#endif
		}
	;
exprlist: /*nothing*/{$$ = ITEM0;}
	| expr
	| STRING
	| exprlist ',' expr
	| exprlist ',' STRING
	;
fromstmt: FROM NAME {pushlocal(); SYM($2) = copylocal(SYM($2));
			SYM($2)->subtype |= INTGER;}
	'=' intexpr TO intexpr opinc stmtlist '}'
		/* translate the whole thing using _lNAME as an integer */
		{ replacstr($1, "{int ");
		poplocal();
		Insertstr($4, ";for (");
		Insertstr($4, SYM($2)->name);
		Insertstr($6, ";");
		Insertstr($6, SYM($2)->name);
		replacstr($6, "<=");
		if ($8) {
			Insertstr($8, ";");
			Insertstr($8, SYM($2)->name);
			replacstr($8, "+=");
		} else {
			Insertstr($9, ";");
			Insertstr($9, SYM($2)->name);
			Insertstr($9, "++");
		}
		Insertstr($9, ")");
		Insertstr($10, "}");
		}
	|FROM error {
myerr("FROM intvar = intexpr TO intexpr BY intexpr { statements }");}
	;
opinc: /*nothing*/ {$$ = ITEM0;}
	| BY intexpr
	;
forallstmt: FORALL1 NAME {pushlocal(); SYM($2) = copylocal(SYM($2));
			  SYM($2)->subtype |= INTGER;
			  if (forallindex) {
diag("Nested FORALL statements not allowed", (char *)0);
			  }
			  forallindex = -1;
			}
	stmtlist '}'
		/* translate the whole thing using _lNAME as an integer */
		{ replacstr($1, "{int ");
		poplocal();
		if (forallindex == -1) {
diag("FORALL range is undefined since no arrays used", " within the statement");
		}
		Sprintf(buf, "; for (%s=0; %s<%d; %s++)", SYM($2)->name,
			SYM($2)->name, forallindex, SYM($2)->name);
		Insertstr($4, buf);
		Insertstr($5, "}");
		}
	|FORALL1 error {
myerr("FORALL intvar { statements }");}
	;
whilestmt: WHILE '(' expr ')' stmtlist '}'
	;
ifstmt:	IF '(' expr ')' stmtlist '}' optelseif optelse
	;
optelseif: /*nothing*/
	| optelseif ELSE IF '(' expr ')' stmtlist '}'
		{
#if VECTORIZE
			vectorize_if_else_stmt(blocktype);
#endif
		}
	;
optelse: /*nothing*/
	| ELSE stmtlist '}'
		{
#if VECTORIZE
			vectorize_if_else_stmt(blocktype);
#endif
		}
	;
derivblk: DERIVATIVE NAME stmtlist '}'
		/* process using massagederiv() */
		{massagederiv($1, $2, $3, $4, sensused); sensused = 0;}
	;
linblk: LINEAR NAME solvefor {init_linblk($2);} stmtlist '}'
		/*massage_linblk()*/
		{massage_linblk($1, $2, $5, $6, sensused);
		lexcontext = 0; sensused = 0;
		}
	;
nonlinblk: NONLINEAR NAME solvefor stmtlist '}'
		/*massagenonlin()*/
		{massagenonlin($1, $2, $4, $5, sensused);
		lexcontext = 0; sensused = 0;
		}
	;
discretblk: DISCRETE NAME stmtlist '}'
		 /*massagediscblk()*/
		{massagediscblk($1, $2, $3, $4);}
	;
partialblk: PARTIAL NAME stmtlist '}'
		/*massagepartial()*/
		{massagepartial($1, $2, $3, $4);
		lexcontext = 0;
		}
	| PARTIAL error {
diag("within the PARTIAL block must occur an equation with the syntax ---\n",
"~ V' = F*DEL2(V) + G\n"); }
	;
pareqn: '~' PRIME '=' NAME '*' DEL2 '(' NAME ')' '+' NAME
		{partial_eqn($2, $4, $8, $11);}
	| '~' DEL NAME '[' firstlast ']' '=' expr
		{partial_bndry(0, $3, $5, $8, lastok);}
	| '~' NAME '[' firstlast ']' '=' expr
		{partial_bndry(2, $2, $4, $7, lastok);}
	;
firstlast: FIRST | LAST
	;
functableblk: FUNCTION_TABLE NAME '(' arglist ')' units
		{Item *b1, *b2;
		Symbol* s = SYM($2);
		s->varnum = argcnt_;
		b1 = insertstr($5->next, "{\n");
		b2 = insertstr(b1->next, "}\n");

#define GLOBFUNCT 1
#if GLOBFUNCT && NMODL
		replacstr($1, "\ndouble");
#else
		replacstr($1, "\nstatic double");
#endif
		defarg($3, $5);
		movelist($1, b2, procfunc);
		if (SYM($2)->subtype & FUNCT) {
			diag(SYM($2)->name, " declared as FUNCTION twice");
		}
		SYM($2)->subtype |= FUNCT;
		SYM($2)->usage |= FUNCT;
#if HMODL || NMODL
		hocfunc(s, $3, $5);
#endif
		function_table(s, $3, $5, b1, b2);
		poplocal();
		}
	;
funcblk: FUNCTION1 NAME '(' arglist ')' units
		{IGNORE(copylocal(SYM($2)));}
	stmtlist '}'
		/* boilerplate added to form double function(){...}
		   Note all arguments have prefix _l */
		{ Symbol *s = SYM($2);
		s->varnum = argcnt_;
		table_massage(table_list, $1, $2, $4); freelist(&table_list);
#if GLOBFUNCT && NMODL
		replacstr($1, "\ndouble");
#else
		replacstr($1, "\nstatic double");
#endif
		defarg($3, $5);
		Sprintf(buf, "double _l%s;\n", s->name);
		Insertstr($8->next, buf);
		Sprintf(buf, "\nreturn _l%s;\n", s->name);
		Insertstr($9, buf);
		movelist($1, $9, procfunc);
		if (SYM($2)->subtype & FUNCT) {
			diag(SYM($2)->name, " declared as FUNCTION twice");
		}
		SYM($2)->subtype |= FUNCT;
		SYM($2)->usage |= FUNCT;
#if HMODL || NMODL
		hocfunc(s, $3, $5);
#endif
		poplocal(); freelist(&$4);}
	;
arglist: /*nothing*/ {pushlocal(); $$ = LIST0; argcnt_ = 0;}
	| {pushlocal();} arglist1 {$$ = $2;}
	;
arglist1: name units
		{SYM($1) = copylocal(SYM($1)); argcnt_ = 1;
		 $$ = newlist(); Lappendsym($$, SYM($1));
		}
	| arglist1 ',' name units
		{SYM($3) = copylocal(SYM($3)); Lappendsym($$, SYM($3));
		 ++argcnt_;
		}
	;
procedblk: PROCEDURE NAME '(' arglist ')' units stmtlist '}'
		{Symbol *s = SYM($2);
		s->u.i = 0; 	/* avoid objectcenter warning if solved */
		s->varnum = argcnt_; /* allow proper number of "double" in prototype */
		table_massage(table_list, $1, $2, $4); freelist(&table_list);
		replacstr($1, "\nstatic int "); defarg($3, $5);
		Insertstr($8, " return 0;");
		movelist($1, $8, procfunc);
		if (SYM($2)->subtype & PROCED) {
			diag(SYM($2)->name, " declared as PROCEDURE twice");
		}
		SYM($2)->subtype |= PROCED;
		SYM($2)->usage |= FUNCT;
#if HMODL || NMODL
		hocfunc(s, $3, $5);
#endif
		poplocal(); freelist(&$4);}
	;
netrecblk: NETRECEIVE '(' arglist ')'
		{
			nr_argcnt_ = argcnt_;
			/* add flag arg */
			if ($3 == LIST0) {
diag("NET_RECEIVE block"," must have at least one argument");
			}
			Lappendsym($3, copylocal(install("flag", NAME)));
		}
	     stmtlist '}'
		{
		replacstr($1, "\nstatic void _net_receive");
		movelist($1, $7, procfunc);
#if NMODL
		net_receive($3, $2, $4, $6, $7);
#endif
		poplocal(); freelist(&$3);
		}
	| NETRECEIVE error { myerr("syntax: NET_RECEIVE ( weight ) {stmtlist}");}
	;
initstmt: INITIAL1 stmtlist '}'
		{
			check_block(NETRECEIVE, blocktype, "INITIAL");
#if NMODL
			net_init($1, $3);
#endif
		}
	;
solveblk: SOLVE NAME ifsolerr
		/* no processing, merely save in solve list */
		{ solvequeue($2, ITEM0, blocktype, $3); }
	| SOLVE NAME USING METHOD ifsolerr
		{ solvequeue($2, $4, blocktype, $5); }
	| SOLVE error { myerr("Illegal SOLVE statement");}
	;
ifsolerr: /*nothing*/
		{ $$ = ITEM0; }
	| IFERROR stmtlist '}'
		{ $$ = $3; }
	;
solvefor: /*nothing*/
		{ if (solveforlist) {
			freelist(&solveforlist);
		  }
		}
	| solvefor1
	;
solvefor1: SOLVEFOR NAME
		{ solveforlist = newlist(); Lappendsym(solveforlist, SYM($2));
		  delete($1); delete($2);
		}
	| solvefor1 ',' NAME
		{ Lappendsym(solveforlist, SYM($3)); delete($2); delete($3);}
	| SOLVEFOR error { myerr("Syntax: SOLVEFOR name, name, ...");}
	;
brkptblk: BREAKPOINT stmtlist '}'
		/* move it all to modelfunc */
		{brkpnt_exists = 1; movelist($2, $3, modelfunc);}
	;
terminalblk: TERMINAL stmtlist '}'
		{movelist($2, $3, termfunc);}
	;
bablk: BREAKPOINT stmtlist '}'
		{bablk(blocktype, BREAKPOINT, $2, $3);}
	| SOLVE stmtlist '}'
		{bablk(blocktype, SOLVE, $2, $3);}
	| INITIAL1 stmtlist '}'
		{bablk(blocktype, INITIAL1, $2, $3);}
	| STEP stmtlist '}'
		{bablk(blocktype, STEP, $2, $3);}
	| error {myerr("[BEFORE AFTER] [BREAKPOINT SOLVE INITIAL STEP] { stmt }");}
	;
watchstmt: WATCH watch1
		{$$ = $2; delete($1);}
	| watchstmt ',' watch1
		{delete($2);}
	| WATCH error {myerr("WATCH (expr > expr) flag");}
	;
watch1:	'(' aexpr watchdir aexpr ')' real
		{watchstmt($1, $3, $5, $6, blocktype); $$=$6;}
	;
watchdir: GT
	| LT
	;
fornetcon: FOR_NETCONS '(' arglist ')' {
			if (blocktype != NETRECEIVE) {
				diag("\"FOR_NETCONS\" statement only allowed in NET_RECEIVE block", (char*)0);
			}
			if (argcnt_ != nr_argcnt_) {
				diag("FOR_NETCONS and NET_RECEIVE do not have same number of arguments", (char*)0);
			}
		}
	stmtlist '}'
		{fornetcon($1, $2, $3, $4, $6, $7); $$ = $7; }
	| FOR_NETCONS error { myerr("syntax: FOR_NETCONS(args,like,netreceive) { stmtlist}");}
	;
aexpr:	varname
	| real units
	| funccall
	| '(' aexpr ')'	{lastok = $3;}
	| aexpr '+' aexpr
	| aexpr '-' aexpr
	| aexpr '*' aexpr
	| aexpr '/' aexpr
	| aexpr '^' aexpr	/* converted to pow() */
		{ $$ = insertstr($1, "pow("); replacstr($2, ",");
			lastok = insertstr(lastok->next, ")"); }
	| '-' aexpr %prec UNARYMINUS
	| error {myerr("Illegal expression");}
	;

sens:	SENS senslist
		/* sensused flag is true */
		{ sensused = 1;
		  delete($1);
		}
	|SENS error {myerr("syntax is SENS var1, var2, var3, etc");}
	;
senslist: varname
		/* put in list. Used when enclosing block is massaged */
		{ sensparm($1); delete($1);}
	| senslist ',' varname
		{ sensparm($3); deltokens($2, $3);}
	;

conserve: CONSERVE {extdef2 = 0; } react '=' expr
		{/* react originally designed for reactions and is unchanged*/
		extdef2 = 0;
		massageconserve($1, $4, lastok);}
	| CONSERVE error {myerr("Illegal CONSERVE syntax");}
	;
compart: COMPARTMENT NAME ','
	 {pushlocal(); SYM($2) = copylocal(SYM($2));}
	 expr '{' namelist '}'
		{massagecompart($5, $6, $8, SYM($2)); poplocal();}
	| COMPARTMENT expr '{' namelist '}'
		{massagecompart($2, $3, $5, SYM0);}
	;
ldifus: LONGDIFUS NAME ','
	 {pushlocal(); SYM($2) = copylocal(SYM($2));}
	 expr '{' namelist '}'
		{massageldifus($5, $6, $8, SYM($2)); poplocal();}
	| LONGDIFUS expr '{' namelist '}'
		{massageldifus($2, $3, $5, SYM0);}
	;
namelist: NAME
	| namelist NAME
	;
kineticblk: KINETIC NAME solvefor stmtlist '}'
		/* put all reactions in a list for processing when relevant
		   SOLVE is processed */
		{massagekinetic($1, $2, $4, $5, sensused);
		lexcontext = 0; sensused = 0;
		}
	;
reaction: REACTION react REACT1 {leftreact();} react '(' expr ',' expr ')'
		{massagereaction($1,$3,$6,$8,$10);}
	| REACTION react LT LT  '(' expr ')'
		{flux($1, $3, $7);}
	| REACTION react '-' GT '(' expr ')'
		{flux($1, $3, $7);}
	| REACTION error {myerr("Illegal reaction syntax");}
	;
react:	varname {reactname($1, lastok, ITEM0);}
	|integer varname	{reactname($2, lastok, $1);}
	|react '+' varname	{reactname($3, lastok, ITEM0);}
	|react '+' integer varname {reactname($4, lastok, $3);}
	;
lagstmt: LAG name BY NAME
		{lag_stmt($1, blocktype);}
	| LAG error {myerr("Lag syntax is: LAG name BY const");}
	;
queuestmt: PUTQ name {queue_stmt($1, $2);}
	| GETQ name {queue_stmt($1, $2);}
	;
matchblk: MATCH {checkmatch(blocktype);} '{' matchlist '}'
		{deltokens($1, $5);}
	;
matchlist: match
	| matchlist match
	;
match:	name
		{matchinitial($1);}
	| matchname '(' expr ')' '=' expr
		{ matchbound($1, $2, $4, $6, lastok, matchindex);
		  if (matchindex) {
			poplocal();
			matchindex = SYM0;
		  }
		}
	| error
		{myerr("MATCH syntax is state0 or state(expr)=expr or\
state[i](expr(i)) = expr(i)");}
	;
matchname: name
		{matchindex = SYM0;}
	| name '[' NAME ']'
		{ pushlocal();
		  matchindex = copylocal(SYM($3));
		}
	;
unitblk: UNITS '{' unitbody '}'
		{ deltokens($1,$4);}
	;
unitbody: /*nothing*/
		{modl_units();}
	| unitbody unitdef
	| unitbody factordef
	;
unitdef: unit '=' unit
		{install_units($1, $3);}
	| unit error {myerr("Unit definition syntax: (units) = (units)");}
	;
factordef: NAME '=' real unit
		{ SYM($1)->subtype |= nmodlCONST;
		  Sprintf(buf, "static double %s = %s;\n", SYM($1)->name,
			STR($3));
		  Lappendstr(firstlist, buf);
		}
	| NAME '=' unit unit
		{
		    SYM($1)->subtype |= nmodlCONST;
		    nrnunit_dynamic_str(buf, SYM($1)->name, $3, $4);
		    Lappendstr(firstlist, buf);
		}
	| NAME '=' unit '-' GT unit 
		{
		    SYM($1)->subtype |= nmodlCONST;
		    nrnunit_dynamic_str(buf, SYM($1)->name, $3, $6);
		    Lappendstr(firstlist, buf);
		}
	| error {myerr("Unit factor syntax: examples:\n\
foot2inch = (foot) -> (inch)\n\
F = 96520 (coulomb)\n\
R = (k-mole) (joule/degC)");
		}
	;
constblk: CONSTANT '{' conststmt '}'
	;
conststmt: /*nothing*/
	| conststmt NAME '=' number units
		{ SYM($2)->subtype |= nmodlCONST;
		  Sprintf(buf, "static double %s = %s;\n", SYM($2)->name,
			STR($4));
		  Lappendstr(firstlist, buf);
		}
	;
tablestmt: TABLE tablst dependlst FROM expr TO expr WITH INTEGER
		{ Item *q;
		  if (table_list) {
			diag("only one TABLE per function", (char *)0);
		  }
		  table_list = newlist();
		  Lappendlst(table_list, $2);
		  q = lappendlst(table_list, newlist());
		  movelist($4->next, $6->prev, LST(q));
		  q = lappendlst(table_list, newlist());
		  movelist($6->next, $8->prev, LST(q));
		  Lappendstr(table_list, STR($9));
		  Lappendlst(table_list, $3);
		  deltokens($1, $9);
		}
	| TABLE error { myerr("syntax: TABLE [list] [DEPEND list] FROM expr TO expr WITH integer");}
	;
tablst: /*Nothing*/
		{$$ = LIST0;}
	| tablst1
	;
tablst1: Name
		{$$ = newlist(); Lappendsym($$, SYM($1));}
	| tablst1 ',' Name
		{ Lappendsym($1, SYM($3));}
	;
dependlst: /*NOTHING*/
		{$$ = LIST0;}
	| DEPEND tablst1
		{$$ = $2;}
	;
neuronblk: NEURON '{' nrnstmt '}'
		{ deltokens($1,$4);}
	;
nrnstmt: /*nothing*/
	| nrnstmt SUFFIX NAME
		{ nrn_list($2, $3);}
	| nrnstmt nrnuse
	| nrnstmt REPRESENTS ONTOLOGY_ID
	| nrnstmt NONSPECIFIC nrnlist
		{ nrn_list($2,$3);}
	| nrnstmt ELECTRODE_CURRENT nrnlist
		{ nrn_list($2,$3);}
	| nrnstmt SECTION nrnlist
		{ nrn_list($2, $3);}
	| nrnstmt RANGE nrnlist
		{ nrn_list($2, $3);}
	| nrnstmt GLOBAL nrnlist
		{ nrn_list($2, $3);}
	| nrnstmt POINTER nrnlist
		{ nrn_list($2, $3);}
	| nrnstmt BBCOREPOINTER nrnlist
		{ nrn_list($2, $3);}
	| nrnstmt EXTERNAL nrnlist
		{ nrn_list($2, $3);}
	| nrnstmt THREADSAFE optnrnlist
		{ threadsafe_seen($2, $3); }
	;
nrnuse: USEION NAME READ nrnlist valence optontology
		{nrn_use($2, $4, ITEM0, $5);}
	|USEION NAME WRITE nrnlist valence optontology
		{nrn_use($2, ITEM0, $4, $5);}
	|USEION NAME READ nrnlist WRITE nrnlist valence optontology
		{nrn_use($2, $4, $6, $7);}
	|USEION error
		{myerr("syntax is: USEION ion READ list WRITE list REPRESENTS curie");}
	;
optontology: { $$ = NULL; }
           | REPRESENTS ONTOLOGY_ID
             { $$ = $2; }
nrnlist: NAME
	| nrnlist ',' NAME
		{ delete($2); $$ = $3;}
	| error
		{myerr("syntax is: keyword name , name, ..., name");}
	;
optnrnlist: /*nothing*/
		{$$ = NULL;}
	| nrnlist
	;
valence: /*nothing*/
		{$$ = ITEM0;}
	| VALENCE real
		{$$ = $2;}
	| VALENCE '-' real
		/* replace the string with -string and discard '-'*/
                { Sprintf(buf, "-%s", STR($3));
		 STR($3) = stralloc(buf, STR($3)); $$ = $3;
		delete($2); lastok = $3;
		}
	;
%%
void yyerror(s)	/* called for yacc syntax error */
	char *s;
{
	Fprintf(stderr, "%s:\n ", s);
}


#if !NMODL
void nrn_list(q1, q2)
	Item *q1, *q2;
{
	/*ARGSUSED*/
}
void nrn_use(q1, q2, q3, q4)
	Item *q1, *q2, *q3, *q4;
{
	/*ARGSUSED*/
}
#endif
