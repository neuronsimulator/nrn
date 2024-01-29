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
    if (p->dparam[4].get<double>() != 1) {
        Printf(" rallbranch=%g", p->dparam[4].get<double>());
    }
    Printf("\n");
    if (sec->parentsec) {
        Printf("	%s ", secname(sec->parentsec));
        Printf("connect %s (%g), %g\n",
               secname(sec),
               p->dparam[3].get<double>(),
               p->dparam[1].get<double>());
    } else {
        v_setup_vectors();
        /*SUPPRESS 440*/
        Printf("	/*location %g attached to cell %d*/\n",
               p->dparam[3].get<double>(),
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
                    Printf(" %s=%g", s->name, p1->param_legacy(s->u.rng.index));
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
