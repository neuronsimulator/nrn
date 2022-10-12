/*

NET_RECEIVE discontinuities for STATE variables are correct for the
variable time step method but, for better numerical accuracy, should be
adjusted for the fixed step method. That is, for secondorder=2, STATE
variable discontinuites should be calculated as the change in state
at t+dt/2 instead of the full weight discontinuity at t. For the implicit
method secondorder=0, the change in state should also be calculated at
t+dt/2. For the vast majority of cases, DERIVATIVE block with METHOD cnexp,
the performance cost is minimal and the
adjustment factor is just fac = 1 / (1 + rate*dt/2) where
rate = ds'/ds. Note that if the choice of cnexp is valid, s'
depends only on s and is linear. dt is just replaced
by dt/2.  Problems with this in general are of 3 kinds.
1) If all parameters of the equations are constant, best performance
would be obtained by precalculating the factors. It is possible to
generalize this precalculation to linear coupled DERIVATIVE and KINETIC
equations. However, precalculation is not valid if the parameters change
during a simulation (e.g. voltage sensitive rates are common, though
not in synapse models).
2) Sometimes in DERIVATIVE blocks, state independent terms involving
functions or several factors use local variable assignment to simplify
the cnexp formula. Of course, the local value calculation that provides
the value of rate, is not available in the NET_RECEIVE block. Here again,
one can imagine voltage sensitive rate functions and also some equations
written in linearized uncoupled form so that cnexp can be used.
3) Coupled and or nonlinear equations should properly use a jacobian
formulation since a discontinuity of a single state at t may distribute
among several states when evaluated at t+dt.

At present we do not take advantage of precalculation of factors.

The standard case is cnexp with rate computable within the NET_RECEIVE block.
I.e rate involves only RANGE variables and no LOCAL variables. Note that
rate is already calculated in solve.c and is given in the cnexp_rate list
as a string. That has to be parsed to check that it satisfies our
requirements for computability in the NET_RECEIVE block
Although best performance for the cnexp case is comes from the rate factor,
cnexp in general allows s' = a + b*s which requires a different factor
if a is nonzero or b is zero. For simplicity, we use the full exponetial
expression to evaluate the discontinuity of s(t) at t+dt/2.

For the general case, i.e. the standard case is insufficient, we evaluate
the dstate/dt vector for DERIVATIVE and KINETIC blocks (ie. via a call
to the model instance cvode specfic _ode_spec1 function)
at the current state and with a small change to the discontinuous state
and use ds'/ds to calculate the relevant change to the several states.
The typical case here is A -> B -> 0 where A is discontinuous on
receipt of an event. Note that no Jacobian inversion is envisioned so
that only dsi/dt that is affected by sj will change si.

*/

#include <../../nmodlconf.h>

#include <stdlib.h>
#include <string.h>
#include "modl.h"
#include "parse1.hpp"

extern int vectorize;
extern int cvode_not_allowed;
extern List* netrec_cnexp; /* STATE symbol and cnexp expr pairs */
static List* info;
static void general_discon_adjust(Item* varname, Item* equal, Item* expr, Item* lastok);
int netrec_state_count; /* 10*numeqn + listnum */
int netrec_need_v;      /* if 1 then need it for the general case */
int netrec_need_thread; /* if 1 then _cvode_sparse_thread needs proper _thread */

/* store statement info */
void netrec_asgn(Item* varname, Item* equal, Item* expr, Item* lastok) {
    Symbol* sym;
    sym = SYM(varname);
    if (sym->type == NAME && sym->subtype & STAT) {
        /* STATE discontinuity adjusted if fixed step method */
        if (!info) {
            info = newlist();
        }
        lappenditem(info, varname);
        lappenditem(info, equal);
        lappenditem(info, expr);
        lappenditem(info, lastok);
    }
}

static void netrec_discon1(Item* varname, Item* equal, Item* expr, Item* lastok) {
    Item* q;
    Symbol* sym = SYM(varname);
    char* cnexp = NULL;
    int case_cnexp = 1;
#if 0
  printf("NET_RECEIVE discontinuity for %s\n", sym->name);
#endif
    if (netrec_cnexp)
        ITERATE(q, netrec_cnexp) {
            if (SYM(q) == sym) {
                cnexp = STR(q->next);
                break;
            }
            q = q->next;
        }

    if (cnexp) {
        /* if there is a local variable involved then use the general case */
        if (strstr(cnexp, " _l")) { /* interior begin token */
            case_cnexp = 0;
        }
    } else {
        /* if not processed by cnexp then use the general case */
        case_cnexp = 0;
    }

#if 0
  printf("cnexp is |%s|\n", cnexp ? cnexp : "NULL");
  if (cnexp && !case_cnexp) {
    printf("because of local variable, use the general case\n");
  }else if (!cnexp) {
    printf("because not cnexp, use the general case\n");
  }
  for (q = varname; q != lastok; q = q->next) {
    debugprintitem(q);
  }
#endif

    if (case_cnexp) {
        char* state = items_as_string(varname, equal);
        char* e = items_as_string(expr, lastok->prev);
        Sprintf(buf,
                "  if (nrn_netrec_state_adjust && !cvode_active_){\n"
                "    /* discon state adjustment for cnexp case (rate uses no local variable) */\n"
                "    double __state = %s;\n"
                "    double __primary = (%s) - __state;\n"
                "    %s;\n"
                "    %s += __primary;\n"
                "  } else {\n",
                state,
                e,
                cnexp,
                state);
        insertstr(varname, buf);
        insertstr(lastok, "  }\n");
        free(state);
        free(e);
    } else { /* the general case */
        general_discon_adjust(varname, equal, expr, lastok);
    }
}

void netrec_discon() {
    Item* q;
    if (info) {
        ITERATE(q, info) {
            Item* varname = ITM(q);
            q = q->next;
            Item* equal = ITM(q);
            q = q->next;
            Item* expr = ITM(q);
            q = q->next;
            Item* lastok = ITM(q)->next->next;
            netrec_discon1(varname, equal, expr, lastok);
        }
    }
}

static void general_discon_adjust(Item* varname, Item* equal, Item* expr, Item* lastok) {
    int listnum = netrec_state_count % 10;
    int neq = netrec_state_count / 10;
    int i;
    Symbol* sym = SYM(varname);
    int sindex;
    if (cvode_not_allowed) {
        fprintf(stderr, "Notice: %s discontinuity adjustment not available.\n", sym->name);
        return;
    }
    sindex = slist_search(listnum, sym);

#if 0
  printf("general_discon_adjust listnum=%d sindex=%d neq=%d\n", listnum, sindex, neq);
#endif

    char* state = items_as_string(varname, equal);
    char* e = items_as_string(expr, lastok->prev);
    char* needv;
    char* needthread;
    if (netrec_need_v) {
        needv = strdup("    v = NODEV(_pnt->node);\n");
    } else {
        needv = strdup("");
    }
    if (netrec_need_thread && vectorize) {
        needthread = strdup(
            "#if NRN_VECTORIZED\n    _thread = _nt->_ml_list[_mechtype]->_thread;\n#endif\n");
    } else {
        needthread = strdup("");
    }
    Sprintf(buf,
            "  if (nrn_netrec_state_adjust && !cvode_active_){\n"
            "    /* discon state adjustment for general derivimplicit and KINETIC case */\n"
            "    int __i, __neq = %d;\n"
            "    double __state = %s;\n"
            "    double __primary_delta = (%s) - __state;\n"
            "    double __dtsav = dt;\n"
            "    for (__i = 0; __i < __neq; ++__i) {\n"
            "      _ml->data(_iml, _dlist%d[__i]) = 0.0;\n"
            "    }\n"
            "    _ml->data(_iml, _dlist%d[%d]) = __primary_delta;\n"
            "    dt *= 0.5;\n"
            "%s%s"
            "    _ode_matsol_instance%d(_threadargs_);\n"
            "    dt = __dtsav;\n"
            "    for (__i = 0; __i < __neq; ++__i) {\n"
            "      _ml->data(_iml, _slist%d[__i]) += _ml->data(_iml, _dlist%d[__i]);\n"
            "    }\n"
            "  } else {\n",
            neq,
            state,
            e,
            listnum,
            listnum,
            sindex,
            needv,
            needthread,
            listnum,
            listnum,
            listnum);
    insertstr(varname, buf);
    insertstr(lastok, "  }\n");
    free(state);
    free(e);
    free(needv);
    free(needthread);
}
