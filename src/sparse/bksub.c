#include <../../nrnconf.h>

#include "lineq.h"

void bksub(void)
{
	int i;
	struct elm *el, *pivot;

	for (i = neqn ; i >= 1 ; i--)
	{
		for (el = rowst[eqord[i]] ; el != NULL ; el = el->c_right)
		{
			if (el->col == varord[i])
				pivot = el;
			else
				rhs[el->row] -= el->value * rhs[el->col];
		}
		rhs[eqord[i]] /= pivot->value;
	}
}
