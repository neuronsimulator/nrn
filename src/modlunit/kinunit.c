#include <../../nmodlconf.h>
#include "model.h"
#include "symbol.h"
#include "units.h"
#include "parse1.h"
extern char *indepunits;
static List *reactnames;

static void set_flux_units();
static void react_unit_err();

void kinunits(type, pass)
	Item *type;
	int pass;
{
	struct unit ux1, ux2, ur1, ur2, uflux;
	Item *q;
	char *s;
	
	if (type->itemsubtype == REACT1) {
		ucopypop(&ux2);
	}
	ucopypop(&ux1);
	if (type->itemsubtype == REACT1) {
		ucopypop(&ur2);
	}
	ucopypop(&ur1);
	if (pass == 0) {
		return;
	}
	
	/* case reaction  with no numbers */
	/* should check that all states have same units */
	Unit_push((char *)0); /* this will go to any units */
	ITERATE(q, reactnames) {
		unit_push(ITM(q));
		s = (char *)(ITMA(SYM(ITM(q))->info)[6]);
		if (s) {
			Unit_push(s);
			unit_mul();
		}
		if (unit_diff()) {
Fprintf(stderr, "REACTION quantity units for %s is: %s\n", SYM(ITM(q))->name, unit_str());
			unit_pop();
Fprintf(stderr, "but the quantity units of the first term is: %s\n", unit_str());
			diag("Inconsistent material quantity units\n",
			 "Need a correct COMPARTMENT statement");
		}
	}
		
	Unit_push(indepunits);
	unit_div();
	ucopypop(&uflux);
	set_flux_units(&uflux);
	
	if (type->itemsubtype == REACT1 || type->itemsubtype == '-') {
		ucopypush(&ux1);
		ucopypush(&uflux);
		ucopypush(&ur1);
		unit_div();
		if (unit_diff()) {
			react_unit_err("forward", &uflux);
		}
		unit_pop();
	}

	if (type->itemsubtype == REACT1) {
		ucopypush(&ux2);
		ucopypush(&uflux);
		ucopypush(&ur2);
		unit_div();
		if (unit_diff()) {
			react_unit_err("backward", &uflux);
		}
		unit_pop();
	}

	if (type->itemsubtype == LT) {
		ucopypush(&ux1);
		ucopypush(&uflux);
		if (unit_diff()) {
Fprintf(stderr, "Flux units are: %s\n", Unit_str(&uflux));
Fprintf(stderr, "But users << flux units are:%s\n", Unit_str(&ux1));
diag("Inconsistent flux units", (char *)0);
		}
		unit_pop();
	}
	
	freelist(&reactnames);
}

static void set_flux_units(up)
	struct unit *up;
{
	Symbol *s;

	Sprintf(buf, "%s", Unit_str(up));
	if ((s = lookup("f_flux")) == SYM0) {
		s = install("f_flux", NAME);
	}
	s->u.str = stralloc(buf, (char *)0);
	if ((s = lookup("b_flux")) == SYM0) {
		s = install("b_flux", NAME);
	}
	s->u.str = stralloc(buf, (char *)0);
	
}

static void react_unit_err(s, up)
	char *s;
	struct unit *up;
{
	
	Fprintf(stderr, "Flux units for this reaction: %s\n", Unit_str(up));
	ucopypop(up);
	Fprintf(stderr, "This implies %s rate units: %s\n", s, Unit_str(up));
	ucopypop(up);
Fprintf(stderr, "But the users %s rate units are: %s\n", s, Unit_str(up));
	diag("inconsistent reaction units", (char *)0);
}

void clear_compartlist() {
	SYMITER_STAT {
		ITMA(s->info)[6] = ITEM0;
	}}
}

void unit_compartlist(q)
	Item *q;
{
	char *ustr;
	
	ustr = (char *)(ITMA(SYM(q)->info)[6]);
	if (ustr) {
		diag(SYM(q)->name, " already in previous COMPARTMENT");
	}
	ITMA(SYM(q)->info)[6] = (Item *) stralloc(unit_str(), (char *)0);
}

void unit_ldifuslist(q, flag)
	Item *q;
	int flag;
{
	char *ustr;
	unitonflag = flag;
	ustr = (char *)(ITMA(SYM(q)->info)[6]);
	if (!ustr) {
		diag(SYM(q)->name, " not declared in previous COMPARTMENT");
	}
	Unit_push("micron4/ms");
	if (!unit_cmp_exact()) {
		unit_pop();
diag(unit_str(), " : relevant area * diffusion constant must\n   be micron2 micron2/ms (1-21 m4/s)");
	}
	unit_pop();
	Unit_push("micron2");
	Unit_push(ustr);
	if (!unit_cmp_exact()) {
		diag(ustr, ": With LONGDITUDINAL_DIFFUSION the compartment \
volume\nmust be measured in micron3/micron (1-12 m2)");
	}
	unit_pop();
	unit_pop();
	unitonflag = 0;
}

void consreact_push(q)
	Item* q;
{
	char* ustr;
	unit_push(q);
	if (SYM(q)->info) {
		ustr = (char *)(ITMA(SYM(q)->info)[6]);
		if (ustr) {
			Unit_push(ustr);
			unit_mul();
		}
	}
}

void ureactadd(q)
	Item *q;
{
	if (!reactnames) {
		reactnames = newlist();
	}
	Lappenditem(reactnames, q);
}
