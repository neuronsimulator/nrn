#include <../../nrnconf.h>
/* /local/src/master/nrn/src/oc/getsym.c,v 1.2 1996/02/16 16:19:26 hines Exp */
/*
getsym.c,v
 * Revision 1.2  1996/02/16  16:19:26  hines
 * OCSMALL used to throw out things not needed by teaching programs
 *
 * Revision 1.1.1.1  1994/10/12  17:22:08  hines
 * NEURON 3.0 distribution
 *
 * Revision 1.4  92/08/18  07:31:38  hines
 * arrays in different objects can have different sizes.
 * Now one uses araypt(symbol, SYMBOL) or araypt(symbol, OBJECTVAR) to
 * return index of an array variable.
 * 
 * Revision 1.3  91/10/18  14:40:36  hines
 * symbol tables now are type Symlist containing pointers to first and last
 * symbols.  New symbols get added onto the end.
 * 
 * Revision 1.2  91/10/17  15:01:28  hines
 * VAR, STRING now handled with pointer to value located in object data space
 * to allow future multiple objects. Ie symbol for var, string, objectvar
 * has offset into a pointer data space.
 * 
 * Revision 1.1  91/10/11  11:12:03  hines
 * Initial revision
 * 
 * Revision 3.9  89/07/20  09:52:17  mlh
 * code functions no longer in hoc.h
 * 
 * Revision 3.7  89/07/13  08:21:30  mlh
 * stack functions involve specific types instead of Datum
 * 
 * Revision 3.4  89/07/12  10:27:00  mlh
 * Lint free
 * 
 * Revision 3.3  89/07/10  15:46:00  mlh
 * Lint pass1 is silent. Inst structure changed to union.
 * 
 * Revision 2.0  89/07/07  11:36:58  mlh
 * *** empty log message ***
 * 
 * Revision 1.1  89/07/07  11:16:01  mlh
 * Initial revision
 * 
*/

/* Psym *hoc_getsym("variable") returns a pointer to a new Psym structure
	that contains info used to find where the variable keeps its data.
	Array indices cannot involve local variables.
	Example: hoc_getsym("a[i][j]") depends on the values of i and j when
	hoc_getsym is called; and not on their values when the Psym is used.

   double hoc_getsymval(Psym *p) returns the value of the variable. If an
   	array, the indices used are the values they had when hoc_getsym
   	was called.

   hoc_assignsym(Psym *p, double val) sets the value of the variable.
   
   hoc_execstr(char *s) compiles and executes the string
*/
#if OCSMALL
#else

#include "hocgetsym.h"
#include  "parse.h"
#include "hocparse.h"
#include "code.h"

Psym *hoc_getsym(const char* cp) {
	Symbol *sp, *sym;
	Symlist *symlist = (Symlist *)0;
	Inst *last, *pcsav;
	int i, n;
	char s[256];
	Psym *p;

	Sprintf(s, "{%s}\n", cp);
	sp = hoc_install("", PROCEDURE, 0., &symlist);
	sp->u.u_proc->defn.in = STOP;
	sp->u.u_proc->list = (Symlist *)0;
	sp->u.u_proc->nauto = 0;
	n = hoc_xopen_run(sp, s);
	last = (Inst *)sp->u.u_proc->defn.in + n;
	if (n < 5 || last[-3].pf != hoc_eval) {
		hoc_execerror(s, " not a variable");
	}
	last[-3].in = STOP;	/*before doing last EVAL*/
	pcsav = hoc_pc;
	hoc_execute(sp->u.u_proc->defn.in);
	hoc_pc = pcsav;
	
	sym = hoc_spop();
	switch(sym->type) {
	case UNDEF:
		hoc_execerror(s, " is undefined");
	case VAR:
		if (ISARRAY(sym)) {
			Arrayinfo* a;
			if (sym->subtype == NOTUSER) {
				a = OPARINFO(sym);
			}else{
				a = sym->arayinfo;
			}
			p = (Psym *)emalloc((unsigned)(sizeof(Psym) +
				a->nsub));
			p->arayinfo = a;
			++a->refcount;
			p->nsub = a->nsub;
			for (i=p->nsub; i > 0;) {
				p->sub[--i] = hoc_xpop();
			}
		} else {
			p = (Psym *)emalloc(sizeof(Psym));
			p->arayinfo = 0;
			p->nsub = 0;
		}
		p->sym = sym;
		break;
	case AUTO:
		hoc_execerror(s, " is local variable");
	default:
		hoc_execerror(s, " not a variable");
	}
	hoc_free_list(&symlist);
	return p;	
}

static void arayonstack(Psym* p) {
	int i;
	double d;
	
	if (p->nsub) {
		if (!ISARRAY(p->sym) || p->nsub != p->arayinfo->nsub) {
			hoc_execerror("wrong number of subscripts for ", p->sym->name);
		}
		for (i=0; i < p->nsub; i++) {
			d = p->sub[i];
			hoc_pushx(d);
		}
	}
}

double hoc_getsymval(Psym* p) {
	arayonstack(p);
	hoc_pushs(p->sym);
	hoc_eval();
	return hoc_xpop();
}

void hoc_assignsym(Psym* p, double val){
	arayonstack(p);
	hoc_pushx(val);
	hoc_pushs(p->sym);
	hoc_assign();
	hoc_nopop();
}

void hoc_execstr(const char* cp) {
	Symbol *sp;
	Symlist *symlist = (Symlist *)0;
	Inst *pcsav;
	char s[256];

	Sprintf(s, "{%s}\n", cp);
	sp = hoc_install("", PROCEDURE, 0., &symlist);
	sp->u.u_proc->defn.in = STOP;
	sp->u.u_proc->list = (Symlist *)0;
	sp->u.u_proc->nauto = 0;
	IGNORE(hoc_xopen_run(sp, s));
	pcsav = hoc_pc;
	hoc_execute(sp->u.u_proc->defn.in);
	hoc_pc = pcsav;
	hoc_free_list(&symlist);
}	
#endif /*OCSMALL*/
