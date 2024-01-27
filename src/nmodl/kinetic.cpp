#include <../../nmodlconf.h>

#define Glass 1

#if Glass
/*
We have found and corrected an MODL bug.

The problem is that if any directive other than a CONSERVE or ~
is used within a KINETIC block the resultant C code is not in the correct
order.  Here is the C code from a kinetic block:
*/
#endif

/* Sets up derivative form  and implicit form*/

#include <stdlib.h>
#include "modl.h"
#include "parse1.hpp"
#include "symbol.h"
extern int numlist;
extern int thread_data_index;
extern List* thread_cleanup_list;
extern int vectorize;

static int cvode_flag;
static void cvode_kin_remove();
static Item *cvode_sbegin, *cvode_send;
static List* kin_items_;
#define CVODE_FLAG     if (cvode_flag)
#define NOT_CVODE_FLAG if (!cvode_flag)

typedef struct Rterm {
    struct Rterm* rnext;
    Symbol* sym;
    char* str;
    int num;
    short isstate; /* 1 if to be solved for */
} Rterm;
static Rterm *rterm = (Rterm*) 0, *lterm; /*list of reaction terms for a side*/

typedef struct Reaction {
    struct Reaction* reactnext;
    Rterm* rterm[2]; /* rterm[0] = null if flux*/
    char* krate[2];  /* one of these is null if flux */
    Item* position;
} Reaction;
static Reaction* reactlist = (Reaction*) 0;
static Reaction* conslist = (Reaction*) 0;
static List* done_list;  /* list of already translated blocks */
static List* done_list1; /* do not emit definitions more than once */
typedef struct Rlist {
    Reaction* reaction;
    Symbol* sym;           /* the kinetic block symbol */
    Item* position;        /*  where we can initialize lists */
    Item* endbrace;        /*  can insert after all statements */
    Symbol** symorder;     /* state symbols in varnum order */
    const char** capacity; /* compartment size expessions in varnum order */
    int nsym;              /* number of symbols in above vector */
    int ncons;             /* and the diagonals for conservation are first */
    struct Rlist* rlistnext;
    int slist_decl;
} Rlist;
static Rlist* rlist = (Rlist*) 0;
static Rlist* clist = (Rlist*) 0;

static List* compartlist; /* list of triples with point to info in
    COMPARTMENT statement. The info is qexpr q'{' q'}' and points
    to items safely enclosed in a C comment.
    see massagecompart() and massagekinetic() */

List* ldifuslist; /* analogous to compartment. Specifies diffusion constant
    times the relevant cross sectional area. The volume/length
    must be specified in the COMPARTMENT statement */

int genconservterms(int eqnum, Reaction* r, int fn, Rlist* rlst);
int number_states(Symbol* fun, Rlist** prlst, Rlist** pclst);
void kinlist(Symbol* fun, Rlist* rlst);
void genderivterms(Reaction* r, int type, int n);
void genmatterms(Reaction* r, int fn);

#define MAXKINBLK 20
static int nstate_[MAXKINBLK];

static int sparse_declared_[10];
static int sparsedeclared(int i) {
    assert(i < 10);
    return sparse_declared_[i]++;
}

char* qconcat(Item* q1, Item* q2) /* return names as single string */
{
    char *cp, *ovrfl, *cs, *n;
    cp = buf;
    ovrfl = buf + 400;
    while (q1 != q2->next) {
        assert(cp < ovrfl);
        *cp++ = ' ';
        if (q1->itemtype == SYMBOL) {
            /* dont prepend *( to ARRAYS anymore */
#if 0
		if (SYM(q1)->type == NAME && (SYM(q1)->subtype & ARRAY)) {
			*cp++ = '*';
			*cp++ = '(';
		}
#endif
            n = SYM(q1)->name;
        } else {
            n = STR(q1);
        }
        for (cs = n; *cs; cs++) {
            *cp++ = *cs;
        }
        q1 = q1->next;
    }
    *cp = '\0';
    return stralloc(buf, (char*) 0);
}

void reactname(Item* q1, Item* lastok, Item* q2) /* NAME [] INTEGER   q2 may be null*/
{                                                /* put on right hand side */
    Symbol *s, *s1;
    Rterm* rnext;

    rnext = rterm;
    rterm = (Rterm*) emalloc(sizeof(Rterm));
    rterm->rnext = rnext;
    rterm->isstate = 0;
    s = SYM(q1);
    if ((s->subtype & STAT) && in_solvefor(s)) {
        Sprintf(buf, "D%s", s->name);
        s1 = lookup(buf);
        s1->usage |= DEP;
        s->used++;
        rterm->isstate = 1;
    } else if (!(s->subtype & (DEP | nmodlCONST | PARM | INDEP | STAT))) {
        diag(s->name, "must be a STATE, CONSTANT, ASSIGNED, or INDEPENDENT");
    }
    if (q2) {
        rterm->num = atoi(STR(q2));
    } else {
        rterm->num = 1;
    }
    if (q1 != lastok) {
        if (!(s->subtype & ARRAY)) {
            diag("REACTION: MUST be scalar or array", (char*) 0);
        }
        rterm->str = qconcat(q1->next->next, lastok->prev);
        /* one too many parentheses since normally a *( is prepended to the name
        during output in cout.c.  Therefore when used to construct a MATELM or RHS
        extra parentheses must be prepended as in MATELM((...,(....
        */ /* this no longer holds, no parentheses are to be prepended */
    } else {
        rterm->str = (char*) 0;
    }
    rterm->sym = s;
}

void leftreact() /* current reaction list is for left hand side */
{
    /* put whole list on left hand side and initialize right hand side */
    lterm = rterm;
    rterm = (Rterm*) 0;
}

void massagereaction(Item* qREACTION, Item* qREACT1, Item* qlpar, Item* qcomma, Item* qrpar) {
    Reaction* r1;

    /*ARGSUSED*/
    r1 = reactlist;
    reactlist = (Reaction*) emalloc(sizeof(Reaction));
    reactlist->reactnext = r1;
    reactlist->rterm[1] = rterm;
    reactlist->rterm[0] = lterm;
    reactlist->krate[0] = qconcat(qlpar->next, qcomma->prev);
    reactlist->krate[1] = qconcat(qcomma->next, qrpar->prev);
    reactlist->position = qrpar;
    /*SUPPRESS 440*/
    replacstr(qREACTION, "/* ~");
    /*SUPPRESS 440*/
    Insertstr(qrpar, ")*/\n");
    /*SUPPRESS 440*/
    replacstr(qrpar, "/*REACTION*/\n");
    rterm = (Rterm*) 0;
}

void flux(Item* qREACTION, Item* qdir, Item* qlast) {
    Reaction* r1;

    r1 = reactlist;
    reactlist = (Reaction*) emalloc(sizeof(Reaction));
    reactlist->reactnext = r1;
    reactlist->rterm[0] = rterm;
    reactlist->rterm[1] = (Rterm*) 0;
    /*SUPPRESS 440*/
    replacstr(qREACTION, "/* ~");
    reactlist->position = qlast;
    if (SYM(qdir)->name[0] == '-') {
        reactlist->krate[0] = qconcat(qdir->next->next, qlast);
        reactlist->krate[1] = (char*) 0;
        /*SUPPRESS 440*/
        replacstr(qlast, "/*REACTION*/\n");
    } else {
        if (rterm->rnext || (rterm->num != 1)) {
            diag("flux equations involve only one state", (char*) 0);
        }
        reactlist->krate[0] = (char*) 0;
        reactlist->krate[1] = qconcat(qdir->next->next, qlast);
        if (ldifuslist) { /* function of current ? */
            Item* q;
            int isfunc;
            for (q = qdir->next->next; q != qlast; q = q->next) {
                Symbol* s;
                if (q->itemtype == SYMBOL) {
                    s = SYM(q);
                    if (s->nrntype & 02 /*NRNCURIN*/) {
                        Symbol* sr;
                        Item* q1;
                        char *c1, *c2;
                        /* associate dflux/dcur with proper state */
                        ITERATE(q1, ldifuslist) {
                            sr = SYM(q1);
                            q1 = q1->next->next->next->next->next;
                            if (sr == rterm->sym) {
                                c1 = qconcat(qdir->next->next, q->prev);
                                c2 = qconcat(q->next, qlast);
                                Sprintf(buf,
                                        "nrn_nernst_coef(_type_%s)*(%s _ion_d%sdv %s)",
                                        s->name,
                                        c1,
                                        s->name,
                                        c2); /* dflux/dv */
                                if (rterm->str) {
                                    replacstr(q1, rterm->str);
                                }
                                if (*STR(q1->next) == '\0') {
                                    replacstr(q1->next, buf);
                                } else {
                                    diag(sr->name, "gets a flux from more than one current");
                                }
                            }
                            q1 = q1->next;
                        }
                    }
                }
            }
        }
        /*SUPPRESS 440*/
        replacstr(qlast, "/*FLUX*/\n");
    }
    /*SUPPRESS 440*/
    Insertstr(qlast, ")*/\n");
    /*SUPPRESS 440*/
    rterm = (Rterm*) 0;
}

/* set up derivative form for use with integration methods */

/* a bunch of states may be marked used but not be in the solveforlist
   we therefore loop through all the rterms and mark only those
*/

void massagekinetic(Item* q1, Item* q2, Item* q3, Item* q4) /*KINETIC NAME stmtlist
                                                                             '}'*/
{
    int count = 0, i, order, ncons;
    Item *q, *qs, *afterbrace;
    Item* qv;
    Symbol *s, *fun;
    Reaction* r1;
    Rterm* rt;
    Rlist* r;

    fun = SYM(q2);
    if ((fun->subtype & (DERF | KINF)) && fun->u.i) {
        diag("Merging kinetic blocks not implemented", (char*) 0);
    }
    fun->subtype |= KINF;
    numlist++;
    fun->u.i = numlist;

    vectorize_substitute(linsertstr(procfunc, "();\n"),
                         "(void* _so, double* _rhs, _internalthreadargsproto_);\n");
    Sprintf(buf, "static int %s", SYM(q2)->name);
    linsertstr(procfunc, buf);

    replacstr(q1, "\nstatic int");
    qv = insertstr(q3, "()\n");
    if (vectorize) {
        kin_vect1(q1, q2, q4);
        vectorize_substitute(qv, "(void* _so, double* _rhs, _internalthreadargsproto_)\n");
    }
    qv = insertstr(q3, "{_reset=0;\n");
    Sprintf(buf, "{int _reset=0;\n");
    vectorize_substitute(qv, buf);
    afterbrace = q3->next;
#if Glass
    /* Make sure that if the next statement was LOCAL xxx, we skip
    past it to do any of the other declarations. DRB */
    if (afterbrace->itemtype == STRING && !strcmp(afterbrace->element.str, "double")) {
        for (afterbrace = afterbrace->next;
             afterbrace->itemtype != STRING || strcmp(afterbrace->element.str, ";\n");
             afterbrace = afterbrace->next)
            ;
        if (afterbrace->itemtype == STRING && !strcmp(afterbrace->element.str, ";\n"))
            afterbrace = afterbrace->next;
    }

#endif
    qv = insertstr(afterbrace, "double b_flux, f_flux, _term; int _i;\n");
    /* also after these declarations will go initilization statements */

    order = 0;
    /* the varnum and order of the states is such that diagonals of
    conservation equations must be first.  This is done by setting
    s->used=-1 for all states and then numbering first with respect
    to the conslist and then the remaining that are still -1
    */
    /* mark only the isstate states */
    SYMITER(NAME) {
        if ((s->subtype & STAT) && s->used) {
            s->used = 0;
        }
    }
    for (r1 = conslist; r1; r1 = r1->reactnext) {
        for (rt = r1->rterm[0]; rt; rt = rt->rnext) {
            if (rt->isstate) {
                rt->sym->used = -1;
            }
        }
    }
    for (r1 = reactlist; r1; r1 = r1->reactnext) {
        for (rt = r1->rterm[0]; rt; rt = rt->rnext) {
            if (rt->isstate) {
                rt->sym->used = -1;
            }
        }
        for (rt = r1->rterm[1]; rt; rt = rt->rnext) {
            if (rt->isstate) {
                rt->sym->used = -1;
            }
        }
    }

    /* diagonals of the conservation relations are first */
    for (r1 = conslist; r1; r1 = r1->reactnext) {
        for (rt = r1->rterm[0]; rt; rt = rt->rnext) {
            if ((rt->sym->used == -1) && !(rt->sym->subtype & ARRAY)) {
                rt->sym->varnum = count++;
                rt->sym->used = ++order; /*first is 1*/
                break;
            }
        }
        if (!rt) {
            diag("Failed to diagonalize the Kinetic matrix", (char*) 0);
        }
    }
    ncons = count; /* can't use array as conservation diagonal */
    /* others can be in any order */
    SYMITER(NAME) {
        if ((s->subtype & STAT) && s->used == -1) {
            s->varnum = count;
            s->used = ++order; /* count and order  distinct states */
            if (s->subtype & ARRAY) {
                int dim = s->araydim;
                count += dim;
            } else {
                count++;
            }
        }
    }
    if (count == 0) {
        diag("KINETIC contains no reactions", (char*) 0);
    }
    fun->used = count;
    Sprintf(buf,
            "static neuron::container::field_index _slist%d[%d], _dlist%d[%d]; static double "
            "*_temp%d;\n",
            numlist,
            count,
            numlist,
            count,
            numlist);
    Linsertstr(procfunc, buf);
    insertstr(q4, "  } return _reset;\n");
    movelist(q1, q4, procfunc);

    r = (Rlist*) emalloc(sizeof(Rlist));
    r->reaction = reactlist;
    reactlist = (Reaction*) 0;
    r->symorder = (Symbol**) emalloc((unsigned) order * sizeof(Symbol*));
    r->slist_decl = 0;
    /*the reason that we can't just keep this info in s->varnum is that
    more than one block can have the same state with a different varnum */
    SYMITER(NAME) {
        if ((s->subtype & STAT) && s->used) {
            r->symorder[s->used - 1] = s;
        }
    }
    r->nsym = order;
    r->ncons = ncons;
    r->position = afterbrace;
    r->endbrace = q4->prev; /* needed for Dstate with COMPARTMENT */
    r->sym = fun;
    r->rlistnext = rlist;
    rlist = r;
    r = (Rlist*) emalloc(sizeof(Rlist));
    r->reaction = conslist;
    conslist = (Reaction*) 0;
    r->sym = fun;
    r->rlistnext = clist;
    clist = r;

    /* handle compartlist if any */
    rlist->capacity = (const char**) emalloc((unsigned) order * sizeof(char*));
    for (i = 0; i < order; i++) {
        rlist->capacity[i] = "";
    }
    if (compartlist) {
        char buf1[NRN_BUFSIZE];
        Item *q, *qexp, *qb, *qend, *q1;
        ITERATE(q, compartlist) {
            qexp = ITM(q);
            q = q->next;
            qb = ITM(q);
            q = q->next;
            qend = ITM(q);
            for (q1 = qb->next; q1 != qend; q1 = q1->next) {
                // if a state compartment variable is not used in
                // the kinetic block then skip it
                if (!SYM(q1)->used) {
                    continue;
                }
                Sprintf(buf1, "(%s)", qconcat(qexp, qb->prev));
                rlist->capacity[SYM(q1)->used - 1] = stralloc(buf1, (char*) 0);
            }
        }
        freelist(&compartlist);
    }

    SYMITER(NAME) {
        if ((s->subtype & STAT) && s->used) {
            s->used = 0;
        }
    }
    cvode_sbegin = q3;
    cvode_send = q4;
    kin_items_ = newlist();
    for (q = cvode_sbegin; q != cvode_send->next; q = q->next) {
        lappenditem(kin_items_, q);
    }
}

#if Glass
void fixrlst(Rlist* rlst) {
    if (rlst->position->prev->itemtype == STRING &&
        !strcmp(rlst->position->prev->element.str, "error =")) {
        rlst->position = rlst->position->prev;
    }
}
#endif

static int ncons; /* the number of conservation equations */

void kinetic_intmethod(Symbol* fun, const char* meth) {
    /*derivative form*/
    Reaction* r1;
    Rlist *rlst, *clst;
    int i, nstate;

    cvode_kin_remove();
    nstate = number_states(fun, &rlst, &clst);
    if (ncons) {
        Fprintf(stderr, "%s method ignores conservation\n", meth);
    }
    ncons = 0;
    Sprintf(buf,
            "{int _i; for(_i=0;_i<%d;_i++) _ml->data(_iml, _dlist%d[_i]) = 0.0;}\n",
            nstate,
            fun->u.i);
    /*goes near beginning of block*/
#if Glass
    fixrlst(rlst);
#endif
    Insertstr(rlst->position, buf);
    for (r1 = rlst->reaction; r1; r1 = r1->reactnext) {
        genderivterms(r1, 0, 0);
    }
    for (i = 0; i < rlst->nsym; i++) {
        if (rlst->capacity[i][0]) {
            if (rlst->symorder[i]->subtype & ARRAY) {
                Sprintf(buf,
                        "for (_i=0; _i < %d; _i++) { _ml->data(_iml, _dlist%d[_i + %d]) /= %s;}\n",
                        rlst->symorder[i]->araydim,
                        fun->u.i,
                        rlst->symorder[i]->varnum,
                        rlst->capacity[i]);
                Insertstr(rlst->endbrace, buf);
            } else {
                Sprintf(buf,
                        "_ml->data(_iml, _dlist%d[%d]) /= %s;\n",
                        fun->u.i,
                        rlst->symorder[i]->varnum,
                        rlst->capacity[i]);
                Insertstr(rlst->endbrace, buf);
            }
        }
    }
    kinlist(fun, rlst);
}

void genderivterms(Reaction* r, int type, int n) {
    Symbol* s;
    Item* q;
    Rterm* rt;
    int i, j;

    if (r->rterm[1] == (Rterm*) 0 && r->krate[1]) {
        genfluxterm(r, type, n);
        return;
    }
    q = r->position;
    for (j = 0; j < 2; j++) {
        if (j == 0) {
            Insertstr(q, "f_flux =");
        } else {
            Insertstr(q, ";\n b_flux =");
        }
        if (r->krate[j]) {
            Insertstr(q, r->krate[j]);
        } else {
            Insertstr(q, "0.");
        }
        for (rt = r->rterm[j]; rt; rt = rt->rnext) {
            for (i = 0; i < rt->num; i++) {
                Insertstr(q, "*");
                Insertsym(q, rt->sym);
                if (rt->str) {
                    Sprintf(buf, "[%s]", rt->str);
                    Insertstr(q, buf);
                }
            }
        }
    }
    Insertstr(q, ";\n");
    for (j = 0; j < 2; j++) {
        for (rt = r->rterm[j]; rt; rt = rt->rnext) {
            if (!(rt->isstate)) {
                continue;
            }
            Sprintf(buf, "D%s", rt->sym->name);
            s = lookup(buf);
            s->usage |= DEP;
            if (rt->sym->varnum < ncons)
                continue; /* equation reserved for conservation*/
            if (type) {
                Sprintf(buf, "_RHS%d(", n);
                Insertstr(q, buf);
                if (rt->str) {
                    Sprintf(buf, "%d + %s)", rt->sym->varnum, rt->str);
                } else {
                    Sprintf(buf, "%d)", rt->sym->varnum);
                }
                Insertstr(q, buf);
            } else {
                Insertsym(q, s); /*needs processing in cout*/
                if (rt->str) {
                    Sprintf(buf, "[%s]", rt->str);
                    Insertstr(q, buf);
                }
            }
            if (j == 0) {
                Insertstr(q, "-=");
            } else {
                Insertstr(q, "+=");
            }
            if (rt->num > 1) {
                Sprintf(buf, "%d.0 *", rt->num);
                Insertstr(q, buf);
            }
            Insertstr(q, "(f_flux - b_flux);\n");
        }
    }
    Insertstr(q, "\n"); /* REACTION comment left in */
}

void genfluxterm(Reaction* r, int type, int n) {
    Symbol* s;
    Rterm* rt;
    Item* q;

    q = r->position;
    rt = r->rterm[0];
    if (!(rt->isstate)) {
        diag(rt->sym->name, "must be (solved) STATE in flux reaction");
    }
    Sprintf(buf, "D%s", rt->sym->name);
    s = lookup(buf);
    if (rt->sym->varnum < ncons)
        diag(rt->sym->name, "is conserved and has a flux");
    /* the right hand side */
    Insertstr(q, "f_flux = b_flux = 0.;\n");
    if (type) {
        Sprintf(buf, "_RHS%d(", n);
        Insertstr(q, buf);
        if (rt->str) {
            Sprintf(buf, "%d + %s)", rt->sym->varnum, rt->str);
        } else {
            Sprintf(buf, "%d)", rt->sym->varnum);
        }
        Insertstr(q, buf);
    } else {
        Insertsym(q, s); /*needs processing in cout*/
        if (rt->str) {
            Sprintf(buf, "[%s]", rt->str);
            Insertstr(q, buf);
        }
    }
    if (r->krate[0]) {
        Sprintf(buf, " -= (f_flux = (%s) * ", r->krate[0]);
        Insertstr(q, buf);
        Insertsym(q, rt->sym);
        if (rt->str) {
            Sprintf(buf, "[%s]", rt->str);
            Insertstr(q, buf);
        }
    } else {
        Insertstr(q, "+= (b_flux = ");
        Insertstr(q, r->krate[1]);
    }
    Insertstr(q, ");\n");

    /* the matrix coefficient */
    if (type && r->krate[0]) {
        Sprintf(buf, " _MATELM%d(", n);
        Insertstr(q, buf);
        if (rt->str) {
            Sprintf(buf, "%d + %s, %d + %s", rt->sym->varnum, rt->str, rt->sym->varnum, rt->str);
        } else {
            Sprintf(buf, "%d, %d)", rt->sym->varnum, rt->sym->varnum);
        }
        Insertstr(q, buf);
        Sprintf(buf, "+= %s;\n", r->krate[0]);
        Insertstr(q, buf);
    }
}

static int linmat; /* 1 if linear */
void kinetic_implicit(Symbol* fun, const char* dt, const char* mname) {
    /*implicit equations _slist are state(t+dt) _dlist are Dstate(t)*/
    Item* q;
    Item* qv;
    Reaction* r1;
    Rlist *rlst, *clst;
    int i, nstate, flag, sparsedec, firsttrans, firsttrans1;

    firsttrans = 0; /* general declarations done only for NOT_CVODE_FLAG */
    firsttrans1 = 0;
    cvode_kin_remove();

    nstate = number_states(fun, &rlst, &clst);

    CVODE_FLAG {
        ncons = 0;
        Sprintf(buf, "static void* _cvsparseobj%d;\n", fun->u.i);
        q = linsertstr(procfunc, buf);
        Sprintf(buf, "static int _cvspth%d = %d;\n", fun->u.i, thread_data_index++);
        vectorize_substitute(q, buf);
        Sprintf(buf,
                "  "
                "_nrn_destroy_sparseobj_thread(static_cast<SparseObj*>(_thread[_cvspth%d].get<void*"
                ">()));\n",
                fun->u.i);
        lappendstr(thread_cleanup_list, buf);
    }
    else {
        if (!done_list) {
            done_list = newlist();
            done_list1 = newlist();
        }
        firsttrans = 1; /* declare the sparseobj and linflag*/
        firsttrans1 = 1;
        ITERATE(q, done_list) {
            if (SYM(q) == fun) {
                firsttrans = 0; /* already declared */
            }
        }
        ITERATE(q, done_list1) {
            if (SYM(q) == fun) {
                firsttrans1 = 0; /* already declared */
            }
        }
        if (firsttrans1) {
            Lappendsym(done_list, fun);
            Lappendsym(done_list1, fun);
            Sprintf(buf, "static void* _sparseobj%d;\n", fun->u.i);
            q = linsertstr(procfunc, buf);
            Sprintf(buf, "static int _spth%d = %d;\n", fun->u.i, thread_data_index++);
            vectorize_substitute(q, buf);
            Sprintf(buf,
                    "  "
                    "_nrn_destroy_sparseobj_thread(static_cast<SparseObj*>(_thread[_spth%d].get<"
                    "void*>()));\n",
                    fun->u.i);
            lappendstr(thread_cleanup_list, buf);
        }
    }
    /*goes near beginning of block. Before first reaction is not
    adequate since the first reaction may be within a while loop */
#if Glass
    fixrlst(rlst);
#endif
    CVODE_FLAG {
        Insertstr(rlst->position, "  b_flux = f_flux = 0.;\n");
    }
    Sprintf(buf,
            "{int _i; double _dt1 = 1.0/%s;\n\
for(_i=%d;_i<%d;_i++){\n",
            dt,
            ncons,
            nstate);
    Insertstr(rlst->position, buf);

    qv = insertstr(rlst->position, "");

    NOT_CVODE_FLAG {
        Sprintf(buf,
                "\
	_RHS%d(_i) = -_dt1*(_ml->data(_iml, _slist%d[_i]) - _ml->data(_iml, _dlist%d[_i]));\n\
	_MATELM%d(_i, _i) = _dt1;\n",
                fun->u.i,
                fun->u.i,
                fun->u.i,
                fun->u.i);
        qv = insertstr(rlst->position, buf);
    }
    CVODE_FLAG {
        Sprintf(buf,
                "\
	_RHS%d(_i) = _dt1*(_ml->data(_iml, _dlist%d[_i]));\n\
	_MATELM%d(_i, _i) = _dt1;\n",
                fun->u.i,
                fun->u.i,
                fun->u.i);
        qv = insertstr(rlst->position, buf);
    }
    qv = insertstr(rlst->position, "");
    Sprintf(buf, "    \n}");
    Insertstr(rlst->position, buf);
    /* Modify to take into account compartment sizes */
    /* separate into scalars and vectors */

    flag = 0;
    for (i = ncons; i < rlst->nsym; i++) {
        if (rlst->capacity[i][0]) {
            if (!(rlst->symorder[i]->subtype & ARRAY)) {
                if (!flag) {
                    flag = 1;
                    qv = insertstr(rlst->position, "");
                }
                Sprintf(buf,
                        "\n_RHS%d(%d) *= %s",
                        fun->u.i,
                        rlst->symorder[i]->varnum,
                        rlst->capacity[i]);
                Insertstr(rlst->position, buf);
                Sprintf(buf,
                        ";\n_MATELM%d(%d, %d) *= %s;",
                        fun->u.i,
                        rlst->symorder[i]->varnum,
                        rlst->symorder[i]->varnum,
                        rlst->capacity[i]);
                Insertstr(rlst->position, buf);
            }
        }
    }
    if (flag) {
        qv = insertstr(rlst->position, "");
    }

    for (i = ncons; i < rlst->nsym; i++) {
        if (rlst->capacity[i][0]) {
            if (rlst->symorder[i]->subtype & ARRAY) {
                Sprintf(buf, "\nfor (_i=0; _i < %d; _i++) {\n", rlst->symorder[i]->araydim);
                Insertstr(rlst->position, buf);
                qv = insertstr(rlst->position, "");

                Sprintf(buf,
                        "	_RHS%d(_i + %d) *= %s",
                        fun->u.i,
                        rlst->symorder[i]->varnum,
                        rlst->capacity[i]);
                Insertstr(rlst->position, buf);
                Sprintf(buf,
                        ";\n_MATELM%d(_i + %d, _i + %d) *= %s;",
                        fun->u.i,
                        rlst->symorder[i]->varnum,
                        rlst->symorder[i]->varnum,
                        rlst->capacity[i]);
                Insertstr(rlst->position, buf);
                qv = insertstr(rlst->position, "");

                Insertstr(rlst->position, "}");
            }
        }
    }

    /*----------*/
    Insertstr(rlst->position, "}\n");
    linmat = 1;
    for (r1 = rlst->reaction; r1; r1 = r1->reactnext) {
        NOT_CVODE_FLAG {
            genderivterms(r1, 1, fun->u.i);
        }
        genmatterms(r1, fun->u.i);
    }
    NOT_CVODE_FLAG { /* to end of function */
        for (i = 0, r1 = clst->reaction; r1; r1 = r1->reactnext) {
            i = genconservterms(i, r1, fun->u.i, rlst);
        }
        if (firsttrans1) {
            Sprintf(buf, "\n#define _linmat%d  %d\n", fun->u.i, linmat);
            Linsertstr(procfunc, buf);
        }
        if (firsttrans) {
            kinlist(fun, rlst);


            if (strcmp(mname, "_advance") == 0) { /* use for simeq */
                Sprintf(
                    buf, "\n#define _RHS%d(arg) _coef%d[arg][%d]\n", fun->u.i, fun->u.i, nstate);
                Linsertstr(procfunc, buf);
                Sprintf(buf,
                        "\n#define _MATELM%d(arg1,arg2) _coef%d[arg1][arg2]\n",
                        fun->u.i,
                        fun->u.i);
                Linsertstr(procfunc, buf);
                Sprintf(buf, "static double **_coef%d;\n", fun->u.i);
                Linsertstr(procfunc, buf);

            } else { /*for sparse matrix solver*/
                /* boilerplate for using sparse matrix solver */
                {
                    static int first = 1;
                    if (first) {
                        first = 0;
                        Sprintf(buf, "static double *_coef%d;\n", fun->u.i);
                        qv = linsertstr(procfunc, buf);
                        vectorize_substitute(qv, "");
                    }
                }
                Sprintf(buf, "\n#define _RHS%d(_arg) _coef%d[_arg + 1]\n", fun->u.i, fun->u.i);
                qv = linsertstr(procfunc, buf);
                Sprintf(buf, "\n#define _RHS%d(_arg) _rhs[_arg+1]\n", fun->u.i);
                vectorize_substitute(qv, buf);
                Sprintf(buf,
                        "\n#define _MATELM%d(_row,_col)\
	*(_getelm(_row + 1, _col + 1))\n",
                        fun->u.i);
                qv = linsertstr(procfunc, buf);
                Sprintf(buf,
                        "\n#define _MATELM%d(_row,_col) "
                        "*(_nrn_thread_getelm(static_cast<SparseObj*>(_so), _row + 1, _col + 1))\n",
                        fun->u.i);
                vectorize_substitute(qv, buf);
            }
        }
    } /* end of NOT_CVODE_FLAG */
}

void genmatterms(Reaction* r, int fn) {
    Symbol *s, *s1;
    Item* q;
    Rterm *rt, *rt1;
    int i, j, j1, n;

    if (r->rterm[1] == (Rterm*) 0 && r->krate[1]) {
        return; /* no fluxes go into matrix */
    }

    q = r->position;
    for (j = 0; j < 2; j++)
        for (rt = r->rterm[j]; rt; rt = rt->rnext) { /*d/dstate*/
            s = rt->sym;
            if (!(rt->isstate)) {
                continue;
            }
            /* construct the term */
            Sprintf(buf, "_term = %s", r->krate[j]);
            Insertstr(q, buf);
            if (rt->num != 1) {
                Sprintf(buf, "* %d", rt->num);
                Insertstr(q, buf);
            }
            for (rt1 = r->rterm[j]; rt1; rt1 = rt1->rnext) {
                n = rt1->num;
                if (rt == rt1) {
                    n--;
                }
                for (i = 0; i < n; i++) {
                    linmat = 0;
                    Insertstr(q, "*");
                    Insertsym(q, rt1->sym);
                    if (rt1->str) {
                        Sprintf(buf, "[%s]", rt1->str);
                        Insertstr(q, buf);
                    }
                }
            }
            Insertstr(q, ";\n");
            /* put in each equation (row) */
            for (j1 = 0; j1 < 2; j1++)
                for (rt1 = r->rterm[j1]; rt1; rt1 = rt1->rnext) {
                    s1 = rt1->sym;
                    if (!(rt1->isstate)) {
                        continue;
                    }
                    if (s1->varnum < ncons)
                        continue;
                    Sprintf(buf, "_MATELM%d(", fn);
                    Insertstr(q, buf);
                    if (rt1->str) {
                        Sprintf(buf, "%d + %s", s1->varnum, rt1->str);
                        Insertstr(q, buf);
                    } else {
                        Sprintf(buf, "%d", s1->varnum);
                        Insertstr(q, buf);
                    }
                    if (rt->str) {
                        Sprintf(buf, ",%d + %s)", s->varnum, rt->str);
                        Insertstr(q, buf);
                    } else {
                        Sprintf(buf, ",%d)", s->varnum);
                        Insertstr(q, buf);
                    }
                    if (j == j1) {
                        Insertstr(q, " +=");
                    } else {
                        Insertstr(q, " -=");
                    }
                    if (rt1->num != 1) {
                        Sprintf(buf, "%d * ", rt1->num);
                        Insertstr(q, buf);
                    }
                    Insertstr(q, "_term;\n");
                }
        }
    /* REACTION comment left in */
}

void massageconserve(Item* q1, Item* q3, Item* q5) /* CONSERVE react '=' expr */
{
    /* the list of states is in rterm at this time with the first at the end */
    Reaction* r1;
    Item* qv;

    r1 = conslist;
    conslist = (Reaction*) emalloc(sizeof(Reaction));
    conslist->reactnext = r1;
    conslist->rterm[0] = rterm;
    conslist->rterm[1] = (Rterm*) 0;
    conslist->krate[0] = qconcat(q3->next, q5);
    conslist->krate[1] = (char*) 0;
    /*SUPPRESS 440*/
    replacstr(q1, "/*");
    qv = insertstr(q1, "");
    /*SUPPRESS 440*/
    Insertstr(q5->next, "*/\n");
    /*SUPPRESS 440*/
    conslist->position = insertstr(q5->next->next, "/*CONSERVATION*/\n");
    rterm = (Rterm*) 0;
}

int genconservterms(int eqnum, Reaction* r, int fn, Rlist* rlst) {
    Item* q;
    Rterm *rt, *rtdiag;
    char eqstr[NRN_BUFSIZE];

    q = r->position;
    /* find the term used for the equation number (important if array)*/
    for (rtdiag = (Rterm*) 0, rt = r->rterm[0]; rt; rt = rt->rnext) {
        if (eqnum == rt->sym->varnum) {
            rtdiag = rt;
            break;
        }
    }
    assert(rtdiag);
    if (rtdiag->str) {
        Sprintf(eqstr, "%d(%d + %s", fn, eqnum, rtdiag->str);
        eqnum += rtdiag->sym->araydim;
        /*SUPPRESS 622*/
        assert(0); /*could ever work only in specialized circumstances
            when same conservation eqn used for all elements*/
    } else {
        Sprintf(eqstr, "%d(%d", fn, eqnum);
        eqnum++;
    }
    Sprintf(buf, "_RHS%s) = %s;\n", eqstr, r->krate[0]);
    Insertstr(q, buf);
    for (rt = r->rterm[0]; rt; rt = rt->rnext) {
        char buf1[NRN_BUFSIZE];
        if (rlst->capacity[rt->sym->used][0]) {
            Sprintf(buf1, " * %s", rlst->capacity[rt->sym->used]);
        } else {
            buf1[0] = '\0';
        }
        if (!(rt->isstate)) {
            diag(rt->sym->name, ": only (solved) STATE are allowed in CONSERVE equations.");
        }
        if (rt->str) {
            if (rlst->capacity[rt->sym->used][0]) {
                Sprintf(buf, "_i = %s;\n", rt->str);
                Insertstr(q, buf);
            }
            Sprintf(buf,
                    "_MATELM%s, %d + %s) = %d%s;\n",
                    eqstr,
                    rt->sym->varnum,
                    rt->str,
                    rt->num,
                    buf1);
            Insertstr(q, buf);
            Sprintf(buf, "_RHS%s) -= %s[%s]%s", eqstr, rt->sym->name, rt->str, buf1);
        } else {
            Sprintf(buf, "_MATELM%s, %d) = %d%s;\n", eqstr, rt->sym->varnum, rt->num, buf1);
            Insertstr(q, buf);
            Sprintf(buf, "_RHS%s) -= %s%s", eqstr, rt->sym->name, buf1);
        }
        Insertstr(q, buf);
        if (rt->num != 1) {
            Sprintf(buf, " * %d;\n", rt->num);
            Insertstr(q, buf);
        } else {
            Insertstr(q, ";\n");
        }
    }
    return eqnum;
}

int number_states(Symbol* fun, Rlist** prlst, Rlist** pclst) {
    /* reaction list has the symorder and this info is put back in sym->varnum*/
    /* also index of symorder goes into sym->used */
    Rlist *rlst, *clst;
    Symbol* s;
    int i, istate;

    /*match fun with proper reaction list*/
    clst = clist;
    for (rlst = rlist; rlst && (rlst->sym != fun); rlst = rlst->rlistnext)
        clst = clst->rlistnext;
    if (rlst == (Rlist*) 0) {
        diag(fun->name, "doesn't exist");
    }
    *prlst = rlst;
    *pclst = clst;
    /* Number the states. */
    for (i = 0, istate = 0; i < rlst->nsym; i++) {
        s = rlst->symorder[i];
        s->varnum = istate;
        s->used = i; /* set back to 0 in kinlist */
        if (s->subtype & ARRAY) {
            istate += s->araydim;
        } else {
            istate++;
        }
    }
    ncons = rlst->ncons;
    if (fun->u.i >= MAXKINBLK) {
        diag("too many solve blocks", (char*) 0);
    }
    nstate_[fun->u.i] = istate;
    return istate;
}

void kinlist(Symbol* fun, Rlist* rlst) {
    int i;
    Symbol* s;
    Item* qv;

    if (rlst->slist_decl) {
        return;
    }
    rlst->slist_decl = 1;
    /* put slist and dlist in initlist */
    for (i = 0; i < rlst->nsym; i++) {
        s = rlst->symorder[i];
        slist_data(s, s->varnum, fun->u.i);
        if (s->subtype & ARRAY) {
            int dim = s->araydim;
            Sprintf(buf,
                    "for(_i=0;_i<%d;_i++){_slist%d[%d+_i] = {%s_columnindex, _i};",
                    dim,
                    fun->u.i,
                    s->varnum,
                    s->name);
            qv = lappendstr(initlist, buf);
            Sprintf(
                buf, " _dlist%d[%d+_i] = {D%s_columnindex, _i};}\n", fun->u.i, s->varnum, s->name);
            qv = lappendstr(initlist, buf);
        } else {
            Sprintf(buf, "_slist%d[%d] = {%s_columnindex, 0};", fun->u.i, s->varnum, s->name);
            qv = lappendstr(initlist, buf);
            Sprintf(buf, " _dlist%d[%d] = {D%s_columnindex, 0};\n", fun->u.i, s->varnum, s->name);
            qv = lappendstr(initlist, buf);
        }
        s->used = 0;
    }
}

/* for now we only check CONSERVE and COMPARTMENT */
void check_block(int standard, int actual, const char* mes) {
    if (standard != actual) {
        diag(mes, "not allowed in this kind of block");
    }
}

/* Syntax to take into account compartment size
compart: COMPARTMENT NAME ',' expr '{' namelist '}'
        {massagecompart($4, $5, $7, SYM($2));}
    | COMPARTMENT expr '{' namelist '}'
        {massagecompart($2, $3, $5, SYM0);}
    | COMPARTMENT error {myerr("Correct syntax is:	\
COMPARTMENT index, expr { vectorstates }\n\
            COMPARTMENT expr { scalarstates }");}
    ;
*/
/* implementation
 We save enough info here so that the rlist built in massagekinetic
 can point to compartment size expressions which are later used
 analogously to c*dv/dt and also affect the conservation equations.
 see Item ** Rlist->capacity[2]

 any index is replaced by _i.
 the name list is checked to make sure they are states

 compartlist is a list of triples of item pointers which
 holds the info for massagekinetic.
*/
void massagecompart(Item* qexp, Item* qb1, Item* qb2, Symbol* indx) {
    Item *q, *qs;

    /* surround the statement with comments so that the expession
       will benignly exist for later use */
    /*SUPPRESS 440*/
    Insertstr(qb2->next, "*/\n");

    if (indx) {
        for (q = qexp; q != qb1; q = q->next) {
            if (q->itemtype == SYMBOL && SYM(q) == indx) {
                replacstr(q, "_i");
            }
        }
        /*SUPPRESS 440*/
        Insertstr(qexp->prev->prev->prev, "/*");
    } else {
        /*SUPPRESS 440*/
        Insertstr(qexp->prev, "/*");
    }
    for (q = qb1->next; q != qb2; q = qs) {
        qs = q->next;
        if (!(SYM(q)->subtype & STAT) && in_solvefor(SYM(q))) {
            remove(q);
#if 0
diag(SYM(q)->name, "must be a (solved) STATE in a COMPARTMENT statement");
#endif
        }
    }
    if (!compartlist) {
        compartlist = newlist();
    }
    Lappenditem(compartlist, qexp);
    Lappenditem(compartlist, qb1);
    Lappenditem(compartlist, qb2);
}

void massageldifus(Item* qexp, Item* qb1, Item* qb2, Symbol* indx) {
    Item *q, *qs, *q1;
    Symbol *s, *s2;

    /* surround the statement with comments so that the expession
       will benignly exist for later use */
    /*SUPPRESS 440*/
    Insertstr(qb2->next, "*/\n");

    if (indx) {
        for (q = qexp; q != qb1; q = q->next) {
            if (q->itemtype == SYMBOL && SYM(q) == indx) {
                replacstr(q, "_i");
            }
        }
        /*SUPPRESS 440*/
        Insertstr(qexp->prev->prev->prev, "/*");
    } else {
        /*SUPPRESS 440*/
        Insertstr(qexp->prev, "/*");
    }
    if (!ldifuslist) {
        ldifuslist = newlist();
    }
    for (q = qb1->next; q != qb2; q = qs) {
        qs = q->next;
        s = SYM(q);
        s2 = SYM0;
        if (!(s->subtype & STAT) && in_solvefor(s)) {
            remove(q);
            diag(SYM(q)->name, "must be a (solved) STATE in a LONGITUDINAL_DIFFUSION statement");
        }
        lappendsym(ldifuslist, s);
        Lappenditem(ldifuslist, qexp);
        Lappenditem(ldifuslist, qb1);
        /* store the COMPARTMENT volume expression for this sym */
        q1 = compartlist;
        if (q1)
            ITERATE(q1, compartlist) {
                Item *qexp, *qb1, *qb2, *q;
                qexp = ITM(q1);
                q1 = q1->next;
                qb1 = ITM(q1);
                q1 = q1->next;
                qb2 = ITM(q1);
                for (q = qb1; q != qb2; q = q->next) {
                    if (q->itemtype == SYMBOL) {
                        s2 = SYM(q);
                        if (s == s2)
                            break;
                    }
                }
                if (s == s2) {
                    lappenditem(ldifuslist, qexp);
                    lappenditem(ldifuslist, qb1);
                    break;
                }
            }
        if (s != s2) {
            diag(SYM(q)->name, "must be declared in COMPARTMENT");
        }
        lappendstr(ldifuslist, "0"); /* will be flux conc index if any */
        lappendstr(ldifuslist, "");  /* will be dflux/dconc if any */
    }
}

static List* kvect;

void kin_vect1(Item* q1, Item* q2, Item* q4) {
    if (!kvect) {
        kvect = newlist();
    }
    Lappenditem(kvect, q1); /* static .. */
    Lappenditem(kvect, q2); /* sym */
    Lappenditem(kvect, q4); /* } */
}

void kin_vect2() {
    Item *q, *q1, *q2, *q4;
    return;
    if (kvect) {
        ITERATE(q, kvect) {
            q1 = ITM(q);
            q = q->next;
            q2 = ITM(q);
            q = q->next;
            q4 = ITM(q);
            kin_vect3(q1, q2, q4);
        }
    }
}

void kin_vect3(Item* q1, Item* q2, Item* q4) {
    Symbol* fun;
    Item *q, *first, *last, *insertitem(Item * item, Item * itm);
    fun = SYM(q2);
    last = insertstr(q4->next, "\n");
    first = insertstr(last, "\n/*copy of previous function */\n");
    for (q = q1; q != q4->next; q = q->next) {
        if (q == q2) {
            Sprintf(buf, "_vector_%s", fun->name);
            insertstr(last, buf);
        } else {
            insertitem(last, q);
        }
    }
    Sprintf(buf,
            "\n#undef WANT_PRAGMA\n#define WANT_PRAGMA 1\
\n#undef _INSTANCE_LOOP\n#define _INSTANCE_LOOP \
for (_ix = _base; _ix < _bound; ++_ix) ");
    insertstr(first, buf);
    Sprintf(buf,
            "\n#undef _RHS%d\n#define _RHS%d(arg) \
_coef%d[arg][_ix]\n",
            fun->u.i,
            fun->u.i,
            fun->u.i);
    insertstr(first, buf);
    Sprintf(buf,
            "\n#undef _MATELM%d\n#define _MATELM%d(row,col) \
_jacob%d[(row)*%d + (col)][_ix]\n",
            fun->u.i,
            fun->u.i,
            fun->u.i,
            nstate_[fun->u.i]);
    insertstr(first, buf);
}


static void cvode_kin_remove() {
    Item *q, *q2;
#if 0
prn(kin_items_, kin_items_->prev);
prn(cvode_sbegin, cvode_send);
#endif
    q2 = cvode_sbegin;
    ITERATE(q, kin_items_) {
        while (ITM(q) != q2) {
            assert(q2 != cvode_send); /* past the list */
            q2 = q2->next;
            remove(q2->prev);
        }
        q2 = q2->next;
    }
}

void prn(Item* q1, Item* q2) {
    Item *qq, *q;
    for (qq = q1; qq != q2; qq = qq->next) {
        if (qq->itemtype == ITEM) {
            fprintf(stderr, "itm ");
            q = ITM(qq);
        } else {
            q = qq;
        }
        switch (q->itemtype) {
        case STRING:
            fprintf(stderr, "%p STRING |%s|\n", q, STR(q));
            break;
        case SYMBOL:
            fprintf(stderr, "%p SYMBOL |%s|\n", q, SYM(q)->name);
            break;
        case ITEM:
            fprintf(stderr, "%p ITEM\n", q);
            break;
        default:
            fprintf(stderr, "%p type %d\n", q, q->itemtype);
            break;
        }
    }
}

void cvode_kinetic(Item* qsol, Symbol* fun, int numeqn, int listnum) {
#if 1
    Item* qn;
    int out;
    Item *q, *pbeg, *pend, *qnext;
    /* get a list of the original items so we can keep removing
       the added ones to get back to the original list.
    */
    if (done_list)
        for (q = done_list->next; q != done_list; q = qn) {
            qn = q->next;
            if (SYM(q) == fun) {
                remove(q);
            }
        }
    kinetic_intmethod(fun, "NEURON's CVode");
    Lappendstr(procfunc, "\n/*CVODE ode begin*/\n");
    Sprintf(buf, "static int _ode_spec%d() {_reset=0;{\n", fun->u.i);
    Lappendstr(procfunc, buf);
    Sprintf(buf,
            "static int _ode_spec%d(_internalthreadargsproto_) {\n"
            "  int _reset=0;\n"
            "  {\n",
            fun->u.i);
    vectorize_substitute(procfunc->prev, buf);
    copyitems(cvode_sbegin, cvode_send, procfunc->prev);

    Lappendstr(procfunc, "\n/*CVODE matsol*/\n");
    Sprintf(buf, "static int _ode_matsol%d() {_reset=0;{\n", fun->u.i);
    Lappendstr(procfunc, buf);
    Sprintf(buf,
            "static int _ode_matsol%d(void* _so, double* _rhs, _internalthreadargsproto_) {int "
            "_reset=0;{\n",
            fun->u.i);
    vectorize_substitute(procfunc->prev, buf);
    cvode_flag = 1;
    kinetic_implicit(fun, "dt", "ZZZ");
    cvode_flag = 0;
    pbeg = procfunc->prev;
    copyitems(cvode_sbegin, cvode_send, procfunc->prev);
    pend = procfunc->prev;
#if 1
    /* remove statements containing f_flux or b_flux */
    for (q = pbeg; q != pend; q = qnext) {
        qnext = q->next;
        if (q->itemtype == SYMBOL &&
            (strcmp(SYM(q)->name, "f_flux") == 0 || strcmp(SYM(q)->name, "b_flux") == 0)) {
            /* find the beginning of the statement */
            out = 0;
            for (;;) {
                switch (q->itemtype) {
                case STRING:
                    if (strchr(STR(q), ';')) {
                        out = 1;
                    }
                    break;
                case SYMBOL:
                    if (SYM(q)->name[0] == ';') {
                        out = 1;
                    }
                    break;
                }
                if (out) {
                    break;
                }
                q = q->prev;
            }
            q = q->next;
            /* delete the statement */
            while (q->itemtype != SYMBOL || SYM(q)->name[0] != ';') {
                qnext = q->next;
                remove(q);
                q = qnext;
            }
            qnext = q->next;
            remove(q);
        }
    }
#endif
    Lappendstr(procfunc, "\n/*CVODE end*/\n");

#endif
}
