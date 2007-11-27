#include <../../nrnconf.h>
/* /local/src/master/nrn/src/oc/nonlin.c,v 1.4 1997/11/24 16:21:38 hines Exp */
/*
nonlin.c,v
 * Revision 1.4  1997/11/24  16:21:38  hines
 * minor mac port fixes for new Codewarror pro 2
 *
 * Revision 1.3  1996/02/16  16:19:31  hines
 * OCSMALL used to throw out things not needed by teaching programs
 *
 * Revision 1.2  1994/10/26  17:25:04  hines
 * access name changed to an explicit hoc_access and taken out of redef.h
 *
 * Revision 1.1.1.1  1994/10/12  17:22:12  hines
 * NEURON 3.0 distribution
 *
 * Revision 1.4  92/08/18  07:31:42  hines
 * arrays in different objects can have different sizes.
 * Now one uses araypt(symbol, SYMBOL) or araypt(symbol, OBJECTVAR) to
 * return index of an array variable.
 * 
 * Revision 1.3  91/10/18  14:40:45  hines
 * symbol tables now are type Symlist containing pointers to first and last
 * symbols.  New symbols get added onto the end.
 * 
 * Revision 1.2  91/10/17  15:01:35  hines
 * VAR, STRING now handled with pointer to value located in object data space
 * to allow future multiple objects. Ie symbol for var, string, objectvar
 * has offset into a pointer data space.
 * 
 * Revision 1.1  91/10/11  11:12:14  hines
 * Initial revision
 * 
 * Revision 4.34  91/08/08  16:39:20  hines
 * saber lint free. Fixed problem when neuron uses s_varn for something
 * other than dependent variables by demanding that all depvar variables
 * have a 0 subtype (created by the user in the interpreter).
 * 
 * Revision 4.15  91/03/07  08:30:31  hines
 * re allocation of varnum for arrays was not initialized to 0
 * 
 * Revision 3.46  90/01/24  06:37:59  mlh
 * emalloc() and ecalloc() are macros which return null if out of space
 * and then call execerror.  This ensures that pointers are set to null.
 * If more cleanup necessary then use hoc_Emalloc() followed by hoc_malchk()
 * 
 * Revision 3.28  89/09/29  16:04:00  mlh
 * need to use -1 as falg for unsigned variable
 * 
 * Revision 3.23  89/09/14  14:34:13  mlh
 * for turboc 2.0 void the signal and malloc, and remove calloc
 * 
 * Revision 3.20  89/08/15  08:29:54  mlh
 * compiles under turbo-c 1.5 -- some significant bugs found
 * 
 * Revision 3.7  89/07/13  08:21:41  mlh
 * stack functions involve specific types instead of Datum
 * 
 * Revision 3.4  89/07/12  10:27:57  mlh
 * Lint free
 * 
 * Revision 3.3  89/07/10  15:46:56  mlh
 * Lint pass1 is silent. Inst structure changed to union.
 * 
 * Revision 2.0  89/07/07  11:32:44  mlh
 * Preparation for newcable
 * 
 * Revision 1.1  89/07/07  11:17:22  mlh
 * Initial revision
 * 
*/

/*version 7.2.1 2-jan-89 */
#include <math.h>
# include	"hoc.h"
# include	"parse.h"
# include	"equation.h"
# include	"lineq.h"

extern Symlist	*symlist;
int	do_equation;	/* switch for determining access to dep vars */
int	*hoc_access;	/* links to next accessed variables */
int  var_access;	/* variable number as pointer into access array */
static double	**varble;	/* pointer to symbol values */
typedef struct elm *Elm;

dep_make()	/* tag the variable as dependent with a variable number */
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


init_access()	/* zero the access array */
{
#if !OCSMALL
	if (hoc_access != (int *)0)
		free((char *)hoc_access);
	hoc_access = (int *)ecalloc((neqn+1), sizeof(int));
	var_access = -1;
#endif
}

static unsigned	row;
static unsigned	maxeqn;

eqn_name()	/* save row number for lhs and/or rhs */
{
#if !OCSMALL
	int	set_varble();	/* found in symbol.c */
	int	eqn_space();

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

set_varble()	/* set up varble array by searching for tags */
{
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

eqinit()	/* built in function to initialize equation solver */
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

eqn_init()	/* initialize equation row */
{
#if !OCSMALL
	struct elm *el;

	for (el = rowst[row]; el != (struct elm *)0; el = el->c_right)
		el->value = 0.;
	rhs[row] = 0.;
#endif
}

eqn_lhs()	/* add terms to left hand side */
{
	eqn_side(1);
}

eqn_rhs()	/* add terms to right hand side */
{
	eqn_side(0);
}



eqn_side(lhs)
	int lhs;
{
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

eqn_space()	/* reallocate space for matrix */
{
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

diag(s)	/* hold over from focsim */
	char *s;
{
	execerror(s, (char *)0);
}

Prmat()
{
#if !OCSMALL
	prmat();
#endif
	ret();
	pushx(1.);
}

solve()
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
