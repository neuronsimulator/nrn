#include <../../nmodlconf.h>
/* /local/src/master/nrn/src/modlunit/init.c,v 1.9 1998/03/25 14:33:55 hines Exp */

#include "model.h"
#include "parse1.hpp"

extern int unitonflag;
List *initlist, *initfunc, *firstlist, *termfunc, *modelfunc;
List *procfunc, *plotlist, *solvelist, *misc;
List* syminorder;
Symbol *semi, *beginblk, *endblk;
List* intoken;
char buf[512]; /* volatile temporary buffer */

static struct { /* Keywords */
    const char* name;
    short kval;
} keywords[] = {{"VERBATIM", VERBATIM},
                {"ENDVERBATIM", END_VERBATIM}, /* explicit in lex.lpp */
                {"COMMENT", COMMENT},
                {"ENDCOMMENT", END_COMMENT}, /* explicit in lex.lpp */
                {"TITLE", TITLE},
                {"CONSTANT", CONSTANT},
                {"PARAMETER", PARAMETER},
                {"INDEPENDENT", INDEPENDENT},
                {"ASSIGNED", ASSIGNED},
                {"INITIAL", INITIAL1},
                {"DERIVATIVE", DERIVATIVE},
                {"EQUATION", EQUATION},
                {"BREAKPOINT", BREAKPOINT},
                {"CONDUCTANCE", CONDUCTANCE},
                {"SOLVE", SOLVE},
                {"STATE", STATE},
                {"LINEAR", LINEAR},
                {"NONLINEAR", NONLINEAR},
                {"DISCRETE", DISCRETE},
                {"FUNCTION", FUNCTION1},
                {"FUNCTION_TABLE", FUNCTION_TABLE},
                {"PROCEDURE", PROCEDURE},
                {"INT", INT},
                {"DEL2", DEL2},
                {"DEL", DEL},
                {"LOCAL", LOCAL},
                {"METHOD", USING},
                {"STEADYSTATE", USING},
                {"STEP", STEP},
                {"WITH", WITH},
                {"FROM", FROM},
                {"TO", TO},
                {"BY", BY},
                {"if", IF},
                {"else", ELSE},
                {"while", WHILE},
                {"IF", IF},
                {"ELSE", ELSE},
                {"WHILE", WHILE},
                {"START", START1},
                {"DEFINE", DEFINE1},
                {"KINETIC", KINETIC},
                {"CONSERVE", CONSERVE},
                {"VS", VS},
                {"LAG", LAG},
                {"SWEEP", SWEEP},
                {"COMPARTMENT", COMPARTMENT},
                {"LONGITUDINAL_DIFFUSION", LONGDIFUS},
                {"SOLVEFOR", SOLVEFOR},
                {"UNITS", UNITBLK},
                {"UNITSON", UNITSON},
                {"UNITSOFF", UNITSOFF},
                {"TABLE", TABLE},
                {"DEPEND", DEPEND},
                {"NEURON", NEURON},
                {"SUFFIX", SUFFIX},
                {"POINT_PROCESS", SUFFIX},
                {"ARTIFICIAL_CELL", SUFFIX},
                {"NONSPECIFIC_CURRENT", NONSPECIFIC},
                {"ELECTRODE_CURRENT", ELECTRODE_CURRENT},
                {"USEION", USEION},
                {"READ", READ},
                {"WRITE", WRITE},
                {"RANGE", RANGE},
                {"RANDOM", RANDOM},
                {"VALENCE", VALENCE},
                {"CHARGE", VALENCE},
                {"GLOBAL", GLOBAL},
                {"POINTER", POINTER},
                {"BBCOREPOINTER", POINTER},
                {"EXTERNAL", EXTERNAL},
                {"INCLUDE", INCLUDE1},
                {"CONSTRUCTOR", CONSTRUCTOR},
                {"DESTRUCTOR", DESTRUCTOR},
                {"NET_RECEIVE", NETRECEIVE},
                {"BEFORE", BEFORE}, /* before NEURON sets up cy' = f(y,t) */
                {"AFTER", AFTER},   /* after NEURON solves cy' = f(y, t) */
                {"WATCH", WATCH},
                {"FOR_NETCONS", FOR_NETCONS},
                {"THREADSAFE", THREADSAFE},
                {"PROTECT", PROTECT},
                {0, 0}};

/*
 * the following special output tokens are used to make the .c file barely
 * readable
 */
static struct { /* special output tokens */
    const char* name;
    long subtype;
    Symbol** p;
} special[] = {{";", SEMI, &semi},
               {"{", BEGINBLK, &beginblk},
               {"}", ENDBLK, &endblk},
               {nullptr, 0, nullptr}};

static struct { /* numerical methods */
    const char* name;
    long subtype; /* All the types that will work with this */
    short varstep;
} methods[] = {{"runge", DERF | KINF, 0},
               {"euler", DERF | KINF, 0},
               {"newton", NLINF, 0},
               {"simeq", LINF, 0},
               {"_advance", KINF, 0},
               {"sparse", KINF, 0},
               {"derivimplicit", DERF, 0}, /* name hard wired in deriv.c */
               {"cnexp", DERF, 0},
               {"after_cvode", 0, 0},
               {"cvode_t", 0, 0},
               {"cvode_t_v", 0, 0},
               {0, 0, 0}};

static const char* extdef[] = {/* external names that can be used as doubles
                                * without giving an error message */
#include "extdef.h"
                               0};

static const char* extargs[] = {/* units of args to external functions */
/* format: name, returnunits, arg1unit, arg2unit, ..., 0, */
#include "extargs.h"
                                0};

void init() {
    int i;
    Symbol* s;

    symbol_init();
    for (i = 0; keywords[i].name; i++) {
        s = install(keywords[i].name, keywords[i].kval);
        s->subtype = KEYWORD;
    }
    for (i = 0; methods[i].name; i++) {
        s = install(methods[i].name, METHOD);
        s->subtype = methods[i].subtype;
        s->u.i = methods[i].varstep;
    }
    for (i = 0; special[i].name; i++) {
        s = install(special[i].name, SPECIAL);
        *(special[i].p) = s;
        s->subtype = special[i].subtype;
    }
    for (i = 0; extdef[i]; i++) {
        s = install(extdef[i], NAME);
        s->subtype = EXTDEF;
    }
    for (i = 0; extargs[i]; ++i) {
        List* lu;
        s = lookup(extargs[i++]);
        assert(s);
        s->u.str = extargs[i++];
        lu = newlist();
        while (extargs[i]) {
            lappendstr(lu, extargs[i]);
            ++i;
        }
        s->info = itemarray(3, ITEM0, ITEM0, lu);
    }
    intoken = newlist();
    initfunc = newlist();
    modelfunc = newlist();
    termfunc = newlist();
    procfunc = newlist();
    initlist = newlist();
    firstlist = newlist();
    syminorder = newlist();
    plotlist = newlist();
    solvelist = newlist();
    misc = newlist();
}

/* init.c,v
 * Revision 1.9  1998/03/25  14:33:55  hines
 * Model descriptions of a POINT_PROCESS may contain a
 * NET_RECEIVE(weight){statementlist}
 * weight is a reference variable and may be changed within this block.
 *
 * Revision 1.8  1998/02/19  20:43:09  hines
 * modlunit more up to date with respect to nmodl
 *
 * Revision 1.7  1997/11/20  21:34:34  hines
 * can specify external function argument units for modlunit checking
 * in nrn/src/modlunit/extargs.h
 * for example at_time must take an argument with units of ms.
 * Not all useful functions are listed at this time.
 *
 * Revision 1.6  1997/11/13  21:45:05  hines
 * modlunit checks for LONGITUDINAL_DIFFUSION expr { state }
 * are that expr has the units of micron4/ms, and that there exist a prior
 * COMPARTMENT statement with a volume expression with units micron3/micron
 *
 * Revision 1.5  1997/07/29  20:21:43  hines
 * mac port of nmodl, modlunit
 *
 * Revision 1.4  1997/06/26  20:12:09  hines
 * modlunit more up to date with respect to allowed external functions.
 *
 * Revision 1.3  1995/07/16  13:05:30  hines
 * FUNCTION_TABLE looks good so far
 *
 * Revision 1.2  1995/07/15  12:12:41  hines
 * CONSTRUCTRO DESTRUCTOR handled
 *
 * Revision 1.1.1.1  1994/10/12  17:22:47  hines
 * NEURON 3.0 distribution
 *
 * Revision 1.12  1994/09/26  18:51:06  hines
 * USEION ... VALENCE real
 *
 * Revision 1.11  1994/05/18  18:08:13  hines
 * INCLUDE "file"
 * tries originalpath/file ./file MODL_INCLUDEpaths/file
 *
 * Revision 1.10  1993/07/08  14:36:28  hines
 * An alternative to NONSPECIFIC_CURRENT is ELECTRODE_CURRENT
 *
 * Revision 1.9  92/06/01  13:25:30  hines
 * NEURON {EXTERNAL name, name, ...} allowed
 *
 * Revision 1.8  92/02/17  12:30:50  hines
 * constant states with a compartment size didn't have space allocated
 * to store the compartment size.
 *
 * Revision 1.7  91/09/16  16:03:36  hines
 * NEURON { RANGE SECTION GLOBAL} syntax
 *
 * Revision 1.6  91/01/29  07:10:29  hines
 * POINT_PROCESS keyword allowed
 *
 * Revision 1.5  90/12/12  11:33:08  hines
 * LOCAL vectors allowed. Some more NEURON syntax added
 *
 * Revision 1.4  90/11/23  13:43:41  hines
 * BREAKPOINT PARAMETER
 *
 * Revision 1.3  90/11/20  15:30:23  hines
 * added 4 varieties of unit factors. They are
 * name = (real)
 * name = ((unit) -> (unit))	must be conformable
 * name = (physical_constant)
 * name = (physical_constant (unit))	must be conformable
 *
 * Revision 1.2  90/11/15  13:00:23  hines
 * function units and number units work. accepts NEURON block
 *
 * Revision 1.1  90/11/13  16:10:08  hines
 * Initial revision
 *  */
