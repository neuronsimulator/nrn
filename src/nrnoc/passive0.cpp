#include <../../nrnconf.h>
/* /local/src/master/nrn/src/nrnoc/passive0.cpp,v 1.2 1997/03/13 14:18:02 hines Exp */

#include "section.h"
#include "membdef.h"
#include "nrniv_mf.h"

#define nparm 2
static const char* mechanism[] = {"0", "fastpas", "g_fastpas", "e_fastpas", 0, 0, 0};
static void pas_alloc(Prop* p);
static void pas_cur(NrnThread* nt, Memb_list* ml, int type);
static void pas_jacob(NrnThread* nt, Memb_list* ml, int type);

extern "C" void passive0_reg_(void) {
    int mechtype;
    register_mech(mechanism, pas_alloc, pas_cur, pas_jacob, (Pvmi) 0, (Pvmi) 0, -1, 1);
    mechtype = nrn_get_mechtype(mechanism[1]);
    hoc_register_prop_size(mechtype, nparm, 0);
}

#define g vdata[i][0]
#define e vdata[i][1]
#define v NODEV(vnode[i])

static void pas_cur(NrnThread* nt, Memb_list* ml, int type) {
    int count = ml->nodecount;
    Node** vnode = ml->nodelist;
    double** vdata = ml->_data;
    Datum** vpdata = ml->pdata;
    int i;
    for (i = 0; i < count; ++i) {
        NODERHS(vnode[i]) -= g * (v - e);
    }
}

static void pas_jacob(NrnThread* nt, Memb_list* ml, int type) {
    int count = ml->nodecount;
    Node** vnode = ml->nodelist;
    double** vdata = ml->_data;
    Datum** vpdata = ml->pdata;
    int i;
    for (i = 0; i < count; ++i) {
        NODED(vnode[i]) += g;
    }
}

/* the rest can be constructed automatically from the above info*/

static void pas_alloc(Prop* p) {
    double* pd;
    pd = nrn_prop_data_alloc(p->_type, nparm, p);
    p->param_size = nparm;
#if defined(__MWERKS__)
    pd[0] = 5.e-4; /*DEF_g;*/
#else
    pd[0] = DEF_g;
#endif
    pd[1] = DEF_e;
    p->param = pd;
}
