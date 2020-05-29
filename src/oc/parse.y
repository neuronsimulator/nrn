/* /local/src/master/nrn/src/oc/parse.y,v 1.29 1998/11/27 13:11:48 hines Exp */
%{

#include <../../nrnconf.h>
/* changes as of 2-jan-89 */
/*  version 7.2.1 2-jan-89 short form of the for statement */

#if AIX
#pragma alloca
#endif

#include "hoc.h"
#include "ocmisc.h"
#include "hocparse.h"
#include "code.h"
#include "equation.h"
#include "nrnfilewrap.h"

void* nrn_parsing_pysec_;

#if LINT
Inst *inlint;
#define code	inlint = Code
#else
#define code	Code
#endif

#define paction(arg) fprintf(stderr, "%s\n", arg)

/* maintain a list of ierr addresses so we can clear them */
#define HOCERRSIZE 20
static int** hoc_err;
static int hoc_errp;
static int localcnt;

static void clean_err(void) {
	int i;
	for (i=0; i < hoc_errp; ++i) {
		*hoc_err[i] = 0;
	}
	hoc_errp = 0;
}

static void pusherr(int* ip) {
	if (!hoc_err) {
		hoc_err = (int**)ecalloc(HOCERRSIZE, sizeof(int*));
		hoc_errp = 0;
	}
	if (hoc_errp >= HOCERRSIZE) {
		clean_err();
		hoc_execerror("error stack full", (char*)0);
	}
	hoc_err[hoc_errp++] = ip;
}

static void yyerror(const char* s);

#if YYBISON
#define myerr(arg) static int ierr=0;\
if (!(ierr++)){pusherr(&ierr);yyerror(arg);} --yyssp; --yyvsp; YYERROR
#else
#define myerr(arg) static int ierr=0;\
if (!(ierr++)){pusherr(&ierr);yyerror(arg);} YYERROR
#endif

#define code2(c1,c2)	code(c1); codein(c2)
#define code3(c1,c2,c3)	code(c1); codesym(c2); code(c3)
#define relative(ip1,ip2,offset)	((ip1-ip2) - offset)
#define CHECK 1	/* check syntactically the consistency of arrays */
#define NOCHECK 0 /* don't check syntactically. For object components */
#define PN pushi(NUMBER)	/* for type checking. expressions are usually numbers */
#define TPD hoc_ob_check(NUMBER);
#define TPDYNAM hoc_ob_check(0);

static Inst *prog_error;			/* needed for stmtlist loc if error */
static int ntab;			/* auto indentation */

static Inst* argrefcode(Pfrv pfrv, int i, int j);
static Inst* argcode(Pfrv pfrv, int i);
static void hoc_opasgn_invalid(int op);
 
%}

%union {				/* stack type */
	Symbol	*sym;			/* symbol table pointer */
	Inst	*inst;			/* machine instruction */
	int	narg;			/* number of arguments */
	void*	ptr;
}

%token		EQNEQ
%token	<sym>	NUMBER STRING PRINT parseDELETE VAR BLTIN UNDEF WHILE IF ELSE FOR
%token	<sym>	FUNCTION PROCEDURE RETURN FUNC parsePROC HOCOBJFUNC READ parseDOUBLE
%token	<sym>	DEBUG EDIT FUN_BLTIN DEPENDENT EQUATION LOCAL HOCOBJFUNCTION
%token	<sym>	BREAK CONTINUE AUTO STRDEF STOPSTMT CSTRING PARALLEL HELP
%token	<sym>	ITERATOR ITERKEYWORD ITERSTMT STRINGFUNC OBJECTFUNC
%token	<sym>	LOCALOBJ AUTOOBJ
%token	<narg>	ARG NUMZERO ARGREF
%token	<ptr>	INTERNALSECTIONNAME PYSEC PYSECNAME PYSECOBJ
%type	<inst>	expr stmt asgn prlist delsym stmtlist strnasgn
%type	<inst>	cond while if begin end for_init for_st for_cond for_inc
%type	<inst>	eqn_list dep_list varname wholearray array pointer
%type	<inst>	doublelist strlist string1 string2
%type	<inst>  iterator
%type	<sym>	procname newname function string ckvar
%type	<sym>	anyname
%type	<narg>	arglist arglist1 local local1 newarray numdimen procstmt
%type	<narg>	localobj local2
%type	<ptr>	pysec pysec1

/* NEWCABLE */
%token	<sym>	SECTIONKEYWORD SECTION CONNECTKEYWORD ACCESSKEYWORD
%token	<sym>	RANGEVAR MECHANISM INSERTKEYWORD FORALL NRNPNTVAR FORSEC IFSEC
%token	<sym>	UNINSERTKEYWORD SETPOINTERKEYWORD SECTIONREF
%type	<sym>	sectiondecl sectionname
%type	<inst>	rangevar rangevar1 section section_or_ob
/* END NEWCABLE */

/* OOP */
%token	<sym>	BEGINTEMPLATE ENDTEMPLATE NEW OBJECTVAR TEMPLATE
%token	<sym>	OBJVARDECL PUBLICDECL EXTERNALDECL OBFUNCTION STRFUNCTION
%token	<narg>	OBJECTARG STRINGARG ROP
%type	<sym>	template publiclist externallist
%type	<sym>	obvarname
%type	<inst>	ob ob1 objvarlist object
%type	<narg>	func_or_range_array_case argrefdim
/* END OOP */

%right	'='
%right	ROP
%left	OR
%left	AND
%left	GT GE LT LE EQ NE
%left	'+' '-'	/* left associative, same precedence */
%left	'*' '/'	'%'/* left assoc., higher precedence */
%left	UNARYMINUS NOT
%right	'^'	/* exponentiation */

%%
list:	/* nothing */
		{ ntab = 0;}
	| list '\n' { return '\n';}
	| list defn '\n' { return '\n';}
	| list asgn '\n'
		{ hoc_ob_check(-1); code2(nopop, STOP); return 1; }
	| list stmt '\n'
		{ codein(STOP); return 1; }
	| list expr '\n'
		{ TPDYNAM; code2(print, STOP); return 1; }
	| list DEBUG '\n'
		{ debug(); return '\n';}
	| list EDIT '\n'
		{ return 'e';}
	| list string1 '\n'
		{code(prstr); code2(hoc_newline, STOP); return 1; }

/* OOP */
	| list template '\n' { return '\n';}
/* END OOP */
/* no longer useful
	| list '' '\n'
		{ plt(-3,0.,0.); return '\n';}
*/
	| list HELP {hoc_help();} '\n' { return '\n'; }
	| list error
		{clean_err(); hoc_execerror("parse error", (char*)0);
#if LINT
if (0) {
if(yydebug);
goto yynewstate;
}
#endif
		}
	;
asgn:	varname ROP expr
		{Symbol *s; TPD; s = spop();
		hoc_obvar_declare(s, VAR, 1);
		code3(varpush, s, assign); codei($2); PN;}
	| ARG ROP expr
		{  TPD; defnonly("$"); argcode(argassign, $1); codei($2); $$=$3; PN;}
	| ARGREF argrefdim ROP expr
		{ TPD; defnonly("$&"); argrefcode(hoc_argrefasgn, $1, $2); codei($3); $$=$4; PN;}
/* NEWCABLE */
	| rangevar ROP expr
		{ TPD; code(range_const); codesym(spop()); codei($2); PN;}
/* END NEWCABLE */
/* OOP */
	|ob1 ROP NEW anyname '(' arglist ')'
		{ Inst* p; hoc_opasgn_invalid($2);
		 code(hoc_newobj); codesym(hoc_which_template($4)); codei($6);
		 p = (Inst*)spop();
		 if (p) { p->i += 2; }
		}
	| ob1 ROP expr
		{Inst* p; TPDYNAM; code(hoc_object_asgn); codei($2);
		 p = (Inst*)spop();
		 if (p) { p->i += 2; }
		}
/* END OOP */
	| varname ROP error {myerr("assignment to variable, make sure right side is a number");}
	;

/* OOP */
object:	OBJECTVAR {pushi(OBJECTVAR);pushs($1); pushi(CHECK);} wholearray
		{$$ = $3; code(hoc_objectvar); spop(); codesym($1);}
	| OBJECTARG
		{defnonly("$o"); $$ = argcode(hoc_objectarg, $1); pushi(OBJECTVAR);}
	| AUTOOBJ
		{$$ = code(hoc_autoobject); codesym($1); pushi(OBJECTVAR);}
	| TEMPLATE '[' expr ']'
		{TPD; $$ = $3; code(hoc_constobject); codesym($1); pushi(OBJECTVAR);}
	| OBJECTFUNC begin '(' arglist ')'
		{ $$ = $2; code(call); codesym($1); codei($4);
		  code(hoc_known_type); codei(OBJECTVAR); pushi(OBJECTVAR);}
	| HOCOBJFUNCTION begin '(' arglist ')'
		{ $$ = $2; code(call); codesym($1); codei($4);
		  code(hoc_known_type); codei(OBJECTVAR); pushi(OBJECTVAR);}
	;

ob: ob1 { spop(); }
	;

ob1:	object { pushs((Symbol*)0); }
	| ob1 '.' anyname {pushs($3);pushi(NOCHECK);} wholearray func_or_range_array_case
		{int isfunc; Inst* p;
		 isfunc = ipop();
		 code(hoc_object_component); codesym(spop()); codei(ipop());
		 codei($6);
		 codei(0); codesym(0);
		 p = codei(isfunc); /* for USE_PYTHON */
		 spop();
		 pushs((Symbol*)p); /* in case assigning to a PythonObject we will want to update isfunc to 2 */
		}
	| OBJECTVAR error {myerr("object syntax is o1.o2.o3.");}
	;
func_or_range_array_case:	/* nothing */
		{$$ = 0; pushi(0);}
	| '(' arglist ')'
		{$$ = $2; pushi(1);}
	;
template: BEGINTEMPLATE anyname {hoc_begintemplate($2);}
	| publiclist
	| externallist
	| ENDTEMPLATE anyname {hoc_endtemplate($2);}
	| BEGINTEMPLATE error {myerr("begintemplate Name\npublic namelist\nexternal namelist\n...\nendtemplate Name");}
	;
objvarlist: OBJVARDECL begin objvarlst1
		{$$ = $2;}
	;
objvarlst1: obvarname
		{code(hoc_objvardecl); codesym($1); codei(0);}
	| obvarname numdimen
		{code(hoc_objvardecl); codesym($1); codei($2);}
	| objvarlst1 ',' obvarname
		{code(hoc_objvardecl); codesym($3); codei(0);}
	| objvarlst1 ',' obvarname numdimen
		{code(hoc_objvardecl); codesym($3); codei($4);}
	;
obvarname: anyname
		{
		  Symbol* s;
		  s = hoc_decl($1);
		  if (s->type != UNDEF && s->type != OBJECTVAR) {
			acterror(s->name, " already declared");
		  }
		  $$ = s;
		}
	;
publiclist: PUBLICDECL anyname
		{hoc_add_publiclist($2);}
	| publiclist ',' anyname
		{hoc_add_publiclist($3);}
	;
externallist: EXTERNALDECL VAR
		{hoc_external_var($2);}
	| externallist ',' VAR
		{hoc_external_var($3);}
	;
/* END OOP */

strnasgn: string2 ROP string1
		{hoc_opasgn_invalid($2); code(assstr);}
	| ob1 ROP string1
		{Inst* p = (Inst*) spop(); pushi(STRING); TPDYNAM; code(hoc_object_asgn);
		 hoc_opasgn_invalid($2); codei($2); hoc_ob_check(-1); code(nopop);
		 if (p) { p->i += 2; }
		}
	| string2 ROP ob
		{code(hoc_asgn_obj_to_str); hoc_opasgn_invalid($2); }
	| string2 error {myerr("string assignment: both sides need to be a string");}
;

string1: string2
	| CSTRING
		{$$ = code(hoc_push_string); codesym($1);}
	| STRINGFUNC begin '(' arglist ')'
		{ $$ = $2; code(call); codesym($1); codei($4);}
;

string2: STRING
		{$$ = code(hoc_push_string); codesym($1);}
	| STRINGARG
		{defnonly("$s"); $$ = argcode(hoc_stringarg, $1);}
	;

strlist: STRDEF begin string
		{ $$ = $2; }
	| strlist ',' string
	;
string:	anyname
		{
			Symbol* s = hoc_decl($1);
			if (s->type == UNDEF) {
				hoc_obvar_declare(s, STRING, 0);
				OPSTR(s) = (char**)emalloc(sizeof(char*));
				*OPSTR(s) = 0;
			}else if (s->type != STRING) {
				acterror(s->name, " already declared");
			}
			hoc_assign_str(OPSTR(s), "");
		}
	;
stmt:	expr
		{ code(nopop); hoc_ob_check(-1); /*don't check*/}
	| strlist
	| doublelist
/* OOP */
	| objvarlist
/* END OOP */
	| strnasgn
	| string1
		{ code(nopop); }
	| parseDELETE delsym
		{ $$ = $2;}
	| RETURN begin
		{ defnonly("return"); $$=$2; code(procret); }
	| RETURN expr
		{  if (indef == 3) {
			TPDYNAM; $$ = $2;
			code(hocobjret);
		   }else{
			TPD; defnonly("return"); $$=$2;
			code(funcret);
		   }
		}
	| RETURN NEW anyname '(' arglist ')'
		{$$ = code(hoc_newobj_ret); codesym(hoc_which_template($3)); codei($5);
		 code(hocobjret);
		}
	| ITERSTMT begin
		{ if (indef != 2) {
acterror("iterator_statement used outside an iterator declaration", 0);
		  }
			code(hoc_iterator_stmt);
		}
	| BREAK begin
		{ code(Break); $$ = $2; }
	| STOPSTMT begin
		{ code(Stop); $$ = $2; }
	| CONTINUE begin
		{ code(Continue); $$ = $2; }
	| PROCEDURE begin '(' arglist ')'
		{ $$ = $2; code(call); codesym($1); codei($4); code(nopop); }
	| PRINT prlist
		{ $$ = $2; code(hoc_newline); }
	| FOR begin iterator stmt end
		{ $$ = $2;
		  ($3)[0].i = relative($4, $3, 0); /* body */
		  ($3)[1].i = relative($5, $3, 1); /* exit */
		}
	| for_init for_st for_cond for_inc stmt end
			{
			($2)[1].i =relative($5, $2, 1);	/* body */
			($2)[2].i =relative($6, $2, 2); /* exit from the for */
			($2)[3].i  =relative($4, $2, 3);	/* increment */
			$$ = $1;
#if LINT
if (0){YYERROR;}
#endif
			}
	| FOR varname
		  { Symbol *s; $<inst>$ = Code(varpush); codesym(s = spop());
			hoc_obvar_declare(s, VAR, 1);
		  }
		ROP expr ',' expr
		  {TPD; TPD; hoc_opasgn_invalid($4); 
		    $<inst>$ = code(shortfor); codei(0); codei(0);}
		stmt end
		  { $$ = $2; ($<inst>8)[1].i = relative($9, $<inst>8, 1); /* body */
			   ($<inst>8)[2].i = relative($10, $<inst>8, 2); /* exit */
		  }
	| PARALLEL FOR varname
		  { Symbol *s; $<inst>$ = Code(varpush); codesym(s = spop());
			hoc_obvar_declare(s, VAR, 1);
		  }
		ROP expr ',' expr
		  {TPD; TPD; hoc_opasgn_invalid($5); 
		     code(hoc_parallel_begin);
		     $<inst>$ = code(shortfor); codei(0); codei(0);}
		stmt end
		  { $$ = $3; ($<inst>9)[1].i = relative($10, $<inst>9, 1); /* body */
			   ($<inst>9)[2].i = relative($11, $<inst>9, 2); /* exit */
			code(hoc_parallel_end);
		  }
	| while cond stmt end
		{
		($1)[1].i = relative($3, $1, 1);	/* body of loop */
		($1)[2].i = relative($4, $1, 2); }/* end, if cond fails */
	| if cond stmt end	/* else-less if */
		{
		($1)[1].i = relative($3, $1, 1);	/* thenpart */
		($1)[3].i = relative($4, $1, 3); }/* end, if cond fails */
	| if cond stmt end ELSE stmt end	/* if with else */
		{
		($1)[1].i = relative($3, $1, 1);	/* thenpart */
		($1)[2].i = relative($6, $1, 2);	/* elsepart */
		($1)[3].i = relative($7, $1, 3); }/* end, if cond fails */
	| '{'
		{ ntab++;}
	stmtlist '}'
		{
		ntab--; $$ = $3;
		}
	| eqn_list

/* NEWCABLE */
	| SECTIONKEYWORD begin sectiondecl { $$ = $2; }
	| CONNECTKEYWORD section_or_ob ',' expr
		{ TPD; $$ = $2; code(connectsection);}
	| CONNECTKEYWORD section_or_ob ',' section '(' expr ')'
		{ TPD; $$ = $2; code(simpleconnectsection);}
	| SETPOINTERKEYWORD rangevar '(' expr ')' ',' pointer
		{ TPD; $$ = $2; code(connectpointer); codesym(spop());}
	| SETPOINTERKEYWORD begin {code(nrn_cppp);} ob ',' pointer
		{ $$ = $2; code(connect_point_process_pointer);
			 hoc_ob_check(-1);}
	| ACCESSKEYWORD section
		{ $$ = $2; code(sec_access);}
	| ACCESSKEYWORD ob
		{ $$ = $2; hoc_ob_check(SECTION); code(sec_access_object);}
	| INSERTKEYWORD anyname
		{ Symbol* s = $2;
			$$ = Code(mech_access);
			if (s->type != MECHANISM) {
				s = hoc_table_lookup(s->name, hoc_built_in_symlist);
				if (!s || s->type != MECHANISM) {
					acterror($2->name, "is not a MECHANISM");
				}
			}
			codesym(s);}
	| UNINSERTKEYWORD MECHANISM
		{ $$ = Code(mech_uninsert); codesym($2);}
	| section stmt
		{ code(sec_access_pop);}
	| ob stmt end
		{ code(sec_access_pop); hoc_ob_check(-1);
			insertcode($2, $3, ob_sec_access);}
	| rangevar '(' expr ':' expr ')' ROP expr ':' expr
		{ TPD; TPD; TPD; TPD; code(range_interpolate); codesym(spop());
		  codei($7);
		}
	| rangevar '(' expr ')' ROP expr
		{ TPD; TPD; code(range_interpolate_single); codesym(spop());
		  codei($5);
		}
	| FOR '(' varname ')' 
		{Symbol *s; code(varpush); codesym(s = spop());
		 hoc_obvar_declare(s, VAR, 1);
		 $<inst>$ = code(for_segment); codei(0); codei(0);}
	stmt end
		{ $$ = $3; ($<inst>5)[1].i = relative($6, $<inst>5, 1); /* body */
			($<inst>5)[2].i = relative($7, $<inst>5, 2); /* exit */
		}
	| FOR '(' varname ','
		{Symbol *s; code(varpush); codesym(s = spop());
		hoc_obvar_declare(s, VAR, 1);}
	 expr ')' 
		{ TPD; $<inst>$ = code(for_segment1); codei(0); codei(0);}
	stmt end
		{ $$ = $3; ($<inst>8)[1].i = relative($9, $<inst>8, 1); /* body */
			($<inst>8)[2].i = relative($10, $<inst>8, 2); /* exit */
		}
	| FORALL begin
		{	code(hoc_push_string); codesym((Symbol*)0);
			$<inst>$ = code(forall_section); codei(0); codei(0);}
	stmt end
		{ $$ = $2; ($<inst>3)[1].i = relative($4, $<inst>3, 1); /* body */
			($<inst>3)[2].i = relative($5, $<inst>3, 2); /* exit */
		}
	| FORSEC begin string1
		{ $<inst>$ = code(forall_section); codei(0); codei(0);}
	stmt end
		{ $$ = $2; ($<inst>4)[1].i = relative($5, $<inst>4, 1); /* body */
			($<inst>4)[2].i = relative($6, $<inst>4, 2); /* exit */
		}
	| IFSEC begin string1
		{ $<inst>$ = code(hoc_ifsec); codei(0); codei(0);}
	  stmt end
		{ $$ = $2; ($<inst>4)[1].i = relative($5, $<inst>4, 1); /* body */
			($<inst>4)[2].i = relative($6, $<inst>4, 2); /* exit */
		}
	| FORSEC begin ob
		{hoc_ob_check(-1);
		$<inst>$ = code(forall_sectionlist); codei(0); codei(0); }
	stmt end
		{ $$ = $2; ($<inst>4)[1].i = relative($5, $<inst>4, 1); /* body */
			($<inst>4)[2].i = relative($6, $<inst>4, 2); /* exit */
		}
	| IFSEC begin ob
		{ hoc_ob_check(-1);
		 $<inst>$ = code(hoc_ifseclist); codei(0); codei(0);
		}

	  stmt end
		{ $$ = $2; ($<inst>4)[1].i = relative($5, $<inst>4, 1); /* body */
			($<inst>4)[2].i = relative($6, $<inst>4, 2); /* exit */
		}
/* END NEWCABLE */
	;

iterator: ITERATOR '(' arglist ')'
		{
		  code(hoc_iterator); codesym($1); codei($3);
		  $$ = progp; codein(STOP); codein(STOP);
		}
	| {code(hoc_push_current_object);} ob
		{codei(ITERATOR);
		  $$ = progp; codein(STOP); codein(STOP);
		}
	;
/* NEWCABLE */
section: SECTION {pushs($1); pushi(CHECK);} wholearray
		{code(sec_access_push); codesym(spop()); $$ = $3;}
	| INTERNALSECTIONNAME
		{
		  $$ = code(hoc_sec_internal_push);
		  hoc_codeptr($1);
		}
	| pysec
		{
		  nrn_parsing_pysec_ = NULL;
		  $$ = code(hoc_sec_internal_push);
		  hoc_codeptr($1);
		}
	;

pysec: PYSEC '.' pysec1
		{ $$ = $3; }
	;

pysec1: PYSECNAME
	| PYSECOBJ '.' PYSECNAME
		{ $$ = $3; }
;

section_or_ob: section '(' expr ')' {TPD;}
	| {$<inst>$ = progp; code(connect_obsec_syntax);} ob
		{
#if 0
		 acterror("Sorry. The \"connect ob.sec...\" syntax ",
			"is not implemented");
#endif
		 hoc_ob_check(SECTION); code(ob_sec_access);
		}
	;

sectiondecl: sectionname
		{ code(add_section); codesym($1); codei(0);}
	| sectionname numdimen
		{ code(add_section); codesym($1); codei($2);}
	| sectiondecl ',' sectionname
		{code(add_section); codesym($3); codei(0);}
	| sectiondecl ',' sectionname numdimen
		{ code(add_section); codesym($3); codei($4);}
	;
sectionname:anyname
		{
			Symbol* s;
			s = hoc_decl($1);
			if (s->type != UNDEF && s->type != SECTION)
				acterror(s->name, " already declared");
		}
	;
rangevar: rangevar1
		{ code(sec_access_push); codesym((Symbol *)0);}
	| section '.' rangevar1
	;
rangevar1: RANGEVAR {pushs($1); pushi(CHECK);} wholearray
		{$$ = $3;}
	;
pointer: varname /*leave pointer on stack*/
		{ code3(varpush, spop(), hoc_evalpointer);}
	| rangevar '(' expr ')'
		{ TPD; code(rangevarevalpointer); codesym(spop());}
	| ob
		{hoc_ipop(); code(hoc_ob_pointer);}
	| rangevar error {myerr("rangevariable needs explicit arc position,eg. v(.5)");}
	| ARGREF
		{$$ = argcode(hoc_argrefarg, $1);}
;

/* END NEWCABLE */

for_init: FOR '(' stmt ';'
		{ $$ = $3;}
	| FOR '(' ';'
		{ $$ = progp; }
	;
for_st:	/* nothing */
		{ $$ = code(forcode); codei(0); codei(0); codei(0); }
	;
for_cond: expr
		{ TPD; $$ = $1; codein(STOP);}
	;
for_inc:  ';' stmt ')'
		{ $$ = $2; codein(STOP);}
	| ';' ')'
		{ $$ = progp; codein(STOP);}
	;
cond:	'(' expr ')'
		{ TPD; codein(STOP); $$ = $2;}
	;
while:	WHILE for_st
		{ $$ = $2; }
	;
if:	IF
		{ $$=code(ifcode); codei(0); codei(0); codei(0); }
	;
begin:	/* nothing */
		{ $$ = progp; }
	;
end:	/* nothing */
		{ codein(STOP); $$ = progp; }
	;
stmtlist: /* nothing */
		{ $$ = progp; prog_error = $$; }
	| stmtlist '\n'
		{
			prog_parse_recover = progp;
			prog_error = $$;
			if (fin && nrn_fw_eq(fin, stdin) && !pipeflag)
			{	int i;
				Printf(">");
				for (i = 0; i < ntab; i++)
					Printf("	");
			}
		}
	| stmtlist stmt
	| error
		{myerr("syntax error in compound statement");}

	;

expr:	NUMBER
		{ $$ = code(constpush); codesym($1); PN;}
	|	NUMZERO
		{ $$ = code(pushzero); PN;}
	| varname
		{ code3(varpush, spop(), eval); PN;}
	| ARG
		{ defnonly("$"); $$ = argcode(arg, $1); PN;}
	| ARGREF argrefdim
		{ defnonly("$&"); $$ = argrefcode(hoc_argref, $1, $2); PN;}
/* NEWCABLE */
	| rangevar
		{code(rangepoint); codesym(spop()); PN;}
	| rangevar '(' expr ')'
		{ TPD; code(rangevareval); codesym(spop()); PN;}
/* END NEWCABLE */
/* OOP */
	| ob
		{code(hoc_object_eval);}
/* END OOP */
	| asgn
	| function begin '(' arglist ')'
		{ $$ = $2; code(call); codesym($1); codei($4); PN;}
	| varname '(' arglist ')'	/* error will be flagged at runtime */
		{ code(call); codesym(spop()); codei($3); PN;}
	| READ '(' varname ')'
		{ $$=$3; code(varread); codesym(spop()); PN;}
	| BLTIN '(' expr ')'
		{ TPD; $$ = $3; code(bltin); codesym($1); PN;}
	| '(' expr ')'
		{ $$ = $2; }
	| '(' error
		{myerr("syntax error in expression");}
	| expr '+' expr
		{ TPD; TPD; code(add); PN; }
	| expr '-' expr
		{ TPD; TPD;code(hoc_sub); PN;}
	| expr '*' expr
		{ TPD; TPD; code(mul); PN;}
	| expr '/' expr
		{ TPD; TPD; code(hoc_div); PN;}
	| expr '%' expr
		{ TPD; TPD; code(hoc_cyclic); PN;}
	| expr '^' expr
		{ TPD; TPD; code(power); PN;}
	| '-' expr %prec UNARYMINUS
		{ TPD; $$ = $2; code(negate); PN;}
	| expr GT expr
		{ TPD; TPD; code(gt); PN;}
	| expr GE expr
		{ TPD; TPD; code(ge); PN;}
	| expr LT expr
		{ TPD; TPD; code(lt); PN;}
	| expr LE expr
		{ TPD; TPD; code(le); PN;}
	| expr EQ expr
		{ hoc_ob_check(-1); hoc_ob_check(-1); code(eq); PN;}
	| expr NE expr
		{ hoc_ob_check(-1); hoc_ob_check(-1); code(ne); PN;}
	| expr AND expr
		{ TPD; TPD; code(hoc_and); PN;}
	| expr OR expr
		{ TPD; TPD; code(hoc_or); PN;}
	| NOT expr
		{ TPD; $$ = $2; code(hoc_not); PN;}
	;
function: FUNCTION
	| FUN_BLTIN
	| OBFUNCTION
	| STRFUNCTION
	;
doublelist: parseDOUBLE begin newarray
		{Symbol *s; code(varpush); codesym(s=spop()); $$ = $2;
		code(arayinstal); codei($3); hoc_obvar_declare(s, VAR, 0);}
	| doublelist ',' newarray
		{Symbol *s; code(varpush); codesym(s = spop());
		code(arayinstal); codei($3); hoc_obvar_declare(s, VAR, 0);}
	;

newarray: newname numdimen
		{pushs($1); $$ = $2;}
	;
numdimen: '[' expr ']'
		{  TPD; $$ = 1; }
	| numdimen '[' expr ']'
		{  TPD;$$ = $$ + 1; }
	;
newname: ckvar
	;

varname: AUTO begin
		{ pushs($1); $$ = $2; }
	| VAR begin
		{ if ($1->subtype == USERPROPERTY) {
			code(sec_access_push); codesym((Symbol *)0);
		  }
		pushs($1); pushi(CHECK);
		}
	wholearray {$$ = $2;}
	| section '.' VAR
		{ if ($3->subtype != USERPROPERTY) {
			acterror($3->name, "not a section variable");
		  }
		$$ = $1; pushs($3);
		}
	;

wholearray:begin array	/* using execution stack to get Symbol from array */
			/* and whether to do syntactic check or not */
			/* object component array may share a name with
				the current symbol table which is not an array.
				object component checking done dynamically */
	/* numindices, checkflag=1, arraysym -> arraysym */
	/* numindices, checkflag=0, arraysym -> arraysym numindices*/
		{
		int d1, chk;
		Symbol *sym;
		d1 = ipop();
		chk = ipop();
		sym = spop();
   if (chk) {
	if (!ISARRAY(sym)) {
		if (d1)
			acterror(sym->name, "not an array variable");
	}else{
		if ( d1 == 0 ) { /*fake index list with all 0's*/
			int i;
			for (i=0; i<sym->arayinfo->nsub; i++) {
				code(pushzero);
			}
		}			
		else if ( d1 != sym->arayinfo->nsub) {
			acterror("wrong # of subscripts",sym->name);
		}
	}
   }else {
	pushi(d1); /* must check dynamically */
   }
		pushs(sym);
		}
	;

argrefdim: array
		{
			$$ = ipop();
		}
	;

array:	/* Nothing */
		{ pushi(0); }
	| array '[' expr ']'
		{  TPD;pushi(ipop() + 1); }
	;

prlist:	expr
		{ TPDYNAM; code(prexpr);}
	| string1
		{ code(prstr); }
	| prlist ',' expr
		{ TPDYNAM; code(prexpr);}
	| prlist ',' string1
		{ code(prstr); }
	;
delsym: VAR
		{ $$ = code(hoc_delete_symbol); codesym($1); }
	;
			
defn:	FUNC procname
		{$2->type=FUNCTION; indef=1; }
	'(' ')' procstmt
		{ code(procret); hoc_define($2);
		 $2->u.u_proc->nobjauto = $6 - localcnt;
		 $2->u.u_proc->nauto=$6; indef=0; }
	| parsePROC procname
		{ $2->type=PROCEDURE; indef=1; }
	'(' ')' procstmt
		{ code(procret); hoc_define($2);
		 $2->u.u_proc->nobjauto = $6 - localcnt;
		 $2->u.u_proc->nauto=$6; indef=0; }
	| ITERKEYWORD procname
		{ $2->type = ITERATOR; indef=2; }
	'(' ')' procstmt
		{code(procret); hoc_define($2);
		 $2->u.u_proc->nobjauto = $6 - localcnt;
		 $2->u.u_proc->nauto = $6; indef = 0; }
	| HOCOBJFUNC procname
		{ $2->type=HOCOBJFUNCTION; indef=3; }
	'(' ')' procstmt
		{ code(procret); hoc_define($2);
		 $2->u.u_proc->nobjauto = $6 - localcnt;
		 $2->u.u_proc->nauto=$6; indef=0; }
	;
procname: ckvar
		{ Symbol *s; s=yylval.sym;
		if(s->type != UNDEF) acterror(s->name, "already defined");
		/* avoid valgrind uninitialized variable error for nautoobj */
		s->u.u_proc = (Proc *)ecalloc(1, sizeof(Proc));
		s->u.u_proc->defn.in = STOP;
		s->u.u_proc->list = (Symlist *)0; }
	| FUNCTION
	| PROCEDURE
	| ITERATOR
	| HOCOBJFUNCTION
	;
procstmt: '{' local localobj {ntab++;} stmtlist '}'
		{
		ntab--;
		$$ = $2 + $3;
		}
	;
arglist: /* nothing */
		{ $$ = 0; }
	| arglist1
	;
arglist1: arglist2
		{$$ = 1;}
	| arglist1 ',' arglist2
		{$$ = $1 + 1;}
	;
arglist2: string1
		{}
	| expr
		{ hoc_ob_check(-1);}
	| '&' pointer
	| NEW anyname '(' arglist ')'
		{
		 code(hoc_newobj_arg); codesym(hoc_which_template($2)); codei($4);
		}
	;
eqn_list: DEPENDENT dep_list
		{ $$ = $2; }
	| EQUATION varname ':'
		{code3(varpush, spop(), eqn_name);
		do_equation = 1; }
	equation
		{ $$ = $2; do_equation = 0; }
	;
dep_list: varname
		{Symbol *s; code3(varpush,s= spop(), dep_make); hoc_obvar_declare(s, VAR, 0);}
	| dep_list ',' varname
		{Symbol *s; code3(varpush, s=spop(), dep_make); hoc_obvar_declare(s, VAR, 0);}
	;
equation: lhs EQNEQ
	| EQNEQ rhs
	| ':'
		{ code(eqn_init); }
	lhs EQNEQ rhs
	;
lhs:
		{ code(eqn_lhs); }
	expr
		{ codein(STOP); TPD; }
	;
rhs:
		{ code(eqn_rhs); }
	expr
		{ codein(STOP); TPD; }
	;
local:	/* nothing */
		{ $$ = 0; localcnt = $$;}
	| local1
	;
local1:	LOCAL anyname
		{
		Symbol *sp;
		$$ = 1; localcnt = $$;
		sp = install($2->name, AUTO, 0.0, &p_symlist);
		sp->u.u_auto = $$;
		}
	| local1 ',' anyname
		{
		Symbol *sp;
		$$ = $1 + 1; localcnt = $$;
		if (hoc_table_lookup($3->name, p_symlist)) {
			acterror($3->name, "already declared local");
		}
		sp = install($3->name, AUTO, 0.0, &p_symlist);
		sp->u.u_auto = $$;
		}
	;
localobj:	/* nothing */
		{ $$ = 0;}
	| local2
	;
local2:	LOCALOBJ anyname
		{
		Symbol *sp;
		$$ = 1;
		if (hoc_table_lookup($2->name, p_symlist)) {
			acterror($2->name, "already declared local");
		}
		sp = install($2->name, AUTOOBJ, 0.0, &p_symlist);
		sp->u.u_auto = $$ + localcnt;
		}
	| local2 ',' anyname
		{
		Symbol *sp;
		$$ = $1 + 1;
		if (hoc_table_lookup($3->name, p_symlist)) {
			acterror($3->name, "already declared local");
		}
		sp = install($3->name, AUTOOBJ, 0.0, &p_symlist);
		sp->u.u_auto = $$ + localcnt;
		}
	;
ckvar: VAR
	{  Symbol* s;
	   s = hoc_decl($1);
	   if (s->subtype != NOTUSER)
		acterror("can't redeclare user variable", s->name);
	   $$ = s;
	}
	;
anyname: STRING|VAR|UNDEF|FUNCTION|PROCEDURE|FUN_BLTIN|SECTION|RANGEVAR
	|NRNPNTVAR|OBJECTVAR|TEMPLATE|OBFUNCTION|AUTO|AUTOOBJ|SECTIONREF
	|MECHANISM|BLTIN|STRFUNCTION|HOCOBJFUNCTION|ITERATOR|STRINGFUNC
	|OBJECTFUNC
	;
%%
	/* end of grammar */

static void yyerror(const char* s)	/* called for yacc syntax error */
{
	execerror(s, (char *)0);
}

void acterror(const char* s, const char*t)	/* recover from action error while parsing */
{
	execerror(s,t);
}

static Inst* argrefcode(Pfrv pfrv, int i, int j){
	Inst* in;
	in = argcode(pfrv, i);
	codei(j);
	return in;
}

static Inst* argcode(Pfrv pfrv, int i) {
	Inst* in;
	if (i == 0) {
		Symbol* si = hoc_lookup("i");
		if (si->type != AUTO) {
			acterror("arg index used and i is not a LOCAL variable", 0);
		}
		in = code3(varpush, si, eval);		
		Code(pfrv);
		codei(0);
	}else{
		in = Code(pfrv);
		codei(i);
	}
	return in;
}

static void hoc_opasgn_invalid(int op) {
        if (op) {
                acterror("Invalid assignment operator.", "Only '=' allowed. ");
        }
} 

