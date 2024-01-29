/*
# =============================================================================
# Copyright (c) 2016 - 2022 Blue Brain Project/EPFL
#
# See top-level LICENSE file for details.
# =============================================================================.
*/

#include <math.h>
#include <string.h>

#include "coreneuron/coreneuron.hpp"
#include "coreneuron/mpi/nrnmpi.h"
#include "coreneuron/mechanism/mech_mapping.hpp"
#include "coreneuron/mechanism/membfunc.hpp"
#include "coreneuron/permute/data_layout.hpp"
#include "coreneuron/utils/nrnoc_aux.hpp"

#define CNRN_FLAT_INDEX_IML_ROW(i) ((i) * (_cntml_padded) + (_iml))

namespace coreneuron {

// for each ion it refers to internal concentration, external concentration, and charge,
const int ion_global_map_member_size = 3;


#define nparm 5
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
    std::array<std::string, 7> buf{};
#define VAL_SENTINAL -10000.
    std::string name_str{name};
    buf[0] = name_str + "_ion";
    buf[1] = "e" + name_str;
    buf[2] = name_str + "i";
    buf[3] = name_str + "o";
    buf[5] = "i" + name_str;
    buf[6] = "di" + name_str + "_dv_";
    std::array<const char*,
               buf.size() + 1 /* shifted by 1 */ + NB_MECH_VAR_CATEGORIES /* trailing nulls */>
        mech{};
    for (int i = 0; i < buf.size(); i++) {
        mech[i + 1] = buf[i].empty() ? nullptr : buf[i].c_str();
    }
    int mechtype = nrn_get_mechtype(buf[0].c_str());
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

        register_mech(mech.data(),
                      nrn_alloc_ion,
                      nrn_cur_ion,
                      nullptr,
                      nullptr,
                      nrn_init_ion,
                      nullptr,
                      nullptr,
                      -1,
                      1);
        mechtype = nrn_get_mechtype(mech[1]);
        _nrn_layout_reg(mechtype, SOA_LAYOUT);
        hoc_register_prop_size(mechtype, nparm, 1);
        hoc_register_dparam_semantics(mechtype, 0, "iontype");
        nrn_writes_conc(mechtype, 1);

        buf[0] = name_str + "i0_" + buf[0];
        buf[1] = name_str + "o0_" + buf[0];
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
                buf[0].c_str(),
                valence,
                global_charge(mechtype));
        nrn_exit(1);
    } else if (valence == VAL_SENTINAL && val == VAL_SENTINAL) {
        fprintf(stderr,
                "%s ion valence must be defined in\n\
the USEION statement of any model using this ion\n",
                buf[0].c_str());
        nrn_exit(1);
    } else if (valence != VAL_SENTINAL) {
        global_charge(mechtype) = valence;
    }
}

#if VECTORIZE
#define erev   pd[CNRN_FLAT_INDEX_IML_ROW(0)] /* From Eion */
#define conci  pd[CNRN_FLAT_INDEX_IML_ROW(1)]
#define conco  pd[CNRN_FLAT_INDEX_IML_ROW(2)]
#define cur    pd[CNRN_FLAT_INDEX_IML_ROW(3)]
#define dcurdv pd[CNRN_FLAT_INDEX_IML_ROW(4)]

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

#define iontype ppd[_iml] /* how _AMBIGUOUS is to be handled */
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
    return ktf(celsius) / charge;
}

/* Must be called prior to any channels which update the currents */
void nrn_cur_ion(NrnThread* nt, Memb_list* ml, int type) {
    int _cntml_actual = ml->nodecount;
    double* pd;
    Datum* ppd;
    (void) nt; /* unused */
    /*printf("ion_cur %s\n", memb_func[type].sym->name);*/
    int _cntml_padded = ml->_nodecount_padded;
    pd = ml->data;
    ppd = ml->pdata;
    // clang-format off
    nrn_pragma_acc(parallel loop present(pd[0:_cntml_padded * 5],
                                         ppd[0:_cntml_actual],
                                         nrn_ion_global_map[0:nrn_ion_global_map_size]
                                                           [0:ion_global_map_member_size])
                                 if (nt->compute_gpu)
                                 async(nt->stream_id))
    // clang-format on
    nrn_pragma_omp(target teams distribute parallel for simd if(nt->compute_gpu))
    for (int _iml = 0; _iml < _cntml_actual; ++_iml) {
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
    int _cntml_padded = ml->_nodecount_padded;
    pd = ml->data;
    ppd = ml->pdata;
    // There was no async(...) clause in the initial OpenACC implementation, so
    // no `nowait` clause has been added to the OpenMP implementation. TODO:
    // verify if this can be made asynchronous or if there is a strong reason it
    // needs to be like this.
    // clang-format off
    nrn_pragma_acc(parallel loop present(pd[0:_cntml_padded * 5],
                                         ppd[0:_cntml_actual],
                                         nrn_ion_global_map[0:nrn_ion_global_map_size]
                                                           [0:ion_global_map_member_size])
                                 if (nt->compute_gpu))
    // clang-format on
    nrn_pragma_omp(target teams distribute parallel for simd if(nt->compute_gpu))
    for (int _iml = 0; _iml < _cntml_actual; ++_iml) {
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
    int _cntml_padded;
    double* pd;
    (void) _nt; /* unused */
    double* _vec_rhs = _nt->_actual_rhs;

    if (secondorder == 2) {
        for (NrnThreadMembList* tml = _nt->tml; tml; tml = tml->next)
            if (nrn_is_ion(tml->index)) {
                Memb_list* ml = tml->ml;
                int _cntml_actual = ml->nodecount;
                int* ni = ml->nodeindices;
                _cntml_padded = ml->_nodecount_padded;
                pd = ml->data;
                nrn_pragma_acc(parallel loop present(pd [0:_cntml_padded * 5],
                                                     ni [0:_cntml_actual],
                                                     _vec_rhs [0:_nt->end]) if (_nt->compute_gpu)
                                   async(_nt->stream_id))
                nrn_pragma_omp(target teams distribute parallel for simd if(_nt->compute_gpu))
                for (int _iml = 0; _iml < _cntml_actual; ++_iml) {
                    cur += dcurdv * (_vec_rhs[ni[_iml]]);
                }
            }
    }
}
}  // namespace coreneuron
#endif
