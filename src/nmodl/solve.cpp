#include <../../nmodlconf.h>
/* /local/src/master/nrn/src/nmodl/solve.c,v 4.4 1998/08/20 21:07:34 hines Exp */

#include "modl.h"
#include "parse1.hpp"
#include "symbol.h"

#include <cstdlib>

/* make it an error if 2 solve statements are called on a single call to
model() */
extern List* indeplist;
Symbol* deltaindep = SYM0;
extern Symbol* indepsym;
extern char* current_line();
extern List* massage_list_;
extern List* nrnstate;
extern int vectorize;
extern int netrec_state_count;
extern int netrec_need_thread;
Item* cvode_cnexp_solve;
Symbol* cvode_nrn_cur_solve_;
Symbol* cvode_nrn_current_solve_;

void whileloop(Item*, long, int);
void check_ss_consist(Item*);

/* Need list of solve statements. We impress the
general list structure to handle it.  The element is a pointer to an
item which is the first item in the statement sequence in another list.
*/
static List* solvq; /* list of the solve statement locations */
int numlist = 0;    /* number of slist's */

void solvequeue(Item* qName, Item* qMethod, int blocktype) /*solve NAME [using METHOD]*/
/* qMethod = nullptr means method wasn't there */
{
    /* the solvq list is organized in groups of an item element
    followed by the method symbol (null if default to be used) */
    /* The itemtype field of the first is used to carry the blocktype*/
    /* SOLVE and METHOD method are deleted */
    if (!solvq) {
        solvq = newlist();
    }
    /* if the blocktype is equation then move the solve statement to
        the nrnstate function.  Everything else stays in the
        model function to be used as the nrncurrent function */
    if (blocktype == BREAKPOINT) {
        if (!nrnstate) {
            nrnstate = newlist();
        }
        Lappendstr(nrnstate, "{");
        if (qMethod) {
            movelist(qName->prev, qMethod, nrnstate);
        } else {
            movelist(qName->prev, qName, nrnstate);
        }
        Lappendstr(nrnstate, "}");
    }
    /* verify that the block defintion for this SOLVE has not yet been seen */
    if (massage_list_) {
        Item* lq;
        ITERATE(lq, massage_list_) {
            if (strcmp(SYM(lq)->name, SYM(qName)->name) == 0) {
                diag("The SOLVE statement must be before the DERIVATIVE block for ", SYM(lq)->name);
            }
        }
    }
    Item* lq = lappendsym(solvq, SYM0);
    ITM(lq) = qName;
    lq->itemtype = blocktype;
    /* handle STEADYSTATE option */
    if (qName->next->itemtype == SYMBOL && strcmp("STEADYSTATE", SYM(qName->next)->name) == 0) {
        lq->itemtype = -blocktype; /* gets put back below */
    }
    if (qMethod) {
        Lappendsym(solvq, SYM(qMethod));
        if (strcmp(SYM(qMethod)->name, "derivimplicit") == 0) {
            add_deriv_imp_list(SYM(qName)->name);
        }
        if (strcmp(SYM(qMethod)->name, "cnexp") == 0) {
            SYM(qMethod)->name = stralloc("derivimplicit", SYM(qMethod)->name);
            add_deriv_imp_list(SYM(qName)->name);
            cvode_cnexp_solve = lq;
        }
        remove(qMethod->prev);
        remove(qMethod);
    } else {
        Lappendsym(solvq, SYM0);
    }
    remove(qName->prev);

    List* errstmt = newlist();
    lq = lappendsym(solvq, SYM0);
    LST(lq) = errstmt;
    Sprintf(buf,
            "if(error){\n"
            "  std_cerr_stream << \"%s\\n\";\n"
            "  std_cerr_stream << _ml << ' ' << _iml << '\\n';\n"
            "  abort_run(error);\n"
            "}\n",
            current_line());
    insertstr(errstmt, buf);
}

/* go through the solvq list and construct the proper while loop and calls*/
void solvhandler() {
    Item *lq, *qsol, *follow;
    List* errstmt;
    Symbol *fun, *method;
    int numeqn, listnum, btype, steadystate;
    int cvodemethod_;

    if (!solvq)
        solvq = newlist();
    init_disc_vars();    /*why not here?*/
    ITERATE(lq, solvq) { /* remember it goes by 3's */
        steadystate = 0;
        btype = lq->itemtype;
        if (btype < 0) {
            btype = lq->itemtype = -btype;
            steadystate = 1;
            check_ss_consist(lq);
        }
        qsol = ITM(lq);
        lq = lq->next;
        method = SYM(lq);
        cvodemethod_ = 0;
        if (method && strcmp(method->name, "after_cvode") == 0) {
            method = (Symbol*) 0;
            lq->element.sym = (Symbol*) 0;
            cvodemethod_ = 1;
        }
        if (method && strcmp(method->name, "cvode_t") == 0) {
            method = (Symbol*) 0;
            lq->element.sym = (Symbol*) 0;
            cvodemethod_ = 2;
        }
        if (method && strcmp(method->name, "cvode_v") == 0) {
            method = (Symbol*) 0;
            lq->element.sym = (Symbol*) 0;
            cvodemethod_ = 3;
        }
        lq = lq->next;
        errstmt = LST(lq);
        /* err stmt handling assumes qsol->next is where it goes. */
        fun = SYM(qsol);
        numeqn = fun->used;
        listnum = fun->u.i;
        if (btype == BREAKPOINT && (fun->subtype == DERF || fun->subtype == KINF)) {
            netrec_state_count = numeqn * 10 + listnum;
            netrec_need_thread = 1;
        }
        follow = qsol->next; /* where p[0] gets updated */
        /* Check consistency of method and block type */
        if (method && !(method->subtype & fun->subtype)) {
            Sprintf(buf, "Method %s can't be used with Block %s", method->name, fun->name);
            diag(buf, (char*) 0);
        }

        switch (fun->subtype) {
        case DERF:
            if (method == SYM0) {
                method = lookup("derivimplicit");
            }
            if (btype == BREAKPOINT && !steadystate) {
                /* derivatives recalculated after while loop */
                if (strcmp(method->name, "cnexp") != 0 &&
                    strcmp(method->name, "derivimplicit") != 0 &&
                    strcmp(method->name, "euler") != 0) {
                    fprintf(stderr,
                            "Notice: %s is not thread safe. Complain to Hines\n",
                            method->name);
                    vectorize = 0;
                    Sprintf(buf, " %s();\n", fun->name);
                    Insertstr(follow, buf);
                }
                cvode_interface(fun, listnum, numeqn);
            }
            if (btype == BREAKPOINT)
                whileloop(qsol, (long) DERF, steadystate);
            solv_diffeq(qsol, fun, method, numeqn, listnum, steadystate, btype);
            break;
        case KINF:
            if (method == SYM0) {
                method = lookup("_advance");
            }
            if (btype == BREAKPOINT && (method->subtype & DERF)) {
                fprintf(
                    stderr,
                    "Notice: KINETIC is thread safe only with METHOD sparse. Complain to Hines\n");
                vectorize = 0;
                /* derivatives recalculated after while loop */
                Sprintf(buf, " %s();\n", fun->name);
                Insertstr(follow, buf);
            }
            if (btype == BREAKPOINT) {
                whileloop(qsol, (long) DERF, steadystate);
                if (strcmp(method->name, "sparse") == 0) {
                    cvode_interface(fun, listnum, numeqn);
                    cvode_kinetic(qsol, fun, numeqn, listnum);
                }
            }
            solv_diffeq(qsol, fun, method, numeqn, listnum, steadystate, btype);
            break;
        case NLINF:
            fprintf(stderr, "Notice: NONLINEAR is not thread safe.\n");
            vectorize = 0;
            if (method == SYM0) {
                method = lookup("newton");
            }
            solv_nonlin(qsol, fun, method, numeqn, listnum);
            break;
        case LINF:
            fprintf(stderr, "Notice: LINEAR is not thread safe.\n");
            vectorize = 0;
            if (method == SYM0) {
                method = lookup("simeq");
            }
            solv_lineq(qsol, fun, method, numeqn, listnum);
            break;
        case DISCF:
            fprintf(stderr, "Notice: DISCRETE is not thread safe.\n");
            vectorize = 0;
            if (btype == BREAKPOINT)
                whileloop(qsol, (long) DISCRETE, 0);
            Sprintf(buf, "0; %s += d%s; %s();\n", indepsym->name, indepsym->name, fun->name);
            replacstr(qsol, buf);
            break;
#if 1 /* really wanted? */
        case PROCED:
            if (btype == BREAKPOINT) {
                whileloop(qsol, (long) DERF, 0);
                if (cvodemethod_ == 1) { /*after_cvode*/
                    cvode_interface(fun, listnum, 0);
                }
                if (cvodemethod_ == 2) { /*cvode_t*/
                    cvode_interface(fun, listnum, 0);
                    insertstr(qsol, "if (!cvode_active_)");
                    cvode_nrn_cur_solve_ = fun;
                    linsertstr(procfunc, "extern int cvode_active_;\n");
                }
                if (cvodemethod_ == 3) { /*cvode_t_v*/
                    cvode_interface(fun, listnum, 0);
                    insertstr(qsol, "if (!cvode_active_)");
                    cvode_nrn_current_solve_ = fun;
                    linsertstr(procfunc, "extern int cvode_active_;\n");
                }
            }
            Sprintf(buf, " %s();\n", fun->name);
            replacstr(qsol, buf);
            Sprintf(buf, "{ %s(_threadargs_); }\n", fun->name);
            vectorize_substitute(qsol, buf);
            break;
#endif
        default:
            diag("Illegal or unimplemented SOLVE type: ", fun->name);
            break;
        }
        if (btype == BREAKPOINT) {
            cvode_valid();
        }
        /* add the error check */
        Insertstr(qsol, "error =");
        move(errstmt->next, errstmt->prev, qsol->next);
        if (errstmt->next == errstmt->prev) {
            vectorize_substitute(qsol->next, "");
            vectorize_substitute(qsol->prev, "");
        } else {
            fprintf(stderr, "Notice: SOLVE with ERROR is not thread safe.\n");
            vectorize = 0;
        }
        freelist(&errstmt);
        /* under all circumstances, on return from model,
         p[0] = current indepvar */
        /* obviously ok if indepvar is original; if it has been changed
        away from time
        then _sav_indep will be reset to starting value of original when
        initmodel is called on every call to model */
    }
}

void save_dt(Item* q) /* save and restore the value of indepvar */
{
    /*ARGSUSED*/
#if 0 /* integrators no longer increment time */
	static int first=1;

	if (first) {
		first = 0;
		Linsertstr(procfunc, "double _savlocal;\n");
	}
	Sprintf(buf, "_savlocal = %s;\n", indepsym->name);
	Insertstr(q, buf);
	Sprintf(buf, "%s = _savlocal;\n", indepsym->name);
	Insertstr(q->next, buf);
#endif
}

const char* saveindep = "";

void whileloop(Item* qsol, long type, int ss) {
    /* no solve statement except this is allowed to
    change the indepvar. Time passed to integrators
    must therefore be saved and restored. */
    /* Note that when the user changes the independent variable within Scop,
    the while loop still uses the original independent variable. In this case
    1) _break must be set to the entry value of the original independent variable
    2) initmodel() must be called after each entry to model()
    3) in initmodel() we are very careful not to destroy the entry value of
       time-- on exit it has its original value.  Note that _sav_indep gets
       any value of time that is set within initmodel().
    */
    /* executing more that one for loop in a single call to model() is an error
    which is trapped in scop */
    static int called = 0, firstderf = 1;
    const char* cp = 0;

    switch (type) {
    case DERF:
    case DISCRETE:
        if (firstderf) { /* time dependent process so create a deltastep */
            double d[3];
            int i;
            Item* q;
            char sval[256];
            Sprintf(buf, "delta_%s", indepsym->name);
            for (i = 0, q = indeplist->next; i < 3; i++, q = q->next) {
                d[i] = atof(STR(q));
            }
            if (type == DERF) {
                Sprintf(sval, "%g", (d[1] - d[0]) / d[2]);
            } else if (type == DISCRETE) {
                Sprintf(sval, "1.0");
            }
            deltaindep = ifnew_parminstall(buf, sval, "", "");
            firstderf = 0;
        }
        if (type == DERF) {
            cp = "dt";
        } else if (type == DISCRETE) {
            cp = "0.0";
        }
        if (ss) {
            return;
        }
        break;
    default:
        /*SUPPRESS 622*/
        assert(0);
    }

    if (called) {
        Fprintf(stderr,
                "Warning: More than one integrating SOLVE statement in an \
BREAKPOINT block.\nThe simulation will be incorrect if more than one is used \
at a time.\n");
    }
    if (!called) {
        /* fix up initmodel as per 3) above.
        In cout.c _save is declared */
        Sprintf(buf, " _save = %s;\n %s = 0.0;\n", indepsym->name, indepsym->name);
        saveindep = stralloc(buf, (char*) 0);
        /* Assert no more additions to initfunc involving
        the value of time */
        Sprintf(buf, " _sav_indep = %s; %s = _save;\n", indepsym->name, indepsym->name);
        Lappendstr(initfunc, buf);
        vectorize_substitute(initfunc->prev, "");
    }
    called++;
}

/* steady state consistency means that KINETIC blocks whenever solved must
invoke same integrator (default is advance) and DERIVATIVE blocks whenever
solved must invoke the derivimplicit method.
*/
void check_ss_consist(Item* qchk) {
    Item* q;
    Symbol *fun, *method;

    ITERATE(q, solvq) {
        fun = SYM(ITM(q));
        if (fun == SYM(qchk)) {
            method = SYM(q->next);
            switch (q->itemtype) {
            case DERF:
                if (!method || strcmp("derivimplicit", method->name) != 0) {
                    diag(
                        "STEADYSTATE requires all SOLVE's of this DERIVATIVE block to use the\n\
`derivimplicit' method:",
                        fun->name);
                }
                break;
            case KINF:
                if (SYM(qchk->next) != method || (method && strcmp("sparse", method->name) != 0)) {
                    diag(
                        "STEADYSTATE requires all SOLVE's of this KINETIC block to use the\n\
same method (`advance' or `sparse'). :",
                        fun->name);
                }
                break;
            default:
                diag("STEADYSTATE only valid for SOLVEing a KINETIC or DERIVATIVE block:",
                     fun->name);
                break;
            }
        }
        q = q->next->next;
    }
}
