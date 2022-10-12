#include <../../nrnconf.h>

#include <stdio.h>
#include "section.h"
#include "parse.hpp"
#include "membfunc.h"

extern void verify_structure(void);

static void pnode(Prop*);

void psection(void) {
    Section* sec;
    Prop *p, *p1;
    verify_structure();
    sec = chk_access();
    p = sec->prop;
    Printf("%s {", secname(sec));
    Printf(" nseg=%d  L=%g  Ra=%g", sec->nnode - 1, section_length(sec), nrn_ra(sec));
    if (static_cast<double>(p->dparam[4]) != 1) {
        Printf(" rallbranch=%g", static_cast<double>(p->dparam[4]));
    }
    Printf("\n");
    if (sec->parentsec) {
        Printf("	%s ", secname(sec->parentsec));
        Printf("connect %s (%g), %g\n",
               secname(sec),
               static_cast<double>(p->dparam[3]),
               static_cast<double>(p->dparam[1]));
    } else {
        v_setup_vectors();
        /*SUPPRESS 440*/
        Printf("	/*location %g attached to cell %d*/\n",
               static_cast<double>(p->dparam[3]),
               sec->parentnode->v_node_index);
    }
    if (sec->nnode) {
        /*SUPPRESS 440*/
        Printf("	/* First segment only */\n");
        p1 = sec->pnode[0]->prop;
        pnode(p1);
    }
    Printf("}\n");
    hoc_retpushx(1.);
}

static void pnode(Prop* p1) {
    Symbol* sym;
    int j;

    if (!p1) {
        return;
    }
    pnode(p1->next); /*print in insert order*/
    sym = memb_func[p1->_type].sym;
    Printf("	insert %s {", sym->name);
    if (sym->s_varn) {
        for (j = 0; j < sym->s_varn; j++) {
            Symbol* s = sym->u.ppsym[j];
            if (nrn_vartype(s) == nrnocCONST) {
                if (p1->ob) {
                    printf(" %s=%g", s->name, p1->ob->u.dataspace[s->u.rng.index].pval[0]);
                } else {
                    Printf(" %s=%g", s->name, p1->param(s->u.rng.index));
                }
            }
        }
    }
    Printf("}\n");
}

extern void print_stim(void);
extern void print_clamp(void);
extern void print_syn(void);

void prstim(void) {
    print_stim();
    print_clamp();
    print_syn();
    hoc_retpushx(1.);
}
