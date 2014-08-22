#include <../../nmodlconf.h>
/* /local/src/master/nrn/src/nmodl/solve.c,v 4.4 1998/08/20 21:07:34 hines Exp */

#include "modl.h"
#include "parse1.h"
#include "symbol.h"

/* make it an error if 2 solve statements are called on a single call to
model() */
extern List *indeplist;
Symbol *deltaindep = SYM0;
extern Symbol *indepsym;
extern char* current_line();
extern List *massage_list_;
#if NMODL
extern List *nrnstate;
#if VECTORIZE
extern int vectorize;
extern char* cray_pragma();
#endif
#endif
#if CVODE
Item* cvode_cnexp_solve;
Symbol* cvode_nrn_cur_solve_;
Symbol* cvode_nrn_current_solve_;
#endif

static void whileloop();
static void check_ss_consist();

/* Need list of solve statements. We impress the
general list structure to handle it.  The element is a pointer to an
item which is the first item in the statement sequence in another list.
*/
static List *solvq;	/* list of the solve statement locations */
int numlist = 0;	/* number of slist's */

void solvequeue(q1, q2, blocktype, qerr) /*solve NAME=q1 [using METHOD=q2]*/
				/* q2 = 0 means method wasn't there */
				/* qerr in ITEM0 or else the closing
				   brace of an IFERROR stmt */
	Item *q1, *q2, *qerr;
	int blocktype;
{
	/* the solvq list is organized in groups of an item element
	followed by the method symbol( null if default to be used) */
	/* The itemtype field of the first is used to carry the blocktype*/
	/* SOLVE and METHOD method are deleted */
	/* The list now consists of triples in which the third element
	   is a list containing the complete IFERROR statement.
	*/
	Item *lq, *qtemp;
	List *errstmt;

	if (!solvq) {
		solvq = newlist();
	}
#if NMODL
	/* if the blocktype is equation then move the solve statement to
		the nrnstate function.  Everything else stays in the
		model function to be used as the nrncurrent function */
	if (blocktype == BREAKPOINT) {
		if (!nrnstate) {
			nrnstate = newlist();
		}
		Lappendstr(nrnstate, "{");
		if (qerr) {
			movelist(q1->prev, qerr, nrnstate);
		}else if (q2) {
			movelist(q1->prev, q2, nrnstate);
		}else{
			movelist(q1->prev, q1, nrnstate);
		}
		Lappendstr(nrnstate, "}");
	}
	/* verify that the block defintion for this SOLVE has not yet been seen */
	if (massage_list_) ITERATE(lq, massage_list_) {
		if (strcmp(SYM(lq)->name, SYM(q1)->name) == 0) {
diag("The SOLVE statement must be before the DERIVATIVE block for ", SYM(lq)->name);
		}
	}
#endif
	lq = lappendsym(solvq, SYM0);
	ITM(lq) = q1;
	lq->itemtype = blocktype;
	/* handle STEADYSTATE option */
	if (q1->next->itemtype == SYMBOL &&
	    strcmp("STEADYSTATE", SYM(q1->next)->name) == 0) {
		lq->itemtype = -blocktype; /* gets put back below */
	}
	if (q2) {
		qtemp = q2->next;	/* The IFERROR location */
		Lappendsym(solvq, SYM(q2));
		if (strcmp(SYM(q2)->name, "derivimplicit") == 0) {
			add_deriv_imp_list(SYM(q1)->name);
		}
		if (strcmp(SYM(q2)->name, "cnexp") == 0) {
			SYM(q2)->name = stralloc("derivimplicit", SYM(q2)->name);
			add_deriv_imp_list(SYM(q1)->name);
#if CVODE
			cvode_cnexp_solve = lq;
#endif
		}
		delete(q2->prev);
		delete(q2);
	}else{
		qtemp = q1->next;
		Lappendsym(solvq, SYM0);
	}
	delete(q1->prev);

	/* handle the error statement */
	/* put one in if it isn't already there */
	if (qerr == ITEM0) {
#if NOCMODL
sprintf(buf, "if(error){fprintf(stderr,\"%s\\n\"); nrn_complain(_p); abort_run(error);}\n",
current_line());
		qtemp = qerr = insertstr(qtemp, buf);
#else
		qtemp = qerr = insertstr(qtemp,
"if(error){abort_run(error);}\n");
#endif
	}else{
		replacstr(qtemp, "if (error)");
	}
	errstmt = newlist();
	lq = lappendsym(solvq, SYM0);
	LST(lq) = errstmt;
	movelist(qtemp, qerr, errstmt);		
}
			
/* go through the solvq list and construct the proper while loop and calls*/
void solvhandler()
{
	Item *lq, *qsol, *follow;
	List *errstmt;
	Symbol *fun, *method;
	int numeqn, listnum, btype, steadystate;
	int cvodemethod_;
	
	if (!solvq)
		solvq = newlist();
	init_disc_vars(); /*why not here?*/
	ITERATE(lq, solvq) { /* remember it goes by 3's */
		steadystate=0;
		btype = lq->itemtype;
		if (btype < 0) {
			btype = lq->itemtype = -btype;
			steadystate = 1;
			check_ss_consist(lq);
		}
		qsol = ITM(lq);
		lq = lq->next;
		method = SYM(lq);
#if CVODE
		cvodemethod_ = 0;
		if (method && strcmp(method->name, "after_cvode") == 0) {
			method = (Symbol*)0;
			lq->element.sym = (Symbol*)0;
			cvodemethod_ = 1;
		}
		if (method && strcmp(method->name, "cvode_t") == 0) {
			method = (Symbol*)0;
			lq->element.sym = (Symbol*)0;
			cvodemethod_ = 2;
		}
		if (method && strcmp(method->name, "cvode_v") == 0) {
			method = (Symbol*)0;
			lq->element.sym = (Symbol*)0;
			cvodemethod_ = 3;
		}
#endif	
		lq = lq->next;
		errstmt = LST(lq);
		/* err stmt handling assumes qsol->next is where it goes. */
		fun = SYM(qsol);
		numeqn = fun->used;
		listnum =fun->u.i;
		follow = qsol->next; /* where p[0] gets updated */
		/* Check consistency of method and block type */
		if (method && !(method->subtype & fun->subtype)) {
			Sprintf(buf, "Method %s can't be used with Block %s",
				method->name, fun->name);
			diag(buf, (char *)0);
		}
		
		switch (fun->subtype) {

		case DERF:
			if (method == SYM0) {
				method = lookup("adrunge");
			}
			if (btype == BREAKPOINT && !steadystate) {
				/* derivatives recalculated after while loop */
if (strcmp(method->name, "cnexp") != 0 && strcmp(method->name, "derivimplicit") != 0) {
fprintf(stderr, "Notice: %s is not thread safe. Complain to Hines\n", method->name);
				vectorize = 0;
				Sprintf(buf, " %s();\n", fun->name);
				Insertstr(follow, buf);
		}
				/* envelope calls go after the while loop */
				sens_nonlin_out(follow, fun);
#if CVODE
				cvode_interface(fun, listnum, numeqn);
#endif
			}
			if (btype == BREAKPOINT) whileloop(qsol, (long)DERF, steadystate);
			solv_diffeq(qsol, fun,  method, numeqn, listnum, steadystate, btype);
			break;
		case KINF:
			if (method == SYM0) {
				method = lookup("_advance");
			}
			if (btype == BREAKPOINT && (method->subtype & DERF)) {
#if VECTORIZE
fprintf(stderr, "Notice: KINETIC with is thread safe only with METHOD sparse. Complain to Hines\n");
				vectorize = 0;
#endif
				/* derivatives recalculated after while loop */
				Sprintf(buf, " %s();\n", fun->name);
				Insertstr(follow, buf);
				/* envelope calls go after the while loop */
				sens_nonlin_out(follow, fun);
			}
			if (btype == BREAKPOINT) {
				whileloop(qsol, (long)DERF, steadystate);
#if CVODE
	if (strcmp(method->name, "sparse") == 0) {
				cvode_interface(fun, listnum, numeqn);
				cvode_kinetic(qsol, fun, numeqn, listnum);
				single_channel(qsol, fun, numeqn, listnum);
	}
#endif
			}
			solv_diffeq(qsol, fun,  method, numeqn, listnum, steadystate, btype);
			break;
		case NLINF:
#if VECTORIZE
fprintf(stderr, "Notice: NONLINEAR is not thread safe.\n");
			vectorize = 0;
#endif
			if (method == SYM0) {
				method = lookup("newton");
			}
			solv_nonlin(qsol, fun, method, numeqn, listnum);
			break;
		case LINF:
#if VECTORIZE
fprintf(stderr, "Notice: LINEAR is not thread safe.\n");
			vectorize = 0;
#endif
			if (method == SYM0) {
				method = lookup("simeq");
			}
			solv_lineq(qsol, fun, method, numeqn, listnum);
			break;
		case DISCF:
#if VECTORIZE
fprintf(stderr, "Notice: DISCRETE is not thread safe.\n");
			vectorize = 0;
#endif
			if (btype == BREAKPOINT) whileloop(qsol, (long)DISCRETE, 0);
			Sprintf(buf, "0; %s += d%s; %s();\n",
				indepsym->name, indepsym->name, fun->name);
			replacstr(qsol, buf);
			break;
#if 1	/* really wanted? */
		case PROCED:
			if (btype == BREAKPOINT) {
				whileloop(qsol, (long)DERF, 0);
#if CVODE
	if (cvodemethod_ == 1) { /*after_cvode*/
				cvode_interface(fun, listnum, 0);
	}
	if (cvodemethod_ == 2) { /*cvode_t*/
				cvode_interface(fun, listnum, 0);
				insertstr(qsol, "if (!cvode_active_)");
				cvode_nrn_cur_solve_ = fun;
				linsertstr(procfunc, "extern int cvode_active_;\n");
	}
	if (cvodemethod_ == 3) { /*cvode_t_v*/
				cvode_interface(fun, listnum, 0);
				insertstr(qsol, "if (!cvode_active_)");
				cvode_nrn_current_solve_ = fun;
				linsertstr(procfunc, "extern int cvode_active_;\n");
	}
#endif
			}
			Sprintf(buf, " %s();\n", fun->name);
			replacstr(qsol, buf);
#if VECTORIZE
	Sprintf(buf, "{ %s(_p, _ppvar, _thread, _nt); }\n",
		fun->name);
	vectorize_substitute(qsol, buf);
#endif
			break;
#endif
		case PARF:
#if VECTORIZE
fprintf(stderr, "Notice: PARTIAL is not thread safe.\n");
			vectorize = 0;
#endif
			if (btype == BREAKPOINT) whileloop(qsol, (long)DERF, 0);
			solv_partial(qsol, fun);
			break;
		default:
			diag("Illegal or unimplemented SOLVE type: ", fun->name);
			break;
		}
#if CVODE
		if (btype == BREAKPOINT) {
			cvode_valid();
		}
#endif
	/* add the error check */
	Insertstr(qsol, "error =");
	move(errstmt->next, errstmt->prev, qsol->next);
#if VECTORIZE
	if (errstmt->next == errstmt->prev) {
		vectorize_substitute(qsol->next, "");
		vectorize_substitute(qsol->prev, "");
	}else{
fprintf(stderr, "Notice: SOLVE with ERROR is not thread safe.\n");
		vectorize = 0;
	}
#endif
	freelist(&errstmt);
	/* under all circumstances, on return from model,
	 p[0] = current indepvar */
	/* obviously ok if indepvar is original; if it has been changed
	away from time
	then _sav_indep will be reset to starting value of original when
	initmodel is called on every call to model */
#if NMODL
#else
		if (btype == BREAKPOINT) {
#if SIMSYS
			Sprintf(buf, "_sav_indep = %s;\n", indepsym->name);
#else
			Sprintf(buf, "_sav_indep = _p[_indepindex];\n");
#endif
			Insertstr(follow, buf);
		}
#endif
	}
}

void save_dt(q)	/* save and restore the value of indepvar */
	Item *q;
{
	/*ARGSUSED*/
#if 0	/* integrators no longer increment time */
	static int first=1;

	if (first) {
		first = 0;
		Linsertstr(procfunc, "double _savlocal;\n");
	}
	Sprintf(buf, "_savlocal = %s;\n", indepsym->name);
	Insertstr(q, buf);
	Sprintf(buf, "%s = _savlocal;\n", indepsym->name);
	Insertstr(q->next, buf);
#endif
}

char *saveindep = "";

static void whileloop(qsol, type, ss)
	Item *qsol;
	long type;
	int ss;
{
/* no solve statement except this is allowed to
change the indepvar. Time passed to integrators
must therefore be saved and restored. */
/* Note that when the user changes the independent variable within Scop,
the while loop still uses the original independent variable. In this case
1) _break must be set to the entry value of the original independent variable
2) initmodel() must be called after each entry to model()
3) in initmodel() we are very careful not to destroy the entry value of
   time-- on exit it has its original value.  Note that _sav_indep gets
   any value of time that is set within initmodel().
*/
/* executing more that one for loop in a single call to model() is an error
which is trapped in scop */
	static int called = 0, firstderf = 1;
	char *cp;

	switch (type) {
	case DERF:
	case DISCRETE:
		if (firstderf) { /* time dependent process so create a deltastep */
			double d[3];
#ifndef NeXT			
			double atof();
#endif
			int i;
			Item *q;
			char sval[30];
			Sprintf(buf, "delta_%s", indepsym->name);
			for (i=0, q=indeplist->next; i<3; i++, q=q->next) {
				d[i] = atof(STR(q));
			}
			if (type == DERF) {
				Sprintf(sval, "%g", (d[1]-d[0])/d[2]);
			}else if (type == DISCRETE) {
				Sprintf(sval, "1.0");
			}
			deltaindep = ifnew_parminstall(buf, sval, "", "");
			firstderf = 0;
#if NMODL
#else
Sprintf(buf, "_modl_set_dt(_dt) double _dt; { %s = _dt;}\n",
		deltaindep->name);
			Lappendstr(procfunc, buf);
#endif
		}
		if (type == DERF) {
			cp = "dt";
		}else if (type == DISCRETE) {
			cp = "0.0";
		}
		if (ss) {
			return;
		}
		break;
	default:
		/*SUPPRESS 622*/
		assert(0);
	}
#if NMODL
	if (strcmp(indepsym->name, "t") != 0) {
		diag("The independent variable name must be `t'", (char *)0);
	}
#else
	Sprintf(buf, "_save = _break = %s; %s = _sav_indep;\n",
		indepsym->name, indepsym->name);
	Insertstr(qsol, buf);
#if !SIMSYS
	Sprintf(buf, "if (_p + _indepindex != &%s) {initmodel(_pp); %s = _sav_indep;}\n",
		indepsym->name, indepsym->name);
	Insertstr(qsol, buf);
#endif
#endif

	if (called) {
Fprintf(stderr, "Warning: More than one integrating SOLVE statement in an \
BREAKPOINT block.\nThe simulation will be incorrect if more than one is used \
at a time.\n");
	}
#if NMODL
	Insertstr(qsol, "{\n");
#else
	Sprintf(buf, "if (%s < _break) {\n",indepsym->name);
	Insertstr(qsol, buf);

/* ensure that there are an integer number of steps / break */
if (type == DERF) {
	Sprintf(buf, " { int _nstep; double _dt, _y;\n\
	_y = _break - %s; _dt = %s;\n", indepsym->name, "dt");
 	Insertstr(qsol, buf);
 	Insertstr(qsol, "_nstep = (int)(_y/_dt + .9);\n if (_nstep==0) _nstep = 1;\n");
 	Sprintf(buf, "%s = _y/((double)_nstep);\n", "dt");
 	Insertstr(qsol, buf);
	Sprintf(buf, "\n  }\n");
	Insertstr(qsol, buf);
}
 
	if (type == DERF) {
		Sprintf(buf, "_break -= .5* %s;\n", "dt");
		Insertstr(qsol, buf);
	}
#endif
	Sprintf(buf, "for (; %s < _break; %s += %s) {\n",
		indepsym->name,
		indepsym->name, cp);
	Insertstr(qsol, buf);
	/* close the while loop; note that integrators have been called */
	if (type == DERF) {
		Sprintf(buf, "\n}}\n %s = _save;\n", indepsym->name);
	}else if (type == DISCRETE) {
		Sprintf(buf, "\n}}\n");
	}		
	Insertstr(qsol->next,buf);
	if (!called) {
		/* fix up initmodel as per 3) above.
		In cout.c _save is declared */
#if NMODL
		Sprintf(buf, " _save = %s;\n %s = 0.0;\n",
			indepsym->name, indepsym->name);
		saveindep = stralloc(buf, (char *)0);
#else
		Sprintf(buf, "%s0", indepsym->name);
		IGNORE(ifnew_parminstall(buf, STR(indeplist->prev->prev),
			STR(indeplist->prev), ""));
		Sprintf(buf, " _save = %s;\n %s = %s0;\n",
			indepsym->name, indepsym->name, indepsym->name);
		saveindep = stralloc(buf, (char *)0);
#endif
		/* Assert no more additions to initfunc involving
		the value of time */
		Sprintf(buf, " _sav_indep = %s; %s = _save;\n",
			indepsym->name, indepsym->name);
		Lappendstr(initfunc, buf);
#if VECTORIZE
		vectorize_substitute(initfunc->prev, "");
#endif
	}
	called++;
}

/* steady state consistency means that KINETIC blocks whenever solved must
invoke same integrator (default is advance) and DERIVATIVE blocks whenever
solved must invoke the derivimplicit method.
*/
static void check_ss_consist(qchk)
	Item *qchk;
{
	Item *q;
	Symbol *fun, *method;
	
	ITERATE(q, solvq) {
		fun = SYM(ITM(q));
		if (fun == SYM(qchk)) {
			method= SYM(q->next);
			switch (q->itemtype) {
			case DERF:			
				if (!method || 
				   strcmp("derivimplicit", method->name)!=0) {
diag("STEADYSTATE requires all SOLVE's of this DERIVATIVE block to use the\n\
`derivimplicit' method:", fun->name);
				}
				break;
			case KINF:
				if (SYM(qchk->next) != method
				   || (method &&
				       strcmp("sparse", method->name) != 0)){
diag("STEADYSTATE requires all SOLVE's of this KINETIC block to use the\n\
same method (`advance' or `sparse'). :", fun->name);
				}
				break;
			default:
diag("STEADYSTATE only valid for SOLVEing a KINETIC or DERIVATIVE block:", fun->name);
				break;
			}
		}
		q = q->next->next;
	}
}
