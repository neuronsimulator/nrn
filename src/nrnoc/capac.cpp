#include <../../nrnconf.h>
/* /local/src/master/nrn/src/nrnoc/capac.cpp,v 1.6 1998/11/25 19:14:28 hines Exp */

#include "section.h"
#include "membdef.h"
#include "nrniv_mf.h"


static const char* mechanism[] = {"0", "capacitance", "cm", 0, "i_cap", 0, 0};
static void cap_alloc(Prop*);
static void cap_init(NrnThread*, Memb_list*, int);

static constexpr auto nparm = 2;
static constexpr auto ndparm = 0;
extern "C" void capac_reg_(void) {
    int mechtype;
    /* all methods deal with capacitance in special ways */
    register_mech(mechanism, cap_alloc, (Pvmi) 0, (Pvmi) 0, (Pvmi) 0, cap_init, -1, 1);
    mechtype = nrn_get_mechtype(mechanism[1]);
    hoc_register_prop_size(mechtype, nparm, 0);
}

static constexpr auto cm_index = 0;
static constexpr auto i_cap_index = 1;

/*
cj is analogous to 1/dt for cvode and daspk
for fixed step second order it is 2/dt and
for pure implicit fixed step it is 1/dt
It used to be static but is now a thread data variable
*/

void nrn_cap_jacob(NrnThread* _nt, Memb_list* ml) {
    neuron::cache::MechanismRange<nparm, ndparm> ml_cache{*ml};
    int count = ml->nodecount;
    Node** vnode = ml->nodelist;
    double cfac = .001 * _nt->cj;
#if CACHEVEC
    if (use_cachevec) {
        int* ni = ml->nodeindices;
        for (int i = 0; i < count; i++) {
            VEC_D(ni[i]) += cfac * ml_cache.fpfield<cm_index>(i);
        }
    } else
#endif /* CACHEVEC */
    {
        for (int i = 0; i < count; ++i) {
            NODED(vnode[i]) += cfac * ml_cache.fpfield<cm_index>(i);
        }
    }
}

static void cap_init(NrnThread* _nt, Memb_list* ml, int type) {
    neuron::cache::MechanismRange<nparm, ndparm> ml_cache{*ml};
    int count = ml->nodecount;
    for (int i = 0; i < count; ++i) {
        ml_cache.fpfield<i_cap_index>(i) = 0;
    }
}

void nrn_capacity_current(NrnThread* _nt, Memb_list* ml) {
    neuron::cache::MechanismRange<nparm, ndparm> ml_cache{*ml};
    int count = ml->nodecount;
    Node** vnode = ml->nodelist;
    double cfac = .001 * _nt->cj;
    /* since rhs is dvm for a full or half implicit step */
    /* (nrn_update_2d() replaces dvi by dvi-dvx) */
    /* no need to distinguish secondorder */
#if CACHEVEC
    if (use_cachevec) {
        int* ni = ml->nodeindices;
        for (int i = 0; i < count; i++) {
            ml_cache.fpfield<i_cap_index>(i) = cfac * ml_cache.fpfield<cm_index>(i) * VEC_RHS(ni[i]);
        }
    } else
#endif /* CACHEVEC */
    {
        for (int i = 0; i < count; ++i) {
            ml_cache.fpfield<i_cap_index>(i) = cfac * ml_cache.fpfield<cm_index>(i) * NODERHS(vnode[i]);
        }
    }
}


void nrn_mul_capacity(NrnThread* _nt, Memb_list* ml) {
    neuron::cache::MechanismRange<nparm, ndparm> ml_cache{*ml};
    int count = ml->nodecount;
    Node** vnode = ml->nodelist;
    double cfac = .001 * _nt->cj;
#if CACHEVEC
    if (use_cachevec) {
        int* ni = ml->nodeindices;
        for (int i = 0; i < count; i++) {
            VEC_RHS(ni[i]) *= cfac * ml_cache.fpfield<cm_index>(i);
        }
    } else
#endif /* CACHEVEC */
    {
        for (int i = 0; i < count; ++i) {
            NODERHS(vnode[i]) *= cfac * ml_cache.fpfield<cm_index>(i);
        }
    }
}

void nrn_div_capacity(NrnThread* _nt, Memb_list* ml) {
    neuron::cache::MechanismRange<nparm, ndparm> ml_cache{*ml};
    int count = ml->nodecount;
    Node** vnode = ml->nodelist;
#if CACHEVEC
    if (use_cachevec) {
        int* ni = ml->nodeindices;
        for (int i = 0; i < count; i++) {
            ml_cache.fpfield<i_cap_index>(i) = VEC_RHS(ni[i]);
            VEC_RHS(ni[i]) /= 1.e-3 * ml_cache.fpfield<cm_index>(i);
        }
    } else
#endif /* CACHEVEC */
    {
        for (int i = 0; i < count; ++i) {
            ml_cache.fpfield<i_cap_index>(i) = NODERHS(vnode[i]);
            NODERHS(vnode[i]) /= 1.e-3 * ml_cache.fpfield<cm_index>(i);
        }
    }
    if (_nt->_nrn_fast_imem) {
        double* p = _nt->_nrn_fast_imem->_nrn_sav_rhs;
        for (int i = 0; i < count; ++i) {
            p[vnode[i]->v_node_index] += ml_cache.fpfield<i_cap_index>(i);
        }
    }
}


/* the rest can be constructed automatically from the above info*/

static void cap_alloc(Prop* p) {
    assert(p->param_size() == nparm);
    p->set_param(0, DEF_cm);  // default capacitance/cm^2
}
