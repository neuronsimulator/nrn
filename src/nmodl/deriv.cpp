#include <../../nmodlconf.h>

#include "modl.h"
#include "symbol.h"
#include "../oc/nrnassrt.h"
#include <ctype.h>
#undef METHOD
#include "parse1.hpp"

static List* deriv_imp_list; /* list of derivative blocks that were
 translated in form suitable for the derivimplicit method */
static char Derivimplicit[] = "derivimplicit";

extern Symbol* indepsym;
extern List* indeplist;
extern int numlist;
int dtsav_for_nrn_state;
void copylist(List*, Item*);
List* massage_list_;
List* netrec_cnexp;

/* SmallBuf size */
#undef SB
#define SB 256

extern int vectorize;
extern int assert_threadsafe;
extern int thread_data_index;
extern List* thread_mem_init_list;
extern List* thread_cleanup_list;

extern char *cvode_deriv(), *cvode_eqnrhs();
extern Item* cvode_cnexp_solve;
void cvode_diffeq(Symbol* ds, Item* qbegin, Item* qend);
static List *cvode_diffeq_list, *cvode_eqn;
static int cvode_cnexp_possible;

void solv_diffeq(Item* qsol,
                 Symbol* fun,
                 Symbol* method,
                 int numeqn,
                 int listnum,
                 int steadystate,
                 int btype) {
    const char* maxerr_str;
    char dindepname[256];
    char deriv1_advance[256], deriv2_advance[256];
    char ssprefix[8];

    if (method && strcmp(method->name, "cnexp") == 0) {
        Sprintf(buf, " %s();\n", fun->name);
        replacstr(qsol, buf);
        Sprintf(buf, " %s(_threadargs_);\n", fun->name);
        vectorize_substitute(qsol, buf);
        return;
    }
    if (steadystate) {
        Strcpy(ssprefix, "_ss_");
    } else {
        Strcpy(ssprefix, "");
    }
    Sprintf(dindepname, "d%s", indepsym->name);
    if (fun->subtype & KINF) { /* translate the kinetic equations */
        /* can be standard integrator, full matrix advancec, or
           sparse matrix advance */
        /* at this time only sparse and standard exists */
        if (method->subtype & DERF) {
            kinetic_intmethod(fun, method->name);
        } else {
            kinetic_implicit(fun, dindepname, method->name);
        }
    }
    save_dt(qsol);
    if (method->subtype & DERF) {
        if (method->u.i == 1) { /* variable step method */
            maxerr_str = ", maxerr";
            IGNORE(ifnew_parminstall("maxerr", "1e-5", "", ""));
        } else {
            maxerr_str = "";
        }

        if (deriv_imp_list) { /* make sure deriv block translation matches method */
            Item* q;
            int found = 0;
            ITERATE(q, deriv_imp_list) {
                if (strcmp(STR(q), fun->name) == 0) {
                    found = 1;
                }
            }
            if ((strcmp(method->name, Derivimplicit) == 0) ^ (found == 1)) {
                diag(
                    "To use the derivimplicit method the SOLVE statement must precede the "
                    "DERIVATIVE block\n",
                    " and all SOLVEs using that block must use the derivimplicit method\n");
            }
            Sprintf(deriv1_advance, "_deriv%d_advance = 1;\n", listnum);
            Sprintf(deriv2_advance, "_deriv%d_advance = 0;\n", listnum);
            Sprintf(buf, "static int _deriv%d_advance = 0;\n", listnum);
            q = linsertstr(procfunc, buf);
            Sprintf(buf,
                    "\n#define _deriv%d_advance _thread[%d].literal_value<int>()\n"
                    "#define _dith%d %d\n"
                    "#define _recurse _thread[%d].literal_value<int>()\n"
                    "#define _newtonspace%d _thread[%d].literal_value<NewtonSpace*>()\n",
                    listnum,
                    thread_data_index,
                    listnum,
                    thread_data_index + 1,
                    thread_data_index + 2,
                    listnum,
                    thread_data_index + 3);
            vectorize_substitute(q, buf);
            Sprintf(buf, "  _thread[_dith%d] = new double[%d]{};\n", listnum, 2 * numeqn);
            lappendstr(thread_mem_init_list, buf);
            Sprintf(buf, "  _newtonspace%d = nrn_cons_newtonspace(%d);\n", listnum, numeqn);
            lappendstr(thread_mem_init_list, buf);
            Sprintf(buf, "  delete[] _thread[_dith%d].get<double*>();\n", listnum);
            lappendstr(thread_cleanup_list, buf);
            Sprintf(buf, "  nrn_destroy_newtonspace(_newtonspace%d);\n", listnum);
            lappendstr(thread_cleanup_list, buf);
            thread_data_index += 4;
        } else {
            Strcpy(deriv1_advance, "");
            Strcpy(deriv2_advance, "");
        }
        Sprintf(buf,
                "%s %s%s(_ninits, %d, _slist%d, _dlist%d, neuron::scopmath::row_view{_ml, _iml}, "
                "&%s, %s, %s, &_temp%d%s);\n%s",
                deriv1_advance,
                ssprefix,
                method->name,
                numeqn,
                listnum,
                listnum,
                indepsym->name,
                dindepname,
                fun->name,
                listnum,
                maxerr_str,
                deriv2_advance);
    } else {
        // examples of ssprefix + method->name: sparse, _ss_sparse
        Sprintf(buf,
                "%s%s(&_sparseobj%d, %d, _slist%d, _dlist%d, "
                "neuron::scopmath::row_view{_ml, _iml}, &%s, "
                "%s, %s, &_coef%d, _linmat%d);\n",
                ssprefix,
                method->name,
                listnum,
                numeqn,
                listnum,
                listnum,
                indepsym->name,
                dindepname,
                fun->name,
                listnum,
                listnum);
    }
    replacstr(qsol, buf);
    if (method->subtype & DERF) { /* derivimplicit */
        // derivimplicit_thread
        Sprintf(buf,
                "%s %s%s_thread(%d, _slist%d, _dlist%d, neuron::scopmath::row_view{_ml, _iml}, %s, "
                "_ml, _iml, _ppvar, _thread, _nt);\n%s",
                deriv1_advance,
                ssprefix,
                method->name,
                numeqn,
                listnum,
                listnum,
                fun->name,
                deriv2_advance);
        vectorize_substitute(qsol, buf);
    } else { /* kinetic */
        if (vectorize) {
            // sparse_thread, _ss_sparse_thread
            Sprintf(buf,
                    "%s%s_thread(&(_thread[_spth%d].literal_value<void*>()), %d, "
                    "_slist%d, _dlist%d, neuron::scopmath::row_view{_ml, _iml}, "
                    "&%s, %s, %s, _linmat%d, _threadargs_);\n",
                    ssprefix,
                    method->name,
                    listnum,
                    numeqn,
                    listnum,
                    listnum,
                    indepsym->name,
                    dindepname,
                    fun->name,
                    listnum);
            vectorize_substitute(qsol, buf);
        }
    }
    dtsav_for_nrn_state = 1;
    Sprintf(buf,
            "   if (secondorder) {\n"
            "    int _i;\n"
            "    for (_i = 0; _i < %d; ++_i) {\n"
            "      _ml->data(_iml, _slist%d[_i]) += dt*_ml->data(_iml, _dlist%d[_i]);\n"
            "    }}\n",
            numeqn,
            listnum,
            listnum);
    insertstr(qsol->next, buf);
}

/* addition of higher order derivatives
 User appearance:
Higher order derivatives are now allowed in DERIVATIVE blocks.
The number of primes following a variable is the order of the derivative.
For example, y'''' is a 4th order derivative.
The highest derivative of a state
must appear on the left hand side of the equation. Lower order derivatives
of a state (including the state itself) may appear on the right hand
side within an arbitrary expression.  It makes no sense, in general,
to have multiple equations involving the same state on the left hand side.
The most common usage will be equations of the form
    y'' = f(y, y', t)

Higher derivatives can be accessed in SCoP as
y'   Dy
y''  D2y
y''' D3y
etc.  Note that all derivatives except the highest derivative are themselves
states on an equal footing with y. E.G.  they can be explicitly declared within
a STATE block (and given START values), and they have associated initial value
constants.  Initial values default to 0. Here is a complicated example which
shows off the syntax (I have no idea if the solution exists).
    INDEPENDENT {t FROM 0 TO 1 WITH 1}
    STATE	{x y
         y' START 1
         x' START -10
        }
    DERIVATIVE d {
        x''' = y + 3*x''*y' + sin(x') + exp(x)
        y'' =  cos(x'' + y) - y'
    EQUATION {SOLVE d}
Note that we had to use Dx0 since x'0 is illegal. Also Dx0 has a
value of -10.

Implementation :
    In parse1.ypp we see that asgn: varname '=' expr and that
    varname gets marked if it is type PRIME.  With respect to
    higher order derivatives, therefore, only the highest order
    actually used in the equations in that block
    should be marked.  Furthermore these highest primes may or may not be
    dependent variables and the lesser primes must be created as states.
    We use a special iterator FORALL which returns each lesser order
    symbol given a dstate.

    The implicit equations of the form  DD2y = D3y do not have to
    be constructed and the dependent variables DD2y do not have to
    be created since the slist and dlist links can carry this
    information. Thus dlist[D2y] == slist[D3y] causes the integrators
    to do the right thing without, in fact, doing any arithmetic.
    We assume that the itegrators either save the *dlist values or
    update *slist using a loop running from 0 to N.  This is why
    the FORALL interator returns states starting with the base state
    (so that *slist[i+1] doesn't change until after *dlist[i] is used.
    (they point to the same value). In the case of a second order
    equation, the lists are:
        slist[0] = &y		dlist[0] = &Dy
        slist[1] = &Dy		dlist[1] = &D2y

    Process
    the code which unmarks the PRIMES and marks the corresponding state
    is inadequate since several states must be marked some of which
    are PRIME. For this reason we distinguish using a -1 to mark states
    for later counting.

    The array problem is solved by using lex to:
    when a PRIME is seen, check against the base state. If the base
    state doesn't exist don't worry. If the STATE is an array
    Then make PRIME an array of the same dimension.

    depinstall automattically creates a Dstate for each state
    passed to it as well as a state0 (optional). This is OK since
    they dissappear if unused.


*/

#define FORALL(state, dstate) for (state = init_forderiv(dstate); state; state = next_forderiv())
/* This returns all states of lower order than dstate in the order of
base state first. Will install PRIME if necessary.
*/
static Symbol* forderiv;     /* base state */
static char base_units[256]; /*base state units */
static int indx, maxindx;    /* current indx, and indx of dstate */

static Symbol* init_forderiv(Symbol* prime) {
    char name[256];
    double d1, d2;

    assert(prime->type == PRIME);
    /*extract maxindx and basename*/
    if (isdigit(prime->name[1])) { /* higher than 1 */
        if (sscanf(prime->name + 1, "%d%s", &maxindx, name) != 2) {
            diag("internal error in init_forderiv in init.c", (char*) 0);
        }
    } else {
        maxindx = 1;
        Strcpy(name, prime->name + 1);
    }
    forderiv = lookup(name);
    if (!forderiv || !(forderiv->subtype & STAT)) {
        diag(name, "must be declared as a state variable");
    }
    if (forderiv->araydim != prime->araydim) {
        Sprintf(buf, "%s and %s have different dimensions", forderiv->name, prime->name);
        diag(buf, (char*) 0);
    }
    indx = 0;
    decode_ustr(forderiv, &d1, &d2, base_units);
    return forderiv;
}

static char* name_forderiv(int i) {
    static char name[256];

    assert(i > 0 && forderiv);
    if (i > 1) {
        Sprintf(name, "D%d%s", i, forderiv->name);
    } else {
        Sprintf(name, "D%s", forderiv->name);
    }
    return name;
}

/* Scop can handle 's so we put the prime style names into the .var file.
We make use of the tools here to reconstruct the original prime name.
*/
char* reprime(Symbol* sym) {
    static char name[256];
    int i;
    char* cp;

    if (sym->type != PRIME) {
        Strcpy(name, sym->name);
        return name;
    }

    IGNORE(init_forderiv(sym));

    Strcpy(name, forderiv->name);
    cp = name + strlen(name);
    for (i = 0; i < maxindx; i++) {
        *cp++ = '\'';
    }
    *cp = '\0';
    return name;
}

static Symbol* next_forderiv() {
    char* name;
    Symbol* s;
    char units[SB];

    if (++indx >= maxindx) {
        return SYM0;
    }
    name = name_forderiv(indx);
    if ((s = lookup(name)) == SYM0) {
        s = install(name, PRIME);
        nrn_assert(snprintf(units, SB, "%s/%s^%d", base_units, STR(indeplist->prev), indx) < SB);
        depinstall(1, s, forderiv->araydim, "0", "1", units, ITEM0, 1, "");
        s->usage |= DEP;
    }
    if (s->araydim != forderiv->araydim) {
        diag(s->name, "must have same dimension as associated state");
    }
    if (!(s->subtype & STAT)) { /* Dstate changes to state */
        nrn_assert(snprintf(units, SB, "%s/%s^%d", base_units, STR(indeplist->prev), indx) < SB);
        s->subtype &= ~DEP;
        depinstall(1, s, forderiv->araydim, "0", "1", units, ITEM0, 1, "");
        depinstall(1, s, forderiv->araydim, "0", "1", units, ITEM0, 1, "");
        s->usage |= DEP;
    }
    return s;
}

/* mixed derivative and nonlinear equations */
/* Implementation:
   The main requirement is to distinguish between the states which
   have derivative specifications and the states which are solved
   for by the nonlinear equations.  We do this by having the
   left hand side derivative saved in a list instead of marking the
   variables in the used field. See deriv_used().  States seen in the
   nonlinear equations are marked as usual.  To leave only nonlinear
   states marked we then cast out any lesser state which is marked.
   (eg if y''=.. then y and y' cannot be states of the nonlinear equation).
   The former version also made use of state->used = -1 for match
   purposes.  We replace this usage with a list of states of the
   derivatives.  This means that the derivative block with respect to
   derivative equations no longer uses the used field.

   To avoid copying the block we (albeit resulting is somewhat poorer
   efficiency) we allow the block to call newton and pass itself as
   an argument. A flag tells the block if its call was by newton or by
   an integrator. My guess is this will still work with match, sens, and
   array states.
*/

/* derivative equations (and possibly some nonlinear equations) solved
   with an implicit method */
/* Implementation:
   Things are starting to get a little bit out of conceptual control.
   We make use of the mixed case, except that the number of nonlinear
   equations may be 0.  The substantive change is that now the number
   of equations is the sum of the derivatives and the nonlinears and the
   extra equations added into the block are of the form
    dlist2[++_counte] = Dstate - (state - statesave1[0])/dt;
   The administrative needs are that newton is called with the total number
   of equations and that we can match state and statesave. Notice that we
   already have two sets of slists floating around and one dlist,
   currently they are the
   slist and dlist for the derivative state and state'
   and the slist for the nonlinear states (the corresponding dlist is just
   the rhs of the equations).  Clearly, statesave should be associated
   with the derivative slist and will be in that order, then the slist for
   newton will be expanded by not resetting the used field.
   The biggest conceptual problem is how to generate the code at the time
   we handle the SOLVE since the actual numbers for the declarations
   of the newton slists depend on the method.
   Here, we assume a flag, deriv_implicit, which tells us which code to
   generate. Whether this means that we must look through the .mod file
   for all the SOLVE statements or whether all this stuff is saved for
   calling from the solve handler as in the kinetic block is not specified
   yet.
   For now, we demand that the SOLVE statement be seen first if it
   invokes the derivimplicit method. Otherwise modl generates an error
   message.
*/

void add_deriv_imp_list(char* name) {
    if (!deriv_imp_list) {
        deriv_imp_list = newlist();
    }
    Lappendstr(deriv_imp_list, name);
}

static List* deriv_used_list;  /* left hand side derivatives of diffeqs */
static List* deriv_state_list; /* states of the derivative equations */

void deriv_used(Symbol* s, Item* q1, Item* q2) /* q1, q2 are begin and end tokens for expression */
{
    if (!deriv_used_list) {
        deriv_used_list = newlist();
        deriv_state_list = newlist();
    }
    Lappendsym(deriv_used_list, s);
    if (!cvode_diffeq_list) {
        cvode_diffeq_list = newlist();
    }
    lappendsym(cvode_diffeq_list, s);
    lappenditem(cvode_diffeq_list, q1);
    lappenditem(cvode_diffeq_list, q2);
}

/* args are --- derivblk: DERIVATIVE NAME stmtlist '}' */
void massagederiv(Item* q1, Item* q2, Item* q3, Item* q4) {
    int count = 0, deriv_implicit, solve_seen;
    char units[SB];
    Item *qs, *q, *mixed_eqns(Item * q2, Item * q3, Item * q4);
    Symbol *s, *derfun, *state;

    /* to allow verification that definition after SOLVE */
    if (!massage_list_) {
        massage_list_ = newlist();
    }
    Lappendsym(massage_list_, SYM(q2));

    /* all this junk is still in the intoken list */
    Sprintf(buf, "static int %s(_internalthreadargsproto_);\n", SYM(q2)->name);
    Linsertstr(procfunc, buf);
    replacstr(q1, "\nstatic int");
    q = insertstr(q3, "() {_reset=0;\n");
    derfun = SYM(q2);
    vectorize_substitute(q,
                         "(_internalthreadargsproto_) {\n"
                         "  int _reset=0;\n"
                         "  int error = 0;\n");

    if (derfun->subtype & DERF && derfun->u.i) {
        diag("DERIVATIVE merging not implemented", (char*) 0);
    }

    /* check if we are to translate using derivimplicit method */
    deriv_implicit = 0;
    if (deriv_imp_list)
        ITERATE(q, deriv_imp_list) {
            if (strcmp(derfun->name, STR(q)) == 0) {
                deriv_implicit = 1;
                break;
            }
        }
    numlist++;
    derfun->u.i = numlist;
    derfun->subtype |= DERF;
    if (!deriv_used_list) {
        diag("No derivative equations in DERIVATIVE block", (char*) 0);
    }
    ITERATE(qs, deriv_used_list) {
        s = SYM(qs);
        if (!(s->subtype & DEP) && !(s->subtype & STAT)) {
            IGNORE(init_forderiv(s));
            nrn_assert(snprintf(units, SB, "%s/%s^%d", base_units, STR(indeplist->prev), maxindx) >
                       SB);
            depinstall(0, s, s->araydim, "0", "1", units, ITEM0, 0, "");
        }
        /* high order: make sure
           no lesser order is marked, and all lesser
           orders exist as STAT */
        FORALL(state, s) {
            if (state->type == PRIME) {
                ITERATE(q, deriv_used_list) if (state == SYM(q)) {
                    diag(state->name,
                         ": Since higher derivative is being used, this state \
is not allowed on the left hand side.");
                }
            }
            Lappendsym(deriv_state_list, state);
            slist_data(state, count, numlist);
            if (s->subtype & ARRAY) {
                int dim = s->araydim;
                Sprintf(buf,
                        "for(_i=0;_i<%d;_i++){_slist%d[%d+_i] = {%s_columnindex, _i};",
                        dim,
                        numlist,
                        count,
                        state->name);
                Lappendstr(initlist, buf);
                Sprintf(buf,
                        " _dlist%d[%d+_i] = {%s_columnindex, _i};}\n",
                        numlist,
                        count,
                        name_forderiv(indx + 1));
                Lappendstr(initlist, buf);
                count += dim;
            } else {
                Sprintf(buf, "_slist%d[%d] = {%s_columnindex, 0};", numlist, count, state->name);
                Lappendstr(initlist, buf);
                Sprintf(buf,
                        " _dlist%d[%d] = {%s_columnindex, 0};\n",
                        numlist,
                        count,
                        name_forderiv(indx + 1));
                Lappendstr(initlist, buf);
                count++;
            }
        }
    }
    if (count == 0) {
        diag("DERIVATIVE contains no derivatives", (char*) 0);
    }
    derfun->used = count;
    Sprintf(buf,
            "static neuron::container::field_index _slist%d[%d], _dlist%d[%d];\n",
            numlist,
            count,
            numlist,
            count);
    Linsertstr(procfunc, buf);

    Lappendstr(procfunc, "\n/*CVODE*/\n");
    Sprintf(buf, "static int _ode_spec%d", numlist);
    Lappendstr(procfunc, buf);
    {
        Item* qq = procfunc->prev;
        copyitems(q1->next, q4, procfunc->prev);
        vectorize_substitute(qq->next, "(_internalthreadargsproto_) {int _reset = 0;");
        vectorize_scan_for_func(qq->next, procfunc);
    }
    lappendstr(procfunc, "return _reset;\n}\n");

    /* don't emit _ode_matsol if the user has defined cvodematsol */
    if (!lookup("cvodematsol")) {
        Item* qq;
        Item* qextra = q1->next->next->next->next;
        Sprintf(buf, "static int _ode_matsol%d", numlist);
        Lappendstr(procfunc, buf);
        vectorize_substitute(lappendstr(procfunc, "() {\n"), "(_internalthreadargsproto_) {\n");
        qq = procfunc->next;
        cvode_cnexp_possible = 1;
        ITERATE(q, cvode_diffeq_list) {
            Symbol* s;
            Item *q1, *q2;
            s = SYM(q);
            q = q->next;
            q1 = ITM(q);
            q = q->next;
            q2 = ITM(q);
#if 1
            while (qextra != q1) { /* must first have any intervening statements */
                switch (qextra->itemtype) {
                case STRING:
                    Lappendstr(procfunc, STR(qextra));
                    break;
                case SYMBOL:
                    Lappendsym(procfunc, SYM(qextra));
                    break;
                }
                qextra = qextra->next;
            }
#endif
            cvode_diffeq(s, q1, q2);
            qextra = q2->next;
        }
#if 1
        /* if we are not at the end, there is more extra */
        while (qextra != q4) {
            switch (qextra->itemtype) {
            case STRING:
                Lappendstr(procfunc, STR(qextra));
                break;
            case SYMBOL:
                Lappendsym(procfunc, SYM(qextra));
                break;
            }
            qextra = qextra->next;
        }
#endif
        Lappendstr(procfunc, " return 0;\n}\n");
        vectorize_scan_for_func(qq, procfunc);
    }

    Lappendstr(procfunc, "/*END CVODE*/\n");
    if (cvode_cnexp_solve && cvode_cnexp_success(q1, q4)) {
        freelist(&deriv_used_list);
        freelist(&deriv_state_list);
        return;
    }
    if (deriv_implicit) {
        Sprintf(buf,
                "static double _savstate%d[%d], *_temp%d = _savstate%d;\n",
                numlist,
                count,
                numlist,
                numlist);
        q = linsertstr(procfunc, buf);
        vectorize_substitute(q, "");
    } else {
        Sprintf(buf, "static double *_temp%d;\n", numlist);
        Linsertstr(procfunc, buf);
    }
    movelist(q1, q4, procfunc);
    Lappendstr(procfunc, "return _reset;}\n");
    /* reset used field for any states that may appear in
    nonlinear equations which should not be solved for. */
    ITERATE(q, deriv_used_list) {
        SYM(q)->used = 0;
    }
    if (deriv_implicit) {
        Symbol* sp;
        ITERATE(q, deriv_state_list) {
            SYM(q)->used = 1;
        }
        Sprintf(buf,
                "{int _id; for(_id=0; _id < %d; _id++) {\n\
if (_deriv%d_advance) {\n",
                count,
                numlist);
        Insertstr(q4, buf);
        sp = install("D", STRING);
        sp->araydim = count;
        q = insertsym(q4, sp);
        eqnqueue(q);
        Sprintf(buf,
                "_ml->data(_iml, _dlist%d[_id]) - (_ml->data(_iml, _slist%d[_id]) - "
                "_savstate%d[_id])/d%s;\n",
                numlist,
                numlist,
                numlist,
                indepsym->name);
        Insertstr(q4, buf);
        Sprintf(
            buf,
            "}else{\n_dlist%d[++_counte] = _ml->data(_iml, _slist%d[_id]) - _savstate%d[_id];}}}\n",
            numlist + 1,
            numlist,
            numlist);
        Insertstr(q4, buf);
    } else {
        ITERATE(q, deriv_state_list) {
            SYM(q)->used = 0;
        }
    }
    /* if there are also nonlinear equations, put in the newton call,
    create the proper lists and fill in the left hand side of each
    equation. */
    q = mixed_eqns(q2, q3, q4); /* numlist now incremented */
    if (deriv_implicit) {
        Sprintf(buf,
                "for(int _id=0; _id < %d; _id++) {\n"
                "  _savstate%d[_id] = _ml->data(_iml, _slist%d[_id]);\n"
                "}\n",
                count,
                derfun->u.i,
                derfun->u.i);
        Insertstr(q, buf);
    }

    freelist(&deriv_used_list);
    freelist(&deriv_state_list);
}


void copylist(List* l, Item* i) /* copy list l before item i */
{
    Item* q;

    ITERATE(q, l) {
        switch (q->itemtype) {
        case STRING:
            Insertstr(i, STR(q));
            break;
        case SYMBOL:
            Insertsym(i, SYM(q));
            break;
        default:
            /*SUPPRESS 622*/
            assert(0);
        }
    }
}

void copyitems(Item* q1, Item* q2, Item* qdest) /* copy items before item */
{
    Item* q;
    for (q = q2; q != q1; q = q->prev) {
        switch (q->itemtype) {
        case STRING:
        case VERBATIM:
            Linsertstr(qdest, STR(q));
            break;
        case SYMBOL:
            Linsertsym(qdest, SYM(q));
            break;
        default:
            /*SUPPRESS 622*/
            assert(0);
        }
    }
}

static int cvode_linear_diffeq(Symbol* ds, Symbol* s, Item* qbegin, Item* qend) {
    char* c;
    List* tlst;
    Item* q;
    tlst = newlist();
    for (q = qbegin; q != qend->next; q = q->next) {
        switch (q->itemtype) {
        case SYMBOL:
            lappendsym(tlst, SYM(q));
            break;
        case STRING:
            lappendstr(tlst, STR(q));
            break;
        default:
            cvode_cnexp_possible = 0;
            return 0;
        }
    }
    cvode_parse(s, tlst);
    freelist(&tlst);
    c = cvode_deriv();
    if (!cvode_eqn) {
        cvode_eqn = newlist();
    }
    lappendsym(cvode_eqn, s);
    lappendstr(cvode_eqn, cvode_deriv());
    lappendstr(cvode_eqn, cvode_eqnrhs());
    if (c) {
        lappendstr(procfunc, c);
        lappendstr(procfunc, "))");
        return 1;
    }
    cvode_cnexp_possible = 0;
    return 0;
}

/* DState symbol, begin, and end of expression */
void cvode_diffeq(Symbol* ds, Item* qbegin, Item* qend) {
    /* try first the assumption of linear. If not, then use numerical diff*/
    Symbol* s;
    Item* q;

    /* get state symbol */
    sscanf(ds->name, "D%s", buf);
    s = lookup(buf);
    assert(s);

    /* ds/(1. - dt*( */
    Lappendsym(procfunc, ds);
    Lappendstr(procfunc, " / (1. - dt*(");
    if (cvode_linear_diffeq(ds, s, qbegin, qend)) {
        return;
    }
    /* ((expr(s+.001))-(expr))/.001; */
    Lappendstr(procfunc, "((");
    for (q = qbegin; q != qend->next; q = q->next) {
        switch (q->itemtype) {
        case STRING:
            Lappendstr(procfunc, STR(q));
            break;
        case SYMBOL:
            if (SYM(q) == s) {
                Lappendstr(procfunc, "(");
                Lappendsym(procfunc, s);
                Lappendstr(procfunc, " + .001)");
            } else {
                Lappendsym(procfunc, SYM(q));
            }
            break;
        default:
            assert(0);
        }
    }
    Lappendstr(procfunc, ") - (");
    for (q = qbegin; q != qend->next; q = q->next) {
        switch (q->itemtype) {
        case STRING:
            Lappendstr(procfunc, STR(q));
            break;
        case SYMBOL:
            Lappendsym(procfunc, SYM(q));
            break;
        default:
            assert(0);
        }
    }
    Lappendstr(procfunc, " )) / .001 ))");
}

/*
the cnexp method was requested but the symbol in the solve queue was
changed to derivimplicit and cvode_cnexp_solve holds a pointer to
the solvq item (1st of three).
The 0=f(state) equations have already been solved and the rhs for
each has been saved. So we know if the translation is possible.
*/
int cvode_cnexp_success(Item* q1, Item* q2) {
    Item *q, *q3, *q4, *qeq;
    if (cvode_cnexp_possible) {
        /* convert Method to nullptr and the type of the block to
           PROCEDURE */
        SYM(cvode_cnexp_solve->next)->name = stralloc("cnexp", 0);
        remove(deriv_imp_list->next);

        /* replace the Dstate = f(state) equations */
        qeq = cvode_eqn->next;
        ITERATE(q, cvode_diffeq_list) {
            Symbol* s;
            Item *q1, *q2;
            char *a, *b;
            s = SYM(qeq);
            qeq = qeq->next;
            a = STR(qeq);
            qeq = qeq->next;
            b = STR(qeq);
            qeq = qeq->next;
            q = q->next;
            q1 = ITM(q);
            q = q->next;
            q2 = ITM(q);
            if (!netrec_cnexp) {
                netrec_cnexp = newlist();
            }
            lappendsym(netrec_cnexp, s);
            if (strcmp(a, "0.0") == 0) {
                assert(b[strlen(b) - 9] == '/');
                b[strlen(b) - 9] = '\0';
                Sprintf(buf, " __primary -= 0.5*dt*( %s )", b);
                lappendstr(netrec_cnexp, buf);
                Sprintf(buf, " %s = %s - dt*(%s)", s->name, s->name, b);
            } else {
                Sprintf(buf,
                        " __primary += ( 1. - exp( 0.5*dt*( %s ) ) )*( %s - __primary )",
                        a,
                        b);
                lappendstr(netrec_cnexp, buf);
                Sprintf(buf,
                        " %s = %s + (1. - exp(dt*(%s)))*(%s - %s)",
                        s->name,
                        s->name,
                        a,
                        b,
                        s->name);
            }
            insertstr(q2->next, buf);
            q2 = q2->next;
            for (q3 = q1->prev->prev; q3 != q2; q3 = q4) {
                q4 = q3->next;
                remove(q3);
            }
        }

        lappendstr(procfunc, "static int");
        {
            Item* qq = procfunc->prev;
            copyitems(q1, q2, procfunc->prev);
            /* more or less redundant with massagederiv */
            vectorize_substitute(qq->next->next, "(_internalthreadargsproto_) {");
            vectorize_scan_for_func(qq->next->next, procfunc);
        }
        lappendstr(procfunc, " return 0;\n}\n");
        return 1;
    }
    fprintf(stderr, "Could not translate using cnexp method; using derivimplicit\n");
    return 0;
}
