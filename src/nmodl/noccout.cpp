#include <../../nmodlconf.h>

/* print the .c file from the lists */
#include "modl.h"
#include "parse1.hpp"
#include "symbol.h"

#define CACHEVEC 1

extern char* nmodl_version_;

#define P(arg) fputs(arg, fcout)
List *procfunc, *initfunc, *modelfunc, *termfunc, *initlist, *firstlist;
/* firstlist gets statements that must go before anything else */

List* nrnstate;
extern List *currents, *set_ion_variables(int), *get_ion_variables(int);
extern List *begin_dion_stmt(), *end_dion_stmt(const char*);
extern List* conductance_;
static void conductance_cout();

extern List* defs_list;
extern const char* saveindep;
char* modelline;
extern int brkpnt_exists;
extern int artificial_cell;
extern int net_receive_;
extern int debugging_;
extern int point_process;
extern int dtsav_for_nrn_state;

extern Symbol* cvode_nrn_cur_solve_;
extern Symbol* cvode_nrn_current_solve_;
extern List* state_discon_list_;

/* VECTORIZE has not been optional for years. We leave the define there but */
/* we no longer update the #else clauses. */
extern int vectorize;
static List* vectorize_replacements; /* pairs of item pointer, strings */
extern int electrode_current;        /* 1 means we should watch out for extracellular
                           and handle it correctly */

#if __TURBOC__ || SYSV || VMS
#define index strchr
#endif

static void initstates();
static void funcdec();

static void ext_vdef() {
    if (artificial_cell) {
        return;
    }
    if (electrode_current) {
        P("#if EXTRACELLULAR\n");
        P(" _nd = _ml->_nodelist[_iml];\n");
        P(" if (_nd->_extnode) {\n");
        P("    _v = NODEV(_nd) +_nd->_extnode->_v[0];\n");
        P(" }else\n");
        P("#endif\n");
        P(" {\n");
#if CACHEVEC == 0
        P("    _v = NODEV(_nd);\n");
#else
        P("#if CACHEVEC\n");
        P("  if (use_cachevec) {\n");
        P("    _v = VEC_V(_ni[_iml]);\n");
        P("  }else\n");
        P("#endif\n");
        P("  {\n");
        P("    _nd = _ml->_nodelist[_iml];\n");
        P("    _v = NODEV(_nd);\n");
        P("  }\n");
#endif

        P(" }\n");
    } else {
#if CACHEVEC == 0
        P(" _nd = _ml->_nodelist[_iml];\n");
        P(" _v = NODEV(_nd);\n");
#else
        P("#if CACHEVEC\n");
        P("  if (use_cachevec) {\n");
        P("    _v = VEC_V(_ni[_iml]);\n");
        P("  }else\n");
        P("#endif\n");
        P("  {\n");
        P("    _nd = _ml->_nodelist[_iml];\n");
        P("    _v = NODEV(_nd);\n");
        P("  }\n");
#endif
    }
}


/* when vectorize = 0 */
void c_out() {
    Item* q;

    Fprintf(fcout, "/* Created by Language version: %s */\n", nmodl_version_);
    Fflush(fcout);

    if (vectorize) {
        vectorize_do_substitute();
        kin_vect2(); /* heh, heh.. bet you can't guess what this is */
        c_out_vectorize();
        return;
    }
    P("/* NOT VECTORIZED */\n#define NRN_VECTORIZED 0\n");
    Fflush(fcout);
    /* things which must go first and most declarations */
    P("#include <stdio.h>\n#include <stdlib.h>\n#include <math.h>\n#include \"mech_api.h\"\n");
    P("#undef PI\n");
    P("#define nil 0\n");
    P("#include \"md1redef.h\"\n");
    P("#include \"section.h\"\n");
    P("#include \"nrniv_mf.h\"\n");
    P("#include \"md2redef.h\"\n");

    printlist(defs_list);
    printlist(firstlist);
    P("static int _reset;\n");
    P("static ");
    if (modelline) {
        Fprintf(fcout, "const char *modelname = \"%s\";\n\n", modelline);
    } else {
        Fprintf(fcout, "const char *modelname = \"\";\n\n");
    }
    Fflush(fcout); /* on certain internal errors partial output
                    * is helpful */
    P("static int error;\n");
    P("static ");
    P("int _ninits = 0;\n");
    P("static int _match_recurse=1;\n");
    P("static void ");
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
    P("\nstatic void initmodel() {\n  int _i; double _save;");
    P("_ninits++;\n");
    P(saveindep); /*see solve.c; blank if not a time dependent process*/
    P("{\n");
    initstates();
    printlist(initfunc);
    P("\n}\n}\n");
    Fflush(fcout);

    /* generation of initmodel interface */
    P("\nstatic void nrn_init(NrnThread* _nt, Memb_list* _ml, int _type){\n");
    P("Node *_nd; double _v; int* _ni; int _iml, _cntml;\n");
    P("#if CACHEVEC\n");
    P("    _ni = _ml->_nodeindices;\n");
    P("#endif\n");
    P("_cntml = _ml->_nodecount;\n");
    P("for (_iml = 0; _iml < _cntml; ++_iml) {\n");
    P(" _p = _ml->_data[_iml]; _ppvar = _ml->_pdata[_iml];\n");
    if (debugging_ && net_receive_) {
        P(" _tsav = -1e20;\n");
    }
    if (!artificial_cell) {
        ext_vdef();
    }
    if (!artificial_cell) {
        P(" v = _v;\n");
    }
    printlist(get_ion_variables(1));
    P(" initmodel();\n");
    printlist(set_ion_variables(2));
    P("}}\n");

    /* standard modl EQUATION without solve computes current */
    P("\nstatic double _nrn_current(double _v){double _current=0.;v=_v;");
    if (cvode_nrn_current_solve_) {
        fprintf(fcout, "if (cvode_active_) { %s(); }\n", cvode_nrn_current_solve_->name);
    }
    P("{");
    if (currents->next != currents) {
        printlist(modelfunc);
    }
    ITERATE(q, currents) {
        Sprintf(buf, " _current += %s;\n", SYM(q)->name);
        P(buf);
    }
    P("\n} return _current;\n}\n");

    /* For the classic BREAKPOINT block, the neuron current also has to compute the dcurrent/dv as
       well as make sure all currents accumulated properly (currents list) */

    if (brkpnt_exists) {
        P("\nstatic void nrn_cur(NrnThread* _nt, Memb_list* _ml, int _type){\n");
        P("Node *_nd; int* _ni; double _rhs, _v; int _iml, _cntml;\n");
        P("#if CACHEVEC\n");
        P("    _ni = _ml->_nodeindices;\n");
        P("#endif\n");
        P("_cntml = _ml->_nodecount;\n");
        P("for (_iml = 0; _iml < _cntml; ++_iml) {\n");
        P(" _p = _ml->_data[_iml]; _ppvar = _ml->_pdata[_iml];\n");
        ext_vdef();
        if (currents->next != currents) {
            printlist(get_ion_variables(0));
            cvode_rw_cur(buf);
            P(buf);
        }
        if (cvode_nrn_cur_solve_) {
            fprintf(fcout, "if (cvode_active_) { %s(); }\n", cvode_nrn_cur_solve_->name);
        }
        if (currents->next != currents) {
            P(" _g = _nrn_current(_v + .001);\n");
            printlist(begin_dion_stmt());
            if (state_discon_list_) {
                P(" state_discon_flag_ = 1; _rhs = _nrn_current(_v); state_discon_flag_ = 0;\n");
            } else {
                P(" _rhs = _nrn_current(_v);\n");
            }
            printlist(end_dion_stmt(".001"));
            P(" _g = (_g - _rhs)/.001;\n");
            /* set the ion variable values */
            printlist(set_ion_variables(0));
            if (point_process) {
                P(" _g *=  1.e2/(_nd_area);\n");
                P(" _rhs *= 1.e2/(_nd_area);\n");
            }
            if (electrode_current) {
#if CACHEVEC == 0
                P("	NODERHS(_nd) += _rhs;\n");
#else
                P("#if CACHEVEC\n");
                P("  if (use_cachevec) {\n");
                P("	VEC_RHS(_ni[_iml]) += _rhs;\n");
                P("  }else\n");
                P("#endif\n");
                P("  {\n");
                P("	NODERHS(_nd) += _rhs;\n");
                P("  }\n");
                P("  if (_nt->_nrn_fast_imem) { _nt->_nrn_fast_imem->_nrn_sav_rhs[_ni[_iml]] += "
                  "_rhs; }\n");
#endif
                P("#if EXTRACELLULAR\n");
                P(" if (_nd->_extnode) {\n");
                P("   *_nd->_extnode->_rhs[0] += _rhs;\n");
                P(" }\n");
                P("#endif\n");
            } else {
#if CACHEVEC == 0
                P("	NODERHS(_nd) -= _rhs;\n");
#else
                P("#if CACHEVEC\n");
                P("  if (use_cachevec) {\n");
                P("	VEC_RHS(_ni[_iml]) -= _rhs;\n");
                P("  }else\n");
                P("#endif\n");
                P("  {\n");
                P("	NODERHS(_nd) -= _rhs;\n");
                P("  }\n");
#endif
            }
        }
        P(" \n}}\n");

        /* for the classic breakpoint block, nrn_cur computed the conductance, _g,
           and now the jacobian calculation merely returns that */
        P("\nstatic void nrn_jacob(NrnThread* _nt, Memb_list* _ml, int _type){\n");
        P("Node *_nd; int* _ni; int _iml, _cntml;\n");
        P("#if CACHEVEC\n");
        P("    _ni = _ml->_nodeindices;\n");
        P("#endif\n");
        P("_cntml = _ml->_nodecount;\n");
        P("for (_iml = 0; _iml < _cntml; ++_iml) {\n");
        P(" _p = _ml->_data[_iml];\n");
        if (electrode_current) {
            P(" _nd = _ml->_nodelist[_iml];\n");
#if CACHEVEC == 0
            P("	NODED(_nd) -= _g;\n");
#else
            P("#if CACHEVEC\n");
            P("  if (use_cachevec) {\n");
            P("	VEC_D(_ni[_iml]) -= _g;\n");
            P("  }else\n");
            P("#endif\n");
            P("  {\n");
            P("	NODED(_nd) -= _g;\n");
            P("  }\n");
            P("  if (_nt->_nrn_fast_imem) { _nt->_nrn_fast_imem->_nrn_sav_d[_ni[_iml]] -= _g; }\n");
#endif
            P("#if EXTRACELLULAR\n");
            P(" if (_nd->_extnode) {\n");
            P("   *_nd->_extnode->_d[0] += _g;\n");
            P(" }\n");
            P("#endif\n");
        } else {
#if CACHEVEC == 0
            P("	NODED(_nd) += _g;\n");
#else
            P("#if CACHEVEC\n");
            P("  if (use_cachevec) {\n");
            P("	VEC_D(_ni[_iml]) += _g;\n");
            P("  }else\n");
            P("#endif\n");
            P("  {\n");
            P("     _nd = _ml->_nodelist[_iml];\n");
            P("	NODED(_nd) += _g;\n");
            P("  }\n");
#endif
        }
        P(" \n}}\n");
    }

    /* nrnstate list contains the EQUATION solve statement so this
       advances states by dt */
    P("\nstatic void nrn_state(NrnThread* _nt, Memb_list* _ml, int _type){\n");
    if (nrnstate || currents->next == currents) {
        P("Node *_nd; double _v = 0.0; int* _ni; int _iml, _cntml;\n");
        if (dtsav_for_nrn_state && nrnstate) {
            P("double _dtsav = dt;\n"
              "if (secondorder) { dt *= 0.5; }\n");
        }
        P("#if CACHEVEC\n");
        P("    _ni = _ml->_nodeindices;\n");
        P("#endif\n");
        P("_cntml = _ml->_nodecount;\n");
        P("for (_iml = 0; _iml < _cntml; ++_iml) {\n");
        P(" _p = _ml->_data[_iml]; _ppvar = _ml->_pdata[_iml];\n");
        P(" _nd = _ml->_nodelist[_iml];\n");
        ext_vdef();
        P(" v=_v;\n{\n");
        printlist(get_ion_variables(1));
        if (nrnstate) {
            printlist(nrnstate);
        }
        if (currents->next == currents) {
            printlist(modelfunc);
        }
        printlist(set_ion_variables(1));
        P("}}\n");
        if (dtsav_for_nrn_state && nrnstate) {
            P(" dt = _dtsav;");
        }
    }
    P("\n}\n");

    P("\nstatic void terminal(){}\n");

    /* initlists() is called once to setup slist and dlist pointers */
    P("\nstatic void _initlists() {\n");
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
        /* ioni and iono should not have initialization lines */
#define IONCONC 010000
        if (s->nrntype & IONCONC) {
            continue;
        }
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

static int newline, indent;

void printitem(Item* q) {
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
        verbatim_adjust(STR(q));
    } else if (q->itemtype == ITEM) {
        printitem(ITM(q));
    } else {
        Fprintf(fcout, " %s", STR(q));
    }
}

void debugprintitem(Item* q) {
    if (q->itemtype == SYMBOL) {
        printf("SYM %s\n", SYM(q)->name);
    } else if (q->itemtype == VERBATIM) {
        printf("VERB %s\n", STR(q));
    } else if (q->itemtype == ITEM) {
        printf("ITM ");
        debugprintitem(ITM(q));
    } else {
        printf("STR %s\n", STR(q));
    }
}

/* does not include q2 */
char* items_as_string(Item* q1, Item* q2) {
    Item* q;
    buf[0] = '\0';
    for (q = q1; q != q2; q = q->next) {
        if (buf[0] != '\0') {
            strcat(buf, " ");
        }
        if (q->itemtype == SYMBOL) {
            strcat(buf, SYM(q)->name);
        } else if (q->itemtype == STRING) {
            strcat(buf, STR(q));
        } else {
            assert(0);
        }
    }
    return strdup(buf);
}

void printlist(List* s) {
    Item* q;
    int i;
    newline = 0, indent = 0;
    /*
     * most of this is merely to decide where newlines and indentation
     * goes so that the .c file can be read if something goes wrong
     */
    if (!s) {
        return;
    }
    ITERATE(q, s) {
        printitem(q);
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
    int j, narg, more;

    SYMITER(NAME) {
        more = 0;
        if (s->subtype & PROCED) {
            Fprintf(fcout, "static int %s(", s->name);
            more = 1;
        }
        if (more) {
            narg = s->varnum;
            if (vectorize) {
                if (narg) {
                    Fprintf(fcout, "_threadargsprotocomma_ ");
                } else {
                    Fprintf(fcout, "_threadargsproto_");
                }
            }
            /*loop over argcount and add ,double */
            if (narg > 0) {
                Fprintf(fcout, "double");
            }
            for (j = 1; j < narg; ++j) {
                Fprintf(fcout, ", double");
            }
            Fprintf(fcout, ");\n");
        }
    }
}

/* when vectorize = 1 */
void c_out_vectorize() {
    Item* q;

    /* things which must go first and most declarations */
    P("/* VECTORIZED */\n#define NRN_VECTORIZED 1\n");
    P("#include <stdio.h>\n#include <stdlib.h>\n#include <math.h>\n#include \"mech_api.h\"\n");
    P("#undef PI\n");
    P("#define nil 0\n");
    P("#include \"md1redef.h\"\n");
    P("#include \"section.h\"\n");
    P("#include \"nrniv_mf.h\"\n");
    P("#include \"md2redef.h\"\n");
    printlist(defs_list);
    printlist(firstlist);
    P("static int _reset;\n");
    if (modelline) {
        Fprintf(fcout, "static const char *modelname = \"%s\";\n\n", modelline);
    } else {
        Fprintf(fcout, "static const char *modelname = \"\";\n\n");
    }
    Fflush(fcout); /* on certain internal errors partial output
                    * is helpful */
    P("static int error;\n");
    P("static int _ninits = 0;\n");
    P("static int _match_recurse=1;\n");
    P("static void _modl_cleanup(){ _match_recurse=1;}\n");

    funcdec();
    Fflush(fcout);

    /*
     * translations of named blocks into functions, procedures, etc. Also
     * some special declarations used by some blocks
     */
    printlist(procfunc);
    Fflush(fcout);

    /* Initialization function must always be present */

    P("\nstatic void initmodel(double* _p, Datum* _ppvar, Datum* _thread, NrnThread* _nt) {\n  int "
      "_i; double _save;");
    P("{\n");
    initstates();
    printlist(initfunc);
    P("\n}\n}\n");
    Fflush(fcout);

    /* generation of initmodel interface */
    P("\nstatic void nrn_init(NrnThread* _nt, Memb_list* _ml, int _type){\n");
    P("double* _p; Datum* _ppvar; Datum* _thread;\n");
    P("Node *_nd; double _v; int* _ni; int _iml, _cntml;\n");
    P("#if CACHEVEC\n");
    P("    _ni = _ml->_nodeindices;\n");
    P("#endif\n");
    P("_cntml = _ml->_nodecount;\n");
    P("_thread = _ml->_thread;\n");
    /*check_tables();*/
    P("for (_iml = 0; _iml < _cntml; ++_iml) {\n");
    P(" _p = _ml->_data[_iml]; _ppvar = _ml->_pdata[_iml];\n");
    check_tables();
    if (debugging_ && net_receive_) {
        P(" _tsav = -1e20;\n");
    }
    if (!artificial_cell) {
        ext_vdef();
    }
    if (!artificial_cell) {
        P(" v = _v;\n");
    }
    printlist(get_ion_variables(1));
    P(" initmodel(_p, _ppvar, _thread, _nt);\n");
    printlist(set_ion_variables(2));
    P("}\n");
    P("}\n");

    /* standard modl EQUATION without solve computes current */
    if (!conductance_) {
        P("\nstatic double _nrn_current(double* _p, Datum* _ppvar, Datum* _thread, NrnThread* _nt, "
          "double _v){double _current=0.;v=_v;");
        if (cvode_nrn_current_solve_) {
            fprintf(fcout,
                    "if (cvode_active_) { %s(_p, _ppvar, _thread, _nt); }\n",
                    cvode_nrn_current_solve_->name);
        }
        P("{");
        if (currents->next != currents) {
            printlist(modelfunc);
        }
        ITERATE(q, currents) {
            if (SYM(q) != breakpoint_current(SYM(q))) {
                diag(
                    "current can only be LOCAL in a BREAKPOINT if CONDUCTANCE statements are "
                    "used. ",
                    SYM(q)->name);
            }
            Sprintf(buf, " _current += %s;\n", SYM(q)->name);
            P(buf);
        }
        P("\n} return _current;\n}\n");
    }

    /* For the classic BREAKPOINT block, the neuron current also has to compute the dcurrent/dv as
       well as make sure all currents accumulated properly (currents list) */

    if (brkpnt_exists) {
        P("\nstatic void nrn_cur(NrnThread* _nt, Memb_list* _ml, int _type) {\n");
        P("double* _p; Datum* _ppvar; Datum* _thread;\n");
        P("Node *_nd; int* _ni; double _rhs, _v; int _iml, _cntml;\n");
        P("#if CACHEVEC\n");
        P("    _ni = _ml->_nodeindices;\n");
        P("#endif\n");
        P("_cntml = _ml->_nodecount;\n");
        P("_thread = _ml->_thread;\n");
        P("for (_iml = 0; _iml < _cntml; ++_iml) {\n");
        P(" _p = _ml->_data[_iml]; _ppvar = _ml->_pdata[_iml];\n");
        ext_vdef();
        if (currents->next != currents) {
            printlist(get_ion_variables(0));
            cvode_rw_cur(buf);
            P(buf);
        }
        if (cvode_nrn_cur_solve_) {
            fprintf(fcout,
                    "if (cvode_active_) { %s(_p, _ppvar, _thread, _nt); }\n",
                    cvode_nrn_cur_solve_->name);
        }
        if (currents->next != currents) {
            if (conductance_) {
                P(" {\n");
                conductance_cout();
                printlist(set_ion_variables(0));
                P(" }\n");
            } else {
                P(" _g = _nrn_current(_p, _ppvar, _thread, _nt, _v + .001);\n");
                printlist(begin_dion_stmt());
                if (state_discon_list_) {
                    P(" state_discon_flag_ = 1; _rhs = _nrn_current(_v); state_discon_flag_ = "
                      "0;\n");
                } else {
                    P(" _rhs = _nrn_current(_p, _ppvar, _thread, _nt, _v);\n");
                }
                printlist(end_dion_stmt(".001"));
                P(" _g = (_g - _rhs)/.001;\n");
                /* set the ion variable values */
                printlist(set_ion_variables(0));
            } /* end of not conductance */
            if (point_process) {
                P(" _g *=  1.e2/(_nd_area);\n");
                P(" _rhs *= 1.e2/(_nd_area);\n");
            }
            if (electrode_current) {
#if CACHEVEC == 0
                P("	NODERHS(_nd) += _rhs;\n");
#else
                P("#if CACHEVEC\n");
                P("  if (use_cachevec) {\n");
                P("	VEC_RHS(_ni[_iml]) += _rhs;\n");
                P("  }else\n");
                P("#endif\n");
                P("  {\n");
                P("	NODERHS(_nd) += _rhs;\n");
                P("  }\n");
                P("  if (_nt->_nrn_fast_imem) { _nt->_nrn_fast_imem->_nrn_sav_rhs[_ni[_iml]] += "
                  "_rhs; }\n");
#endif
                P("#if EXTRACELLULAR\n");
                P(" if (_nd->_extnode) {\n");
                P("   *_nd->_extnode->_rhs[0] += _rhs;\n");
                P(" }\n");
                P("#endif\n");
            } else {
#if CACHEVEC == 0
                P("	NODERHS(_nd) -= _rhs;\n");
#else
                P("#if CACHEVEC\n");
                P("  if (use_cachevec) {\n");
                P("	VEC_RHS(_ni[_iml]) -= _rhs;\n");
                P("  }else\n");
                P("#endif\n");
                P("  {\n");
                P("	NODERHS(_nd) -= _rhs;\n");
                P("  }\n");
#endif
            }
        }
        P(" \n}\n");
        P(" \n}\n");
        /* for the classic breakpoint block, nrn_cur computed the conductance, _g,
           and now the jacobian calculation merely returns that */
        P("\nstatic void nrn_jacob(NrnThread* _nt, Memb_list* _ml, int _type) {\n");
        P("double* _p; Datum* _ppvar; Datum* _thread;\n");
        P("Node *_nd; int* _ni; int _iml, _cntml;\n");
        P("#if CACHEVEC\n");
        P("    _ni = _ml->_nodeindices;\n");
        P("#endif\n");
        P("_cntml = _ml->_nodecount;\n");
        P("_thread = _ml->_thread;\n");
        P("for (_iml = 0; _iml < _cntml; ++_iml) {\n");
        P(" _p = _ml->_data[_iml];\n");
        if (electrode_current) {
            P(" _nd = _ml->_nodelist[_iml];\n");
#if CACHEVEC == 0
            P("	NODED(_nd) -= _g;\n");
#else
            P("#if CACHEVEC\n");
            P("  if (use_cachevec) {\n");
            P("	VEC_D(_ni[_iml]) -= _g;\n");
            P("  }else\n");
            P("#endif\n");
            P("  {\n");
            P("	NODED(_nd) -= _g;\n");
            P("  }\n");
            P("  if (_nt->_nrn_fast_imem) { _nt->_nrn_fast_imem->_nrn_sav_d[_ni[_iml]] -= _g; }\n");
#endif
            P("#if EXTRACELLULAR\n");
            P(" if (_nd->_extnode) {\n");
            P("   *_nd->_extnode->_d[0] += _g;\n");
            P(" }\n");
            P("#endif\n");
        } else {
#if CACHEVEC == 0
            P("	NODED(_nd) += _g;\n");
#else
            P("#if CACHEVEC\n");
            P("  if (use_cachevec) {\n");
            P("	VEC_D(_ni[_iml]) += _g;\n");
            P("  }else\n");
            P("#endif\n");
            P("  {\n");
            P("     _nd = _ml->_nodelist[_iml];\n");
            P("	NODED(_nd) += _g;\n");
            P("  }\n");
#endif
        }
        P(" \n}\n");
        P(" \n}\n");
    }

    /* nrnstate list contains the EQUATION solve statement so this
       advances states by dt */
    P("\nstatic void nrn_state(NrnThread* _nt, Memb_list* _ml, int _type) {\n");
    if (nrnstate || currents->next == currents) {
        P("double* _p; Datum* _ppvar; Datum* _thread;\n");
        P("Node *_nd; double _v = 0.0; int* _ni; int _iml, _cntml;\n");
        if (dtsav_for_nrn_state && nrnstate) {
            P("double _dtsav = dt;\n"
              "if (secondorder) { dt *= 0.5; }\n");
        }
        P("#if CACHEVEC\n");
        P("    _ni = _ml->_nodeindices;\n");
        P("#endif\n");
        P("_cntml = _ml->_nodecount;\n");
        P("_thread = _ml->_thread;\n");
        P("for (_iml = 0; _iml < _cntml; ++_iml) {\n");
        P(" _p = _ml->_data[_iml]; _ppvar = _ml->_pdata[_iml];\n");
        P(" _nd = _ml->_nodelist[_iml];\n");
        ext_vdef();
        P(" v=_v;\n{\n");
        printlist(get_ion_variables(1));
        if (nrnstate) {
            printlist(nrnstate);
        }
        if (currents->next == currents) {
            printlist(modelfunc);
        }
        printlist(set_ion_variables(1));
        P("}}\n");
        if (dtsav_for_nrn_state && nrnstate) {
            P(" dt = _dtsav;");
        }
    }
    P("\n}\n");

    P("\nstatic void terminal(){}\n");

    /* vectorized: data must have own copies of slist and dlist
       for now we don't vectorize if slist or dlist exists. Eventually
       must separate initialization of static things from vectorized
       things.
    */
    /* initlists() is called once to setup slist and dlist pointers */
    P("\nstatic void _initlists(){\n");
    P(" double _x; double* _p = &_x;\n");
    P(" int _i; static int _first = 1;\n");
    P("  if (!_first) return;\n");
    printlist(initlist);
    P("_first = 0;\n}\n");
}

void vectorize_substitute(Item* q, const char* str) {
    if (!vectorize_replacements) {
        vectorize_replacements = newlist();
    }
    lappenditem(vectorize_replacements, q);
    lappendstr(vectorize_replacements, str);
}

Item* vectorize_replacement_item(Item* q) {
    Item* q1;
    if (vectorize_replacements) {
        ITERATE(q1, vectorize_replacements) {
            if (ITM(q1) == q) {
                return q1->next;
            }
        }
    }
    return (Item*) 0;
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

static void conductance_cout() {
    int i = 0;
    Item* q;
    List* m;

    /* replace v with _v */
    m = newlist();
    ITERATE(q, modelfunc) {
        if (q->itemtype == SYMBOL) {
            if (strcmp(SYM(q)->name, "v") == 0) {
                lappendstr(m, "_v");
            } else {
                lappendsym(m, SYM(q));
            }
        } else if (q->itemtype == STRING) {
            lappendstr(m, STR(q));
        } else {
            diag("modelfunc contains item which is not a SYMBOL or STRING", (char*) 0);
        }
    }
    /* eliminate first { */
    ITERATE(q, m) {
        if (q->itemtype == SYMBOL) {
            if (strcmp(SYM(q)->name, "{") == 0) {
                remove(q);
                break;
            }
        }
    }
    /* eliminate last } */
    for (q = m->prev; q != m; q = q->prev) {
        if (q->itemtype == SYMBOL) {
            if (strcmp(SYM(q)->name, "}") == 0) {
                remove(q);
                break;
            }
        }
    }

    printlist(m);

    ITERATE(q, currents) {
        if (i == 0) {
            sprintf(buf, "  _rhs = %s", breakpoint_current(SYM(q))->name);
        } else {
            sprintf(buf, " + %s", breakpoint_current(SYM(q))->name);
        }
        P(buf);
        i += 1;
    }
    if (i > 0) {
        P(";\n");
    }

    i = 0;
    ITERATE(q, conductance_) {
        if (i == 0) {
            sprintf(buf, "  _g = %s", SYM(q)->name);
        } else {
            sprintf(buf, " + %s", SYM(q)->name);
        }
        P(buf);
        i += 1;
        q = q->next;
    }
    if (i > 0) {
        P(";\n");
    }

    ITERATE(q, conductance_) {
        if (SYM(q->next)) {
            sprintf(buf, "  _ion_di%sdv += %s", SYM(q->next)->name, SYM(q)->name);
            P(buf);
            if (point_process) {
                P("* 1.e2/(_nd_area)");
            }
            P(";\n");
        }
        q = q->next;
    }
}
