#include <../../nrnconf.h>
/* /local/src/master/nrn/src/nrnoc/passive0.cpp,v 1.2 1997/03/13 14:18:02 hines Exp */

#include "section.h"
#include "membdef.h"
#include "nrniv_mf.h"

#define nparm 2
static const char* mechanism[] = {"0", "fastpas", "g_fastpas", "e_fastpas", 0, 0, 0};
static void pas_alloc(Prop* p);
static void pas_cur(neuron::model_sorted_token const&, NrnThread* nt, Memb_list* ml, int type);
static void pas_jacob(neuron::model_sorted_token const&, NrnThread* nt, Memb_list* ml, int type);

extern "C" void passive0_reg_(void) {
    int mechtype;
    register_mech(mechanism, pas_alloc, pas_cur, pas_jacob, nullptr, nullptr, -1, 1);
    mechtype = nrn_get_mechtype(mechanism[1]);
    using neuron::mechanism::field;
    neuron::mechanism::register_data_fields(mechtype,
                                            field<double>{"g_fastpas"},
                                            field<double>{"e_fastpas"});
    hoc_register_prop_size(mechtype, nparm, 0);
}

static constexpr auto g_index = 0;
static constexpr auto e_index = 1;

static void pas_cur(neuron::model_sorted_token const&, NrnThread* nt, Memb_list* ml, int type) {
    int count = ml->nodecount;
    Node** vnode = ml->nodelist;
    for (int i = 0; i < count; ++i) {
        NODERHS(vnode[i]) -= ml->data(i, g_index) * (NODEV(vnode[i]) - ml->data(i, e_index));
    }
}

static void pas_jacob(neuron::model_sorted_token const&, NrnThread* nt, Memb_list* ml, int type) {
    auto const count = ml->nodecount;
    auto* const ni = ml->nodeindices;
    auto* const vec_d = nt->node_d_storage();
    for (int i = 0; i < count; ++i) {
        vec_d[ni[i]] += ml->data(i, g_index);
    }
}

/* the rest can be constructed automatically from the above info*/

static void pas_alloc(Prop* p) {
    assert(p->param_size() == nparm);
    p->param(0) = DEF_g;
    p->param(1) = DEF_e;
}
