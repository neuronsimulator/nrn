#include <../../nrnconf.h>
#include <stdlib.h>
#include <hocdec.h>
#include "lineq.h"

#define diag(s) hoc_execerror(s, (char*)0);

struct elm* getelm(struct elm* el, unsigned row, unsigned col)
   /* return pointer to row col element maintaining order in rows */
{
	struct elm *new_elem;

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

	if ( (new_elem = (struct elm *)malloc(sizeof(struct elm))) ==
		(struct elm *)0)
		diag("out of space for elements");
    new_elem->row = row;
    new_elem->col = col;
    new_elem->value = 0.;
	{
        new_elem->r_up = ELM0;	/* place new element first in column list */
        new_elem->r_down = colst[col];
        if (colst[col] != ELM0)
            colst[col]->r_up = new_elem;
        colst[col] = new_elem;
	}
	if (el == ELM0)		/* the new elm belongs at the beginning */
	{			/* of the row list */
        new_elem->c_left = ELM0;
        new_elem->c_right = rowst[row];
		if (rowst[row] != ELM0)
			rowst[row]->c_left = new_elem;
		rowst[row] = new_elem;
	}
	else			/* the new elm belongs after el */
	{
        new_elem->c_left = el;
        new_elem->c_right = el->c_right;
		el->c_right = new_elem;
		if (new_elem->c_right != ELM0)
            new_elem->c_right->c_left = new_elem;
	}
	return(new_elem);
}
