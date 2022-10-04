#include <../../nmodlconf.h>

/* print the .c file from the lists */

#include "modl.h"
#include "parse1.hpp"
#include "symbol.h"

#define P(arg) fputs(arg, fcout)
List *procfunc, *initfunc, *modelfunc, *termfunc, *initlist, *firstlist;
/* firstlist gets statements that must go before anything else */

List* nrnstate;
extern List *currents, *set_ion_variables(int), *get_ion_variables(int);
extern List *begin_dion_stmt(), *end_dion_stmt(char*);

extern Symbol* indepsym;
extern List* indeplist;
extern List* defs_list;
extern char* saveindep;
extern char* RCS_version;
extern char* RCS_date;
char* modelline;

extern int vectorize;
static List* vectorize_replacements; /* pairs of item pointer, strings */
extern char* cray_pragma();

#if __TURBOC__ || SYSV || VMS
#define index strchr
#endif

static void initstates();
static void funcdec();

void c_out() {
    Item* q;
    extern int point_process;

    if (vectorize) {
        vectorize_do_substitute();
        c_out_vectorize();
        return;
    }
    Fprintf(fcout, "/* Created by Language version: %s  of %s */\n", RCS_version, RCS_date);
    P("/* NOT VECTORIZED */\n");
    Fflush(fcout);
    /* things which must go first and most declarations */
#if SIMSYS
    P("#include <stdio.h>\n#include <math.h>\n#include \"mathlib.h\"\n");
    P("#include \"common.h\"\n#include \"softbus.h\"\n");
    P("#include \"sbtypes.h\"\n#include \"Solver.h\"\n");
#else
    P("#include <stdio.h>\n#include <math.h>\n#include \"mech_api.h\"\n");
    P("#undef PI\n");
#endif
    printlist(defs_list);
    printlist(firstlist);
    RCS_version[strlen(RCS_version) - 2] = '\0';
    RCS_date[strlen(RCS_date) - 11] = '\0';
    RCS_version = index(RCS_version, ':') + 2;
    RCS_date = index(RCS_date, ':') + 2;
    P("static int _reset;\n");
    P("static ");
    if (modelline) {
        Fprintf(fcout, "char *modelname = \"%s\";\n\n", modelline);
    } else {
        Fprintf(fcout, "char *modelname = \"\";\n\n");
    }
    Fflush(fcout); /* on certain internal errors partial output
                    * is helpful */
    P("static int error;\n");
    P("static ");
    P("int _ninits = 0;\n");
    P("static int _match_recurse=1;\n");
    P("static ");
    P("_modl_cleanup(){ _match_recurse=1;}\n");
    /*
     * many machinations are required to make the infinite number of
     * definitions involving _p in defs.h to be invisible to the user
     */
    /*
     * This one allows scop variables in functions which do not have the
     * p array as an argument
     */
    funcdec();
    Fflush(fcout);

    /*
     * translations of named blocks into functions, procedures, etc. Also
     * some special declarations used by some blocks
     */
    printlist(procfunc);
    Fflush(fcout);

    /* Initialization function must always be present */
    P("\nstatic initmodel() {\n  int _i; double _save;");
#if SIMSYS || HMODL
    P("\ninitmodel() {\n  int _i; double _save;");
#endif
    P("_ninits++;\n");
    P(saveindep); /*see solve.c; blank if not a time dependent process*/
    P("{\n");
    initstates();
    printlist(initfunc);
    P("\n}\n}\n");
    Fflush(fcout);

    /* generation of initmodel interface */
    P("\nstatic nrn_init(_pp, _ppd, _v) double *_pp, _v; Datum* _ppd; {\n");
    P(" _p = _pp; _ppvar = _ppd;\n");
    P(" v = _v;\n");
    printlist(get_ion_variables(1));
    P(" initmodel();\n");
    printlist(set_ion_variables(1));
    P("}\n");

    /* standard modl EQUATION without solve computes current */
    P("\nstatic double _nrn_current(_v) double _v;{double _current=0.;v=_v;{");
    if (currents->next != currents) {
        printlist(modelfunc);
    }
    ITERATE(q, currents) {
        Sprintf(buf, " _current += %s;\n", SYM(q)->name);
        P(buf);
    }
    P("\n} return _current;\n}\n");

    /* the neuron current also has to compute the dcurrent/dv as well
       as make sure all currents accumulated properly (currents list) */
    P("\nstatic double nrn_cur(_pp, _ppd, _pdiag, _v) double *_pp, *_pdiag, _v; Datum* _ppd; {\n");
    if (currents->next != currents) {
        P(" double _g, _rhs;\n");
        P(" _p = _pp; _ppvar = _ppd;\n");
        printlist(get_ion_variables(0));
        P(" _g = _nrn_current(_v + .001);\n");
        printlist(begin_dion_stmt());
        P(" _rhs = _nrn_current(_v);\n");
        printlist(end_dion_stmt(".001"));
        P(" _g = (_g - _rhs)/.001;\n");
        /* set the ion variable values */
        printlist(set_ion_variables(0));
        if (point_process) {
            P(" *_pdiag += _g * 1.e2/(_nd_area);\n return (_rhs - _g*_v) * 1.e2/(_nd_area);\n}\n");
        } else {
            P(" *_pdiag += _g;\n return _rhs - _g*_v;\n}\n");
        }
    } else {
        P(" return 0.;\n}\n");
    }

    /* nrnstate list contains the EQUATION solve statement so this
       advances states by dt */
    P("\nstatic nrn_state(_pp, _ppd, _v) double *_pp, _v; Datum* _ppd; {\n");
    if (nrnstate || currents->next == currents) {
        P(" double _break, _save;\n");
        P(" _p = _pp; _ppvar = _ppd;\n");
        P(" _break = t + .5*dt; _save = t; delta_t = dt;\n");
        P(" v=_v;\n{\n");
        printlist(get_ion_variables(1));
        if (nrnstate) {
            printlist(nrnstate);
        }
        if (currents->next == currents) {
            printlist(modelfunc);
        }
        printlist(set_ion_variables(1));
        P("\n}");
    }
    P("\n}\n");

    P("\nstatic terminal(){}\n");

    /* initlists() is called once to setup slist and dlist pointers */
    P("\nstatic _initlists() {\n");
    P(" int _i; static int _first = 1;\n");
    P("  if (!_first) return;\n");
    printlist(initlist);
    P("_first = 0;\n}\n");
}

/*
 * One of the things initmodel() must do is initialize all states to the
 * value of state0.  This generated code goes before any explicit initialize
 * code written by the user.
 */
static void initstates() {
    int i;
    Item* qs;
    Symbol* s;

    SYMITER_STAT {
        Sprintf(buf, "%s0", s->name);
        if (lookup(buf)) { /* if no constant associated
                            * with a state such as the
                            * ones automattically
                            * generated by SENS then
                            * there is no initialization
                            * line */
            if (s->subtype & ARRAY) {
                Fprintf(fcout,
                        " for (_i=0; _i<%d; _i++) %s[_i] = %s0;\n",
                        s->araydim,
                        s->name,
                        s->name);
            } else {
                Fprintf(fcout, "  %s = %s0;\n", s->name, s->name);
            }
        }
    }
}

/*
 * here is the only place as of 18-apr-89 where we don't explicitly know the
 * type of a list element
 */

void printlist(List* s) {
    Item* q;
    int newline = 0, indent = 0, i;

    /*
     * most of this is merely to decide where newlines and indentation
     * goes so that the .c file can be read if something goes wrong
     */
    if (!s) {
        return;
    }
    ITERATE(q, s) {
        if (q->itemtype == SYMBOL) {
            if (SYM(q)->type == SPECIAL) {
                switch (SYM(q)->subtype) {
                case SEMI:
                    newline = 1;
                    break;
                case BEGINBLK:
                    newline = 1;
                    indent++;
                    break;
                case ENDBLK:
                    newline = 1;
                    indent--;
                    break;
                }
            }
            Fprintf(fcout, " %s", SYM(q)->name);
        } else if (q->itemtype == VERBATIM) {
            Fprintf(fcout, "%s", STR(q));
        } else {
            Fprintf(fcout, " %s", STR(q));
        }
        if (newline) {
            newline = 0;
            Fprintf(fcout, "\n");
            for (i = 0; i < indent; i++) {
                Fprintf(fcout, "  ");
            }
        }
    }
}

static void funcdec() {
    int i;
    Symbol* s;
    List* qs;

    SYMITER(NAME) {
        /*EMPTY*/ /*maybe*/
        if (s->subtype & FUNCT) {
#define GLOBFUNCT 1
#if GLOBFUNCT
#else
            Fprintf(fcout, "static double %s();\n", s->name);
#endif
        }
        if (s->subtype & PROCED) {
            Fprintf(fcout, "static %s();\n", s->name);
        }
    }
}

void c_out_vectorize() {
    Item* q;
    extern int point_process;

    Fprintf(fcout, "/* Created by Language version: %s  of %s */\n", RCS_version, RCS_date);
    Fflush(fcout);
    /* things which must go first and most declarations */
    P("/* VECTORIZED */\n");
    P("#include <stdio.h>\n#include <math.h>\n#include \"mech_api.h\"\n");
    P("#undef PI\n");
    printlist(defs_list);
    printlist(firstlist);
    RCS_version[strlen(RCS_version) - 2] = '\0';
    RCS_date[strlen(RCS_date) - 11] = '\0';
    RCS_version = index(RCS_version, ':') + 2;
    RCS_date = index(RCS_date, ':') + 2;
    P("static int _reset;\n");
    if (modelline) {
        Fprintf(fcout, "static char *modelname = \"%s\";\n\n", modelline);
    } else {
        Fprintf(fcout, "static char *modelname = \"\";\n\n");
    }
    Fflush(fcout); /* on certain internal errors partial output
                    * is helpful */
    P("static int error;\n");
    P("static int _ninits = 0;\n");
    P("static int _match_recurse=1;\n");
    P("static _modl_cleanup(){ _match_recurse=1;}\n");

    funcdec();
    Fflush(fcout);

    /*
     * translations of named blocks into functions, procedures, etc. Also
     * some special declarations used by some blocks
     */
    printlist(procfunc);
    Fflush(fcout);

    /* Initialization function must always be present */

    P("\nstatic initmodel(_ix) int _ix; {\n  int _i; double _save;");

#if 0
	P("_initlists(_ix);\n");
	P("_ninits++;\n");
	P(saveindep); /*see solve.c; blank if not a time dependent process*/
#endif
    P("{\n");
    initstates();
    printlist(initfunc);
    P("\n}\n}\n");
    Fflush(fcout);

    /* generation of initmodel interface */
    P("\nstatic nrn_init(_count, _nodes, _data, _pdata)\n");
    P("	int _count; Node** _nodes; double** _data; Datum** _pdata;\n");
    P("{ int _ix;\n");
    P(" _p = _data; _ppvar = _pdata;\n");
    check_tables();
    P(cray_pragma());
    P("	for (_ix = 0; _ix < _count; ++_ix) {\n");
    P("		v = _nodes[_ix]->_v;\n");
    printlist(get_ion_variables(1));
    P("		initmodel(_ix);\n");
    printlist(set_ion_variables(1));
    P("	}\n");
    P("}\n");

    /* standard modl EQUATION without solve computes current */
    P("\nstatic double _nrn_current(_ix, _v) int _ix; double _v;{\n double _current=0.;v=_v;\n  {");
    if (currents->next != currents) {
        printlist(modelfunc);
    }
    ITERATE(q, currents) {
        Sprintf(buf, " _current += %s;\n", SYM(q)->name);
        P(buf);
    }
    P("\n} return _current;\n}\n");


    /* the neuron current also has to compute the dcurrent/dv as well
       as make sure all currents accumulated properly (currents list) */
    P("\nstatic double nrn_cur(_count, _nodes, _data, _pdata)\n");
    P("	int _count; Node** _nodes; double** _data; Datum** _pdata;\n");
    P("{\n");
    if (currents->next != currents) {
        P("int _ix;\n");
        P(" _p = _data; _ppvar = _pdata;\n");
        check_tables();

        P("\n#if METHOD3\n   if (_method3) {\n");
        /*--- reprduction of normal with minor changes ---*/
        P(cray_pragma());
        P("	for (_ix = 0; _ix < _count; ++_ix) {\n");
        P("		double _g, _rhs, _v;\n");
        P("		_v = _nodes[_ix]->_v;\n");
        printlist(get_ion_variables(0));
        P("		_g = _nrn_current(_ix, _v + .001);\n");
        printlist(begin_dion_stmt());
        P("\n		_rhs = _nrn_current(_ix, _v);\n");
        printlist(end_dion_stmt(".001"));
        P("		_g = (_g - _rhs)/.001;\n");
        /* set the ion variable values */
        printlist(set_ion_variables(0));
        if (point_process) {
            P("		_nodes[_ix]->_d += _g * 1.e2/(_nd_area);\n");
            P("		_nodes[_ix]->_rhs += (_g*_v - _rhs) * 1.e2/(_nd_area);\n");
        } else {
            P("		_nodes[_ix]->_thisnode._GC += _g;\n");
            P("		_nodes[_ix]->_thisnode._EC += (_g*_v - _rhs);\n");
        }
        P("	}\n");
        /*-------------*/
        P("\n   }else\n#endif\n   {\n");

        /*--- normal ---*/
        P(cray_pragma());
        P("	for (_ix = 0; _ix < _count; ++_ix) {\n");
        P("		double _g, _rhs, _v;\n");
        P("		_v = _nodes[_ix]->_v;\n");
        printlist(get_ion_variables(0));
        P("		_g = _nrn_current(_ix, _v + .001);\n");
        printlist(begin_dion_stmt());
        P("\n		_rhs = _nrn_current(_ix, _v);\n");
        printlist(end_dion_stmt(".001"));
        P("		_g = (_g - _rhs)/.001;\n");
        /* set the ion variable values */
        printlist(set_ion_variables(0));
        if (point_process) {
            P("		_nodes[_ix]->_d += _g * 1.e2/(_nd_area);\n");
            P("		_nodes[_ix]->_rhs += (_g*_v - _rhs) * 1.e2/(_nd_area);\n");
        } else {
            P("		_nodes[_ix]->_d += _g;\n");
            P("		_nodes[_ix]->_rhs += (_g*_v - _rhs);\n");
        }
        P("	}\n");
        /*------------*/

        P("   }\n");
    }
    P("	return 0.;\n}\n");

    /* nrnstate list contains the EQUATION solve statement so this
       advances states by dt */

    P("\nstatic nrn_state(_count, _nodes, _data, _pdata)\n");
    P("	int _count; Node** _nodes; double** _data; Datum** _pdata;\n");
    P("{\n");
    if (nrnstate || currents->next == currents) {
        P(" int _ix;\n");
        P(" double _break, _save;\n");
        P(" _p = _data; _ppvar = _pdata;\n");
        check_tables();
        P(" _break = t + .5*dt; _save = t; delta_t = dt;\n");

        P(cray_pragma());
        P("	for (_ix = 0; _ix < _count; ++_ix) {\n");
        P("		v = _nodes[_ix]->_v;\n");
        printlist(get_ion_variables(1));
        P("	}\n");

        P("\n{\n");
        if (nrnstate) {
            printlist(nrnstate);
        }
        P("}\n");
        P(cray_pragma());
        P("	for (_ix = 0; _ix < _count; ++_ix) {\n");
        if (currents->next == currents) {
            printlist(modelfunc);
        }
        printlist(set_ion_variables(1));
        P("	}\n");
    }
    P("}\n");
    Fflush(fcout);

    P("\nstatic terminal(){}\n");

    /* vectorized: data must have own copies of slist and dlist
       for now we don't vectorize if slist or dlist exists. Eventually
       must separate initialization of static things from vectorized
       things.
    */
    /* initlists() is called once to setup slist and dlist pointers */
    P("\nstatic _initlists(){\n");
    P(" int _i; static int _first = 1;\n");
    P("  if (!_first) return;\n");
    printlist(initlist);
    P("_first = 0;\n}\n");
}

void vectorize_substitute(Item* q, char* str) {
    if (!vectorize_replacements) {
        vectorize_replacements = newlist();
    }
    lappenditem(vectorize_replacements, q);
    lappendstr(vectorize_replacements, str);
}

void vectorize_do_substitute() {
    Item *q, *q1;
    if (vectorize_replacements) {
        ITERATE(q, vectorize_replacements) {
            q1 = ITM(q);
            q = q->next;
            replacstr(q1, STR(q));
        }
    }
}

char* cray_pragma() {
    static char buf[] =
        "\
\n#if _CRAY\
\n#pragma _CRI ivdep\
\n#endif\
\n";
    return buf;
}

