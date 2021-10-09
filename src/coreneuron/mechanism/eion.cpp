/*
# =============================================================================
# Copyright (c) 2016 - 2021 Blue Brain Project/EPFL
#
# See top-level LICENSE file for details.
# =============================================================================.
*/

/// THIS FILE IS AUTO GENERATED DONT MODIFY IT.

#include <math.h>
#include <string.h>

#include "coreneuron/coreneuron.hpp"
#include "coreneuron/mpi/nrnmpi.h"
#include "coreneuron/mechanism/membfunc.hpp"
#include "coreneuron/utils/nrnoc_aux.hpp"

#if !defined(LAYOUT)
/* 1 means AoS, >1 means AoSoA, <= 0 means SOA */
#define LAYOUT 1
#endif
#if LAYOUT >= 1
#define _STRIDE LAYOUT
#else
#define _STRIDE _cntml_padded + _iml
#endif

// clang-format off

#if defined(_OPENACC)
#define _PRAGMA_FOR_INIT_ACC_LOOP_  \
    _Pragma(                        \
        "acc parallel loop present(pd[0:_cntml_padded*5], ppd[0:1], nrn_ion_global_map[0:nrn_ion_global_map_size][0:ion_global_map_member_size]) if(nt->compute_gpu)")
#define _PRAGMA_FOR_CUR_ACC_LOOP_   \
    _Pragma(                        \
        "acc parallel loop present(pd[0:_cntml_padded*5], nrn_ion_global_map[0:nrn_ion_global_map_size][0:ion_global_map_member_size]) if(nt->compute_gpu) async(stream_id)")
#define _PRAGMA_FOR_SEC_ORDER_CUR_ACC_LOOP_     \
    _Pragma(                                    \
        "acc parallel loop present(pd[0:_cntml_padded*5], ni[0:_cntml_actual], _vec_rhs[0:_nt->end]) if(_nt->compute_gpu) async(stream_id)")
#else
#define _PRAGMA_FOR_INIT_ACC_LOOP_          _Pragma("")
#define _PRAGMA_FOR_CUR_ACC_LOOP_           _Pragma("")
#define _PRAGMA_FOR_SEC_ORDER_CUR_ACC_LOOP_ _Pragma("")
#endif

// clang-format on

namespace coreneuron {

// for each ion it refers to internal concentration, external concentration, and charge,
const int ion_global_map_member_size = 3;


#define nparm 5
static const char* mechanism[] = {/*just a template*/
                                  "0",
                                  "na_ion",
                                  "ena",
                                  "nao",
                                  "nai",
                                  0,
                                  "ina",
                                  "dina_dv_",
                                  0,
                                  0};

void nrn_init_ion(NrnThread*, Memb_list*, int);
void nrn_alloc_ion(double*, Datum*, int);

static int na_ion, k_ion, ca_ion; /* will get type for these special ions */

int nrn_is_ion(int type) {
    // Old: commented to remove dependency on memb_func and alloc function
    // return (memb_func[type].alloc == ion_alloc);
    return (type < nrn_ion_global_map_size            // type smaller than largest ion's
            && nrn_ion_global_map[type] != nullptr);  // allocated ion charge variables
}

int nrn_ion_global_map_size;
double** nrn_ion_global_map;
#define global_conci(type)  nrn_ion_global_map[type][0]
#define global_conco(type)  nrn_ion_global_map[type][1]
#define global_charge(type) nrn_ion_global_map[type][2]

double nrn_ion_charge(int type) {
    return global_charge(type);
}

void ion_reg(const char* name, double valence) {
    char buf[7][50];
#define VAL_SENTINAL -10000.

    sprintf(buf[0], "%s_ion", name);
    sprintf(buf[1], "e%s", name);
    sprintf(buf[2], "%si", name);
    sprintf(buf[3], "%so", name);
    sprintf(buf[5], "i%s", name);
    sprintf(buf[6], "di%s_dv_", name);
    for (int i = 0; i < 7; i++) {
        mechanism[i + 1] = buf[i];
    }
    mechanism[5] = nullptr; /* buf[4] not used above */
    int mechtype = nrn_get_mechtype(buf[0]);
    if (mechtype >= nrn_ion_global_map_size ||
        nrn_ion_global_map[mechtype] == nullptr) {  // if hasn't yet been allocated

        // allocates mem for ion in ion_map and sets null all non-ion types
        if (nrn_ion_global_map_size <= mechtype) {
            int size = mechtype + 1;
            nrn_ion_global_map = (double**) erealloc(nrn_ion_global_map, sizeof(double*) * size);

            for (int i = nrn_ion_global_map_size; i < mechtype; i++) {
                nrn_ion_global_map[i] = nullptr;
            }
            nrn_ion_global_map_size = mechtype + 1;
        }
        nrn_ion_global_map[mechtype] = (double*) emalloc(ion_global_map_member_size *
                                                         sizeof(double));

        register_mech((const char**) mechanism,
                      nrn_alloc_ion,
                      nrn_cur_ion,
                      (mod_f_t) 0,
                      (mod_f_t) 0,
                      (mod_f_t) nrn_init_ion,
                      -1,
                      1);
        mechtype = nrn_get_mechtype(mechanism[1]);
        _nrn_layout_reg(mechtype, LAYOUT);
        hoc_register_prop_size(mechtype, nparm, 1);
        hoc_register_dparam_semantics(mechtype, 0, "iontype");
        nrn_writes_conc(mechtype, 1);

        sprintf(buf[0], "%si0_%s", name, buf[0]);
        sprintf(buf[1], "%so0_%s", name, buf[0]);
        if (strcmp("na", name) == 0) {
            na_ion = mechtype;
            global_conci(mechtype) = DEF_nai;
            global_conco(mechtype) = DEF_nao;
            global_charge(mechtype) = 1.;
        } else if (strcmp("k", name) == 0) {
            k_ion = mechtype;
            global_conci(mechtype) = DEF_ki;
            global_conco(mechtype) = DEF_ko;
            global_charge(mechtype) = 1.;
        } else if (strcmp("ca", name) == 0) {
            ca_ion = mechtype;
            global_conci(mechtype) = DEF_cai;
            global_conco(mechtype) = DEF_cao;
            global_charge(mechtype) = 2.;
        } else {
            global_conci(mechtype) = DEF_ioni;
            global_conco(mechtype) = DEF_iono;
            global_charge(mechtype) = VAL_SENTINAL;
        }
    }
    double val = global_charge(mechtype);
    if (valence != VAL_SENTINAL && val != VAL_SENTINAL && valence != val) {
        fprintf(stderr,
                "%s ion valence defined differently in\n\
two USEION statements (%g and %g)\n",
                buf[0],
                valence,
                global_charge(mechtype));
        nrn_exit(1);
    } else if (valence == VAL_SENTINAL && val == VAL_SENTINAL) {
        fprintf(stderr,
                "%s ion valence must be defined in\n\
the USEION statement of any model using this ion\n",
                buf[0]);
        nrn_exit(1);
    } else if (valence != VAL_SENTINAL) {
        global_charge(mechtype) = valence;
    }
}

#ifndef CORENRN_USE_LEGACY_UNITS
#define CORENRN_USE_LEGACY_UNITS 0
#endif

#if CORENRN_USE_LEGACY_UNITS == 1
#define FARADAY     96485.309
#define gasconstant 8.3134
#else
#include "coreneuron/nrnoc/nrnunits_modern.h"
#define FARADAY     _faraday_codata2018
#define gasconstant _gasconstant_codata2018
#endif

#define ktf (1000. * gasconstant * (celsius + 273.15) / FARADAY)

double nrn_nernst(double ci, double co, double z, double celsius) {
    /*printf("nrn_nernst %g %g %g\n", ci, co, z);*/
    if (z == 0) {
        return 0.;
    }
    if (ci <= 0.) {
        return 1e6;
    } else if (co <= 0.) {
        return -1e6;
    } else {
        return ktf / z * log(co / ci);
    }
}

void nrn_wrote_conc(int type,
                    double* p1,
                    int p2,
                    int it,
                    double** gimap,
                    double celsius,
                    int _cntml_padded) {
    if (it & 040) {
#if LAYOUT <= 0 /* SoA */
        int _iml = 0;
/* passing _nt to this function causes cray compiler to segfault during compilation
 * hence passing _cntml_padded
 */
#else
        (void) _cntml_padded;
#endif
        double* pe = p1 - p2 * _STRIDE;
        pe[0] = nrn_nernst(pe[1 * _STRIDE], pe[2 * _STRIDE], gimap[type][2], celsius);
    }
}

static double efun(double x) {
    if (fabs(x) < 1e-4) {
        return 1. - x / 2.;
    } else {
        return x / (exp(x) - 1);
    }
}

double nrn_ghk(double v, double ci, double co, double z) {
    double temp = z * v / ktf;
    double eco = co * efun(temp);
    double eci = ci * efun(-temp);
    return (.001) * z * FARADAY * (eci - eco);
}

#if VECTORIZE
#define erev   pd[0 * _STRIDE] /* From Eion */
#define conci  pd[1 * _STRIDE]
#define conco  pd[2 * _STRIDE]
#define cur    pd[3 * _STRIDE]
#define dcurdv pd[4 * _STRIDE]

/*
 handle erev, conci, conc0 "in the right way" according to ion_style
 default. See nrn/lib/help/nrnoc.help.
ion_style("name_ion", [c_style, e_style, einit, eadvance, cinit])

 ica is assigned
 eca is parameter but if conc exists then eca is assigned
 if conc is nrnocCONST then eca calculated on finitialize
 if conc is STATE then eca calculated on fadvance and conc finitialize
        with global nai0, nao0

 nernst(ci, co, charge) and ghk(v, ci, co, charge) available to hoc
 and models.
*/

#define iontype ppd[0] /* how _AMBIGUOUS is to be handled */
/*the bitmap is
03	concentration unused, nrnocCONST, DEP, STATE
04	initialize concentrations
030	reversal potential unused, nrnocCONST, DEP, STATE
040	initialize reversal potential
0100	calc reversal during fadvance
0200	ci being written by a model
0400	co being written by a model
*/

#define charge global_charge(type)
#define conci0 global_conci(type)
#define conco0 global_conco(type)

double nrn_nernst_coef(int type) {
    /* for computing jacobian element dconc'/dconc */
    return ktf / charge;
}

/* Must be called prior to any channels which update the currents */
void nrn_cur_ion(NrnThread* nt, Memb_list* ml, int type) {
    int _cntml_actual = ml->nodecount;
    double* pd;
    Datum* ppd;
    (void) nt; /* unused */
#if defined(_OPENACC)
    int stream_id = nt->stream_id;
#endif
/*printf("ion_cur %s\n", memb_func[type].sym->name);*/
// AoS
#if LAYOUT == 1
    for (int _iml = 0; _iml < _cntml_actual; ++_iml) {
        pd = ml->data + _iml * nparm;
        ppd = ml->pdata + _iml * 1;
// SoA
#elif LAYOUT == 0
    int _cntml_padded = ml->_nodecount_padded;
    pd = ml->data;
    ppd = ml->pdata;
    _PRAGMA_FOR_CUR_ACC_LOOP_
    for (int _iml = 0; _iml < _cntml_actual; ++_iml) {
#else
#error AoSoA not implemented.
#endif
        dcurdv = 0.;
        cur = 0.;
        if (iontype & 0100) {
            erev = nrn_nernst(conci, conco, charge, celsius);
        }
    };
}

/* Must be called prior to other models which possibly also initialize
        concentrations based on their own states
*/
void nrn_init_ion(NrnThread* nt, Memb_list* ml, int type) {
    int _cntml_actual = ml->nodecount;
    double* pd;
    Datum* ppd;
    (void) nt; /* unused */

    // skip initialization if restoring from checkpoint
    if (_nrn_skip_initmodel == 1) {
        return;
    }

/*printf("ion_init %s\n", memb_func[type].sym->name);*/
// AoS
#if LAYOUT == 1
    for (int _iml = 0; _iml < _cntml_actual; ++_iml) {
        pd = ml->data + _iml * nparm;
        ppd = ml->pdata + _iml * 1;
// SoA
#elif LAYOUT == 0
    int _cntml_padded = ml->_nodecount_padded;
    pd = ml->data;
    ppd = ml->pdata;
    _PRAGMA_FOR_INIT_ACC_LOOP_
    for (int _iml = 0; _iml < _cntml_actual; ++_iml) {
#else
#error AoSoA not implemented.
#endif
        if (iontype & 04) {
            conci = conci0;
            conco = conco0;
        }
        if (iontype & 040) {
            erev = nrn_nernst(conci, conco, charge, celsius);
        }
    }
}

void nrn_alloc_ion(double* p, Datum* ppvar, int _type) {
    assert(0);
}

void second_order_cur(NrnThread* _nt, int secondorder) {
#if LAYOUT == 0
    int _cntml_padded;
#endif
    double* pd;
    (void) _nt; /* unused */
#if defined(_OPENACC)
    int stream_id = _nt->stream_id;
#endif
    double* _vec_rhs = _nt->_actual_rhs;

    if (secondorder == 2) {
        for (NrnThreadMembList* tml = _nt->tml; tml; tml = tml->next)
            if (nrn_is_ion(tml->index)) {
                Memb_list* ml = tml->ml;
                int _cntml_actual = ml->nodecount;
                int* ni = ml->nodeindices;
// AoS
#if LAYOUT == 1
                for (int _iml = 0; _iml < _cntml_actual; ++_iml) {
                    pd = ml->data + _iml * nparm;
// SoA
#elif LAYOUT == 0
                _cntml_padded = ml->_nodecount_padded;
                pd = ml->data;
                _PRAGMA_FOR_SEC_ORDER_CUR_ACC_LOOP_
                for (int _iml = 0; _iml < _cntml_actual; ++_iml) {
#else
#error AoSoA not implemented.
#endif
                    cur += dcurdv * (_vec_rhs[ni[_iml]]);
                }
            }
    }
}
}  // namespace coreneuron
#endif
