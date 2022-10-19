#include <../../nmodlconf.h>
/* /local/src/master/nrn/src/modlunit/consist.c,v 1.2 1995/09/05 20:24:01 hines Exp */

/*
 * Check that names do not have conflicting types.  This is done after the
 * entire file is read in and allows declaring a variable after its use (very
 * bad style) to work.
 */
#include "model.h"
#include "parse1.hpp"
#include "symbol.h"

extern Item** scop_indep;

#define con(arg1, arg2, arg3)                               \
    if (t & (arg2))                                         \
        if (t & (~(arg2 | arg3))) {                         \
            Fprintf(stderr, "%s is a %s\n", s->name, arg1); \
            err = 1;                                        \
        }

void consistency() {
    int tu, err = 0;
    long t;
    Symbol* sindep;


    /* the scop_indep can also be a automatic constant */
    sindep = SYM(scop_indep[0]);
    if (sindep != indepsym && sindep->subtype == (modlunitCONST | INDEP)) {
        sindep->subtype = INDEP;
    }

    SYMITERALL {
        t = s->subtype;
        con("KEYWORD", KEYWORD, 0);
        con("CONSTANT", modlunitCONST, ARRAY);
        con("INDEPENDENT", INDEP, 0);
        con("DEPENDENT", DEP, ARRAY);
        con("STATE", STAT, ARRAY);
        con("FUNCTION", FUNCT, 0);
        con("PROCEDURE", PROCED, 0);
        con("DERIVATIVE", DERF, 0);
        con(" KINETIC", KINF, 0);
        con("LINEAR", LINF, 0);
        con("NONLINEAR", NLINF, 0);
        con("DISCRETE", DISCF, 0);
        con("PARTIAL", PARF, 0);
        tu = s->usage;
        if ((tu & DEP) && (tu & FUNCT))
            diag(s->name, "used as both variable and function");
        if ((t == 0) && tu)
            Fprintf(stderr, "Warning: %s undefined. (declared within VERBATIM?)\n", s->name);
    }
}
if (err) {
    diag("multiple uses for same variable", (char*) 0);
}
if (indepsym == SYM0) {
    diag("Independent variable is not defined", (char*) 0);
}
}

/* consist.c,v
 * Revision 1.2  1995/09/05  20:24:01  hines
 * paramter arrays and limits syntax is now < a, b >
 *
 * Revision 1.1.1.1  1994/10/12  17:22:47  hines
 * NEURON 3.0 distribution
 *
 * Revision 1.1  90/11/13  16:09:24  hines
 * Initial revision
 *  */
