#include <../../nmodlconf.h>
/* /local/src/master/nrn/src/nmodl/init.c,v 4.5 1998/03/25 14:33:42 hines Exp */

#include "modl.h"
#include "parse1.h"

extern List    *firstlist;
extern List    *syminorder;
Symbol         *semi, *beginblk, *endblk;
List           *intoken;
char		buf[NRN_BUFSIZE];	/* volatile temporary buffer */

static struct {			/* Keywords */
	char           *name;
	short           kval;
}               keywords[] = {
                        "VERBATIM", VERBATIM,
                        "COMMENT", COMMENT,
                        "TITLE", MODEL,
                        "CONSTANT", CONSTANT,
                        "PARAMETER", PARAMETER,
                        "INDEPENDENT", INDEPENDENT,
                        "ASSIGNED", DEPENDENT,
                        "INITIAL", INITIAL1,
                        "TERMINAL", TERMINAL,
                        "DERIVATIVE", DERIVATIVE,
                        "EQUATION", EQUATION,
                        "BREAKPOINT", BREAKPOINT,
                        "CONDUCTANCE", CONDUCTANCE,
                        "SOLVE", SOLVE,
                        "STATE", STATE,
                        "STEPPED", STEPPED,
                        "LINEAR", LINEAR,
                        "NONLINEAR", NONLINEAR,
                        "DISCRETE", DISCRETE,
                        "FUNCTION", FUNCTION1,
                        "FUNCTION_TABLE", FUNCTION_TABLE,
                        "PROCEDURE", PROCEDURE,
                        "PARTIAL", PARTIAL,
                        "INT", INT,
                        "DEL2", DEL2,
                        "DEL", DEL,
                        "LOCAL", LOCAL,
                        "METHOD", USING,
                        "STEADYSTATE", USING,
                        "SENS", SENS,
                        "STEP", STEP,
                        "WITH", WITH,
                        "FROM", FROM,
                        "FORALL", FORALL1,
                        "TO", TO,
                        "BY", BY,
                        "if", IF,
                        "else", ELSE,
                        "while", WHILE,
                        "START", START1,
                        "DEFINE", DEFINE1,
                        "KINETIC", KINETIC,
                        "CONSERVE", CONSERVE,
                        "PLOT", PLOT,
                        "VS", VS,
                        "LAG", LAG,
                        "RESET", RESET,
                        "MATCH", MATCH,
                        "MODEL_LEVEL", MODEL_LEVEL,	/* inserted by merge */
                        "SWEEP", SWEEP,
                        "FIRST", FIRST,
                        "LAST", LAST,
                        "COMPARTMENT", COMPARTMENT,
                        "LONGITUDINAL_DIFFUSION", LONGDIFUS,
                        "PUTQ", PUTQ,
                        "GETQ", GETQ,
                        "IFERROR", IFERROR,
                        "SOLVEFOR", SOLVEFOR,
                        "UNITS", UNITS,
                        "UNITSON", UNITSON,
                        "UNITSOFF", UNITSOFF,
                        "TABLE", TABLE,
                        "DEPEND", DEPEND,
                        "NEURON", NEURON,
                        "SUFFIX", SUFFIX,
                        "POINT_PROCESS", SUFFIX,
                        "ARTIFICIAL_CELL", SUFFIX,
                        "NONSPECIFIC_CURRENT", NONSPECIFIC,
                        "ELECTRODE_CURRENT", ELECTRODE_CURRENT,
                        "SECTION", SECTION,
                        "RANGE", RANGE,
                        "USEION", USEION,
                        "READ", READ,
                        "WRITE", WRITE,
                        "VALENCE", VALENCE,
                        "CHARGE", VALENCE,
                        "GLOBAL", GLOBAL,
                        "POINTER", POINTER,
                        "BBCOREPOINTER", BBCOREPOINTER,
                        "EXTERNAL", EXTERNAL,
                        "INCLUDE", INCLUDE1,
                        "CONSTRUCTOR", CONSTRUCTOR,
                        "DESTRUCTOR", DESTRUCTOR,
                        "NET_RECEIVE", NETRECEIVE,
                        "BEFORE", BEFORE, /* before NEURON sets up cy' = f(y,t) */
                        "AFTER", AFTER, /* after NEURON solves cy' = f(y, t) */
                        "WATCH", WATCH,
                        "FOR_NETCONS", FOR_NETCONS,
                        "THREADSAFE", THREADSAFE,
                        "PROTECT", PROTECT,
                        "MUTEXLOCK", NRNMUTEXLOCK,
                        "MUTEXUNLOCK", NRNMUTEXUNLOCK,
                        "REPRESENTS", REPRESENTS,
	                0, 0
};

/*
 * the following special output tokens are used to make the .c file barely
 * readable 
 */
static struct {			/* special output tokens */
	char           *name;
	short           subtype;
	Symbol        **p;
}               special[] = {
                        ";", SEMI, &semi,
                        "{", BEGINBLK, &beginblk,
                        "}", ENDBLK, &endblk,
	                0, 0, 0
};

static struct {			/* numerical methods */
	char           *name;
	long           subtype;	/* All the types that will work with this */
	short		varstep;
}               methods[] = {
                        "adams", DERF | KINF, 0,
                        "runge", DERF | KINF, 0,
                        "euler", DERF | KINF, 0,
                        "adeuler", DERF | KINF, 1,
                        "heun", DERF | KINF, 0,
                        "adrunge", DERF | KINF, 1,
                        "gear", DERF | KINF, 1,
                        "newton", NLINF, 0,
                        "simplex", NLINF, 0,
                        "simeq", LINF, 0,
                        "seidel", LINF, 0,
                        "_advance", KINF, 0,
                        "sparse", KINF, 0,
                        "derivimplicit", DERF, 0, /* name hard wired in deriv.c */
                        "cnexp", DERF, 0, /* see solve.c */
                        "clsoda", DERF | KINF, 1,   /* Tolerance built in to scopgear.c */
                        "after_cvode", 0, 0,
                        "cvode_t", 0, 0,
                        "cvode_t_v", 0, 0,
	                0, 0, 0
};

static char    *extdef[] = {	/* external names that can be used as doubles
				 * without giving an error message */
#include "extdef.h"
			    0
};

static char    *extdef2[] = {	/* external function names that can be used
				 * with array and function name arguments  */
#include "extdef2.h"
			    0
};

static char	*extdef3[] = {	/* function names that get two reset arguments
				 * added */
        "threshold",
        "squarewave",
        "sawtooth",
        "revsawtooth",
        "ramp",
        "pulse",
        "perpulse",
        "step",
        "perstep",
        "stepforce",
        "schedule",
	0
};

static char *extdef4[] = { /* functions that need a first arg of NrnThread* */
        "at_time",
	0
};

static char    *extdef5[] = {	/* the extdef names that are not threadsafe */
#include "extdef5.h"
			    0
};

List *constructorfunc, *destructorfunc;

void init()
{
	int             i;
	Symbol         *s;

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
	for (i = 0; extdef2[i]; i++) {
		s = install(extdef2[i], NAME);
		s->subtype = EXTDEF2;
	}
	for (i = 0; extdef3[i]; i++) {
		s = lookup(extdef3[i]);
		assert(s && (s->subtype & EXTDEF));
		s->subtype |= EXTDEF3;
	}
	for (i = 0; extdef4[i]; i++) {
		s = lookup(extdef4[i]);
		assert(s && (s->subtype & EXTDEF));
		s->subtype |= EXTDEF4;
	}
	for (i = 0; extdef5[i]; i++) {
		s = lookup(extdef5[i]);
		assert(s);
		s->subtype |= EXTDEF5;
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
	constructorfunc = newlist();
	destructorfunc = newlist();
#if NMODL
	nrninit();
#endif
}
