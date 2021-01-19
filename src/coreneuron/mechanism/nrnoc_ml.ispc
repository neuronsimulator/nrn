/*
# =============================================================================
# Copyright (C) 2016-2021 Blue Brain Project
#
# See top-level LICENSE file for details.
# =============================================================================
*/

/// list of data structures required for ISPC backend

typedef int Datum;


struct ThreadDatum {
    int i;
    double* uniform pval;
    void* uniform _pvoid;
};


struct NetReceiveBuffer_t {
    int* uniform _displ;
    int* uniform _nrb_index;

    int* uniform _pnt_index;
    int* uniform _weight_index;
    double* uniform _nrb_t;
    double* uniform _nrb_flag;
    int _cnt;
    int _displ_cnt;
    int _size;
    int _pnt_offset;
};


struct NetSendBuffer_t {
  int* _sendtype;  // net_send, net_event, net_move
  int* _vdata_index;
  int* _pnt_index;
  int* _weight_index;
  double* _nsb_t;
  double* _nsb_flag;
  uniform int _cnt;
  uniform int _size;       /* capacity */
  uniform int reallocated; /* if buffer resized/reallocated, needs to be copy to cpu */
};


struct Memb_list {
    uniform int* nodeindices;
    uniform int* _permute;
    uniform double* data;
    uniform Datum* pdata;
    uniform ThreadDatum* _thread;
    uniform NetReceiveBuffer_t* _net_receive_buffer;
    uniform NetSendBuffer_t* _net_send_buffer; // not needed
    uniform int nodecount;
    uniform int _nodecount_padded;
    void* uniform instance;
};

struct Point_process {
    uniform int _i_instance;
    uniform int16 _type;
    uniform int16 _tid;
};

struct NrnThreadMembList {

};

struct PreSyn {

};

struct PreSynHelper {

};

struct NetCon {

};

struct NrnThreadBAList {

};

struct NrnFastImem {
    double* uniform nrn_sav_rhs;
    double* uniform nrn_sav_d;
};

struct NrnThread {
    uniform double _t;
    uniform double _dt;
    uniform double cj;

    uniform NrnThreadMembList* uniform tml;
    uniform Memb_list** uniform _ml_list;
    Point_process* uniform pntprocs;
    PreSyn* uniform presyns;
    PreSynHelper* uniform presyns_helper;
    int** uniform pnt2presyn_ix;
    NetCon* uniform netcons;
    double* uniform weights;

    uniform int n_pntproc, n_presyn, n_input_presyn, n_netcon, n_weight;

    uniform int ncell;
    uniform int end;
    uniform int id;
    uniform int _stop_stepping;
    uniform int n_vecplay;

    uniform unsigned int64 _ndata, _nidata, _nvdata;
    double* uniform _data;
    int* uniform _idata;

    void** uniform _vdata;
    void** uniform _vecplay;

    double* uniform _actual_rhs;
    double* uniform _actual_d;
    double* uniform _actual_a;
    double* uniform _actual_b;
    double* uniform _actual_v;
    double* uniform _actual_area;
    double* uniform _actual_diam;
    double* uniform _shadow_rhs;
    double* uniform _shadow_d;
    NrnFastImem* uniform nrn_fast_imem;
    int* uniform _v_parent_index;
    int* uniform _permute;
    int8* uniform _sp13mat;
    Memb_list* _ecell_memb_list;
    double _ctime;
    NrnThreadBAList* tbl[1];

    uniform int shadow_rhs_cnt;
    uniform int compute_gpu;
    uniform int stream_id;
    uniform int _net_send_buffer_size;
    uniform int _net_send_buffer_cnt;
    int* _net_send_buffer;
    int* _watch_types;
    void* mapping;
};

extern uniform double ispc_celsius;

// NOTE : this implementation is duplicated from "coreneuron/network/cvodestb.cpp"
// If changes are required, make sure to change CPP as well.
static inline int at_time(uniform NrnThread* nt, varying double te) {
    varying double x = te - 1e-11;
    if (x <= nt->_t && x > (nt->_t - nt->_dt)) {
        return 1;
    }
    return 0;
}
