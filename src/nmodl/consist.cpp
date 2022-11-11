#include <../../nmodlconf.h>

/*
 * Check that names do not have conflicting types.  This is done after the
 * entire file is read in and allows declaring a variable after its use (very
 * bad style) to work in some circumstances.
 */
#include "modl.h"
#include "parse1.hpp"
#include "symbol.h"

extern Symbol* scop_indep;
extern Symbol* indepsym;
extern int vectorize;

#define con(arg1, arg2, arg3)                               \
    if (t & (arg2))                                         \
        if (t & (~(arg2 | arg3))) {                         \
            Fprintf(stderr, "%s is a %s\n", s->name, arg1); \
            err = 1;                                        \
        }

/**
 * Check if variable declared in NEURON block is conflicting with a PROCEDURE or FUNCTION
 *
 * In order to avoid "declaring" variable in NEURON block and then defining
 * PROCEDURE or FUNCTION with the same name, we check if the variable is
 * function and has one of the NRN* type.
 */
int is_var_declared_as_function(Symbol* s) {
    int type = s->nrntype;
    int usage = s->usage;

    // if not function or procedure name then return already
    if (!(usage & FUNCT)) {
        return 0;
    }

    // conflicting name if presence of NEURON block variable type
    return (type & NRNRANGE || type & NRNGLOBAL || type & NRNPOINTER || type & NRNBBCOREPOINTER ||
            type & NRNEXTRN);
}

void consistency() {
    Symbol* s;
    Item* qs;
    int i, tu, err = 0;
    long t;


    /* the scop_indep can also be a automatic parameter */
    if (scop_indep != indepsym && scop_indep->subtype == (PARM | INDEP)) {
        scop_indep->subtype = INDEP;
    }

    SYMITER(NAME) {
        t = s->subtype;
        con("KEYWORD", KEYWORD, 0);
        con("RESERVED WORD", EXTDEF | EXTDEF2 | EXTDEF3 | EXTDEF4 | EXTDEF5, 0);
        con("CONSTANT", nmodlCONST, 0);
        con("PARAMETER", PARM, ARRAY);
        con("ASSIGNED", DEP, ARRAY);
        con("INDEPENDENT", INDEP, 0);
        con("STATE", STAT, ARRAY);
        con("FUNCTION", FUNCT, 0);
        con("PROCEDURE", PROCED, 0);
        con("DERIVATIVE", DERF, 0);
        con(" KINETIC", KINF, 0);
        con("LINEAR", LINF, 0);
        con("NONLINEAR", NLINF, 0);
        con("DISCRETE", DISCF, 0);
        con("PARTIAL", PARF, 0);
        con("CONSTANT UNITS FACTOR", UNITDEF, 0);
        tu = s->usage;
        if ((tu & DEP) && (tu & FUNCT)) {
            diag(s->name, " used as both variable and function");
        }
        // check for conflicting variable declaration with function
        // do not use diag() because line number might be misleading
        if (is_var_declared_as_function(s)) {
            Fprintf(stderr,
                    "Error: %s used as both variable and function in file %s\n",
                    s->name,
                    finname);
            exit(1);
        }
        if ((t == 0) && tu) {
            // variable `v` is always defined in data array for vectorized mod files
            if (!(vectorize && strcmp(s->name, "v") == 0)) {
                Fprintf(stderr, "Warning: %s undefined. (declared within VERBATIM?)\n", s->name);
            }
        }
    }
    if (err) {
        diag("multiple uses for same variable", (char*) 0);
    }
    if (indepsym == SYM0) {
        diag("Independent variable is not defined", (char*) 0);
    }
    /* avoid the problem where person inadvertently is using Dstate as a state*/
    SYMITER(NAME) {
        if ((s->subtype & STAT) && (s->name[0] == 'D')) {
            Symbol* s1 = lookup(s->name + 1);
            if (s1 && s->type == NAME && (s->subtype & STAT)) {
                fprintf(stderr, "%s is a STATE so %s is a %s' and", s1->name, s->name, s1->name);
                diag(" cannot be declared as a STATE\n", (char*) 0);
            }
        }
    }
}
