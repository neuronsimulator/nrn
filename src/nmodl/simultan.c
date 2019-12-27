#include <../../nmodlconf.h>
#include "modl.h"
#include "parse1.h"
#include "symbol.h"

extern int sens_parm;
extern int numlist;
static List *eqnq;

static int nonlin_common();

void solv_nonlin(qsol, fun, method, numeqn, listnum)
	Item *qsol;
	Symbol *fun, *method;
	int numeqn, listnum;
{
	Sprintf(buf, "%s(%d,_slist%d, _p, %s, _dlist%d);\n",
		method->name, numeqn, listnum, fun->name, listnum);
	replacstr(qsol, buf);
	/* if sens statement appeared in fun then the steadysens call list,
	built during the massagenonlin phase
	gets added after the call to newton */
	sens_nonlin_out(qsol->next, fun);
}

void solv_lineq(qsol, fun, method, numeqn, listnum)
	Item *qsol;
	Symbol *fun, *method;
	int numeqn, listnum;
{
	Sprintf(buf, " 0; %s();\n error = %s(%d, _coef%d, _p, _slist%d);\n",
		fun->name, method->name, numeqn, listnum, listnum);
	replacstr(qsol, buf);
	sens_nonlin_out(qsol->next, fun);
}

void eqnqueue(q1)
	Item *q1;
{
	Item *lq;

	if (!eqnq) {
		eqnq = newlist();
	}
	lq = lappendsym(eqnq, SYM0);
	ITM(lq) = q1;
	return;
}

static void freeqnqueue()
{
	freelist(&eqnq);
}

/* args are --- nonlinblk: NONLINEAR NAME stmtlist '}' */
void massagenonlin(q1, q2, q3, q4, sensused)
	Item *q1, *q2, *q3, *q4;
	int sensused;
{
/* declare a special _counte variable to number the equations.
before each equation we increment it by 1 during run time.  This
gives us a current equation number */
	Symbol *nonfun;
	
	/* all this junk is still in the intoken list */
	Sprintf(buf, "static void %s();\n", SYM(q2)->name);
	Linsertstr(procfunc, buf);
	replacstr(q1, "\nstatic void"); Insertstr(q3, "()\n");
	Insertstr(q3->next, " int _counte = -1;\n");
	nonfun = SYM(q2);
	if ((nonfun->subtype) & NLINF && nonfun->u.i) {
		diag("NONLINEAR merging not implemented", (char *)0);
	}
	numlist++;
	nonfun->subtype |= NLINF;
	nonfun->u.i = numlist;
	nonfun->used = nonlin_common(q4, sensused);
	movelist(q1, q4, procfunc);
	if (sensused) {
		sensmassage(NONLINEAR, q2, numlist);
	}
}

static int nonlin_common(q4, sensused)	/* used by massagenonlin() and mixed_eqns() */
	Item *q4;
	int sensused;
{
	Item *lq, *qs, *q;
	int i, counts = 0, counte = 0, using_array;
	Symbol *s;

	using_array=0;
	SYMITER_STAT {
		if (s->used) {
			s->varnum = counts;
#if CVODE
			slist_data(s, counts, numlist);
#endif
			if (s->subtype & ARRAY) {int dim = s->araydim;
				using_array=1;
		        	Sprintf(buf, "for(_i=0;_i<%d;_i++){_slist%d[%d+_i] = (%s + _i) - _p;}\n"
			                ,dim, numlist , counts, s->name);
				counts += dim;
			}else{
				Sprintf(buf, "_slist%d[%d] = &(%s) - _p;\n",
					numlist, counts, s->name);
				counts++;
			}
		        Lappendstr(initlist, buf);
			s->used = 0;
			if (sensused) {
				add_sens_statelist(s);
			}
		}
	}

	ITERATE(lq, eqnq) {
		char *eqtype = SYM(ITM(lq))->name;
		if (strcmp(eqtype, "~+") == 0) { /* add equation to previous */
			if (counte == -1) {
				diag("no previous equation for adding terms", (char *)0);
			}
			Sprintf(buf, "_dlist%d[_counte] +=", numlist);
		} else if (eqtype[0] == 'D') {
			/* derivative equations using implicit method */
			int count_deriv = SYM(ITM(lq))->araydim;
			Sprintf(buf, "_dlist%d[++_counte] =", numlist);
			counte += count_deriv;
		}else{
			Sprintf(buf, "_dlist%d[++_counte] =", numlist);
			counte++;
		}
		replacstr(ITM(lq), buf);
	}
	if (!using_array) {
		if(counte != counts) {
Sprintf(buf ,"Number of equations, %d, does not equal number, %d", counte, counts);
			diag(buf, " of states used");
		}
	} else {
#if 1	/* can give message when running */
Sprintf(buf, "if(_counte != %d) printf( \"Number of equations, %%d,\
 does not equal number of states, %d\", _counte + 1);\n",
			counts-1, counts);
		Insertstr(q4, buf);
#endif
	}
	if (counte == 0) {
		diag("NONLINEAR contains no equations", (char *)0);
	}
	freeqnqueue();
Sprintf(buf, "static int _slist%d[%d]; static double _dlist%d[%d];\n",
numlist, counts*(1 + sens_parm), numlist, counts);
	q = linsertstr(procfunc, buf);
Sprintf(buf, "static int _slist%d[%d];\n",
numlist, counts*(1 + sens_parm));
	vectorize_substitute(q, buf);	
	return counts;
}

Item *mixed_eqns(q2, q3, q4)	/* name, '{', '}' */
	Item *q2, *q3, *q4;
{
	int counts;
	Item *qret;
	Item* q;
	
	if (!eqnq) {
		return ITEM0; /* no nonlinear algebraic equations */
	}
	/* makes use of old massagenonlin split into the guts and
	the header stuff */
	numlist++;
	counts = nonlin_common(q4, 0);
	Insertstr(q4, "}");
	q = insertstr(q3, "{ static int _recurse = 0;\n int _counte = -1;\n");
	sprintf(buf, "{ double* _savstate%d = _thread[_dith%d]._pval;\n\
 double* _dlist%d = _thread[_dith%d]._pval + %d;\n int _counte = -1;\n",
numlist-1, numlist-1,
numlist, numlist-1, counts);
	vectorize_substitute(q, buf);
	Insertstr(q3, "if (!_recurse) {\n _recurse = 1;\n");
	Sprintf(buf, "error = newton(%d,_slist%d, _p, %s, _dlist%d);\n",
		counts, numlist, SYM(q2)->name, numlist);
	qret = insertstr(q3, buf);
	Sprintf(buf, "error = nrn_newton_thread(_newtonspace%d, %d,_slist%d, _p, %s, _dlist%d, _ppvar, _thread, _nt);\n",
		numlist-1, counts, numlist, SYM(q2)->name, numlist);
	vectorize_substitute(qret, buf);
	Insertstr(q3, "_recurse = 0; if(error) {abort_run(error);}}\n");
	return qret;
}

/* linear simultaneous equations */
/* declare a _counte variable to dynamically contain the current
equation number.  This is necessary to allow use of state vectors.
It is no longer necessary to count equations here but we do it
anyway in case of future move to named equations.
It is this change which requires a varnum field in Symbols for states
since the arraydim field cannot be messed with anymore.
*/
static int nlineq = -1; /* actually the current index of the equation */
			/* is only good if there are no arrays */
static int using_array;	/* 1 if vector state in equations */
static int nstate = 0;
static Symbol *linblk;
static Symbol *statsym;

void init_linblk(q) /* NAME */
	Item *q;
{
	using_array = 0;
	nlineq = -1;
	nstate = 0;
	linblk = SYM(q);
	numlist++;
}

void init_lineq(q1) /* the colon */
	Item *q1;
{
	if (strcmp(SYM(q1)->name, "~+") == 0) {
		replacstr(q1, "");
	}else {
		nlineq++; /* current index will start at 0 */
		replacstr(q1, " ++_counte;\n");
	}
}

static char *indexstr;	/* set in lin_state_term, used in linterm */

void lin_state_term(q1, q2) /* term last*/
	Item *q1, *q2;
{
	char *qconcat(); /* but puts extra ) at end */
	
	statsym = SYM(q1);
	replacstr(q1, "1.0");
	if (statsym->subtype & ARRAY) { 
		indexstr = qconcat(q1->next->next, q2->prev);
		deltokens(q1->next, q2->prev); /*can't erase lastok*/
		replacstr(q2, "");
	}
	if (statsym->used == 1) {
		statsym->varnum = nstate;
		if (statsym->subtype & ARRAY) {int dim = statsym->araydim;
			using_array=1;
		        Sprintf(buf, "for(_i=0;_i<%d;_i++){_slist%d[%d+_i] = (%s + _i) - _p;}\n"
		                ,dim, numlist , nstate, statsym->name);
			nstate += dim;
		}else{
			Sprintf(buf, "_slist%d[%d] = &(%s) - _p;\n",
				numlist, nstate, statsym->name);
			nstate++;
		}
	        Lappendstr(initlist, buf);
	}
}

void linterm(q1, q2, pstate, sign) /*primary, last ,, */
	Item *q1, *q2;
	int pstate, sign;
{
	char *signstr;
	
	if (pstate == 0) {
		sign *= -1;
	}
	if (sign == -1) {
		signstr = " -= ";
	} else {
		signstr = " += ";
	}
	
	if (pstate == 1) {
		if (statsym->subtype & ARRAY) {
			Sprintf(buf, "_coef%d[_counte][%d + %s]%s",
				numlist, statsym->varnum, indexstr, signstr);
		} else {
			Sprintf(buf, "_coef%d[_counte][%d]%s",
				numlist, statsym->varnum, signstr);
		}
		Insertstr(q1, buf);
	} else if (pstate == 0) {
		Sprintf(buf, "_RHS%d(_counte)%s", numlist, signstr);
		Insertstr(q1, buf);
	}else{
		diag("more than one state in preceding term", (char *)0);
	}
	Insertstr(q2->next, ";\n");
}
	
void massage_linblk(q1, q2, q3, q4, sensused) /* LINEAR NAME stmtlist '}' */
	Item *q1, *q2, *q3, *q4;
	int sensused;
{
	Item *qs;
	Symbol *s;
	int i;
	
#if LINT
assert(q2);
#endif
	if (++nlineq == 0) {
		diag(linblk->name, " has no equations");
	}
	Sprintf(buf, "static void %s();\n", SYM(q2)->name);
	Linsertstr(procfunc, buf);
	replacstr(q1, "\nstatic void"); Insertstr(q3, "()\n");
	Insertstr(q3->next, " int _counte = -1;\n");
	linblk->subtype |= LINF;
	linblk->u.i = numlist;
	SYMITER(NAME){
		if ((s->subtype &STAT) && s->used) {
			if (sensused) {
				add_sens_statelist(s);
			}
			s->used = 0;
		}
	}
	if (!using_array) {
		if (nlineq != nstate) {
Sprintf(buf,"Number states, %d, unequal to equations, %d in ", nstate, nlineq);
			diag(buf, linblk->name);
		}
	} else {
#if 1	/* can give message when running */
Sprintf(buf, "if(_counte != %d) printf( \"Number of equations, %%d,\
 does not equal number of states, %d\", _counte + 1);\n",
			nstate-1, nstate);
		Insertstr(q4, buf);
#endif
	}
	linblk->used = nstate;
	Sprintf(buf, "static int _slist%d[%d];static double **_coef%d;\n",
		numlist, nstate*(1 + sens_parm), numlist);
	Linsertstr(procfunc, buf);
	Sprintf(buf, "\n#define _RHS%d(arg) _coef%d[arg][%d]\n",
		numlist, numlist, nstate);
	Linsertstr(procfunc, buf);
	Sprintf(buf, "if (_first) _coef%d = makematrix(%d, %d);\n",
		numlist, nstate, nstate+1);
	Lappendstr(initlist, buf);
	Sprintf(buf, "zero_matrix(_coef%d, %d, %d);\n{\n",
		numlist, nstate, nstate+1);
	Insertstr(q3->next, buf);
	Insertstr(q4, "\n}\n");
	movelist(q1, q4, procfunc);
	if (sensused) {
		sensmassage(LINEAR, q2, numlist);
	}
	nstate = 0; nlineq = 0;
}


/* It is sometimes convenient to not use some states in solving equations.
   We use the SOLVEFOR statement to list the states in LINEAR, NONLINEAR,
   and KINETIC blocks that are to be treated as states in fact. States
   not listed are treated in that block as assigned variables.
   If the SOLVEFOR statement is absent all states are assumed to be in the
   list.
   
   Syntax is:
      blocktype blockname [SOLVEFOR name, name, ...] { statement }

   The implementation uses the varname: production that marks the state->used
   record. The old if statement was
   	if (inequation && (SYM($1)->subtype & STAT)) { then mark states}
   now we add && in_solvefor() to indicate that it Really should be marked.
   The hope is that no further change to diagnostics for LINEAR or NONLINEAR
   will be required.  Some more work on KINETIC is required since the checking
   on whether a name is a STAT is done much later.
   The solveforlist is freed at the end of each block.
*/

List *solveforlist = (List *)0;

int in_solvefor(s)
	Symbol *s;
{
	Item *q;
	
	if (!solveforlist) {
		return 1;
	}
	ITERATE(q, solveforlist) {
		if (s == SYM(q)) {
			return 1;
		}
	}
	return 0;
}
