#include <../../nrnconf.h>
#include <stdlib.h>
#include <hocdec.h>
#include "lineq.h"

#define diag(s) hoc_execerror(s, (char*)0);

struct elm* getelm(struct elm* el, unsigned row, unsigned col)
   /* return pointer to row col element maintaining order in rows */
{
	register struct elm *new;

	if (el == ELM0)
		el = rowst[row];
	/*EMPTY*/
	if (el == ELM0);
	else if (el->col > col)
		el = ELM0;
	else
	{
		while ( el->c_right != ELM0 && el->c_right->col <= col)
			el = el->c_right;
		if (el->col == col)
			return(el);
	}

	if ( (new = (struct elm *)malloc(sizeof(struct elm))) ==
		(struct elm *)0)
		diag("out of space for elements");
	new->row = row;
	new->col = col;
	new->value = 0.;
	{
	new->r_up = ELM0;	/* place new element first in column list */
	new->r_down = colst[col];
	if (colst[col] != ELM0)
		colst[col]->r_up = new;
	colst[col] = new;
	}
	if (el == ELM0)		/* the new elm belongs at the beginning */
	{			/* of the row list */
		new->c_left = ELM0;
		new->c_right = rowst[row];
		if (rowst[row] != ELM0)
			rowst[row]->c_left = new;
		rowst[row] = new;
	}
	else			/* the new elm belongs after el */
	{
		new->c_left = el;
		new->c_right = el->c_right;
		el->c_right = new;
		if (new->c_right != ELM0)
			new->c_right->c_left = new;
	}
	return(new);
}
