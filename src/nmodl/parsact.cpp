#include <../../nmodlconf.h>

/*
 * some parse actions to reduce size of parse.y the installation routines can
 * also be used, e.g. in sens to automattically construct variables 
 */

#include <stdlib.h>
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

int protect_;
int protect_include_;
extern Item* vectorize_replacement_item(Item*);
extern int artificial_cell;
extern int vectorize;
extern int assert_threadsafe;

int type_change(Symbol* sym, int level);

static long previous_subtype;	/* subtype at the sym->level */
static char *previous_str;	/* u.str on last install */

void explicit_decl(int level, Item* q)
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
		diag(sym->name, "has different values at same level");
	}
}

/* restricted type changes are allowed in hierarchical models with each
one producing a message. Notice that multiple declarations at level 0 are
caught as errors in the function above. */

int type_change(Symbol* sym, int level) /*return 1 if type change, 0 otherwise*/
{
	long s, d, c;
	
	s = sym->subtype & STAT;
	d = sym->subtype & DEP;
	c = sym->subtype & PARM;

	if (s && c) {
		sym->subtype &= ~c;
Fprintf(stderr, "Notice: %s is promoted from a PARAMETER to a STATE\n", sym->name);
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
Fprintf(stderr, "Notice: %s is promoted from a PARAMETER to an ASSIGNED\n", sym->name);
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
	
void parm_array_install(Symbol* n, char* num, char* units, char* limits, int index)
{
	char buf[NRN_BUFSIZE];

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

void parminstall(Symbol* n, char* num, char* units, char* limits)
{
	char buf[NRN_BUFSIZE];

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
Symbol *ifnew_parminstall(char* name, char* num, char* units, char* limits)
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
			diag(s->name, "can't be declared a parameter by default");
		}
	}
	return s;
}

void steppedinstall(Symbol* n, Item* q1, Item* q2, char* units)
{
	int             i;

	char buf[NRN_BUFSIZE];
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

void indepinstall(Symbol* n, char* from, char* to, char* with, Item* qstart, char* units, int scop)
{
	char buf[NRN_BUFSIZE];

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
void depinstall(int type, Symbol* n, int index, char* from, char* to, char* units, Item* qs, int makeconst, char* abstol)
{
	char buf[NRN_BUFSIZE], *pstr;
	int             c;

	if (!type && strlen(abstol)>0) {
		printf("abstol = |%s|\n", abstol);
		diag(n->name, "tolerance can be specified only for a STATE");
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

void statdefault(Symbol* n, int index, char* units, Item* qs, int makeconst)
{
	char            nam[256], *un;
	Symbol         *s;

	if (n->type != NAME && n->type != PRIME) {
		diag(n->name, "can't be a STATE");
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

/* the problem is that qpar->next may already have a _p, ..., _nt
vectorize_substitute, and qpar->next is often normally "" instead of ')'
for the no arg case.
*/
static int func_arg_examine(Item* qpar, Item* qend) {
	Item* q;
	int b = 1; /* real args exist case */
	q = qpar->next;
	if (q->itemtype == SYMBOL && strcmp(SYM(q)->name, ")") == 0) {
		b = 0; /* definitely no arg */
	}
	if (q->itemtype == STRING && strcmp(STR(q), "") == 0) {
		if (vectorize_replacement_item(q)) {
			b = 2; /* _p,..._nt already there */
		} else if (q->next->itemtype == SYMBOL && strcmp(SYM(q->next)->name, ")") == 0) {
			b = 0; /* definitely no arg */
		}
	}
	return b;
}

void vectorize_scan_for_func(Item* q1, Item* q2) {
	Item* q, *qq;
	int b;
	return;
	for (q = q1; q != q2; q = q->next) {
		if (q->itemtype == SYMBOL) {
			Symbol* s = SYM(q);
			if ((s->usage & FUNCT) && !(s->subtype & (EXTDEF))) {
				if (q->next->itemtype == SYMBOL && strcmp(SYM(q->next)->name, "(") == 0) {
					int b = func_arg_examine(q->next, q2);
					if (b == 0) { /* no args */
						vectorize_substitute(q->next, "(_p, _ppvar, _thread, _nt");
					}else if (b == 1) { /* real args */
						vectorize_substitute(q->next, "(_p, _ppvar, _thread, _nt,");
					} /* else no _p.._nt already there */
				}
			}
		}
	}
}

void defarg(Item* q1, Item* q2)			/* copy arg list and define as doubles */
{
	Item           *q3, *q;

	if (q1->next == q2) {
#if VECTORIZE
		vectorize_substitute(insertstr(q2, ""), "_threadargsproto_");
#endif
		return;
	}
	for (q = q1->next; q != q2; q = q->next) {
		if (strcmp(SYM(q)->name, ",") != 0) {
			insertstr(q, "double");
		}
	}		
#if VECTORIZE
		vectorize_substitute(insertstr(q1->next, ""), "_threadargsprotocomma_");
#endif
}

void lag_stmt(Item* q1, int blocktype)	/* LAG name1 BY name2 */
{
	Symbol *name1, *name2, *lagval;

	/*ARGSUSED*/
	/* parse */
	name1 = SYM(q1->next);
	dlete(q1->next);
	dlete(q1->next);
	name2 = SYM(q1->next);
	dlete(q1->next);
	name1->usage |= DEP;
	name2->usage |= DEP;
	/* check */
	if (!indepsym) {
		diag("INDEPENDENT variable must be declared to process",
			 "the LAG statement");
	}
	if (!(name1->subtype & (DEP | STAT))) {
		diag(name1->name, "not a STATE or DEPENDENT variable");
	}
	if (!(name2->subtype & (PARM | nmodlCONST))) {
		diag(name2->name, "not a CONSTANT or PARAMETER");
	}
	Sprintf(buf, "lag_%s_%s", name1->name, name2->name);
	if (lookup(buf)) {
		diag(buf, "already in use");
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

void queue_stmt(Item* q1, Item* q2)
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
		diag(s->name, "not declared");
	}
	if (s->subtype & ARRAY) {
		Sprintf(buf, "%s, %d);\n", s->name, s->araydim);
	}else{
		Sprintf(buf, "&(%s), 1);\n", s->name);
	}
	replacstr(q2, buf);
}

void add_reset_args(Item* q)
{
	static int reset_fun_cnt=0;
	
	reset_fun_cnt++;
	Sprintf(buf, "&_reset, &_freset%d,", reset_fun_cnt);
	Insertstr(q->next, buf);
	Sprintf(buf, "static double _freset%d;\n", reset_fun_cnt);
	Lappendstr(firstlist, buf);
}

void add_nrnthread_arg(Item* q)
{
	vectorize_substitute(insertstr(q->next, "nrn_threads,"), "_nt,");
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

void check_tables() {
	/* for threads do this differently */
	if (check_table_statements) {
		fprintf(fcout, "\n#if %d\n", 0);
		printlist(check_table_statements);
		fprintf(fcout, "#endif\n");
	}
}

/* this way we can make sure the tables are up to date in the main thread
at critical points in the finitialize, nrn_fixed_step, etc. The only
requirement is that the function that generates the table not use
any except GLOBAL parameters and assigned vars not requiring
an initial value, because we are probably going to
call this with nonsense _p, _ppvar, and _thread
*/
static List* check_table_thread_list;
int check_tables_threads(List* p) {
	Item* q;
	if (check_table_thread_list) {
		ITERATE(q, check_table_thread_list) {
			sprintf(buf, "\nstatic void %s(double*, Datum*, Datum*, _NrnThread*);", STR(q));
			lappendstr(p, buf);
		}
		lappendstr(p, "\nstatic void _check_table_thread(double* _p, Datum* _ppvar, Datum* _thread, _NrnThread* _nt, int _type) {\n");
		ITERATE(q, check_table_thread_list) {
			sprintf(buf, "  %s(_p, _ppvar, _thread, _nt);\n", STR(q));
			lappendstr(p, buf);
		}
		lappendstr(p, "}\n");
		return 1;
	}
	return 0;
}

void table_massage(List* tablist, Item* qtype, Item* qname, List* arglist)
{
	Symbol *fsym, *s, *arg=0;
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

	ifnew_parminstall( "usetable", "1", "", "0 1");
	if (!check_table_statements) {
		check_table_statements = newlist();
	}
	sprintf(buf, "_check_%s();\n", fname);
	q = lappendstr(check_table_statements, buf);
	sprintf(buf, "_check_%s(_p, _ppvar, _thread, _nt);\n", fname);
	vectorize_substitute(q, buf);
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
	SYM(qname)->varnum = fsym->varnum;
	if (type == FUNCTION1) {
		fsym->subtype |= FUNCT;
		Sprintf(buf, "static double _n_%s(double);\n", fname);
		q = linsertstr(procfunc, buf);
#if VECTORIZE
		Sprintf(buf, "static double _n_%s(_threadargsprotocomma_ double _lv);\n", fname);
		vectorize_substitute(q, buf);
#endif
	}else{
		fsym->subtype |= PROCED;
		Sprintf(buf, "static void _n_%s(double);\n", fname);
		q = linsertstr(procfunc, buf);
#if VECTORIZE
		Sprintf(buf, "static void _n_%s(_threadargsprotocomma_ double _lv);\n", fname);
		vectorize_substitute(q, buf);
#endif
	}
	fsym->usage |= FUNCT;
		
	/* declare communication between func and check_func */
	Sprintf(buf, "static double _mfac_%s, _tmin_%s;\n",
		fname, fname);
	Lappendstr(procfunc, buf);

	/* create the check function */
	if (!check_table_thread_list) {
		check_table_thread_list = newlist();
	}
	sprintf(buf, "_check_%s", fname);
	lappendstr(check_table_thread_list, buf);
	Sprintf(buf, "static void _check_%s();\n", fname);
	q = insertstr(procfunc, buf);
	vectorize_substitute(q, "");
	Sprintf(buf, "static void _check_%s() {\n", fname);
	q = lappendstr(procfunc, buf);
	Sprintf(buf, "static void _check_%s(double* _p, Datum* _ppvar, Datum* _thread, _NrnThread* _nt) {\n", fname);
	vectorize_substitute(q, buf);
	Lappendstr(procfunc, " static int _maktable=1; int _i, _j, _ix = 0;\n");
	Lappendstr(procfunc, " double _xi, _tmax;\n");
	ITERATE(q, depend) {
		Sprintf(buf, " static double _sav_%s;\n", SYM(q)->name);
		Lappendstr(procfunc, buf);
	}
	lappendstr(procfunc, " if (!usetable) {return;}\n");
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
			Sprintf(buf, "   _t_%s[_i] = _f_%s(_p, _ppvar, _thread, _nt, _x);\n", s->name, fname);
			vectorize_substitute(procfunc->prev, buf);
#endif
		}
	}else{
		Sprintf(buf, "   _f_%s(_x);\n", fname);
		Lappendstr(procfunc, buf);
#if VECTORIZE
		Sprintf(buf, "   _f_%s(_p, _ppvar, _thread, _nt, _x);\n", fname);
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
		Lappendstr(procfunc, "static int");
#endif
		Lappendstr(procfunc, "double");
	}else{
		Lappendstr(procfunc, "static int");
	}
	Sprintf(buf, "%s(double %s){",
			fname, arg->name);
	Lappendstr(procfunc, buf);		
#if VECTORIZE
	Sprintf(buf, "%s(double* _p, Datum* _ppvar, Datum* _thread, _NrnThread* _nt, double %s) {",
		fname, arg->name);
	vectorize_substitute(procfunc->prev, buf);
#endif
	/* check the table */
	Sprintf(buf, "_check_%s();\n", fname);
	q = lappendstr(procfunc, buf);
#if VECTORIZE
	Sprintf(buf, "\n#if 0\n_check_%s(_p, _ppvar, _thread, _nt);\n#endif\n", fname);
	vectorize_substitute(q, buf);
#endif
	if (type == FUNCTION1) {
		Lappendstr(procfunc, "return");
	}
	Sprintf(buf, "_n_%s(%s);\n", fname, arg->name);
	Lappendstr(procfunc, buf);
#if VECTORIZE
	Sprintf(buf, "_n_%s(_p, _ppvar, _thread, _nt, %s);\n", fname, arg->name);
	vectorize_substitute(procfunc->prev, buf);
#endif
	if (type != FUNCTION1) {
		Lappendstr(procfunc, "return 0;\n");
	}
	Lappendstr(procfunc, "}\n\n"); /* end of new function */

	/* _n_name function for table lookup with no checking */
	if (type == FUNCTION1) {
		Lappendstr(procfunc, "static double");
	}else{
		Lappendstr(procfunc, "static void");
	}
	Sprintf(buf, "_n_%s(double %s){",
			fname, arg->name);
	Lappendstr(procfunc, buf);		
#if VECTORIZE
	Sprintf(buf, "_n_%s(double* _p, Datum* _ppvar, Datum* _thread, _NrnThread* _nt, double %s){",
		fname, arg->name);
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
	Sprintf(buf, "_f_%s(_p, _ppvar, _thread, _nt, %s);", fname, arg->name);
	vectorize_substitute(procfunc->prev, buf);
#endif
	if (type != FUNCTION1) {
		Lappendstr(procfunc, "return;");
	}
	Lappendstr(procfunc, "\n}\n");

	/* table lookup */
	Sprintf(buf, "_xi = _mfac_%s * (%s - _tmin_%s);\n",
		fname, arg->name, fname);
	Lappendstr(procfunc, buf);
	Lappendstr(procfunc, "if (isnan(_xi)) {\n");
	if (type == FUNCTION1) {
 		Lappendstr(procfunc, " return _xi; }\n");
 	}else{
		ITERATE(q, table) {
			s = SYM(q);
			if (s->subtype & ARRAY) {
Sprintf(buf, " for (_j = 0; _j < %d; _j++) { %s[_j] = _xi;\n}",
s->araydim, s->name);
			}else{
Sprintf(buf, " %s = _xi;\n", s->name);
			}
			Lappendstr(procfunc, buf);
		}
		Lappendstr(procfunc, " return;\n }\n");
	}
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
	Sprintf(buf, "if (_xi >= %d.) {\n", ntab);
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
	Lappendstr(procfunc, "_i = (int) _xi;\n");
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
void hocfunchack(Symbol* n, Item* qpar1, Item* qpar2, int hack)
{
#if NOCMODL
	extern int point_process;
#endif
	Item *q;
	int i;
#if VECTORIZE
	Item* qp=0;
#endif
	
   if (point_process) {
	Sprintf(buf, "\nstatic double _hoc_%s(void* _vptr) {\n double _r;\n", n->name);
   }else{
	Sprintf(buf, "\nstatic void _hoc_%s(void) {\n  double _r;\n", n->name);
   }
	Lappendstr(procfunc, buf);
	vectorize_substitute(lappendstr(procfunc, ""), "\
  double* _p; Datum* _ppvar; Datum* _thread; _NrnThread* _nt;\n\
");
	if (point_process) {
		vectorize_substitute(lappendstr(procfunc, "  _hoc_setdata(_vptr);\n"), "\
  _p = ((Point_process*)_vptr)->_prop->param;\n\
  _ppvar = ((Point_process*)_vptr)->_prop->dparam;\n\
  _thread = _extcall_thread;\n\
  _nt = (_NrnThread*)((Point_process*)_vptr)->_vnt;\n\
");
	}else{
		vectorize_substitute(lappendstr(procfunc, ""), "\
  if (_extcall_prop) {_p = _extcall_prop->param; _ppvar = _extcall_prop->dparam;}else{ _p = (double*)0; _ppvar = (Datum*)0; }\n\
  _thread = _extcall_thread;\n\
  _nt = nrn_threads;\n\
");
	}
#if VECTORIZE
	if (n == last_func_using_table) {
		qp = lappendstr(procfunc, "");
		sprintf(buf,"\n#if 1\n _check_%s(_p, _ppvar, _thread, _nt);\n#endif\n", n->name);
		vectorize_substitute(qp, buf);
	}
#endif
	if (n->subtype & FUNCT) {
		Lappendstr(procfunc, "_r = ");
	}else{
		Lappendstr(procfunc, "_r = 1.;\n");
	}
	Lappendsym(procfunc, n);
	lappendstr(procfunc, "(");
#if VECTORIZE
	qp = lappendstr(procfunc, "");
#endif
	for (i=0; i < n->varnum; ++i) {
		Sprintf(buf, "*getarg(%d)", i+1);
		Lappendstr(procfunc, buf);
		if (i+1 < n->varnum) {
			Lappendstr(procfunc, ",");
		}
	}
#if NOCMODL
   if (point_process) {
	Lappendstr(procfunc, ");\n return(_r);\n}\n");
   }else
#endif
	Lappendstr(procfunc, ");\n hoc_retpushx(_r);\n}\n");
#if VECTORIZE
	if (i) {
		vectorize_substitute(qp, "_p, _ppvar, _thread, _nt,");
	}else if (!hack) {
		vectorize_substitute(qp, "_p, _ppvar, _thread, _nt");
	}
#endif
}

void hocfunc(Symbol* n, Item* qpar1, Item* qpar2) /*interface between modl and hoc for proc and func */
{
	/* Hack prevents FUNCTION_TABLE bug of 'double table_name()' extra args
	   replacing the double in 'double name(...) */
	hocfunchack(n, qpar1, qpar2, 0);
}

#if VECTORIZE
/* ARGSUSED */
void vectorize_use_func(Item* qname, Item* qpar1, Item* qexpr, Item* qpar2, int blocktype)
{
	Item* q;
	if (SYM(qname)->subtype &  EXTDEF) {
		if (strcmp(SYM(qname)->name, "nrn_pointing") == 0) {
			Insertstr(qpar1->next, "&");
		}else if (strcmp(SYM(qname)->name, "state_discontinuity") == 0) {
#if CVODE
		    if (blocktype == NETRECEIVE) {
			Item* qeq = NULL;
			/* convert to state = expr form and process with netrec_discon(...) */
			replacstr(qname, "");
			replacstr(qpar1, "");
			replacstr(qpar2, "");
			/* qexpr begins state, expr */
			/* find the first , and replace by = */
			for (q = qexpr; q != qpar2; q = q->next) {
			  if (q->itemtype == SYMBOL && strcmp(SYM(q)->name, ",") == 0) {
			    qeq = q;
			    replacstr(qeq, "=");
			    break;
			  }
			}
			assert(qeq);
			netrec_asgn(qexpr, qeq,  qeq->next, qpar2);
		    }else{
fprintf(stderr, "Notice: Use of state_discontinuity is not thread safe except in a NET_RECEIVE block");
			vectorize = 0;
			if (!state_discon_list_) {
				state_discon_list_ = newlist();
				Linsertstr(procfunc, "extern int state_discon_flag_;\n");
			}
			lappenditem(state_discon_list_, qpar1->next);
			Insertstr(qpar1->next, "-1, &");
		    }
#endif
		}else if (strcmp(SYM(qname)->name, "net_send") == 0) {
			net_send_seen_ = 1;
			if (artificial_cell) {
				replacstr(qname, "artcell_net_send");
			}
			Insertstr(qexpr, "t + ");
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
diag("net_event", "only allowed in NET_RECEIVE block");
			}
		}else if (strcmp(SYM(qname)->name, "net_move") == 0) {
			if (artificial_cell) {
				replacstr(qname, "artcell_net_move");
			}
			if (blocktype == NETRECEIVE) {
				Insertstr(qpar1->next, "_tqitem, _pnt,");
			}else{
diag("net_move", "only allowed in NET_RECEIVE block");
			}
		}
		return;
	}
#if 1
	if (qexpr) {
		q = insertstr(qpar1->next, "_threadargscomma_");
	}else{
		q = insertstr(qpar1->next, "_threadargs_");
	}
#else
	q = insertstr(qpar1->next, "");
	if (qexpr) {
		vectorize_substitute(q, "_p, _ppvar, _thread, _nt,");
	}else{
		vectorize_substitute(q, "_p, _ppvar, _thread, _nt");
	}
#endif
}
#endif

#endif
		
void function_table(Symbol* s, Item* qpar1, Item* qpar2, Item* qb1, Item* qb2) /* s ( ... ) { ... } */
{
	Symbol* t;
	int i;
	Item* q, *q1, *q2;
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
	insertstr(qb2, "}\n/* "); /* kludge to avoid a bad vectorize_substitute */
	insertstr(qb2->next, " */\n");

	sprintf(buf, "table_%s", s->name);
	t = install(buf, NAME);
	t->subtype |= FUNCT;
	t->usage |= FUNCT;
	t->no_threadargs = 1;
	t->varnum = 0;

	sprintf(buf,"double %s", t->name);
	lappendstr(procfunc, buf);
	q1 = lappendsym(procfunc, SYM(qpar1));
	q2 = lappendsym(procfunc, SYM(qpar2));
	sprintf(buf,"{\n\thoc_spec_table(&_ptable_%s, %d);\n\treturn 0.;\n}\n",
		s->name, i);
	lappendstr(procfunc, buf);
	sprintf(buf, "\nstatic void* _ptable_%s = (void*)0;\n", s->name);
	linsertstr(procfunc, buf);
	hocfunchack(t, q1, q2, 1);
}

void watchstmt(Item* par1, Item* dir, Item* par2, Item* flag, int blocktype )
{
	if (!watch_seen_) {
		++watch_seen_;
	}
	if (blocktype != NETRECEIVE) {
		diag("\"WATCH\" statement only allowed in NET_RECEIVE block", (char*)0);
	}
	sprintf(buf, "\nstatic double _watch%d_cond(_pnt) Point_process* _pnt; {\n",
		watch_seen_);
	lappendstr(procfunc, buf);
	vectorize_substitute(lappendstr(procfunc, ""),(char*)"\tdouble* _p; Datum* _ppvar; Datum* _thread; _NrnThread* _nt;\n\t_thread= (Datum*)0; _nt = (_NrnThread*)_pnt->_vnt;\n");
	sprintf(buf, "\t_p = _pnt->_prop->param; _ppvar = _pnt->_prop->dparam;\n\tv = NODEV(_pnt->node);\n	return ");
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

void threadsafe(char* s) {
	if (!assert_threadsafe) {
		fprintf(stderr, "Notice: %s\n", s);
		vectorize = 0;
	}
}


Item* protect_astmt(Item* q1, Item* q2) { /* PROTECT, ';' */
	Item* q;
	replacstr(q1, "/* PROTECT */_NMODLMUTEXLOCK\n");
	q = insertstr(q2->next, "\n _NMODLMUTEXUNLOCK /* end PROTECT */\n");
	protect_include_ = 1;
	return q;
}

void nrnmutex(int on, Item* q) { /* MUTEXLOCK or MUTEXUNLOCK */
	static int toggle = 0;
	if (on == 1) {
		if (toggle != 0) {
			diag("MUTEXLOCK invoked after MUTEXLOCK", (char*)0);
		}
		toggle = 1;
		replacstr(q, "_NMODLMUTEXLOCK\n");
		protect_include_ = 1;
	}else if (on == 0) {
		if (toggle != 1) {
			diag("MUTEXUNLOCK invoked with no earlier MUTEXLOCK", (char*)0);
		}
		toggle = 0;
		replacstr(q, "_NMODLMUTEXUNLOCK\n");
		protect_include_ = 1;
	}else{
		if (toggle != 0) {
			diag("MUTEXUNLOCK not invoked after MUTEXLOCK", (char*)0);
		}
		toggle = 0;
	}
}
