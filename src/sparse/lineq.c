#include <../../nrnconf.h>
/*LINTLIBRARY*/
#include "lineq.h"
#include <math.h>

struct elm **rowst;		/* link to first element in row */
struct elm **colst;		/* link to a column element */
unsigned neqn;			/* number of equations */
unsigned *eqord;			/* row order for pivots */
unsigned *varord;			/* column order for pivots */
double *rhs;			/* initially- right hand side
					finally - answer */
#define SMALL 0.

int matsol(void)
{
	register struct elm *pivot;
	register struct elm *el;
	struct elm *hold;
	int i, j;
	double max;

	/* Upper triangularization */
	for (i=1 ; i <= neqn ; i++)
	{
		if (fabs((pivot = getelm(ELM0, eqord[i], varord[i]))->value) <= SMALL)
		{
			/* use max row element as pivot */
			remelm(pivot);
			max = SMALL;
			pivot = ELM0;
			for (el = rowst[eqord[i]] ; el != ELM0 ;
			   el = el->c_right)
				if (fabs(el->value) > max)
					max = fabs((pivot = el)->value);
			if (pivot == ELM0)
				return(0);
			else
			{
				for (j = i; j<= neqn ; j++)
					if (varord[j] == pivot->col)
						break;
				varord[j] = varord[i];
				varord[i] = pivot->col;
			}
		}
		/* Eliminate all elements in pivot column */
		for (el = colst[pivot->col] ; el != ELM0 ; el = hold)
		{
			hold = el->r_down;	/* el will be freed below */
			if (el != pivot)
			{
				subrow(pivot, el);
				remelm(el);
			}
		}
		/* Remove pivot row from further upper triangle work */
		for (el = rowst[pivot->row] ; el != ELM0 ; el = el->c_right)
		{
			if (el->r_up != ELM0)
				el->r_up->r_down = el->r_down;
			else 
				colst[el->col] = el->r_down;
			if (el->r_down != ELM0)
				el->r_down->r_up = el->r_up;
		}
	}
	bksub();
	return(1);
}
