/***
  THIS FILE IS AUTO GENERATED DONT MODIFY IT.
 ***/
/*
# =============================================================================
# Copyright (c) 2016 - 2021 Blue Brain Project/EPFL
#
# See top-level LICENSE file for details.
# =============================================================================.
*/

#include "coreneuron/coreneuron.hpp"
#include "coreneuron/permute/data_layout.hpp"

// clang-format off

#if defined(_OPENACC)
#define _PRAGMA_FOR_INIT_ACC_LOOP_  \
    _Pragma("acc parallel loop present(vdata[0:_cntml_padded*nparm]) if(_nt->compute_gpu)")
#define _PRAGMA_FOR_CUR_ACC_LOOP_   \
    _Pragma(                        \
        "acc parallel loop present(vdata[0:_cntml_padded*nparm], ni[0:_cntml_actual], _vec_rhs[0:_nt->end]) if(_nt->compute_gpu) async(stream_id)")
#define _PRAGMA_FOR_JACOB_ACC_LOOP_ \
    _Pragma(                        \
        "acc parallel loop present(vdata[0:_cntml_padded*nparm], ni[0:_cntml_actual], _vec_d[0:_nt->end]) if(_nt->compute_gpu) async(stream_id)")
#else
#define _PRAGMA_FOR_INIT_ACC_LOOP_  _Pragma("")
#define _PRAGMA_FOR_CUR_ACC_LOOP_   _Pragma("")
#define _PRAGMA_FOR_JACOB_ACC_LOOP_ _Pragma("")
#endif

// clang-format on

#define _STRIDE _cntml_padded + _iml

namespace coreneuron {

static const char* mechanism[] = {"0", "capacitance", "cm", 0, "i_cap", 0, 0};
void nrn_alloc_capacitance(double*, Datum*, int);
void nrn_init_capacitance(NrnThread*, Memb_list*, int);
void nrn_jacob_capacitance(NrnThread*, Memb_list*, int);
void nrn_div_capacity(NrnThread*, Memb_list*, int);
void nrn_mul_capacity(NrnThread*, Memb_list*, int);

#define nparm 2

void capacitance_reg(void) {
    /* all methods deal with capacitance in special ways */
    register_mech(mechanism,
                  nrn_alloc_capacitance,
                  (mod_f_t) 0,
                  (mod_f_t) 0,
                  (mod_f_t) 0,
                  (mod_f_t) nrn_init_capacitance,
                  -1,
                  1);
    int mechtype = nrn_get_mechtype(mechanism[1]);
    _nrn_layout_reg(mechtype, SOA_LAYOUT);
    hoc_register_prop_size(mechtype, nparm, 0);
}

#define cm    vdata[0 * _STRIDE]
#define i_cap vdata[1 * _STRIDE]

/*
cj is analogous to 1/dt for cvode and daspk
for fixed step second order it is 2/dt and
for pure implicit fixed step it is 1/dt
It used to be static but is now a thread data variable
*/

void nrn_jacob_capacitance(NrnThread* _nt, Memb_list* ml, int /* type */) {
    int _cntml_actual = ml->nodecount;
    int _cntml_padded = ml->_nodecount_padded;
    int _iml;
    double* vdata;
    double cfac = .001 * _nt->cj;
    (void) _cntml_padded; /* unused when layout=1*/

    double* _vec_d = _nt->_actual_d;
#if defined(_OPENACC)
    int stream_id = _nt->stream_id;
#endif

    { /*if (use_cachevec) {*/
        int* ni = ml->nodeindices;

        vdata = ml->data;
        _PRAGMA_FOR_JACOB_ACC_LOOP_
        for (_iml = 0; _iml < _cntml_actual; _iml++) {
            _vec_d[ni[_iml]] += cfac * cm;
        }
    }
}

void nrn_init_capacitance(NrnThread* _nt, Memb_list* ml, int /* type */) {
    int _cntml_actual = ml->nodecount;
    int _cntml_padded = ml->_nodecount_padded;
    double* vdata;
    (void) _cntml_padded; /* unused */

    // skip initialization if restoring from checkpoint
    if (_nrn_skip_initmodel == 1) {
        return;
    }

    vdata = ml->data;
    _PRAGMA_FOR_INIT_ACC_LOOP_
    for (int _iml = 0; _iml < _cntml_actual; _iml++) {
        i_cap = 0;
    }
}

void nrn_cur_capacitance(NrnThread* _nt, Memb_list* ml, int /* type */) {
    int _cntml_actual = ml->nodecount;
    int _cntml_padded = ml->_nodecount_padded;
    double* vdata;
    double cfac = .001 * _nt->cj;

    /*@todo: verify cfac is being copied !! */

    (void) _cntml_padded; /* unused when layout=1*/

    /* since rhs is dvm for a full or half implicit step */
    /* (nrn_update_2d() replaces dvi by dvi-dvx) */
    /* no need to distinguish secondorder */
    int* ni = ml->nodeindices;
    double* _vec_rhs = _nt->_actual_rhs;
#if defined(_OPENACC)
    int stream_id = _nt->stream_id;
#endif

    vdata = ml->data;
    _PRAGMA_FOR_CUR_ACC_LOOP_
    for (int _iml = 0; _iml < _cntml_actual; _iml++) {
        i_cap = cfac * cm * _vec_rhs[ni[_iml]];
    }
}

/* the rest can be constructed automatically from the above info*/

void nrn_alloc_capacitance(double* data, Datum* pdata, int type) {
    (void) pdata;
    (void) type;      /* unused */
    data[0] = DEF_cm; /*default capacitance/cm^2*/
}

void nrn_div_capacity(NrnThread* _nt, Memb_list* ml, int type) {
    (void) type;
    int _cntml_actual = ml->nodecount;
    int _cntml_padded = ml->_nodecount_padded;
    int _iml;
    double* vdata;
    (void) _nt;
    (void) type;
    (void) _cntml_padded; /* unused */

    int* ni = ml->nodeindices;

    vdata = ml->data;
    _PRAGMA_FOR_INIT_ACC_LOOP_
    for (_iml = 0; _iml < _cntml_actual; _iml++) {
        i_cap = VEC_RHS(ni[_iml]);
        VEC_RHS(ni[_iml]) /= 1.e-3 * cm;
        // fprintf(stderr, "== nrn_div_cap: RHS[%d]=%.12f\n", ni[_iml], VEC_RHS(ni[_iml])) ;
    }
}

void nrn_mul_capacity(NrnThread* _nt, Memb_list* ml, int type) {
    (void) type;
    int _cntml_actual = ml->nodecount;
    int _cntml_padded = ml->_nodecount_padded;
    int _iml;
    double* vdata;
    (void) _nt;
    (void) type;
    (void) _cntml_padded; /* unused */

    int* ni = ml->nodeindices;

    const double cfac = .001 * _nt->cj;

    vdata = ml->data;
    _PRAGMA_FOR_INIT_ACC_LOOP_
    for (_iml = 0; _iml < _cntml_actual; _iml++) {
        VEC_RHS(ni[_iml]) *= cfac * cm;
    }
}
}  // namespace coreneuron
