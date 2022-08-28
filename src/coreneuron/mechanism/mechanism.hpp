/*
# =============================================================================
# Copyright (c) 2016 - 2021 Blue Brain Project/EPFL
#
# See top-level LICENSE file for details.
# =============================================================================
*/

#pragma once

#include <string.h>

#include "coreneuron/nrnconf.h"
#include "coreneuron/utils/memory.h"

namespace coreneuron {
// OpenACC with PGI compiler has issue when union is used and hence use struct
// \todo check if newer PGI versions has resolved this issue
#if defined(_OPENACC)
struct ThreadDatum {
    int i;
    double* pval;
    void* _pvoid;
};
#else
union ThreadDatum {
    double val;
    int i;
    double* pval;
    void* _pvoid;
};
#endif

/* will go away at some point */
struct Point_process {
    int _i_instance;
    short _type;
    short _tid; /* NrnThread id */
};

struct NetReceiveBuffer_t {
    int* _displ;     /* _displ_cnt + 1 of these */
    int* _nrb_index; /* _cnt of these (order of increasing _pnt_index) */

    int* _pnt_index;
    int* _weight_index;
    double* _nrb_t;
    double* _nrb_flag;
    int _cnt;
    int _displ_cnt; /* number of unique _pnt_index */
    int _size;      /* capacity */
    int _pnt_offset;
    size_t size_of_object() {
        size_t nbytes = 0;
        nbytes += _size * sizeof(int) * 3;
        nbytes += (_size + 1) * sizeof(int);
        nbytes += _size * sizeof(double) * 2;
        return nbytes;
    }
};

struct NetSendBuffer_t: MemoryManaged {
    int* _sendtype;  // net_send, net_event, net_move
    int* _vdata_index;
    int* _pnt_index;
    int* _weight_index;
    double* _nsb_t;
    double* _nsb_flag;
    int _cnt;
    int _size;       /* capacity */
    int reallocated; /* if buffer resized/reallocated, needs to be copy to cpu */

    NetSendBuffer_t(int size)
        : _size(size) {
        _cnt = 0;

        _sendtype = (int*) ecalloc_align(_size, sizeof(int));
        _vdata_index = (int*) ecalloc_align(_size, sizeof(int));
        _pnt_index = (int*) ecalloc_align(_size, sizeof(int));
        _weight_index = (int*) ecalloc_align(_size, sizeof(int));
        // when == 1, NetReceiveBuffer_t is newly allocated (i.e. we need to free previous copy
        // and recopy new data
        reallocated = 1;
        _nsb_t = (double*) ecalloc_align(_size, sizeof(double));
        _nsb_flag = (double*) ecalloc_align(_size, sizeof(double));
    }

    size_t size_of_object() {
        size_t nbytes = 0;
        nbytes += _size * sizeof(int) * 4;
        nbytes += _size * sizeof(double) * 2;
        return nbytes;
    }

    ~NetSendBuffer_t() {
        free_memory(_sendtype);
        free_memory(_vdata_index);
        free_memory(_pnt_index);
        free_memory(_weight_index);
        free_memory(_nsb_t);
        free_memory(_nsb_flag);
    }

    void grow() {
#ifdef CORENEURON_ENABLE_GPU
        int cannot_reallocate_on_device = 0;
        assert(cannot_reallocate_on_device);
#else
        int new_size = _size * 2;
        grow_buf(&_sendtype, _size, new_size);
        grow_buf(&_vdata_index, _size, new_size);
        grow_buf(&_pnt_index, _size, new_size);
        grow_buf(&_weight_index, _size, new_size);
        grow_buf(&_nsb_t, _size, new_size);
        grow_buf(&_nsb_flag, _size, new_size);
        _size = new_size;
#endif
    }

  private:
    template <typename T>
    void grow_buf(T** buf, int size, int new_size) {
        T* new_buf = nullptr;
        new_buf = (T*) ecalloc_align(new_size, sizeof(T));
        memcpy(new_buf, *buf, size * sizeof(T));
        free(*buf);
        *buf = new_buf;
    }
};

struct Memb_list {
    /* nodeindices contains all nodes this extension is responsible for,
     * ordered according to the matrix. This allows to access the matrix
     * directly via the nrn_actual_* arrays instead of accessing it in the
     * order of insertion and via the node-structure, making it more
     * cache-efficient */
    int* nodeindices = nullptr;
    int* _permute = nullptr;
    double* data = nullptr;
    Datum* pdata = nullptr;
    ThreadDatum* _thread = nullptr; /* thread specific data (when static is no good) */
    NetReceiveBuffer_t* _net_receive_buffer = nullptr;
    NetSendBuffer_t* _net_send_buffer = nullptr;
    int nodecount; /* actual node count */
    int _nodecount_padded;
    void* instance{nullptr}; /* mechanism instance struct */
    // nrn_acc_manager.cpp handles data movement to/from the accelerator as the
    // "private constructor" in the translated MOD file code is called before
    // the main nrn_acc_manager methods that copy thread/mechanism data to the
    // device
    void* global_variables{nullptr};
    std::size_t global_variables_size{};
};
}  // namespace coreneuron
