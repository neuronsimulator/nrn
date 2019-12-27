#include <../../nmodlconf.h>
/* /local/src/master/nrn/src/modlunit/units1.c,v 1.1.1.1 1994/10/12 17:22:51 hines Exp */
/* Just a connection to units.c so that file doesn't need to include modl.h */
#include "model.h"
#include "parse1.h"

void unit_push(q)
	Item *q;
{
	Unit_push(decode_units(SYM(q)));
}

char *
decode_units(sym)
	Symbol *sym;
{
	if (sym->u.str) {
		return sym->u.str;
	}
	if (sym->info && ITMA(sym->info)[1]) {
		return STR(ITMA(sym->info)[1]);
	}
	return "";
}

void ifcnvfac(q3) /* '(' expr ')' */
	Item *q3;
{
	Item *q1, *q2;
	double d;

	q2 = prev_parstok(q3);
	q1 = prev_parstok(q2);
	if (q1->itemsubtype == '(') {
		if(q2->itemsubtype == REAL || q2->itemsubtype == INTEGER) {
			unit_pop();
			sscanf(STR(q2), "%lf", &d);
			unit_push_num(1./d);
		}else if (q2->itemsubtype == DEFINEDVAR) {
			unit_pop();
			d = (double) (SYM(q2)->u.i);
			unit_push_num(1./d);
		}
	}
}
	
/* x^y  y must be dimensionless. If x is dimensionless then
y can be any expression. If x has dimensions then y must be a positive integer
and the units can be computed */

void unit_exponent(y, lastok) /*x ^ y*/
	Item *y, *lastok;
{
	int i;
	double yval;
	
	unit_less();
	if ( y == lastok && y->itemsubtype == INTEGER) {
		i = sscanf(STR(y), "%lf", &yval);
		assert(i == 1);
		if (yval - (double)((int)yval)) {
			unit_less();
			Unit_push("");
		} else {
			Unit_exponent((int)yval);
		}
	}else{
		unit_less();
		Unit_push("");
	}
}

static Item *qexpr[3];
void unit_cmp(q1, q2, q3)
	Item *q1, *q2, *q3;
{
	qexpr[0] = q1;
	qexpr[1] = q2;
	qexpr[2] = q3;
	Unit_cmp();
}

void print_unit_expr(i)
	int i;
{
	if (i==1) {
		if (qexpr[0]) {
			printitems(qexpr[0], qexpr[1]->prev);
		}
	}else{
		if (qexpr[1]) {
			printitems(qexpr[1]->next, qexpr[2]);
		}
	}
}
	
	
void unit_logic(type, q1, q2, q3)
	int type;
	Item *q1, *q2, *q3;
{
	/* if type is 1 then it doesn't matter what the
	top two elements are: the result is dimensionless.
	If the type is 2 then the top two elements must have
	the same dimensions and the result is dimensionless.
	*/
	
	if (type == 2) {
		unit_cmp(q1, q2, q3);
	}else{
		unit_pop();
	}
	unit_pop();
	Unit_push("");
}

#define NLEVEL	10 /* 10 levels of call! */
static int argnumstk[NLEVEL], pargnum = -1;

void unit_push_args(q1)
	Item *q1;
{
	List *larg;
	Item *q;
	Symbol *s;
	
	pargnum++;
	assert(pargnum < NLEVEL);
	
	s = SYM(q1);

	/* push the args with units in reverse order */
	if (s->info && (larg = (List *) ITMA(s->info)[2])) { /*declared*/
		argnumstk[pargnum] = 0;
		for (q=larg->prev; q != larg; q = q->prev) {
			argnumstk[pargnum]++;
			Unit_push(STR(q));
		}
	}else{
		argnumstk[pargnum] = -1;
	}
}

void unit_done_args()
{
	if (argnumstk[pargnum] > 0) {
		diag("too few arguments", (char *)0);
	}
	assert(pargnum >= 0);
	pargnum--;
}

void unit_chk_arg(q1, q2)
	Item *q1, *q2;
{
	if (argnumstk[pargnum] > 0) {
		argnumstk[pargnum]--;
		unit_cmp(ITEM0, q1->prev, q2);
		unit_pop();
	}else if (argnumstk[pargnum] == 0) {
		diag("too many arguments", (char *)0);
	}else{
		unit_less();
	}
}

void func_unit(q1, q2)
	Item *q1, *q2;
{
	Symbol *s, *checklocal();
	
	s = SYM(q1);
	s = checklocal(s);	/* hidden with pushlocal */
	s->subtype = DEP;
	if (q2) {
		s->u.str = STR(q2);
	}else{
		s->u.str = (char *)0;
	}
}

void unit_del(i)	/* push 1/delta_x ^ i units */
	int i;
{
	Symbol *s;
	char *cp;
	
	s = lookup("delta_x");
	if(!s) {
		diag("delta_x not declared", (char *)0);
	}
	cp = decode_units(s);
	Unit_push("");
	while(i--) {
		Unit_push(cp);
		unit_div();
	}
}
