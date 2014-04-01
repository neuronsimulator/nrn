#include <../../nrnconf.h>

#include <stdlib.h>
#include "lineq.h"
	
void subrow(struct elm *pivot, struct elm*rowsub) {
	unsigned row;
	double r;
	register struct elm *el;

	r = rowsub->value / pivot->value;
	rhs[rowsub->row] -= rhs[pivot->row] * r;
	row = rowsub->row;
	rowsub = ELM0;
	for (el = rowst[pivot->row] ; el != ELM0 ; el = el->c_right)
		if (el != pivot)
			(rowsub = getelm(rowsub, row, el->col))->value
			 -= el->value * r;
}

void remelm(struct elm *el) {
	if (el->c_right != ELM0)
		(el->c_right)->c_left = el->c_left;
	if (el->c_left != ELM0)
		(el->c_left)->c_right = el->c_right;
	else
		rowst[el->row] = el->c_right;

	if (el->r_down != ELM0)
		(el->r_down)->r_up = el->r_up;
	if (el->r_up != ELM0)
		(el->r_up)->r_down = el->r_down;
	else
		colst[el->col] = el->r_down;
	free((char *)el);
}
