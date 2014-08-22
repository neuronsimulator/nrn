#include <../../nmodlconf.h>

#include "modl.h"
#include "symbol.h"
#include <ctype.h>
#undef METHOD
#include "parse1.h"

static List *deriv_imp_list;	/* list of derivative blocks that were
	translated in form suitable for the derivimplicit method */
static char Derivimplicit[] = "derivimplicit";	

extern Symbol *indepsym;
extern List *indeplist;
extern int sens_parm, numlist;
static void copylist();
List* massage_list_;

#if VECTORIZE
extern int vectorize;
extern int assert_threadsafe;
extern int thread_data_index;
extern List* thread_mem_init_list;
extern List* thread_cleanup_list;
#endif

#if CVODE
extern char* cvode_deriv(), *cvode_eqnrhs();
extern Item* cvode_cnexp_solve;
static void cvode_diffeq();
static List* cvode_diffeq_list, *cvode_eqn;
static int cvode_cnexp_possible;
#endif

void solv_diffeq(qsol, fun, method, numeqn, listnum, steadystate, btype)
	Item *qsol;
	Symbol *fun, *method;
	int numeqn, listnum;
	int btype;
{
	char *maxerr_str, dindepname[50];
	char deriv1_advance[30], deriv2_advance[30];
	char ssprefix[8];
	
	if (method && strcmp(method->name, "cnexp") == 0) {
		sprintf(buf, " %s();\n", fun->name);
		replacstr(qsol, buf);
		sprintf(buf, " %s(_p, _ppvar, _thread, _nt);\n", fun->name);
		vectorize_substitute(qsol, buf); 
		return;
	}
	if (steadystate) {
		Strcpy(ssprefix, "_ss_");
	}else{
		Strcpy(ssprefix, "");
	}
	Sprintf(dindepname, "d%s", indepsym->name);
	if (fun->subtype & KINF) { /* translate the kinetic equations */
		/* can be standard integrator, full matrix advancec, or
		   sparse matrix advance */
		/* at this time only sparse and standard exists */
		if (method->subtype & DERF) {
			kinetic_intmethod(fun, method->name);
		} else {
			kinetic_implicit(fun, dindepname, method->name);
		}
	}
	save_dt(qsol);
	if (method->subtype & DERF) {
		if (method->u.i == 1) { /* variable step method */
			maxerr_str = ", maxerr";
			IGNORE(ifnew_parminstall("maxerr", "1e-5", "", ""));
		} else {
			maxerr_str = "";
		}

if (deriv_imp_list) {	/* make sure deriv block translation matches method */
	Item *q; int found=0;
	ITERATE(q, deriv_imp_list) {
		if (strcmp(STR(q), fun->name) == 0) {
			found = 1;
		}
	}
	if ((strcmp(method->name, Derivimplicit) == 0) ^ (found == 1)) {
	diag("To use the derivimplicit method the SOLVE statement must\
 precede the DERIVATIVE block\n",
" and all SOLVEs using that block must use the derivimplicit method\n");
	}
	Sprintf(deriv1_advance, "_deriv%d_advance = 1;\n", listnum);
	Sprintf(deriv2_advance, "_deriv%d_advance = 0;\n", listnum);
	Sprintf(buf, "static int _deriv%d_advance = 0;\n", listnum);
	q = linsertstr(procfunc, buf);
	Sprintf(buf, "\n#define _deriv%d_advance _thread[%d]._i\n\
#define _dith%d %d\n#define _recurse _thread[%d]._i\n#define _newtonspace%d _thread[%d]._pvoid\nextern void* nrn_cons_newtonspace(int);\n\
", listnum, thread_data_index, listnum, thread_data_index+1, thread_data_index+2,
listnum, thread_data_index+3);
	vectorize_substitute(q, buf);
	Sprintf(buf, "  _thread[_dith%d]._pval = (double*)ecalloc(%d, sizeof(double));\n", listnum, 2*numeqn);
	lappendstr(thread_mem_init_list, buf);
	Sprintf(buf, "  _newtonspace%d = nrn_cons_newtonspace(%d);\n", listnum, numeqn);
	lappendstr(thread_mem_init_list, buf);
	Sprintf(buf, "  free((void*)(_thread[_dith%d]._pval));\n", listnum);
	lappendstr(thread_cleanup_list, buf);
	Sprintf(buf, "  nrn_destroy_newtonspace(_newtonspace%d);\n", listnum);
	lappendstr(thread_cleanup_list, buf);
	thread_data_index += 4;
}else{
	Strcpy(deriv1_advance, "");
	Strcpy(deriv2_advance, "");
}
Sprintf(buf,"%s %s%s(_ninits, %d, _slist%d, _dlist%d, _p, &%s, %s, %s, &_temp%d%s);\n%s",
deriv1_advance, ssprefix,
method->name, numeqn, listnum, listnum, indepsym->name, dindepname, fun->name, listnum, maxerr_str,
deriv2_advance);
	}else{
Sprintf(buf, "%s%s(&_sparseobj%d, %d, _slist%d, _dlist%d, _p, &%s, %s, %s\
,&_coef%d, _linmat%d);\n",
ssprefix, method->name, listnum, numeqn, listnum, listnum, indepsym->name,
dindepname, fun->name, listnum, listnum);
	}
	replacstr(qsol, buf);
#if VECTORIZE
	if (method->subtype & DERF) { /* derivimplicit */
Sprintf(buf,"%s %s%s_thread(%d, _slist%d, _dlist%d, _p, %s, _ppvar, _thread, _nt);\n%s",
deriv1_advance, ssprefix, method->name,
numeqn, listnum, listnum, fun->name,
deriv2_advance);
	vectorize_substitute(qsol, buf);
	}else{ /* kinetic */
   if (vectorize) {
Sprintf(buf, "%s%s_thread(&_thread[_spth%d]._pvoid, %d, _slist%d, _dlist%d, _p, &%s, %s, %s\
, _linmat%d, _ppvar, _thread, _nt);\n",
ssprefix, method->name, listnum, numeqn, listnum, listnum, indepsym->name,
dindepname, fun->name, listnum);
	vectorize_substitute(qsol, buf);
   }
#endif
	}
}

/* addition of higher order derivatives
 User appearance:
Higher order derivatives are now allowed in DERIVATIVE blocks.
The number of primes following a variable is the order of the derivative.
For example, y'''' is a 4th order derivative.
The highest derivative of a state
must appear on the left hand side of the equation. Lower order derivatives
of a state (including the state itself) may appear on the right hand
side within an arbitrary expression.  It makes no sense, in general,
to have multiple equations involving the same state on the left hand side.
The most common usage will be equations of the form
	y'' = f(y, y', t)

Higher derivatives can be accessed in SCoP as
y'   Dy
y''  D2y
y''' D3y
etc.  Note that all derivatives except the highest derivative are themselves
states on an equal footing with y. E.G. they can be used within
a MATCH block, they can be explicitly declared within a STATE block (and
given START values), and they have associated initial value constants.
Initial values default to 0. Here is a complicated example which
shows off the syntax (I have no idea if the solution exists).
	INDEPENDENT {t FROM 0 TO 1 WITH 1}
	STATE	{x y
		 y' START 1
		 x' START -10
		}
	DERIVATIVE d {
		x''' = y + 3*x''*y' + sin(x') + exp(x)
		y'' =  cos(x'' + y) - y'
		MATCH { y0  y(1)=1
			x(1)=1  Dx0 x''(1)=0  
		}
	EQUATION {SOLVE d}
Note that we had to use Dx0 since x'0 is illegal. Also Dx0 has a
value of -10.

Implementation :
	In parse1.y we see that asgn: varname '=' expr and that
	varname gets marked if it is type PRIME.  With respect to
	higher order derivatives, therefore, only the highest order
	actually used in the equations in that block
	should be marked.  Furthermore these highest primes may or may not be
	dependent variables and the lesser primes must be created as states.
	We use a special iterator FORALL which returns each lesser order
	symbol given a dstate.

	The implicit equations of the form  DD2y = D3y do not have to
	be constructed and the dependent variables DD2y do not have to
	be created since the slist and dlist links can carry this
	information. Thus dlist[D2y] == slist[D3y] causes the integrators
	to do the right thing without, in fact, doing any arithmetic.
	We assume that the itegrators either save the *dlist values or
	update *slist using a loop running from 0 to N.  This is why
	the FORALL interator returns states starting with the base state
	(so that *slist[i+1] doesn't change until after *dlist[i] is used.
	(they point to the same value). In the case of a second order
	equation, the lists are:
		slist[0] = &y		dlist[0] = &Dy
		slist[1] = &Dy		dlist[1] = &D2y

	With respect to the MATCH process
	the code which unmarks the PRIMES and marks the corresponding state
	is inadequate since several states must be marked some of which
	are PRIME. For this reason we distinguish using a -1 to mark states
	for later counting.
	
	The array problem is solved by using lex to:
	when a PRIME is seen, check against the base state. If the base
	state doesn't exist don't worry. If the STATE is an array
	Then make PRIME an array of the same dimension.
	
	depinstall automattically creates a Dstate for each state
	passed to it as well as a state0 (optional). This is OK since
	they dissappear if unused.

	
*/

#define	FORALL(state,dstate) \
	for (state = init_forderiv(dstate); state; state = next_forderiv())
/* This returns all states of lower order than dstate in the order of
base state first. Will install PRIME if necessary.
*/
static Symbol	*forderiv;	/* base state */
static char	base_units[50];	/*base state units */
static int	indx, maxindx;	/* current indx, and indx of dstate */

static Symbol * init_forderiv(prime)
	Symbol *prime;
{
	char name[100];
	double d1, d2;

	assert(prime->type == PRIME);
	/*extract maxindx and basename*/
	if (isdigit(prime->name[1])) { /* higher than 1 */
		if(sscanf(prime->name + 1, "%d%s", &maxindx, name) != 2) {
			diag("internal error in init_forderiv in init.c", (char *)0);
		}
	}else{
		maxindx = 1;
		Strcpy(name, prime->name + 1);
	}
	forderiv = lookup(name);
	if (!forderiv || !(forderiv->subtype & STAT)) {
		diag(name, " must be declared as a state variable");
	}
	if (forderiv->araydim != prime->araydim) {
		Sprintf(buf, "%s and %s have different dimensions",
			forderiv->name, prime->name);
		diag(buf, (char *)0);
	}
	indx = 0;
	decode_ustr(forderiv, &d1, &d2, base_units);
	return forderiv;
}

static char *name_forderiv(i)
	int i;
{
	static char name[100];
	
	assert(i > 0  && forderiv);
	if (i > 1) {
		Sprintf(name, "D%d%s", i, forderiv->name);
	}else{
		Sprintf(name, "D%s", forderiv->name);
	}
	return name;
}

/* Scop can handle 's so we put the prime style names into the .var file.
We make use of the tools here to reconstruct the original prime name.
*/
char *reprime(sym)
	Symbol *sym;
{
	static char name[100];
	int i;
	char *cp;
	
	if (sym->type != PRIME) {
		Strcpy(name, sym->name);
		return name;
	}

	IGNORE(init_forderiv(sym));
	
	Strcpy(name, forderiv->name);
	cp = name + strlen(name);
	for (i=0; i<maxindx; i++) {
		*cp++ = '\'';
	}
	*cp = '\0';
	return name;
}

static Symbol *next_forderiv()
{
	char *name;
	Symbol *s;
	char units[50];
	
	if (++indx >= maxindx) {
		return SYM0;
	}
	name = name_forderiv(indx);
	if((s = lookup(name)) == SYM0) {
		s = install(name, PRIME);
Sprintf(units, "%s/%s^%d", base_units, STR(indeplist->prev), indx);
		depinstall(1, s, forderiv->araydim, "0", "1", units, ITEM0, 1, "");
		s->usage |= DEP;
	}
	if (s->araydim != forderiv->araydim) {
		diag(s->name, " must have same dimension as associated state");
	}
	if (!(s->subtype & STAT)) {/* Dstate changes to state */
Sprintf(units, "%s/%s^%d", base_units, STR(indeplist->prev), indx);
		s->subtype &= ~DEP;
		depinstall(1, s, forderiv->araydim, "0", "1", units, ITEM0, 1, "");
		depinstall(1, s, forderiv->araydim, "0", "1", units, ITEM0, 1, "");
		s->usage |= DEP;
	}
	return s;
}

/* mixed derivative and nonlinear equations */
/* Implementation:
   The main requirement is to distinguish between the states which
   have derivative specifications and the states which are solved
   for by the nonlinear equations.  We do this by having the
   left hand side derivative saved in a list instead of marking the
   variables in the used field. See deriv_used().  States seen in the
   nonlinear equations are marked as usual.  To leave only nonlinear
   states marked we then cast out any lesser state which is marked.
   (eg if y''=.. then y and y' cannot be states of the nonlinear equation).
   The former version also made use of state->used = -1 for match
   purposes.  We replace this usage with a list of states of the
   derivatives.  This means that the derivative block with respect to
   derivative equations no longer uses the used field.
   
   To avoid copying the block we (albeit resulting is somewhat poorer
   efficiency) we allow the block to call newton and pass itself as
   an argument. A flag tells the block if its call was by newton or by
   an integrator. My guess is this will still work with match, sens, and
   array states.
*/

/* derivative equations (and possibly some nonlinear equations) solved
   with an implicit method */
/* Implementation:
   Things are starting to get a little bit out of conceptual control.
   We make use of the mixed case, except that the number of nonlinear
   equations may be 0.  The substantive change is that now the number
   of equations is the sum of the derivatives and the nonlinears and the
   extra equations added into the block are of the form
	dlist2[++_counte] = Dstate - (state - statesave1[0])/dt;
   The administrative needs are that newton is called with the total number
   of equations and that we can match state and statesave. Notice that we
   already have two sets of slists floating around and one dlist,
   currently they are the
   slist and dlist for the derivative state and state'
   and the slist for the nonlinear states (the corresponding dlist is just
   the rhs of the equations).  Clearly, statesave should be associated
   with the derivative slist and will be in that order, then the slist for
   newton will be expanded by not resetting the used field.
   The biggest conceptual problem is how to generate the code at the time
   we handle the SOLVE since the actual numbers for the declarations
   of the newton slists depend on the method.
   Here, we assume a flag, deriv_implicit, which tells us which code to
   generate. Whether this means that we must look through the .mod file
   for all the SOLVE statements or whether all this stuff is saved for
   calling from the solve handler as in the kinetic block is not specified
   yet.
   For now, we demand that the SOLVE statement be seen first if it
   invokes the derivimplicit method. Otherwise modl generates an error
   message.
*/

void add_deriv_imp_list(name)
	char *name;
{
	if (!deriv_imp_list) {
		deriv_imp_list = newlist();
	}
	Lappendstr(deriv_imp_list, name);
}

static List *deriv_used_list; /* left hand side derivatives of diffeqs */
static List *deriv_state_list;	/* states of the derivative equations */

void deriv_used(s, q1, q2)	/* q1, q2 are begin and end tokens for expression */
	Symbol *s;
	Item* q1, *q2;
{
	if (!deriv_used_list) {
		deriv_used_list = newlist();
		deriv_state_list = newlist();
	}
	Lappendsym(deriv_used_list, s);
#if CVODE
	if (!cvode_diffeq_list) {
		cvode_diffeq_list = newlist();
	}
	lappendsym(cvode_diffeq_list, s);
	lappenditem(cvode_diffeq_list, q1);
	lappenditem(cvode_diffeq_list, q2);
#endif
}
   
static int matchused = 0;	/* set when MATCH seen */
/* args are --- derivblk: DERIVATIVE NAME stmtlist '}' */
void massagederiv(q1, q2, q3, q4, sensused)
	Item *q1, *q2, *q3, *q4;
{
	int count = 0, deriv_implicit, solve_seen;
	char units[50];
	Item *qs, *q, *mixed_eqns();
	Symbol *s, *derfun, *state;

	/* to allow verification that definition after SOLVE */
	if (!massage_list_) { massage_list_ = newlist(); }
	Lappendsym(massage_list_, SYM(q2));
	
	/* all this junk is still in the intoken list */
	Sprintf(buf, "static int %s(_threadargsproto_);\n", SYM(q2)->name);
	Linsertstr(procfunc, buf);
	replacstr(q1, "\nstatic int"); q = insertstr(q3, "() {_reset=0;\n");
	derfun = SYM(q2);
	vectorize_substitute(q, "(double* _p, Datum* _ppvar, Datum* _thread, _NrnThread* _nt) {int _reset=0; int error = 0;\n");

	if (derfun->subtype & DERF && derfun->u.i) {
		diag("DERIVATIVE merging not implemented", (char *)0);
	}

	/* check if we are to translate using derivimplicit method */
	deriv_implicit = 0;
	if (deriv_imp_list) ITERATE(q, deriv_imp_list) {
		if (strcmp(derfun->name, STR(q)) == 0) {
			deriv_implicit = 1;
			break;
		}
	}
	numlist++;
	derfun->u.i = numlist;
	derfun->subtype |= DERF;
	if (!deriv_used_list) {
		diag("No derivative equations in DERIVATIVE block", (char*)0);
	}
	ITERATE(qs, deriv_used_list) {
		s = SYM(qs);
		if (!(s->subtype & DEP) && !(s->subtype & STAT)) {
IGNORE(init_forderiv(s));
Sprintf(units, "%s/%s^%d", base_units, STR(indeplist->prev), maxindx);
depinstall(0, s, s->araydim, "0", "1", units, ITEM0, 0, "");
		}
		/* high order: make sure
		   no lesser order is marked, and all lesser
		   orders exist as STAT */
		FORALL(state, s) {
			if (state->type == PRIME) {
			    ITERATE(q, deriv_used_list) if (state == SYM(q)) {
diag(state->name, ": Since higher derivative is being used, this state \
is not allowed on the left hand side.");
			    }
			}
			Lappendsym(deriv_state_list, state);
			if (sensused) {
				add_sens_statelist(state);
				state->varnum = count;
			}
#if CVODE
			slist_data(state, count, numlist);
#endif
if (s->subtype & ARRAY) { int dim = s->araydim;
	Sprintf(buf, "for(_i=0;_i<%d;_i++){_slist%d[%d+_i] = (%s + _i) - _p;"
		,dim, numlist , count, state->name);
	Lappendstr(initlist, buf);
	Sprintf(buf, " _dlist%d[%d+_i] = (%s + _i) - _p;}\n"
		, numlist, count, name_forderiv(indx + 1));
	Lappendstr(initlist, buf);
	count += dim;
}else{
			Sprintf(buf, "_slist%d[%d] = &(%s) - _p;",
				numlist, count, state->name);
			Lappendstr(initlist, buf);
			Sprintf(buf, " _dlist%d[%d] = &(%s) - _p;\n",
			   numlist, count, name_forderiv(indx + 1));
			Lappendstr(initlist, buf);
			count++;
}
		}
	}
	if (count == 0) {
		diag("DERIVATIVE contains no derivatives", (char *)0);
	}
	derfun->used = count;
Sprintf(buf, "static int _slist%d[%d], _dlist%d[%d];\n",
   numlist, count*(1 + 2*sens_parm), numlist, count*(1 + 2*sens_parm));
		Linsertstr(procfunc, buf);
	
#if CVODE
	Lappendstr(procfunc, "\n/*CVODE*/\n");
	Sprintf(buf, "static int _ode_spec%d", numlist);
	Lappendstr(procfunc, buf);
	{Item* qq = procfunc->prev;
	copyitems(q1->next, q4, procfunc->prev);
	vectorize_substitute(qq->next, "(double* _p, Datum* _ppvar, Datum* _thread, _NrnThread* _nt) {int _reset = 0;");
	vectorize_scan_for_func(qq->next, procfunc);
	}
	lappendstr(procfunc, "return _reset;\n}\n");

/* don't emit _ode_matsol if the user has defined cvodematsol */
  if (!lookup("cvodematsol")) {
	Item* qq;
	Item* qextra = q1->next->next->next->next;
	Sprintf(buf, "static int _ode_matsol%d", numlist);
	Lappendstr(procfunc, buf);
	vectorize_substitute(lappendstr(procfunc, "() {\n"), "(double* _p, Datum* _ppvar, Datum* _thread, _NrnThread* _nt) {\n");
	qq = procfunc->next;
	cvode_cnexp_possible = 1;
	ITERATE(q, cvode_diffeq_list) {
		Symbol* s; Item* q1, *q2;
		s = SYM(q); q = q->next; q1=ITM(q); q = q->next; q2 = ITM(q);
#if 1
		while (qextra != q1) { /* must first have any intervening statements */
			switch (qextra->itemtype) {
			case STRING:
				Lappendstr(procfunc, STR(qextra));
				break;
			case SYMBOL:
				Lappendsym(procfunc, SYM(qextra));
				break;
			}
			qextra = qextra->next;
		}
#endif
		cvode_diffeq(s, q1, q2);
		qextra = q2->next;
	}
	Lappendstr(procfunc, ";\n return 0;\n}\n");
	vectorize_scan_for_func(qq, procfunc);
  }

	Lappendstr(procfunc, "/*END CVODE*/\n");
	if (cvode_cnexp_solve && cvode_cnexp_success(q1, q4)) {
		freelist(&deriv_used_list);
		freelist(&deriv_state_list);
		return;
	}
#endif
	if (deriv_implicit) {
		Sprintf(buf, "static double _savstate%d[%d], *_temp%d = _savstate%d;\n",
			 numlist, count*(1 + 2*sens_parm), numlist, numlist);
		q = linsertstr(procfunc, buf);
		vectorize_substitute(q, "");
	}else{
		Sprintf(buf, "static double *_temp%d;\n", numlist);
		Linsertstr(procfunc, buf);
	}
	movelist(q1, q4, procfunc);
	Lappendstr(procfunc, "return _reset;}\n");
	if (sensused)
		sensmassage(DERIVATIVE, q2, numlist); /*among other things
			the name of q2 is changed. ie a new item */
	if (matchused) {
		matchmassage(count);
	}
	/* reset used field for any states that may appear in
	nonlinear equations which should not be solved for. */
	ITERATE(q, deriv_used_list) {
		SYM(q)->used = 0;
	}
	if (deriv_implicit) {
		Symbol *sp;
		ITERATE(q, deriv_state_list) {
			SYM(q)->used = 1;
		}
Sprintf(buf, "{int _id; for(_id=0; _id < %d; _id++) {\n\
if (_deriv%d_advance) {\n", count, numlist);
		Insertstr(q4, buf);
		sp = install("D", STRING);
		sp->araydim = count; /*this breaks SENS*/
		q = insertsym(q4, sp);
		eqnqueue(q);
Sprintf(buf,
"_p[_dlist%d[_id]] - (_p[_slist%d[_id]] - _savstate%d[_id])/d%s;\n",
   numlist, numlist, numlist, indepsym->name);
		Insertstr(q4, buf);
Sprintf(buf,
"}else{\n_dlist%d[++_counte] = _p[_slist%d[_id]] - _savstate%d[_id];}}}\n",
   numlist+1, numlist, numlist);
		Insertstr(q4, buf);
	}else{
		ITERATE(q, deriv_state_list) {
			SYM(q)->used = 0;
		}
	}
	/* if there are also nonlinear equations, put in the newton call,
	create the proper lists and fill in the left hand side of each
	equation. */
	q = mixed_eqns(q2, q3, q4); /* numlist now incremented */
	if (deriv_implicit) {
		Sprintf(buf,
"{int _id; for(_id=0; _id < %d; _id++) { _savstate%d[_id] = _p[_slist%d[_id]];}}\n",
		count, derfun->u.i, derfun->u.i);
		Insertstr(q, buf);
	}

	freelist(&deriv_used_list);
	freelist(&deriv_state_list);
}

static List	*match_init;	/* list of states for which initial values
					are known. */
List		*match_bound;	/* list of triples or quadruples.
				First is the state symbol.
				Second is a string giving the matchtime
				expression. Third is a string giving the
				matchtarget expression
				Fourth is the loop index string if the state
				is an array*/
				/* if non null then cout.c will put proper
				call at end of initmodel() */
				
/* note that the number of states in match_init plus the number of states
in match_bound must be equal to the number of differential equations */
/* we limit ourselves to one matched boundary problem per model */

void matchinitial(q1)	/* name */
	Item *q1;
{
	/* must be of form state0. Later we can check if state' is in fact
	used. Save the state symbol in the initialvalue matchlist */
	Symbol *s, *state;

	s = SYM(q1);
	if ((s->subtype & (PARM | INDEP)) || !(s->subtype)) {
		/* possibly used before declared */
		if (s->name[strlen(s->name) - 1] == '0') {
			Strcpy(buf, s->name);
			buf[strlen(buf) - 1] = '\0';
			state = lookup(buf);
			if((state && (state->subtype & STAT))
			   || (state->type == PRIME)) {
				Lappendsym(match_init, state);
				return;
			}
		}
	}
	diag(s->name, " must be an initial state parameter");
	return;
}	
			
void matchbound(q1, q2, q3, q4, q5, sindex) /* q1name q2'(' q3')' '=' q4exprq5 */
	Item *q1, *q2, *q3, *q4, *q5;
	Symbol *sindex;
{
	/* q1 must be a state */
	Symbol *state;
	Item *q;
	List *l;

	state = SYM(q1);
	if (!(state->subtype & STAT) && state->type != PRIME) {
		diag(state->name, " is not a state");
	}
	if ((state->subtype & ARRAY) && !sindex) {
		diag(state->name, " must have an index for the implicit loop");
	}
	if (!(state->subtype & ARRAY) && sindex) {
		diag(state->name, " is not an array");
	}

	Lappendsym(match_bound, state);

	q = lappendsym(match_bound, SYM0);
	l = newlist();
	movelist(q2, q3, l);
	LST(q) = l;

	q = lappendsym(match_bound, SYM0);
	l = newlist();
	movelist(q4, q5, l);
	LST(q) = l;
	
	if (sindex) {
		Lappendstr(match_bound, sindex->name);
	}
}

void checkmatch(blocktype) int blocktype; {
	if (blocktype != DERIVATIVE) {
		diag("MATCH block can only be in DERIVATIVE block", (char *)0);
	}
	matchused = 1; /*communicate with massagederiv*/
	if (match_bound || match_init) {
		diag("Only one MATCH block allowed", (char *)0);
	}
	if (!indepsym) {
		diag("INDEPENDENT variable must be declared before MATCH",
		 " statement");
	}
	match_bound = newlist();
	match_init = newlist();
}

void matchmassage(nderiv)
	int nderiv;
{
	int count, nunknown, j;
	Item *q, *q1, *setup;
	Symbol *s;
	List *tmatch, *vmatch;
	char *imatch;

	matchused = 0;
	/* we have a list of states at which the initial values are known
	and a list of information about state(time) = match with implicit loop
	index if array.
	
	check that the total number of conditions = nderiv.
	the number of unknown initial conditions is the complement of the
	states which have known initial conditions ( the number of states
	in the match_bound list. Note that the complement of match_init
	states is NOT the list of states in match_bound.
	
	We create
	  1) array of doubles which will receive the values of
	  the found initial conditions. (_found_init)
	  2) array of doubles which receives the match times
	  3) array of doubles which receives the match values
	  4) array of state pointers(state_match). we solve
	     *statematch(matchtime) = matchvalue
	  5) array of state pointers(state_get). We initialize with
	     *state_get = _found_init
	  6) since 2, 3, 4 must be sorted according to match times we
	     create pointer arrays to 2 and 3 and pass those.
	  7) Spec of shoot requires the passing of pointer array to
	     found_init.


	At this time I don't know how this can be restarted.
	
	Initmodel() calls _initmatch() which
	1) if first=1 then sets first=0
	sets up match times, match values, initializes
	_found_init, calls shoot(), calls initmodel(),
	sets first = 1 and exits.
	2) if first=0 then put _found_init into states and exit.
	
	Call to _initmatch must be the last thing done in initmodel() in
	case the user desires to give good starting initial values to
	help shoot().
	*/

	count = 0;
	
	ITERATE(q, match_init) { /* these initializations are done
		automatically by initmodel(), so just remove the state
		from the deriv_state_list */
		s = SYM(q);
		ITERATE(q1, deriv_state_list) {
			if (SYM(q1) == s) {
				delete(q1);
				break;
			}
		}
		if (!(s->subtype & STAT)) {
			diag(s->name, " is not a state");
		}
		if (s->subtype & ARRAY) {
			count += s->araydim;
		}else{
			count++;
		}
	}
	nunknown = nderiv - count;
	if (nunknown <= 0) {
		diag("Nothing to match", (char *)0);
	}
	/* the ones that are still marked are the ones to solve for */
	
	/* add the boilerplate for _initmatch and save the location
	where model specific info goes.
	*/
Lappendstr(procfunc, "\n_init_match(_save) double _save;{ int _i;\nif (_match_recurse) {_match_recurse = 0;\n");
Sprintf(buf, "for (_i=0; _i<%d; _i++) _found_init[_i] = _p[_state_get[_i]];\n",
		nunknown);
	setup = lappendstr(procfunc, buf);
Sprintf(buf, "error=shoot(%d, &(%s) - _p, _pmatch_time, _pmatch_value, _state_match,\
 _found_init, _p, &(d%s));\n if(error){abort_run(error);}; %s = _save;",
		nunknown, indepsym->name, indepsym->name, indepsym->name);
		/*deltaindep may not be declared yet */
	Lappendstr(procfunc, buf);
	Lappendstr(procfunc,"\n initmodel(_p); _match_recurse = 1;\n}\n");
	Sprintf(buf, "for (_i=0; _i<%d; _i++) _p[_state_get[_i]] = _found_init[_i];",
		nunknown);
	Lappendstr(procfunc, buf);
	Lappendstr(procfunc, "\n}\n\n");

	/* construct _state_get from the marked states */
	j = 0;

	ITERATE(q, deriv_state_list) {
		s = SYM(q);
		if (s->subtype & ARRAY) {
Sprintf(buf, "for (_i=0; _i<%d; _i++) {_state_get[%d+_i] = (%s + _i) - _p;}\n",
	s->araydim, j, s->name);
			j += s->araydim;
		} else {
			Sprintf(buf, "_state_get[%d] = &(%s) - _p;\n", j, s->name);
			j++;
		}
		Lappendstr(initlist, buf);
	}
	/* declare the arrays */
	Sprintf(buf, "static int _state_get[%d], _state_match[%d];\n\
static double _match_time[%d], _match_value[%d], _found_init[%d];\n",
		nunknown, nunknown, nunknown, nunknown, nunknown);
	Linsertstr(procfunc, buf);
	Sprintf(buf, "static double *_pmatch_time[%d], *_pmatch_value[%d];\n",
 		nunknown, nunknown);
	Linsertstr(procfunc, buf);
	
	/* create the _state_match stuff */
	j = 0;
	ITERATE(q, match_bound) {
		s = SYM(q);
		if (!(s->subtype & STAT)) {
			diag(s->name, " is not a state");
		}
		tmatch = LST(q = q->next);
		vmatch = LST(q = q->next);
		if (s->subtype & ARRAY) {
			imatch = STR(q = q->next);
Sprintf(buf, "for (_i=0; _i<%d; _i++) {_state_match[%d+_i] = (%s + _i) - _p;}\n",
			s->araydim, j, s->name);
			Lappendstr(initlist, buf);
Sprintf(buf, "{int %s; for (%s=0; %s<%d; %s++) {\n",
imatch, imatch, imatch, s->araydim, imatch);
			Insertstr(setup, buf);
Sprintf(buf, "_match_time[%s + %d] = ", imatch, j);
			Insertstr(setup, buf);
			copylist(tmatch, setup);
Sprintf(buf, ";\n _match_value[%s + %d] = ", imatch, j);
			Insertstr(setup, buf);
			copylist(vmatch, setup);
			Insertstr(setup, ";\n}}\n");
			j += s->araydim;
			count += s->araydim;
		}else{
Sprintf(buf, "_state_match[%d] = &(%s) - _p;\n", j, s->name);
			Lappendstr(initlist, buf);
Sprintf(buf, "_match_time[%d] = ", j);
			Insertstr(setup, buf);
			copylist(tmatch, setup);
Sprintf(buf, ";\n _match_value[%d] = ", j);
			Insertstr(setup, buf);
			copylist(vmatch, setup);
			Insertstr(setup, ";\n");
			j++;
			count++;
		}
	}
	/* set up the trivial pointer arrays */
	Sprintf(buf, "for(_i=0; _i<%d; _i++) { _pmatch_time[_i] = _match_time + _i;\n",
		nunknown);
	Lappendstr(initlist, buf);
	Lappendstr(initlist, "_pmatch_value[_i] = _match_value + _i;\n }\n");
	if (count != nderiv) {
		Sprintf(buf, "%d equations != %d MATCH specs", nderiv, count);
		diag(buf, (char *)0);
	}
			
}
	
	
static void copylist(l, i)	/* copy list l before item i */
	List *l;
	Item *i;
{
	Item *q;
	
	ITERATE(q, l) {
		switch(q->itemtype) {

		case STRING:
			Insertstr(i, STR(q));
			break;
		case SYMBOL:
			Insertsym(i, SYM(q));
			break;
		default:
			/*SUPPRESS 622*/
			assert(0);
		}
	}
}

void copyitems(q1, q2, qdest) /* copy items before item */
	Item* q1, *q2, *qdest;
{
	Item* q;
	for (q = q2; q != q1; q = q->prev) {
		switch(q->itemtype) {

		case STRING:
			Linsertstr(qdest, STR(q));
			break;
		case SYMBOL:
			Linsertsym(qdest, SYM(q));
			break;
		default:
			/*SUPPRESS 622*/
			assert(0);
		}
	}
}

#if CVODE
static int cvode_linear_diffeq(ds, s, qbegin, qend)
Symbol*ds, *s; Item* qbegin, *qend; {
	char* c;
	List* tlst;
	Item* q;
	tlst = newlist();
	for (q = qbegin; q != qend->next; q = q->next) {
		switch (q->itemtype) {
		case SYMBOL:
			lappendsym(tlst, SYM(q));
			break;
		case STRING:
			lappendstr(tlst, STR(q));
			break;
		default:
			cvode_cnexp_possible = 0;
			return 0;
		}
	}
	cvode_parse(s, tlst);
	freelist(&tlst);
	c = cvode_deriv();
	if (!cvode_eqn) {
		cvode_eqn = newlist();
	}
	lappendsym(cvode_eqn, s);
	lappendstr(cvode_eqn, cvode_deriv());
	lappendstr(cvode_eqn, cvode_eqnrhs());
	if (c) {
		lappendstr(procfunc, c);
		lappendstr(procfunc, "))");
		return 1;
	}
	cvode_cnexp_possible = 0;
	return 0;
}

/* DState symbol, begin, and end of expression */
static void cvode_diffeq(ds, qbegin, qend) Symbol* ds; Item* qbegin, *qend; {
	/* try first the assumption of linear. If not, then use numerical diff*/
	Symbol* s;
	Item* q;

	/* get state symbol */
	sscanf(ds->name, "D%s", buf);
	s = lookup(buf);
	assert(s);

	/* ds/(1. - dt*( */
	Lappendsym(procfunc, ds);
	Lappendstr(procfunc, " / (1. - dt*(");
	if (cvode_linear_diffeq(ds, s, qbegin, qend)) {
		return;
	}
	/* ((expr(s+.001))-(expr))/.001; */
	Lappendstr(procfunc, "((");
	for (q = qbegin; q != qend->next; q = q->next) {
		switch(q->itemtype) {
		case STRING:
			Lappendstr(procfunc, STR(q));
			break;
		case SYMBOL:
			if (SYM(q) == s) {
				Lappendstr(procfunc, "(");
				Lappendsym(procfunc, s);
				Lappendstr(procfunc, " + .001)");
			}else{
				Lappendsym(procfunc, SYM(q));
			}
			break;
		default:
			assert(0);
		}
	}
	Lappendstr(procfunc, ") - (");
	for (q = qbegin; q != qend->next; q = q->next) {
		switch(q->itemtype) {
		case STRING:
			Lappendstr(procfunc, STR(q));
			break;
		case SYMBOL:
			Lappendsym(procfunc, SYM(q));
			break;
		default:
			assert(0);
		}
	}
	Lappendstr(procfunc, " )) / .001 ))");
}

/*
the cnexp method was requested but the symbol in the solve queue was
changed to derivimplicit and cvode_cnexp_solve holds a pointer to 
the solvq item (1st of three).
The 0=f(state) equations have already been solved and the rhs for
each has been saved. So we know if the translation is possible.
*/
int cvode_cnexp_success(q1, q2) Item* q1, *q2; {
	Item* q, *q3, *q4, *qeq;
	if ( cvode_cnexp_possible) {
		/* convert Method to nil and the type of the block to
		   PROCEDURE */
		SYM(cvode_cnexp_solve->next)->name = stralloc("cnexp", 0);
		delete(deriv_imp_list->next);

		/* replace the Dstate = f(state) equations */
		qeq = cvode_eqn->next;
		ITERATE(q, cvode_diffeq_list) {
			Symbol* s; Item* q1, *q2; char* a, *b;
			s = SYM(qeq); qeq = qeq->next; a = STR(qeq);
			qeq = qeq->next; b = STR(qeq); qeq = qeq->next;
			q = q->next; q1=ITM(q); q = q->next; q2 = ITM(q);
			if (strcmp(a, "0.0") == 0) {
				assert(b[strlen(b) - 9] == '/');
				b[strlen(b) - 9] = '\0';
sprintf(buf," %s = %s - dt*(%s)", s->name, s->name, b);
			}else{
sprintf(buf," %s = %s + (1. - exp(dt*(%s)))*(%s - %s)",
					s->name, s->name,
					a, b,
					s->name
			);
			}
			insertstr(q2->next, buf);
			q2 = q2->next;
			for(q3=q1->prev->prev; q3 != q2; q3 = q4) {
				q4 = q3->next;
				delete(q3);
			}
		}
		
		lappendstr(procfunc, "static int");
		{ Item* qq = procfunc->prev;
		copyitems(q1, q2, procfunc->prev);
		/* more or less redundant with massagederiv */
		vectorize_substitute(qq->next->next, "(double* _p, Datum* _ppvar, Datum* _thread, _NrnThread* _nt) {");
		vectorize_scan_for_func(qq->next->next, procfunc);
		}
		lappendstr(procfunc, " return 0;\n}\n");
		return 1;
	}
	fprintf(stderr, "Could not translate using cnexp method; using derivimplicit\n");
	return 0;
}
#endif
