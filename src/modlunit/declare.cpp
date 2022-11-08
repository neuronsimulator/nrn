#include <../../nmodlconf.h>
#include <stdlib.h>
#include "model.h"
#include "parse1.hpp"
#include "symbol.h"
#ifdef HAVE_STRINGS_H
#include <strings.h>
#endif

Symbol* indepsym;  /* mathematical independent variable */
Item** scop_indep; /* the scop swept information */
const char* indepunits = "";

/* subtype of variables using explicit declarations */

static int promote(Symbol*, long);
static int nprime(char*);

void declare(long subtype, Item* q, Item* qa) {
    Symbol* sym = SYM(q);
    if (!sym->subtype) { /* not previously declared */
        sym->subtype = subtype;
        sym->info = qa;
    } else {
        diag("Multiple declaration of ", sym->name);
    }
    declare_array(sym);
    if (sym->subtype == INDEP) {
        declare_indep(sym);
    }
    /*fprintf(stderr, "declared %s with subtype %ld\n", sym->name, sym->subtype);*/
}

static int promote(Symbol* sym, long sub) {
    /*ARGSUSED*/
    diag("promotion not programmed yet", (char*) 0);
    return 0;
}

void declare_indep(Symbol* sym) {
    Item** qa;

    qa = ITMA(sym->info);
    if (!qa[7]) { /* no explicit SWEEP */
        if (indepsym) {
            diag("Only one independent variable can be defined", (char*) 0);
        }
        indepsym = sym;
        if (ITMA(sym->info)[1]) {
            indepunits = STR(ITMA(sym->info)[1]);
        }
        if (!scop_indep) {
            scop_indep = qa;
        }
    } else {
        if (scop_indep && scop_indep[7]) {
            diag("Only one SWEEP declaration is allowed", (char*) 0);
        }
        scop_indep = qa;
    }
}

void define_value(Item* q1, Item* q2) {
    Symbol* s;
    s = SYM(q1);
    if (s->subtype) {
        diag(s->name, "already declared");
    }
    if (s->usage) {
        diag(s->name, "used before DEFINE'ed");
    }
    s->type = DEFINEDVAR;
    if (q2->itemtype == SYMBOL) {
        s->u.i = SYM(q2)->u.i;
    } else {
        s->u.i = atoi(STR(q2));
    }
}

/* fix up array info */
void declare_array(Symbol* s) {
    Item* q;

    if (s->subtype & (modlunitCONST | DEP | STAT)) {
        q = ITMA(s->info)[2];
        if (q) {
            decdim(s, q);
            /*fprintf(stderr, "declared array %s[%d]\n", s->name, s->araydim);*/
        }
    }
}

void decdim(Symbol* s, Item* q) {
    s->subtype |= ARRAY;
    if (q->itemtype == SYMBOL && SYM(q)->type == DEFINEDVAR) {
        s->araydim = SYM(q)->u.i;
    } else if (q->itemtype == STRING) {
        s->araydim = atoi(STR(q));
    } else {
        /*SUPPRESS 622*/
        assert(0);
    }
    if (s->araydim < 1) {
        diag(s->name, "Array index must be > 0");
    }
}

Item* listtype(Item* q) {
    static int i = 0;

    if (i > 1) {
        diag("internal error inlisttype: First element of LIST is a LIST", (char*) 0);
    }
    switch (q->itemtype) {
    case SYMBOL:
        break;
    case LIST:
        i++;
        q = listtype(car(LST(q)));
        i--;
        break;
    case ITEM:
        q = listtype(ITM(q));
        break;
    default:
        diag("internal error in listtype: SYMBOL not first element", (char*) 0);
    }
    return q;
}

void declare_implied() {
    Symbol *sbase, *basestate(Symbol*);

#if NRNUNIT
    if (!indepsym) {
        List *save, *qa;
        Item *name, *units, *from, *to, *with, *num;
        save = intoken;
        intoken = newlist();
        name = putintoken("t", NAME, 0);
        units = putintoken("ms", STRING, UNITS);
        from = putintoken("0", INTEGER, INTEGER);
        to = putintoken("1", INTEGER, INTEGER);
        with = putintoken("WITH", NAME, 0);
        num = putintoken("1", INTEGER, INTEGER);
        qa = itemarray(8, name, units, from, to, with, num, ITEM0, ITEM0);
        declare(INDEP, ITMA(qa)[0], qa);
        intoken = save;
    }
#endif

    if (!indepsym) {
        diag("No INDEPENDENT variable has been declared", (char*) 0);
    }
    SYMITERALL {
        if (!s->subtype) {
            sbase = basestate(s);
            if (s->type == PRIME) {
                if (!sbase) {
                    diag(s->name, "is used but its corresponding STATE is not declared");
                }
                s->subtype = DEP;
                if (nprime(s->name) == 1) {
                    Sprintf(buf, "%s/%s", decode_units(sbase), indepunits);
                } else {
                    Sprintf(buf, "%s/%s%d", decode_units(sbase), indepunits, nprime(s->name));
                }
                s->u.str = stralloc(buf, (char*) 0);
            } else {
                if (sbase) {
                    s->subtype = modlunitCONST;
                    s->u.str = stralloc(decode_units(sbase), (char*) 0);
                }
            }
        }
    }
}
}

Symbol* basestate(Symbol* s) /* base state symbol for state''' or state0 */
{
    Symbol* base = SYM0;

    strcpy(buf, s->name);
    if (s->type == PRIME) {
        buf[strlen(buf) - nprime(s->name)] = '\0';
        base = lookup(buf);
    } else if (s->name[strlen(s->name) - 1] == '0') {
        buf[strlen(buf) - 1] = '\0';
        base = lookup(buf);
    }
    if (base && !(base->subtype & STAT)) {
        base = SYM0;
    }
    return base;
}

#if SYSV || !defined(HAVE_INDEX) || defined(HAVE_STRINGS_H)
#undef index
#define index strchr
#endif

static int nprime(char* s) {
    char* cp;

    cp = index(s, '\'');
    return strlen(s) - (cp - s);
}

void install_cfactor(Item* qname, Item* q1, Item* q2) /* declare conversion factor */
{
    declare(CNVFAC, qname, ITEM0);
    Unit_push(STR(q2));
    Unit_push(STR(q1));
    unit_div();
    SYM(qname)->u.str = stralloc(unit_str(), (char*) 0);
    unit_pop();
}
