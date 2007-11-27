#include <../../nmodlconf.h>

/*
 * some parse actions to reduce size of parse.y the installation routines can
 * also be used, e.g. in sens to automattically construct variables 
 */

#include "modl.h"
#include "parse1.h"

Symbol	       *scop_indep;	/* independent used by SCoP */
Symbol         *indepsym;	/* only one independent variable */
Symbol         *stepsym;	/* one or fewer stepped variables */
List		*indeplist;	/* FROM TO WITH START UNITS */
extern List    *syminorder;	/* Order in which variables are output to
				 * .var file */
#if CVODE
extern List* state_discon_list_;
extern int net_send_seen_;
extern int net_event_seen_;
extern int watch_seen_;
#endif

extern int artificial_cell;

static type_change();

static long previous_subtype;	/* subtype at the sym->level */
static char *previous_str;	/* u.str on last install */

explicit_decl(level, q)
	int level;
	Item *q;
{
	/* used to be inside parse1.y without the lastvars condition
	   Without the condition it served two purposes.
	   1) variables explicitly declared were so marked so that they
	      would appear first in the .var file.  Unmarked variables
	      appear last.
	   2) Give error message if a variable was explicitly declared
	      more than once.
	   Now, the merge program produces declaration blocks from
	   submodels with a prepended LAST_VARS keyword.  This implies
	   1) that variables in such blocks should appear last (if they
	      don't appear at the top level) and
	   2) multiple declarations are not errors.
	   Hence we merely enclose the old code in an if statement
	   The question arises, on multiple declarations, which value
	   does the .var file get.  In the current implementation it
	   is the last one seen. If this is not right (and a better
	   method would be keep the value declared closest to the root)
	   then it will be the responsibility of merge to delete
	   multiple declarations.
	*/
	/* Solving the multiple declaration problem.
	   merge now gives the level number of the declaration with
	   the root file having level number 0, all its submodels having
	   level number 1, submodels of level 1 submodels having level 2,
	   etc.  The rule is that the lowest level declaration is used.
	   If two declarations exist at the same level then it is an
	   error unless their u.str are identical.  Since, by the time
	   this routine is called the latest declaration has already been
	   installed, each installation routine saves the previous u.str
	   in a static variable. Also a new field is added to the
	   symbol structure to keep track of its level. At this time
	   we retain the EXPLICIT_DECL field for explicit declarations
	   at the root level. The default level when the symbol is
	   allocated is 100. 
	 */
	
	Symbol *sym;

	sym = SYM(q);
	if (!level) { /* No multiple declarations at the root level and
			the symbol is marked explicitly declared */
		if (sym->usage & EXPLICIT_DECL) {
			diag("Multiple declaration of ", sym->name);
		}
		sym->usage |= EXPLICIT_DECL;
	}
	/* this ensures that declared PRIMES will appear in .var file */
	sym->usage |= DEP;

	if (level >= sym->level) {
		assert(previous_str);
	}
	/* resolve possible type conflicts */
	if (type_change(sym, level)) {
		return;
	}
	/* resolve which declaration takes precedence */
	if (level < sym->level) { /* new one takes precedence */
		sym->level = level;
	}else if (level > sym->level) { /* old one takes precedence */
		sym->u.str = previous_str;
	}else if (strcmp(sym->u.str, previous_str) != 0) { /* not identical */
		diag(sym->name, " has different values at same level");
	}
}

/* restricted type changes are allowed in hierarchical models with each
one producing a message. Notice that multiple declarations at level 0 are
caught as errors in the function above. */

static int
type_change(sym, level) /*return 1 if type change, 0 otherwise*/
	Symbol *sym;
{
	long s, d, c;
	
	s = sym->subtype & STAT;
	d = sym->subtype & DEP;
	c = sym->subtype & PARM;

	if (s && c) {
		sym->subtype &= ~c;
Fprintf(stderr, "NOTICE: %s is promoted from a PARAMETER to a STATE\n", sym->name);
		if (previous_subtype & STAT) {
			sym->u.str = previous_str;
		}
	}else if (s && d) {
		sym->subtype &= ~d;
Fprintf(stderr, "WARNING: %s is promoted from an ASSIGNED to a STATE\n", sym->name);
		if (previous_subtype & STAT) {
			sym->u.str = previous_str;
		}
	}else if (d && c) {
		sym->subtype &= ~c;
Fprintf(stderr, "NOTICE: %s is promoted from a PARAMETER to an ASSIGNED\n", sym->name);
		if (previous_subtype & DEP) {
			sym->u.str = previous_str;
		}
	}else{
		return 0;
	}
	if (level < sym->level) {
		sym->level = level;
	}
	return 1;
}
	
parm_array_install(n, num, units, limits, index)
	Symbol         *n;
	char           *num, *units, *limits;
{
	char buf[512];

	previous_subtype = n->subtype;	
	previous_str = n->u.str;
	if (n->u.str == (char *) 0)
		Lappendsym(syminorder, n);
	n->subtype |= PARM;
	n->subtype |= ARRAY;
	n->araydim = index;
	Sprintf(buf, "[%d]\n%s\n%s\n%s\n", index, num, units, limits);
	n->u.str = stralloc(buf, (char *) 0);
}

parminstall(n, num, units, limits)
	Symbol         *n;
	char           *num, *units, *limits;
{
	char buf[512];

	previous_subtype = n->subtype;	
	previous_str = n->u.str;
	if (n->u.str == (char *) 0)
		Lappendsym(syminorder, n);
	n->subtype |= PARM;
	Sprintf(buf, "\n%s\n%s\n%s\n", num, units, limits);
	n->u.str = stralloc(buf, (char *) 0);
}

/* often we want to install a parameter by default but only
if the user hasn't declared it herself.
*/
Symbol *
ifnew_parminstall(name, num, units, limits)
	char *name, *num, *units, *limits;
{
	Symbol *s;

	if ((s = lookup(name)) == SYM0) {
		s = install(name, NAME);
		parminstall(s, num, units, limits);
	}
	if (!(s->subtype)) {
		/* can happen when PRIME used in MATCH */
		parminstall(s, num, units, limits);
	}
	if (!(s->subtype & (PARM | STEP1))) {
		/* special case is scop_indep can be a PARM but not indepsym */
		if (scop_indep == indepsym || s != scop_indep) {
			diag(s->name, " can't be declared a parameter by default");
		}
	}
	return s;
}

steppedinstall(n, q1, q2, units)
	Symbol         *n;
	Item           *q1, *q2;
	char           *units;
{
	int             i;

	char buf[512];
	static int      seestep = 0;

	previous_subtype = n->subtype;
	previous_str = n->u.str;
	if (seestep) {
		diag("Only one STEPPED variable can be defined", (char *) 0);
	}
	seestep = 1;
	stepsym = n;
	i = 0;
	Strcpy(buf, "\n");
	Strcat(buf, STR(q1));
	while (q1 != q2) {
		q1 = q1->next;
		Strcat(buf, SYM(q1)->name);	/* , is a symbol */
		q1 = q1->next;
		Strcat(buf, STR(q1));
		i++;
		if (i > 5) {
			diag("Maximum of 5 steps in a stepped variable",
			     (char *) 0);
		}
	}
	Strcat(buf, "\n");
	Strcat(buf, units);
	Strcat(buf, "\n");
	n->subtype |= STEP1;
	n->u.str = stralloc(buf, (char *) 0);
}

static char    *indepunits = "";
#if NMODL
int using_default_indep;
#endif

indepinstall(n, from, to, with, qstart, units, scop)
	Symbol         *n;
	char           *from, *to, *with, *units;
	Item		*qstart;	/* ITEM0 if not present */
	int scop;	/*1 if declaring the scop independent*/
{
	char buf[512];

/* scop_indep may turn out to be different from indepsym. If this is the case
   then indepsym will be a constant in the .var file (see parout.c).
   If they are the same, then u.str gets the info from SCOP.
*/
if (!scop) {
#if NMODL
	if (using_default_indep) {
		using_default_indep = 0;
		if (indepsym != n) {
			indepsym->subtype &= ~INDEP;
			parminstall(indepsym, "0", "ms", "");
		}
		indepsym = (Symbol*)0;
		
	}
#endif
	if (indepsym) {
diag("Only one independent variable can be defined", (char *) 0);
	}
	indeplist = newlist();
	Lappendstr(indeplist, from);
	Lappendstr(indeplist, to);
	Lappendstr(indeplist, with);
	if (qstart) {
		Lappendstr(indeplist, STR(qstart));
	}else{
		Lappendstr(indeplist, from);
	}
	Lappendstr(indeplist, units);
	n->subtype |= INDEP;
	indepunits = stralloc(units, (char *) 0);
	if (n != scop_indep) {
		Sprintf(buf, "\n%s*%s(%s)\n%s\n", from, to, with, units);
		n->u.str = stralloc(buf, (char *) 0);
	}
	indepsym = n;
	if (!scop_indep) {
		scop_indep = indepsym;
	}
}else{
	n->subtype |= INDEP;
	Sprintf(buf, "\n%s*%s(%s)\n%s\n", from, to, with, units);
	n->u.str = stralloc(buf, (char *) 0);
	scop_indep = n;
}
}

/*
 * installation of dependent and state variables type 0 -- dependent; 1 --
 * state index 0 -- scalar; otherwise -- array qs -- item pointer to START
 * const string makeconst 0 -- do not make a default constant for state 1 --
 * make sure name0 exists For states Dname and name0 are normally created.
 * However Dname will not appear in the .var file unless it is used -- see
 * parout.c. 
 */
depinstall(type, n, index, from, to, units, qs, makeconst, abstol)
	int             type, index, makeconst;
	Symbol         *n;
	char           *from, *to, *units, *abstol;
	Item           *qs;
{
	char buf[512], *pstr;
	int             c;

	if (!type && strlen(abstol)>0) {
		printf("abstol = |%s|\n", abstol);
		diag(n, "tolerance can be specified only for a STATE");
	}
	pstr = n->u.str;	/* make it work even if recursive */
	if (n->u.str == (char *) 0)
		Lappendsym(syminorder, n);
	if (type) {
		n->subtype |= STAT;
		c = ':';
		statdefault(n, index, units, qs, makeconst);
	} else {
		n->subtype |= DEP;
		c = ';';
		if (qs) {
			diag("START not legal except in  STATE block", (char *) 0);
		}
	}
	if (index) {
		Sprintf(buf, "[%d]\n%s%c%s\n%s\n%s\n", index, from, c, to, units, abstol);
		n->araydim = index;
		n->subtype |= ARRAY;
	} else {
		Sprintf(buf, "\n%s%c%s\n%s\n%s\n", from, c, to, units, abstol);
	}
	n->u.str = stralloc(buf, (char *) 0);
	previous_subtype = n->subtype;
	previous_str = pstr;
}

statdefault(n, index, units, qs, makeconst)
	Symbol         *n;
	int             index, makeconst;
	char           *units;
	Item           *qs;
{
	char            nam[30], *un;
	Symbol         *s;

	if (n->type != NAME && n->type != PRIME) {
		diag(n->name, " can't be a STATE");
	}
	if (makeconst) {
		Sprintf(nam, "%s0", n->name);
		s = ifnew_parminstall(nam, "0", units, "");
		if (qs) { /*replace with proper default*/
			parminstall(s, STR(qs), units, "");
		}
	}
	Sprintf(nam, "%s/%s", units, indepunits);
	un = stralloc(nam, (char *) 0);
	Sprintf(nam, "D%s", n->name);
	if ((s = lookup(nam)) == SYM0) {	/* install the prime as a DEP */
		s = install(nam, PRIME);
		depinstall(0, s, index, "0", "1", un, ITEM0, 0, "");
	}
}

defarg(q1, q2)			/* copy arg list and define as doubles */
	Item           *q1, *q2;
{
	Item           *q3, *q;

	if (q1->next == q2) {
#if VECTORIZE
		vectorize_substitute(insertstr(q2, ""), "_ix");
		vectorize_substitute(insertstr(q2->next, ""), "int _ix;");
#endif
		return;
	}
	for (q3 = q2->next, q = q1; q != q2; q = q->next, q3 = q3->next) {
		q3 = insertsym(q3, SYM(q));
	}
	replacstr(q2->next, "\n\tdouble");
	Insertstr(q3, ";\n");
#if VECTORIZE
		vectorize_substitute(insertstr(q1->next, ""), "_ix,");
		vectorize_substitute(insertstr(q2->next, ""), "int _ix;");
#endif
}

lag_stmt(q1, blocktype)	/* LAG name1 BY name2 */
	Item *q1;
	int blocktype;
{
	Symbol *name1, *name2, *lagval;

	/*ARGSUSED*/
	/* parse */
	name1 = SYM(q1->next);
	delete(q1->next);
	delete(q1->next);
	name2 = SYM(q1->next);
	delete(q1->next);
	name1->usage |= DEP;
	name2->usage |= DEP;
	/* check */
	if (!indepsym) {
		diag("INDEPENDENT variable must be declared to process",
			" the LAG statement");
	}
	if (!(name1->subtype & (DEP | STAT))) {
		diag(name1->name, " not a STATE or DEPENDENT variable");
	}
	if (!(name2->subtype & (PARM | CONST))) {
		diag(name2->name, " not a CONSTANT or PARAMETER");
	}
	Sprintf(buf, "lag_%s_%s", name1->name, name2->name);
	if (lookup(buf)) {
		diag(buf, " already in use");
	}
	/* create */
	lagval = install(buf, NAME);
	lagval->usage |= DEP;
	lagval->subtype |= DEP;
	if (name1->subtype & ARRAY) {
		lagval->subtype |= ARRAY;
		lagval->araydim = name1->araydim;
	}
	if (lagval->subtype & ARRAY) {
		Sprintf(buf, "static double *%s;\n", lagval->name);
		Linsertstr(procfunc, buf);
		Sprintf(buf, "%s = lag(%s, %s, %s, %d);\n", lagval->name,
		name1->name, indepsym->name, name2->name, lagval->araydim);
	}else{
		Sprintf(buf, "static double %s;\n", lagval->name);
		Linsertstr(procfunc, buf);
		Sprintf(buf, "%s = *lag(&(%s), %s, %s, 0);\n", lagval->name,
			name1->name, indepsym->name, name2->name);
	}
	replacstr(q1, buf);
}

queue_stmt(q1, q2)
	Item *q1, *q2;
{
	Symbol *s;
	static int first=1;
	
	if (first) {
		first = 0;
		Linsertstr(initfunc, "initqueue();\n");
	}
	if (SYM(q1)->type == PUTQ) {
		replacstr(q1, "enqueue(");
	}else{
		replacstr(q1, "dequeue(");
	}
	
	s = SYM(q2);
	s->usage |= DEP;
	if (!(s->subtype)) {
		diag(s->name, " not declared");
	}
	if (s->subtype & ARRAY) {
		Sprintf(buf, "%s, %d);\n", s->name, s->araydim);
	}else{
		Sprintf(buf, "&(%s), 1);\n", s->name);
	}
	replacstr(q2, buf);
}

add_reset_args(q)
	Item *q;
{
	static int reset_fun_cnt=0;
	
	reset_fun_cnt++;
	Sprintf(buf, "&_reset, &_freset%d,", reset_fun_cnt);
	Insertstr(q->next, buf);
	Sprintf(buf, "static double _freset%d;\n", reset_fun_cnt);
	Lappendstr(firstlist, buf);
}

/* table manipulation */
/* arglist must have exactly one argument
   tablist contains 1) list of names to be looked up (must be empty if
   qtype is FUNCTION and nonempty if qtype is PROCEDURE).
   2) From expression list
   3) To expression list
   4) With integer string
   5) DEPEND list as list of names
   The qname does not have a _l if a function. The arg names all have _l
   prefixes.
*/
/* checking and creation of table has been moved to separate function called
   static _check_func.
*/
/* to allow vectorization the table functions are separated into
	name	robust function. makes sure
		table is uptodate (calls check_name)
	_check_name	if table not up to date then builds table
	_f_name		analytic
	_n_name		table lookup with no checking if usetable=1
			otherwise calls _f_name.
*/

static List* check_table_statements;
static Symbol* last_func_using_table;

check_tables() {
	if (check_table_statements) {
		fprintf(fcout, "\n#if _CRAY\n");
		printlist(check_table_statements);
		fprintf(fcout, "#endif\n");
	}
}

table_massage(tablist, qtype, qname, arglist)
	List *tablist, *arglist;
	Item *qtype, *qname;
{
	Symbol *fsym, *s, *arg;
	char* fname;
	List *table, *from, *to, *depend;
	int type, ntab;
	Item *q;
	
	if (!tablist) {
		return;
	}
	fsym = SYM(qname);
	last_func_using_table = fsym;
	fname = fsym->name;
	table = LST(q = tablist->next);
	from = LST(q = q->next);
	to = LST(q = q->next);
	ntab = atoi(STR(q = q->next));
	depend = LST(q = q->next);
	type = SYM(qtype)->type;

	ifnew_parminstall("usetable", "1", "", "0 1");
	if (!check_table_statements) {
		check_table_statements = newlist();
	}
	sprintf(buf, "_check_%s();\n", fname);
	Lappendstr(check_table_statements, buf);
	/*checking*/
	if (type == FUNCTION1) {
		if (table) {
diag("TABLE stmt in FUNCTION cannot have a table name list", (char *)0);
		}
		table = newlist();
		Lappendsym(table, fsym);
	}else{
		if (!table) {
diag("TABLE stmt in PROCEDURE must have a table name list", (char *)0);
		}
	}
	if (arglist->next == arglist || arglist->next->next != arglist) {
diag("FUNCTION or PROCEDURE containing a TABLE stmt\n",
"must have exactly one argument");
	}else{
		arg = SYM(arglist->next);
	}
	if (!depend) {
		depend = newlist();
	}

	/*translation*/
	/* new name for original function */
	Sprintf(buf, "_f_%s", fname);
	SYM(qname) = install(buf, fsym->type);
	SYM(qname)->subtype = fsym->subtype;
	if (type == FUNCTION1) {
		fsym->subtype |= FUNCT;
		Sprintf(buf, "static double _n_%s();\n", fname);
		Linsertstr(procfunc, buf);
	}else{
		fsym->subtype |= PROCED;
		Sprintf(buf, "static _n_%s();\n", fname);
		Linsertstr(procfunc, buf);
	}
	fsym->usage |= FUNCT;
		
	/* declare communication between func and check_func */
	Sprintf(buf, "static double _mfac_%s, _tmin_%s;\n",
		fname, fname);
	Lappendstr(procfunc, buf);

	/* create the check function */
	Sprintf(buf, "static _check_%s();\n", fname);
	Linsertstr(procfunc, buf);
	Sprintf(buf, "static _check_%s() {\n", fname);
	Lappendstr(procfunc, buf);
	Lappendstr(procfunc, " static int _maktable=1; int _i, _j, _ix = 0;\n");
	Lappendstr(procfunc, " double _xi, _tmax;\n");
	ITERATE(q, depend) {
		Sprintf(buf, " static double _sav_%s;\n", SYM(q)->name);
		Lappendstr(procfunc, buf);
	}
	Lappendstr(procfunc, " if (!usetable) {return;}\n");
	/*allocation*/
	ITERATE(q, table) {
		s = SYM(q);
		if (s->subtype & ARRAY) {
			Sprintf(buf, " for (_i=0; _i < %d; _i++) {\
  _t_%s[_i] = makevector(%d*sizeof(double)); }\n", s->araydim, s->name, ntab+1);
		}else{
			Sprintf(buf, "  _t_%s = makevector(%d*sizeof(double));\n",
			s->name, ntab+1);
		}
		Lappendstr(initlist, buf);
	}
	/* check dependency */
	ITERATE(q, depend) {
		Sprintf(buf, " if (_sav_%s != %s) { _maktable = 1;}\n",
		 SYM(q)->name, SYM(q)->name);
		Lappendstr(procfunc, buf);
	}
	/* make the table */
	Lappendstr(procfunc, " if (_maktable) { double _x, _dx; _maktable=0;\n");
	Sprintf(buf, "  _tmin_%s = ", fname);
	Lappendstr(procfunc, buf);
	move(from->next, from->prev, procfunc);
	Sprintf(buf, ";\n   _tmax = ");
	Lappendstr(procfunc, buf);
	move(to->next, to->prev, procfunc);
	Lappendstr(procfunc, ";\n");
	Sprintf(buf,"  _dx = (_tmax - _tmin_%s)/%d.; _mfac_%s = 1./_dx;\n",
		fname, ntab, fname);
	Lappendstr(procfunc, buf);
	Sprintf(buf,"  for (_i=0, _x=_tmin_%s; _i < %d; _x += _dx, _i++) {\n",
		fname, ntab+1);
	Lappendstr(procfunc, buf);
	if (type == FUNCTION1) {
		ITERATE(q, table) {
			s = SYM(q);
			Sprintf(buf, "   _t_%s[_i] = _f_%s(_x);\n", s->name, fname);
			Lappendstr(procfunc, buf);
#if VECTORIZE
			Sprintf(buf, "   _t_%s[_i] = _f_%s(_ix, _x);\n", s->name, fname);
			vectorize_substitute(procfunc->prev, buf);
#endif
		}
	}else{
		Sprintf(buf, "   _f_%s(_x);\n", fname);
		Lappendstr(procfunc, buf);
#if VECTORIZE
		Sprintf(buf, "   _f_%s(_ix, _x);\n", fname);
		vectorize_substitute(procfunc->prev, buf);
#endif
		ITERATE(q, table) {
			s = SYM(q);
			if (s->subtype & ARRAY) {
Sprintf(buf, "   for (_j = 0; _j < %d; _j++) { _t_%s[_j][_i] = %s[_j];\n}",
s->araydim, s->name, s->name);
			}else{
Sprintf(buf, "   _t_%s[_i] = %s;\n", s->name, s->name);
			}
			Lappendstr(procfunc, buf);
		}
	}
	Lappendstr(procfunc, "  }\n"); /*closes loop over _i index*/
	/* save old dependency values */
	ITERATE(q, depend) {
		s = SYM(q);
		Sprintf(buf, "  _sav_%s = %s;\n", s->name, s->name);
		Lappendstr(procfunc, buf);
	}
	Lappendstr(procfunc, " }\n"); /* closes if(maktable)) */
	Lappendstr(procfunc, "}\n\n");


	/* create the new function (steers to analytic or table) */
	/*declaration*/
	if (type == FUNCTION1) {
#define GLOBFUNC 1
#if !GLOBFUNC
		Lappendstr(procfunc, "static");
#endif
		Lappendstr(procfunc, "double");
	}else{
		Lappendstr(procfunc, "static");
	}
	Sprintf(buf, "%s(%s) double %s;{",
			fname, arg->name, arg->name);
	Lappendstr(procfunc, buf);		
#if VECTORIZE
	Sprintf(buf, "%s(_ix, %s) int _ix; double %s;{",
		fname, arg->name, arg->name);
	vectorize_substitute(procfunc->prev, buf);
#endif
	/* check the table */
	Sprintf(buf, "_check_%s();\n", fname);
	Lappendstr(procfunc, buf);
#if VECTORIZE
	Sprintf(buf, "\n#ifndef _CRAY\n_check_%s();\n#endif\n", fname);
	vectorize_substitute(procfunc->prev, buf);
#endif
	if (type == FUNCTION1) {
		Lappendstr(procfunc, "return");
	}
	Sprintf(buf, "_n_%s(%s);\n", fname, arg->name);
	Lappendstr(procfunc, buf);
#if VECTORIZE
	Sprintf(buf, "_n_%s(_ix, %s);\n", fname, arg->name);
	vectorize_substitute(procfunc->prev, buf);
#endif
	if (type != FUNCTION1) {
		Lappendstr(procfunc, "return;\n");
	}
	Lappendstr(procfunc, "}\n\n"); /* end of new function */		

	/* _n_name function for table lookup with no checking */
	if (type == FUNCTION1) {
		Lappendstr(procfunc, "static double");
	}else{
		Lappendstr(procfunc, "static");
	}
	Sprintf(buf, "_n_%s(%s) double %s;{",
			fname, arg->name, arg->name);
	Lappendstr(procfunc, buf);		
#if VECTORIZE
	Sprintf(buf, "_n_%s(_ix, %s) int _ix; double %s;{",
		fname, arg->name, arg->name);
	vectorize_substitute(procfunc->prev, buf);
#endif
	Lappendstr(procfunc, "int _i, _j;\n");
	Lappendstr(procfunc, "double _xi, _theta;\n");

	/* usetable */
	Lappendstr(procfunc, "if (!usetable) {\n");
	if (type == FUNCTION1) {
		Lappendstr(procfunc, "return");
	}
	Sprintf(buf, "_f_%s(%s);", fname, arg->name);
	Lappendstr(procfunc, buf);
#if VECTORIZE
	Sprintf(buf, "_f_%s(_ix, %s);", fname, arg->name);
	vectorize_substitute(procfunc->prev, buf);
#endif
	if (type != FUNCTION1) {
		Lappendstr(procfunc, "return;");
	}
	Lappendstr(procfunc, "\n}\n");

	/* table lookup */
	Sprintf(buf, "_xi = _mfac_%s * (%s - _tmin_%s);\n _i = (int) _xi;\n",
		fname, arg->name, fname);
	Lappendstr(procfunc, buf);
	Lappendstr(procfunc, "if (_xi <= 0.) {\n");
	if (type == FUNCTION1) {
		Sprintf(buf, "return _t_%s[0];\n", SYM(table->next)->name);
		Lappendstr(procfunc, buf);
	}else{
		ITERATE(q, table) {
			s = SYM(q);
			if (s->subtype & ARRAY) {
Sprintf(buf, "for (_j = 0; _j < %d; _j++) { %s[_j] = _t_%s[_j][0];\n}",
s->araydim, s->name, s->name);
			}else{
Sprintf(buf, "%s = _t_%s[0];\n", s->name, s->name);
			}
			Lappendstr(procfunc, buf);
		}
		Lappendstr(procfunc, "return;");
	}
	Lappendstr(procfunc, "}\n");
	Sprintf(buf, "if (_i >= %d) {\n", ntab);
	Lappendstr(procfunc, buf);
	if (type == FUNCTION1) {
		Sprintf(buf, "return _t_%s[%d];\n", SYM(table->next)->name, ntab);
		Lappendstr(procfunc, buf);
	}else{
		ITERATE(q, table) {
			s = SYM(q);
			if (s->subtype & ARRAY) {
Sprintf(buf, "for (_j = 0; _j < %d; _j++) { %s[_j] = _t_%s[_j][%d];\n}",
s->araydim, s->name, s->name, ntab);
			}else{
Sprintf(buf, "%s = _t_%s[%d];\n", s->name, s->name, ntab);
			}
			Lappendstr(procfunc, buf);
		}
		Lappendstr(procfunc, "return;");
	}
	Lappendstr(procfunc, "}\n");
	/* table interpolation */
	if (type == FUNCTION1) {
		s = SYM(table->next);
Sprintf(buf, "return _t_%s[_i] + (_xi - (double)_i)*(_t_%s[_i+1] - _t_%s[_i]);\n",
 s->name, s->name, s->name);
		Lappendstr(procfunc, buf);
	}else{
		Lappendstr(procfunc, "_theta = _xi - (double)_i;\n");
		ITERATE(q, table) {
			s = SYM(q);
			if (s->subtype & ARRAY) {
Sprintf(buf, "for (_j = 0; _j < %d; _j++) {double *_t = _t_%s[_j];",
s->araydim, s->name);
				Lappendstr(procfunc, buf);
Sprintf(buf, "%s[_j] = _t[_i] + _theta*(_t[_i+1] - _t[_i]);}\n",
 s->name);
			}else{
Sprintf(buf, "%s = _t_%s[_i] + _theta*(_t_%s[_i+1] - _t_%s[_i]);\n",
s->name, s->name, s->name, s->name);
			}
			Lappendstr(procfunc, buf);
		}
	}
	Lappendstr(procfunc, "}\n\n"); /* end of new function */		
	
	/* table declaration */
	ITERATE(q, table) {
		s = SYM(q);
		if (s->subtype & ARRAY) {
			Sprintf(buf, "static double *_t_%s[%d];\n",
			 s->name, s->araydim);
		}else{
			Sprintf(buf, "static double *_t_%s;\n", s->name);
		}
		Lappendstr(firstlist, buf);
	}
	
	/*cleanup*/
	freelist(&table);
	freelist(&depend);
	freelist(&from);
	freelist(&to);
}

#if HMODL || NMODL
hocfunc(n, qpar1, qpar2) /*interface between modl and hoc for proc and func */
	Item *qpar1, *qpar2;
	Symbol *n;
{
#if NOCMODL
	extern int point_process;
#endif
	Item *q;
	int i;
#if VECTORIZE
	Item* qp;
#endif
	
#if NOCMODL
   if (point_process) {
	Sprintf(buf, "static double _hoc_%s(_vptr) void* _vptr; {\n double _r;\n", n->name);
	Lappendstr(procfunc, buf);
	Sprintf(buf, "	_hoc_setdata(_vptr);\n");
   }else
#endif
	Sprintf(buf, "static int _hoc_%s() {\n double _r;\n", n->name);
	Lappendstr(procfunc, buf);
#if VECTORIZE
	if (n == last_func_using_table) {
		qp = lappendstr(procfunc, "");
		sprintf(buf,"\n#if _CRAY\n _check_%s();\n#endif\n", n->name);
		vectorize_substitute(qp, buf);
	}
#endif
	if (n->subtype & FUNCT) {
		Lappendstr(procfunc, "_r = ");
	}else{
		Lappendstr(procfunc, "_r = 1.;\n");
	}
	Lappendsym(procfunc, n);
	for (i=0, q=qpar1; q->prev != qpar2; q = q->next) {
#if VECTORIZE
		if (i == 0 && q->itemtype == STRING) {
			qp = lappendstr(procfunc, "");
			continue;
		}
#endif
		if (SYM(q)->type == NAME) {
			Sprintf(buf, "*getarg(%d)", ++i);
			Lappendstr(procfunc, buf);
		}else{
			Lappendsym(procfunc, SYM(q));
		}
	}
#if NOCMODL
   if (point_process) {
	Lappendstr(procfunc, ";\n return(_r);\n}\n");
   }else
#endif
	Lappendstr(procfunc, ";\n ret(_r);\n}\n");
#if VECTORIZE
	if (i) {
		vectorize_substitute(qp, "0,");
	}else{
		vectorize_substitute(qp, "0");
	}
#endif
}

#if VECTORIZE
/* ARGSUSED */
vectorize_use_func(qname, qpar1, qexpr, qpar2, blocktype)
	Item* qname, *qpar1, *qexpr, *qpar2;
	int blocktype;
{
	Item* q;
	if (SYM(qname)->subtype &  EXTDEF) {
		if (strcmp(SYM(qname)->name, "nrn_pointing") == 0) {
			Insertstr(qpar1->next, "&");
		}else if (strcmp(SYM(qname)->name, "state_discontinuity") == 0) {
#if CVODE
			if (!state_discon_list_) {
				state_discon_list_ = newlist();
				Linsertstr(procfunc, "extern int state_discon_flag_;\n");
			}
			lappenditem(state_discon_list_, qpar1->next);
#endif
			Insertstr(qpar1->next, "-1, &");
		}else if (strcmp(SYM(qname)->name, "net_send") == 0) {
			net_send_seen_ = 1;
			if (artificial_cell) {
				replacstr(qname, "artcell_net_send");
			}
			if (blocktype == NETRECEIVE) {
				Insertstr(qpar1->next, "_tqitem, _args, _pnt,");
			}else if (blocktype == INITIAL1){
				Insertstr(qpar1->next, "_tqitem, (double*)0, _ppvar[1]._pvoid,");
			}else{
diag("net_send allowed only in INITIAL and NET_RECEIVE blocks", (char*)0);
			}
		}else if (strcmp(SYM(qname)->name, "net_event") == 0) {
			net_event_seen_ = 1;
			if (blocktype == NETRECEIVE) {
				Insertstr(qpar1->next, "_pnt,");
			}else{
diag("net_event", " only allowed in NET_RECEIVE block");
			}
		}else if (strcmp(SYM(qname)->name, "net_move") == 0) {
			if (artificial_cell) {
				replacstr(qname, "artcell_net_move");
			}
			if (blocktype == NETRECEIVE) {
				Insertstr(qpar1->next, "_tqitem, _pnt,");
			}else{
diag("net_move", " only allowed in NET_RECEIVE block");
			}
		}
		return;
	}
	q = insertstr(qpar1->next, "");
	if (qexpr) {
		vectorize_substitute(q, "_ix,");
	}else{
		vectorize_substitute(q, "_ix");
	}
}
#endif

#endif
		
function_table(s, qpar1, qpar2, qb1, qb2) /* s ( ... ) { ... } */
	Symbol* s;
	Item *qpar1, *qpar2, *qb1, *qb2;
{
	Symbol* t;
	int i;
	Item* q, *q1, *q2;
	static int first = 1;
	
	if (first) {
		first = 0;
		linsertstr(procfunc, "\nextern double hoc_func_table();\n");
	}	
	for (i=0, q=qpar1->next; q != qpar2; q = q->next) {
#if VECTORIZE
		if (q->itemtype == STRING || SYM(q)->name[0] != '_') {
			continue;
		}
#endif
		sprintf(buf, "_arg[%d] = %s;\n", i, SYM(q)->name);
		insertstr(qb2, buf);
		++i;
	}
	if (i == 0) {
		diag("FUNCTION_TABLE declaration must have one or more arguments:",
			s->name);
	}
	sprintf(buf, "double _arg[%d];\n", i);
	insertstr(qb1->next, buf);
	sprintf(buf, "return hoc_func_table(_ptable_%s, %d, _arg);\n", s->name, i);
	insertstr(qb2, buf);

	sprintf(buf, "table_%s", s->name);
	t = install(buf, NAME);
	t->subtype |= FUNCT;
	t->usage |= FUNCT;

	sprintf(buf,"double %s", t->name);
	lappendstr(procfunc, buf);
	q1 = lappendsym(procfunc, SYM(qpar1));
	q2 = lappendsym(procfunc, SYM(qpar2));
	sprintf(buf,"{\n\thoc_spec_table(&_ptable_%s, %d);\n\treturn 0.;\n}\n",
		s->name, i);
	lappendstr(procfunc, buf);
	sprintf(buf, "\nstatic void* _ptable_%s = (void*)0;\n", s->name);
	linsertstr(procfunc, buf);
	hocfunc(t, q1, q2);
}

watchstmt(par1, dir, par2, flag, blocktype )Item *par1, *dir, *par2, *flag;
	int blocktype;
{
	if (!watch_seen_) {
		++watch_seen_;
	}
	if (blocktype != NETRECEIVE) {
		diag("\"WATCH\" statement only allowed in NET_RECEIVE block", (char*)0);
	}
	sprintf(buf, "\nstatic double _watch%d_cond(_pnt) Point_process* _pnt; {\n	_p = _pnt->_prop->param; _ppvar = _pnt->_prop->dparam; v = NODEV(_pnt->node);\n	return ",
		watch_seen_);
	lappendstr(procfunc, buf);
	movelist(par1, par2, procfunc);
	movelist(dir->next, par2, procfunc);
	if (SYM(dir)->name[0] == '<') {
		insertstr(par1, "-(");
		insertstr(par2->next, ")");
	}
	replacstr(dir, ") - (");
	lappendstr(procfunc, ";\n}\n");

	sprintf(buf,
 "  _nrn_watch_activate(_watch_array, _watch%d_cond, %d, _pnt, _watch_rm++, %s);\n",
		watch_seen_, watch_seen_, STR(flag));
	replacstr(flag, buf);
	++watch_seen_;
}
