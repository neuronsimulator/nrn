#include <../../nmodlconf.h>
/* /local/src/master/nrn/src/nmodl/partial.c,v 4.1 1997/08/30 20:45:33 hines Exp */
/*
partial.c,v
 * Revision 4.1  1997/08/30  20:45:33  hines
 * cvs problem with branches. Latest nmodl stuff should now be a top level
 *
 * Revision 4.0.1.1  1997/08/08  17:24:00  hines
 * nocmodl version 4.0.1
 *
 * Revision 4.0  1997/08/08  17:06:28  hines
 * proper nocmodl version number
 *
 * Revision 1.1.1.1  1994/10/12  17:21:37  hines
 * NEURON 3.0 distribution
 *
 * Revision 9.76  90/12/07  09:27:23  hines
 * new list structure that uses unions instead of void *element
 * 
 * Revision 9.67  90/11/27  12:10:07  hines
 * bug in bnd introduced by change to partial 
 * 
 * Revision 9.66  90/11/27  10:47:26  hines
 * allow multiple partial equations within a partial block
 * 
 * Revision 9.58  90/11/20  17:24:15  hines
 * CONSTANT changed to PARAMETER
 * CONSTANT now refers to variables that don't get put in .var file
 * 
 * Revision 9.20  90/08/15  15:16:16  hines
 * need to pass error value from crank back through the
 * block called by SOLVE.
 * 
 * Revision 8.1  90/01/16  11:06:14  mlh
 * error checking and cleanup after error and call to abort_run()
 * 
 * Revision 8.0  89/09/22  17:26:54  nfh
 * Freezing
 * 
 * Revision 7.0  89/08/30  13:32:29  nfh
 * Rev 7 is now Experimental; Rev 6 is Testing
 * 
 * Revision 6.0  89/08/14  16:27:10  nfh
 * Rev 6.0 is latest of 4.x; now the Experimental version
 * 
 * Revision 4.1  89/07/27  13:27:30  mlh
 * crank calling sequence sends indepsym as a sentinal value
 * ugh!
 * 
 * Revision 4.0  89/07/24  17:03:40  nfh
 * Freezing rev 3.  Rev 4 is now Experimental
 * 
 * Revision 3.3  89/07/18  11:55:16  mlh
 * first_time removed and MODEL_LEVEL used for declaration precedence
 * 
 * Revision 1.4  89/07/18  11:22:16  mlh
 * eliminate first_time, etc.
 * 
 * Revision 1.3  89/07/11  16:51:58  mlh
 * remove p array from calling sequence of crank
 * 
 * Revision 1.2  89/07/07  08:55:01  mlh
 * START field in INDEPENDENT block
 * SWEEP keyword replaces SCOP in INDEPENDENT block
 * FIRST and LAST keywords in PARTIAL block for boundaries rplace index
 * y'0 etc. allowed for constants
 * 
 * Revision 1.1  89/07/06  14:50:25  mlh
 * Initial revision
 * 
*/

#include "modl.h"
#include "parse1.h"
#include "symbol.h"

extern Symbol *indepsym;

/* for partial block communication with solve */
static Symbol *pv[4]; /* DV, F, V, G */
static Item *partialcolon;
static List *parinfo; /* DV, F, V, G, ~ with ident */
				
void solv_partial(qsol, fun)
	Item *qsol;
	Symbol *fun;
{
	int i, ident;
	Item *q;
	Symbol *dspace;
	
	if ((dspace = lookup("delta_x")) == SYM0) {
		dspace = install("delta_x", NAME);
		parminstall(dspace, "1", "");
	}
	Sprintf(buf, "%s();\n", fun->name);
	replacstr(qsol, buf);
	save_dt(partialcolon);
   ITERATE(q, parinfo) {
	for (i=0; i<4; i++) {
		pv[i] = SYM(q);
		q = q->next;
	}
	partialcolon = ITM(q);
	ident = q->itemtype;
	
Sprintf(buf, "if (error=crank(%d, %s, %s, %s, delta_%s, %s, %s, _pbound%d, &_parwork%d))return error;\n",
	pv[2]->araydim, pv[2]->name, pv[1]->name, pv[3]->name,
	indepsym->name, dspace->name, indepsym->name, ident, ident);
	replacstr(partialcolon, buf);
	Sprintf(buf, "static double *_pbound%d[4], _bnd%d_0, _bnd%d_N, *_parwork%d;\n",
		ident, ident, ident, ident);
	Linsertstr(procfunc, buf);
   }
}

void partial_eqn(q2, q4, q8, q11) /*V' F V G*/
	Item *q2, *q4, *q8, *q11;
{
	int i, dim;
	Item *q;
	static int ident=1;
	
	if (!parinfo) {
		parinfo = newlist();
	}
	pv[0] = SYM(q2);
	pv[1] = SYM(q4);
	pv[2] = SYM(q8);
	pv[3] = SYM(q11);
	ITERATE(q, parinfo) {
		if (SYM(q) == SYM(q2)) {
diag("Two partial equations for same state: ", SYM(q8)->name);
		}
		q = q->next->next->next->next;
	}
	Lappendsym(parinfo, SYM(q2));
	Lappendsym(parinfo, SYM(q4));
	Lappendsym(parinfo, SYM(q8));
	Lappendsym(parinfo, SYM(q11));
	q = lappendsym(parinfo, SYM0);
	ITM(q) = q2->prev;
	q->itemtype = ident++;

	dim = pv[2]->araydim;
	if (!(pv[2]->subtype & STAT)){
		diag(pv[2]->name, " is not a STATE");
	}
	if (strcmp(pv[2]->name, pv[0]->name + 1)) {
		diag(pv[2]->name, "' must be the time derivative");
	}
	for (i=0; i<4; i++) {
		if (!(pv[i]->subtype & ARRAY) || (pv[i]->araydim != dim)) {
			diag(pv[i]->name, " dimension differs from STATE var");
		}
	}
	replacstr(q2->prev, "\n/*where partial call goes*/\n");
	partialcolon = q2->prev;
	deltokens(q2, q11);
}

static List *bndinfo;	/* symbol, expression list with itemtype 0-3 */
static List *bnd[4];	/* for each condition consisting of
				symbol, list of tokens for expression
			in the order DEL y[0]   DEL  y[N]   y[0]   y[N]*/

void massagepartial(q1, q2, q3, q6) /*PARTIAL NAME stmtlist '}'*/
	Item *q1, *q2, *q3, *q6;
{
	
	int i, ident;
	Item *q, *qb;
	Symbol *s;
	
	static int seepartial = 0;
	if (seepartial++) {
		diag("Only one partial block currently allowed", (char *)0);
	}
	if (!pv[0]) {
diag(
"within the PARTIAL block must occur at least one equation with the syntax ---\n",
"~ V' = F*DEL2(V) + G\n");
	}

	if (!bndinfo) {
		bndinfo = newlist();
	}
   ITERATE(q, parinfo) {
	for (i=0; i<4; i++) {
		pv[i] = SYM(q);
		q = q->next;
		bnd[i] = (List *)0;
	}
	partialcolon = ITM(q);
	ident = q->itemtype;
	/*look through bndinfo list to reconstruct bnd*/
	ITERATE(qb, bndinfo) {
		s = SYM(qb); qb = qb->next;
		if (s == pv[2]) {
			if (bnd[qb->itemtype]) {
diag("Duplicate boundary condition for ", s->name);
			}		
			bnd[qb->itemtype] = LST(qb);
			qb->itemtype = -1; /* mark used */
		}
	}

	/* ensure that there is one condition on each side */
	for (i=0; i<2; i++) {
		if (bnd[i]) { /* Neumann */
			if (bnd[i+2]) { /* and also dirichlet */
				diag("Neumann and Dirichlet conditions",
				 " specified on same side");
			}
		} else { /* Neumann not specified */
			if (!bnd[i+2]) { /* neither is dirichlet */
				/* then default Neumann = 0 */
				bnd[i] = newlist();
				Lappendstr(bnd[i], "0");
			}
		}
	}
	/* place the boundary conditions before the call to crank */
	/* and set up the pointers */
	for (i=0; i<4; i++) {
		if (bnd[i]) {
			if (i%2) {
				Sprintf(buf, "_bnd%d_N = ", ident);
				Insertstr(partialcolon, buf);
				Sprintf(buf, "_pbound%d[%d] = &_bnd%d_N;\n",
				  ident, i, ident);
			}else{
				Sprintf(buf, "_bnd%d_0 = ", ident);
				Insertstr(partialcolon, buf);
				Sprintf(buf, "_pbound%d[%d] = &_bnd%d_0;\n",
				  ident, i, ident);
			}
			Lappendstr(initlist, buf);
			move(bnd[i]->next, bnd[i]->prev, partialcolon);
			Insertstr(partialcolon, ";\n");
		}
	}
   }	
	/* check that all boundary conditions have been used */
	ITERATE(qb, bndinfo) {
		s = SYM(qb);
		qb = qb->next;
		if (qb->itemtype != -1) {
			diag("Boundary condition: No partial equation for ",
			  s->name);
		}
	}
	replacstr(q1, "int");
	Insertstr(q3, "()\n");
	SYM(q2)->subtype |= PARF;
	SYM(q2)->u.i = 0;	/* not used: but see listnum in solvhandler */
	Insertstr(q6, "return 0;\n");
	movelist(q1, q6, procfunc);
}

/* ~ optionalDEL var[index] = expr */
/* type 0 Dirichlet (no DEL), type 1 Neumann (with DEL) */
void partial_bndry(type, qvar, qfirstlast, qexpr, qlast)
	int type;
	Item *qvar, *qfirstlast, *qexpr, *qlast;
{
	int indx;
	Item *q;
	List *l;
	
	if (!bndinfo) {
		bndinfo = newlist();
	}
	if (SYM(qfirstlast)->type == FIRST) {
		indx = type;
	}else{
		indx = type + 1;
	}
	Lappendsym(bndinfo, SYM(qvar));
	q = lappendsym(bndinfo, SYM0);
	q->itemtype = indx;
	l = newlist();
	LST(q) = l;

	if (indx < 2) { /* ~ DEL y [N] = */
		deltokens(qvar->prev->prev, qexpr->prev);
	}else{ /* ~ ... = */
		deltokens(qvar->prev, qexpr->prev);
	}
	movelist(qexpr, qlast, l);
}
