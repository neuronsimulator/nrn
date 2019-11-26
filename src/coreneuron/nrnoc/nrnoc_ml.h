/*
Copyright (c) 2019, Blue Brain Project
All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors
   may be used to endorse or promote products derived from this software
   without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
THE POSSIBILITY OF SUCH DAMAGE.
*/
#pragma once

#include "coreneuron/nrnconf.h"
namespace coreneuron {
#if PG_ACC_BUGS
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
};

struct NetSendBuffer_t {
    int* _sendtype;  // net_send, net_event, net_move
    int* _vdata_index;
    int* _pnt_index;
    int* _weight_index;
    double* _nsb_t;
    double* _nsb_flag;
    int _cnt;
    int _size;       /* capacity */
    int reallocated; /* if buffer resized/reallocated, needs to be copy to cpu */
};

struct Memb_list {
    /* nodeindices contains all nodes this extension is responsible for,
     * ordered according to the matrix. This allows to access the matrix
     * directly via the nrn_actual_* arrays instead of accessing it in the
     * order of insertion and via the node-structure, making it more
     * cache-efficient */
    int* nodeindices;
    int* _permute;
    double* data;
    Datum* pdata;
    ThreadDatum* _thread; /* thread specific data (when static is no good) */
    NetReceiveBuffer_t* _net_receive_buffer;
    NetSendBuffer_t* _net_send_buffer;
    int nodecount; /* actual node count */
    int _nodecount_padded;
    void* instance; /* mechanism instance */
};
}  // namespace coreneuron
