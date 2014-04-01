#include <../../nrnconf.h>
#include <stdlib.h>
#include <math.h>
#include "hoc.h"
#include "parse.h"
#include "hocparse.h"
#include "equation.h"
#include "lineq.h"
#include "code.h"

int	do_equation;	/* switch for determining access to dep vars */
int	*hoc_access;	/* links to next accessed variables */
int  var_access;	/* variable number as pointer into access array */
static double	**varble;	/* pointer to symbol values */
typedef struct elm *Elm;

#define diag(s) hoc_execerror(s, (char*)0);

void dep_make(void)/* tag the variable as dependent with a variable number */
{
#if !OCSMALL
	Symbol *sym;
	unsigned	*numpt;
#if	defined(__TURBOC__)
	Inst *pcsav=pc;
#endif

	sym = spop();
#if	defined(__TURBOC__)
	pc = pcsav;
#endif
	switch (sym->type)
	{

	case UNDEF:
		hoc_execerror(sym->name, "undefined in dep_make");
		sym->type = VAR;
		OPVAL(sym) = (double *)emalloc(sizeof(double));
		*(OPVAL(sym)) = 0.;
	case VAR:
		if (sym->subtype != NOTUSER) {
			execerror(sym->name, "can't be a dependent variable");
		}
if (!ISARRAY(sym)) {
			numpt = &(sym->s_varn);
}else{
		Arrayinfo *aray = OPARINFO(sym);
		if (sym->s_varn == 0)	/* allocate varnum array */
		{
			int total = 1;
			int i;
			for (i=0; i < aray->nsub; i++)
				total *= (aray->sub)[i];
aray->a_varn = (unsigned *)ecalloc((unsigned)total, sizeof(unsigned));
			sym->s_varn = (unsigned)total;	/* set_varble() uses this */
		}
		numpt = &((aray->a_varn)[araypt(sym, OBJECTVAR)]);
}
		break;

	default:
		execerror(sym->name, "can't be a dependent variable");
	}
	if (*numpt > 0)
		execerror(sym->name, "made dependent twice");
	*numpt = ++neqn;
#endif
}


void init_access(void) /* zero the access array */
{
#if !OCSMALL
	if (hoc_access != (int *)0)
		free((char *)hoc_access);
	hoc_access = (int *)ecalloc((neqn+1), sizeof(int));
	var_access = -1;
#endif
}

static void eqn_space(void);	/* reallocate space for matrix */
static void set_varble(void);	/* set up varble array by searching for tags */
static void eqn_side(int lhs);
static unsigned	row;
static unsigned	maxeqn;

void eqn_name(void) /* save row number for lhs and/or rhs */
{
#if !OCSMALL

	if (maxeqn != neqn)	/* discard equations and reallocate space */
	{
		eqn_space();
		set_varble();
	}
	init_access();
	do_equation = 1;
	eval();
	do_equation = 0;
	if (var_access < 1)
		execerror("illegal equation name",(pc - 2)->sym->name);
	row = var_access;
	nopop();
#endif
}

static void set_varble(void) {	/* set up varble array by searching for tags */
#if !OCSMALL
	Symbol	*sp;

	for (sp = symlist->first; sp != (Symbol *)0; sp = sp->next)
	{
		if (sp->s_varn != 0)
		{
			if (sp->type == VAR) {			
				if (!ISARRAY(sp)) {
					varble[sp->s_varn] = OPVAL(sp);
				}else{
					int i;
					Arrayinfo	*aray = OPARINFO(sp);
					for (i = 0; i < sp->s_varn; i++)
						if ((aray->a_varn)[i] > 0)
varble[(aray->a_varn)[i]] = OPVAL(sp) + i;
				}
			}
		}
	}
#endif
}

static double Delta = .001;	/* variable variation */

void eqinit(void)	/* built in function to initialize equation solver */
{
#if !OCSMALL
	Symbol	*sp;

	if (ifarg(1))
		Delta = *getarg(1);
	for (sp = symlist->first; sp != (Symbol *)0; sp = sp->next)
	{
		if (sp->s_varn != 0)
		{
			if (ISARRAY(sp)) {
				if (OPARINFO(sp)->a_varn != (unsigned *)0)
					free((char *)(OPARINFO(sp)->a_varn));
			}
			sp->s_varn = 0;
		}
	}
	neqn = 0;
	eqn_space();
#endif
	ret();
	pushx(0.);
}

void eqn_init(void)	/* initialize equation row */
{
#if !OCSMALL
	struct elm *el;

	for (el = rowst[row]; el != (struct elm *)0; el = el->c_right)
		el->value = 0.;
	rhs[row] = 0.;
#endif
}

void eqn_lhs(void)	/* add terms to left hand side */
{
	eqn_side(1);
}

void eqn_rhs(void)	/* add terms to right hand side */
{
	eqn_side(0);
}



static void eqn_side(int lhs) {
#if !OCSMALL
	int i;
	struct elm *el;
	double f0, f1;
	Inst *savepc = pc;
#if	defined(__TURBOC__)
	Inst *pc1;
#endif

	init_access();
	do_equation = 1;
	execute(savepc);
#if	defined(__TURBOC__)
	pc1 = pc;
#endif
	do_equation = 0;

	if (lhs)
	{
		f0 = xpop();
	}
	else
	{
		f0 = -xpop();
	}

	rhs[row] -= f0;
	for (i = var_access; i > 0; i = hoc_access[i])
	{
		*varble[i] += Delta;
		execute(savepc);
		*varble[i] -= Delta;
		if (lhs)
		{
			f1 = xpop();
		}
		else
		{
			f1 = -xpop();
		}
		el = getelm((struct elm *)0, row, (unsigned)i);
		el->value += (f1 - f0)/Delta;
	}
#if	defined(__TURBOC__)
	pc=pc1;
#endif
	pc++;
#endif
}

static void eqn_space(void) {	/* reallocate space for matrix */
#if !OCSMALL
	register int i;
	register struct elm *el;

	if (maxeqn > 0 && rowst == (Elm *)0)
		diag("matrix coefficients cannot be released");
	for(i=1 ; i <= maxeqn ; i++)
		for(el = rowst[i] ; el; el = el->c_right)
			free((char *)el);
	maxeqn = neqn;
	if (varble)
		free((char *)varble);
	if (rowst)
		free((char *)rowst);
	if (colst)
		free((char *)colst);
	if (eqord)
		free((char *)eqord);
	if (varord)
		free((char *)varord);
	if (rhs)
		free((char *)rhs);
	varble = (double **)0;
	rowst = colst = (Elm *)0;
	eqord = varord = (unsigned *)0;
	rhs = (double *)0;
	rowst = (Elm *)ecalloc((maxeqn+1),sizeof(struct elm *));
	varble = (double **)emalloc((maxeqn+1)*sizeof(double *));
	colst = (Elm *)ecalloc((maxeqn+1),sizeof(struct elm *));
	eqord = (unsigned *)emalloc((maxeqn+1)*sizeof(unsigned));
	varord = (unsigned *)emalloc((maxeqn+1)*sizeof(unsigned));
	rhs = (double *)emalloc((maxeqn+1)*sizeof(double));
	for (i=1 ; i<= maxeqn ; i++)
	{
		eqord[i] = i;
		varord[i] = i;
	}
#endif
}

void hoc_Prmat(void)
{
#if !OCSMALL
	prmat();
#endif
	ret();
	pushx(1.);
}

void solve(void)
{
#if !OCSMALL
	/* Sum is a measure of the dependent variable accuracy
	   and how well the equations are solved */

	register int i;
	double sum;
	struct elm	*el;

	sum = 0.;
	for (i=1 ; i <= neqn ; i++)
		sum += fabs(rhs[i]);
	if (!matsol())
		diag("indeterminate system");
	for (i=1 ; i<= neqn ; i++)
	{
		*varble[varord[i]] += rhs[eqord[i]];
		sum += fabs(rhs[i]);
	}
	/* free all elements */
	for (i=1; i <= neqn; i++)
	{
		struct elm *el2;
		for (el = rowst[i]; el != (struct elm *)0; el = el2) {
			el2 = el->c_right;
			free((char*)el);
		}
		rowst[i] = colst[i] = (struct elm *)0;
	}
#else
	double sum = 0;
#endif
	ret();
	pushx(sum);
}
