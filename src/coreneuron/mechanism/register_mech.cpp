/*
# =============================================================================
# Copyright (c) 2016 - 2021 Blue Brain Project/EPFL
#
# See top-level LICENSE file for details.
# =============================================================================.
*/

#include <cstring>

#include "coreneuron/nrnconf.h"
#include "coreneuron/sim/multicore.hpp"
#include "coreneuron/membrane_definitions.h"
#include "coreneuron/mechanism/eion.hpp"
#include "coreneuron/mechanism/mech_mapping.hpp"
#include "coreneuron/mechanism/membfunc.hpp"
#include "coreneuron/coreneuron.hpp"
#include "coreneuron/utils/nrnoc_aux.hpp"

namespace coreneuron {
int secondorder = 0;
double t, dt, celsius, pi;
// declare copyin required for correct initialization
#pragma acc declare copyin(secondorder)
#pragma acc declare copyin(celsius)
#pragma acc declare copyin(pi)
int rev_dt;

using Pfrv = void (*)();

static void ion_write_depend(int type, int etype);

void hoc_reg_bbcore_read(int type, bbcore_read_t f) {
    if (type == -1) {
        return;
    }
    corenrn.get_bbcore_read()[type] = f;
}
void hoc_reg_bbcore_write(int type, bbcore_write_t f) {
    if (type == -1) {
        return;
    }
    corenrn.get_bbcore_write()[type] = f;
}

void add_nrn_has_net_event(int type) {
    if (type == -1) {
        return;
    }
    corenrn.get_has_net_event().push_back(type);
}

/* values are type numbers of mechanisms which have FOR_NETCONS statement */
int nrn_fornetcon_cnt_;    /* how many models have a FOR_NETCONS statement */
int* nrn_fornetcon_type_;  /* what are the type numbers */
int* nrn_fornetcon_index_; /* what is the index into the ppvar array */

void add_nrn_fornetcons(int type, int indx) {
    if (type == -1)
        return;

    int i = nrn_fornetcon_cnt_++;
    nrn_fornetcon_type_ = (int*) erealloc(nrn_fornetcon_type_, (i + 1) * sizeof(int));
    nrn_fornetcon_index_ = (int*) erealloc(nrn_fornetcon_index_, (i + 1) * sizeof(int));
    nrn_fornetcon_type_[i] = type;
    nrn_fornetcon_index_[i] = indx;
}

void add_nrn_artcell(int type, int qi) {
    if (type == -1) {
        return;
    }

    corenrn.get_is_artificial()[type] = 1;
    corenrn.get_artcell_qindex()[type] = qi;
}

void set_pnt_receive(int type,
                     pnt_receive_t pnt_receive,
                     pnt_receive_t pnt_receive_init,
                     short size) {
    if (type == -1) {
        return;
    }
    corenrn.get_pnt_receive()[type] = pnt_receive;
    corenrn.get_pnt_receive_init()[type] = pnt_receive_init;
    corenrn.get_pnt_receive_size()[type] = size;
}

void alloc_mech(int memb_func_size_) {
    corenrn.get_memb_funcs().resize(memb_func_size_);
    corenrn.get_pnt_map().resize(memb_func_size_);
    corenrn.get_pnt_receive().resize(memb_func_size_);
    corenrn.get_pnt_receive_init().resize(memb_func_size_);
    corenrn.get_pnt_receive_size().resize(memb_func_size_);
    corenrn.get_watch_check().resize(memb_func_size_);
    corenrn.get_is_artificial().resize(memb_func_size_, false);
    corenrn.get_artcell_qindex().resize(memb_func_size_);
    corenrn.get_prop_param_size().resize(memb_func_size_);
    corenrn.get_prop_dparam_size().resize(memb_func_size_);
    corenrn.get_mech_data_layout().resize(memb_func_size_, 1);
    corenrn.get_bbcore_read().resize(memb_func_size_);
    corenrn.get_bbcore_write().resize(memb_func_size_);
}

void initnrn() {
    secondorder = DEF_secondorder; /* >0 means crank-nicolson. 2 means currents
                              adjusted to t+dt/2 */
    t = 0.;                        /* msec */
    dt = DEF_dt;                   /* msec */
    rev_dt = (int) (DEF_rev_dt);   /* 1/msec */
    celsius = DEF_celsius;         /* degrees celsius */
}

/* if vectorized then thread_data_size added to it */
int register_mech(const char** m,
                  mod_alloc_t alloc,
                  mod_f_t cur,
                  mod_f_t jacob,
                  mod_f_t stat,
                  mod_f_t initialize,
                  int /* nrnpointerindex */,
                  int vectorized) {
    auto& memb_func = corenrn.get_memb_funcs();

    int type = nrn_get_mechtype(m[1]);

    // No mechanism in the .dat files
    if (type == -1)
        return type;

    assert(type);
#ifdef DEBUG
    printf("register_mech %s %d\n", m[1], type);
#endif
    if (memb_func[type].sym) {
        assert(strcmp(memb_func[type].sym, m[1]) == 0);
    } else {
        memb_func[type].sym = (char*) emalloc(strlen(m[1]) + 1);
        strcpy(memb_func[type].sym, m[1]);
    }
    memb_func[type].current = cur;
    memb_func[type].jacob = jacob;
    memb_func[type].alloc = alloc;
    memb_func[type].state = stat;
    memb_func[type].initialize = initialize;
    memb_func[type].constructor = nullptr;
    memb_func[type].destructor = nullptr;
#if VECTORIZE
    memb_func[type].vectorized = vectorized ? 1 : 0;
    memb_func[type].thread_size_ = vectorized ? (vectorized - 1) : 0;
    memb_func[type].thread_mem_init_ = nullptr;
    memb_func[type].thread_cleanup_ = nullptr;
    memb_func[type].thread_table_check_ = nullptr;
    memb_func[type].is_point = 0;
    memb_func[type].setdata_ = nullptr;
    memb_func[type].dparam_semantics = nullptr;
#endif
    register_all_variables_offsets(type, &m[2]);
    return type;
}

void nrn_writes_conc(int type, int /* unused */) {
    static int lastion = EXTRACELL + 1;
    if (type == -1)
        return;

#if DEBUG
    printf("%s reordered from %d to %d\n", corenrn.get_memb_func(type).sym, type, lastion);
#endif
    if (nrn_is_ion(type)) {
        ++lastion;
    }
}

void _nrn_layout_reg(int type, int layout) {
    corenrn.get_mech_data_layout()[type] = layout;
}

void hoc_register_net_receive_buffering(NetBufReceive_t f, int type) {
    corenrn.get_net_buf_receive().emplace_back(f, type);
}

void hoc_register_net_send_buffering(int type) {
    corenrn.get_net_buf_send_type().push_back(type);
}

void hoc_register_watch_check(nrn_watch_check_t nwc, int type) {
    corenrn.get_watch_check()[type] = nwc;
}

void hoc_register_prop_size(int type, int psize, int dpsize) {
    if (type == -1)
        return;

    int pold = corenrn.get_prop_param_size()[type];
    int dpold = corenrn.get_prop_dparam_size()[type];
    if (psize != pold || dpsize != dpold) {
        corenrn.get_different_mechanism_type().push_back(type);
    }
    corenrn.get_prop_param_size()[type] = psize;
    corenrn.get_prop_dparam_size()[type] = dpsize;
    if (dpsize) {
        corenrn.get_memb_func(type).dparam_semantics = (int*) ecalloc(dpsize, sizeof(int));
    }
}
void hoc_register_dparam_semantics(int type, int ix, const char* name) {
    /* needed for SoA to possibly reorder name_ion and some "pointer" pointers. */
    /* only interested in area, iontype, cvode_ieq,
       netsend, pointer, pntproc, bbcorepointer, watch, diam, fornetcon
       xx_ion and #xx_ion which will get
       a semantics value of -1, -2, -3,
       -4, -5, -6, -7, -8, -9, -10,
       type, and type+1000 respectively
    */
    auto& memb_func = corenrn.get_memb_funcs();
    if (strcmp(name, "area") == 0) {
        memb_func[type].dparam_semantics[ix] = -1;
    } else if (strcmp(name, "iontype") == 0) {
        memb_func[type].dparam_semantics[ix] = -2;
    } else if (strcmp(name, "cvodeieq") == 0) {
        memb_func[type].dparam_semantics[ix] = -3;
    } else if (strcmp(name, "netsend") == 0) {
        memb_func[type].dparam_semantics[ix] = -4;
    } else if (strcmp(name, "pointer") == 0) {
        memb_func[type].dparam_semantics[ix] = -5;
    } else if (strcmp(name, "pntproc") == 0) {
        memb_func[type].dparam_semantics[ix] = -6;
    } else if (strcmp(name, "bbcorepointer") == 0) {
        memb_func[type].dparam_semantics[ix] = -7;
    } else if (strcmp(name, "watch") == 0) {
        memb_func[type].dparam_semantics[ix] = -8;
    } else if (strcmp(name, "diam") == 0) {
        memb_func[type].dparam_semantics[ix] = -9;
    } else if (strcmp(name, "fornetcon") == 0) {
        memb_func[type].dparam_semantics[ix] = -10;
    } else {
        int i = name[0] == '#' ? 1 : 0;
        int etype = nrn_get_mechtype(name + i);
        memb_func[type].dparam_semantics[ix] = etype + i * 1000;
        /* note that if style is needed (i==1), then we are writing a concentration */
        if (i) {
            ion_write_depend(type, etype);
        }
    }
#if DEBUG
    printf("dparam semantics %s ix=%d %s %d\n",
           memb_func[type].sym,
           ix,
           name,
           memb_func[type].dparam_semantics[ix]);
#endif
}

/* only ion type ion_write_depend_ are non-nullptr */
/* and those are array of integers with first integer being array size */
/* and remaining size-1 integers containing the mechanism types that write concentrations to that
 * ion */
static void ion_write_depend(int type, int etype) {
    auto& memb_func = corenrn.get_memb_funcs();
    auto& ion_write_depend_ = corenrn.get_ion_write_dependency();
    if (ion_write_depend_.size() < memb_func.size()) {
        ion_write_depend_.resize(memb_func.size());
    }

    int size = !ion_write_depend_[etype].empty() ? ion_write_depend_[etype][0] + 1 : 2;

    ion_write_depend_[etype].resize(size, 0);
    ion_write_depend_[etype][0] = size;
    ion_write_depend_[etype][size - 1] = type;
}

static int depend_append(int idep, int* dependencies, int deptype, int type) {
    /* append only if not already in dependencies and != type*/
    bool add = true;
    if (deptype == type) {
        return idep;
    }
    for (int i = 0; i < idep; ++i) {
        if (deptype == dependencies[i]) {
            add = false;
            break;
        }
    }
    if (add) {
        dependencies[idep++] = deptype;
    }
    return idep;
}

/* return list of types that this type depends on (10 should be more than enough) */
/* dependencies must be an array that is large enough to hold that array */
/* number of dependencies is returned */
int nrn_mech_depend(int type, int* dependencies) {
    int dpsize = corenrn.get_prop_dparam_size()[type];
    int* ds = corenrn.get_memb_func(type).dparam_semantics;
    int idep = 0;
    if (ds)
        for (int i = 0; i < dpsize; ++i) {
            if (ds[i] > 0 && ds[i] < 1000) {
                int deptype = ds[i];
                int idepnew = depend_append(idep, dependencies, deptype, type);
                if ((idepnew > idep) && !corenrn.get_ion_write_dependency().empty() &&
                    !corenrn.get_ion_write_dependency()[deptype].empty()) {
                    auto& iwd = corenrn.get_ion_write_dependency()[deptype];
                    int size = iwd[0];
                    for (int j = 1; j < size; ++j) {
                        idepnew = depend_append(idepnew, dependencies, iwd[j], type);
                    }
                }
                idep = idepnew;
            }
        }
    return idep;
}

void register_constructor(mod_f_t c) {
    corenrn.get_memb_funcs().back().constructor = c;
}

void register_destructor(mod_f_t d) {
    corenrn.get_memb_funcs().back().destructor = d;
}

int point_reg_helper(const Symbol* s2) {
    static int next_pointtype = 1; /* starts at 1 since 0 means not point in pnt_map */
    int type = nrn_get_mechtype(s2);

    // No mechanism in the .dat files
    if (type == -1)
        return type;

    corenrn.get_pnt_map()[type] = next_pointtype++;
    corenrn.get_memb_func(type).is_point = 1;

    return corenrn.get_pnt_map()[type];
}

int point_register_mech(const char** m,
                        mod_alloc_t alloc,
                        mod_f_t cur,
                        mod_f_t jacob,
                        mod_f_t stat,
                        mod_f_t initialize,
                        int nrnpointerindex,
                        mod_f_t constructor,
                        mod_f_t destructor,
                        int vectorized) {
    (void) constructor;
    const Symbol* s = m[1];
    register_mech(m, alloc, cur, jacob, stat, initialize, nrnpointerindex, vectorized);
    register_constructor(constructor);
    register_destructor(destructor);
    return point_reg_helper(s);
}

void _modl_cleanup() {}

int state_discon_allowed_;
int state_discon_flag_ = 0;
void state_discontinuity(int /* i */, double* pd, double d) {
    if (state_discon_allowed_ && state_discon_flag_ == 0) {
        *pd = d;
        /*printf("state_discontinuity t=%g pd=%lx d=%g\n", t, (long)pd, d);*/
    }
}

void hoc_reg_ba(int mt, mod_f_t f, int type) {
    if (type == -1)
        return;

    switch (type) { /* see bablk in src/nmodl/nocpout.c */
        case 11:
            type = BEFORE_BREAKPOINT;
            break;
        case 22:
            type = AFTER_SOLVE;
            break;
        case 13:
            type = BEFORE_INITIAL;
            break;
        case 23:
            type = AFTER_INITIAL;
            break;
        case 14:
            type = BEFORE_STEP;
            break;
        default:
            printf("before-after processing type %d for %s not implemented\n",
                   type,
                   corenrn.get_memb_func(mt).sym);
            nrn_exit(1);
    }
    auto bam = (BAMech*) emalloc(sizeof(BAMech));
    bam->f = f;
    bam->type = mt;
    bam->next = corenrn.get_bamech()[type];
    corenrn.get_bamech()[type] = bam;
}

void _nrn_thread_reg0(int i, void (*f)(ThreadDatum*)) {
    if (i == -1)
        return;

    corenrn.get_memb_func(i).thread_cleanup_ = f;
}

void _nrn_thread_reg1(int i, void (*f)(ThreadDatum*)) {
    if (i == -1)
        return;

    corenrn.get_memb_func(i).thread_mem_init_ = f;
}

void _nrn_thread_table_reg(int i,
                           void (*f)(int, int, double*, Datum*, ThreadDatum*, NrnThread*, int)) {
    if (i == -1)
        return;

    corenrn.get_memb_func(i).thread_table_check_ = f;
}

void _nrn_setdata_reg(int i, void (*call)(double*, Datum*)) {
    if (i == -1)
        return;

    corenrn.get_memb_func(i).setdata_ = call;
}

Memb_func::~Memb_func() {
    if (sym != nullptr) {
        free(sym);
    }
    if (dparam_semantics != nullptr) {
        free(dparam_semantics);
    }
}

}  // namespace coreneuron
