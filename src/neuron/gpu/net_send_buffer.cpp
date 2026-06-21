#include "neuron/gpu/net_send_buffer.hpp"

#include "coreneuron/utils/offload.hpp"
#include "multicore.h"
#include "nrnoc_ml.h"
#include "section_fwd.hpp"

#include <algorithm>
#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <vector>

extern void nrn_net_send(void* v, double* weight, Point_process* pnt, double td, double flag);
extern void net_event(Point_process* pnt, double time);
extern void nrn_net_move(Datum* v, Point_process* pnt, double tt);

namespace neuron::gpu {

std::vector<int> net_buf_send_types;

namespace {

template <typename T>
T* alloc_buffer(int size) {
    return static_cast<T*>(std::calloc(static_cast<std::size_t>(size), sizeof(T)));
}

template <typename T>
void grow_buffer(T** buf, int old_size, int new_size) {
    T* new_buf = alloc_buffer<T>(new_size);
    if (*buf) {
        std::copy_n(*buf, old_size, new_buf);
        std::free(*buf);
    }
    *buf = new_buf;
}

}  // namespace

NetSendBuffer_t::NetSendBuffer_t(int size)
    : _size(size) {
    _sendtype = alloc_buffer<int>(_size);
    _vdata_index = alloc_buffer<int>(_size);
    _pnt_index = alloc_buffer<int>(_size);
    _weight_index = alloc_buffer<int>(_size);
    _nsb_t = alloc_buffer<double>(_size);
    _nsb_flag = alloc_buffer<double>(_size);
    reallocated = 1;
}

NetSendBuffer_t::~NetSendBuffer_t() {
    std::free(_sendtype);
    std::free(_vdata_index);
    std::free(_pnt_index);
    std::free(_weight_index);
    std::free(_nsb_t);
    std::free(_nsb_flag);
}

void NetSendBuffer_t::grow() {
#if defined(NRN_ENABLE_GPU)
    // GPU execution cannot reallocate on device; host grow is used before upload.
    if (_cnt >= _size) {
        int const new_size = std::max(_size * 2, _cnt + 1);
        grow_buffer(&_sendtype, _size, new_size);
        grow_buffer(&_vdata_index, _size, new_size);
        grow_buffer(&_pnt_index, _size, new_size);
        grow_buffer(&_weight_index, _size, new_size);
        grow_buffer(&_nsb_t, _size, new_size);
        grow_buffer(&_nsb_flag, _size, new_size);
        _size = new_size;
        reallocated = 1;
    }
#else
    (void) 0;
#endif
}

std::size_t NetSendBuffer_t::size_of_object() const {
    std::size_t nbytes = 0;
    nbytes += static_cast<std::size_t>(_size) * sizeof(int) * 4;
    nbytes += static_cast<std::size_t>(_size) * sizeof(double) * 2;
    return nbytes;
}

void net_send_buffer_ensure(Memb_list* ml) {
    if (!ml || ml->_net_send_buffer) {
        return;
    }
    int const capacity = std::max(8, ml->nodecount * 2);
    ml->_net_send_buffer = new NetSendBuffer_t(capacity);
}

void update_net_send_buffer_on_host(NrnThread* nt, NetSendBuffer_t* nsb) {
#if defined(NRN_ENABLE_GPU)
    if (!nt || !nsb || !nt->compute_gpu) {
        return;
    }
    if (nsb->_cnt > nsb->_size) {
        fprintf(stderr,
                "ERROR: NetSendBuffer exceeded during GPU execution (thread %d)\n",
                nt->id);
        std::abort();
    }
    if (!nsb->_cnt) {
        return;
    }
    nrn_pragma_acc(update self(nsb->_sendtype[:nsb->_cnt],
                               nsb->_vdata_index[:nsb->_cnt],
                               nsb->_pnt_index[:nsb->_cnt],
                               nsb->_weight_index[:nsb->_cnt],
                               nsb->_nsb_t[:nsb->_cnt],
                               nsb->_nsb_flag[:nsb->_cnt])
                       if (nsb->_cnt))
    nrn_pragma_omp(target update from(nsb->_sendtype[:nsb->_cnt],
                                      nsb->_vdata_index[:nsb->_cnt],
                                      nsb->_pnt_index[:nsb->_cnt],
                                      nsb->_weight_index[:nsb->_cnt],
                                      nsb->_nsb_t[:nsb->_cnt],
                                      nsb->_nsb_flag[:nsb->_cnt])
                                 if (nsb->_cnt))
#else
    (void) nt;
    (void) nsb;
#endif
}

void deliver_net_send_buffer_events(NrnThread* nt, NetSendBuffer_t* nsb) {
    if (!nt || !nsb || !nsb->_cnt) {
        return;
    }
    for (int i = 0; i < nsb->_cnt; ++i) {
        auto* const pnt = reinterpret_cast<Point_process*>(static_cast<intptr_t>(nsb->_pnt_index[i]));
        auto* const weight = nsb->_weight_index[i] >= 0
                                 ? reinterpret_cast<double*>(static_cast<intptr_t>(nsb->_weight_index[i]))
                                 : nullptr;
        auto* const vdata = nsb->_vdata_index[i] >= 0
                                ? reinterpret_cast<void*>(static_cast<intptr_t>(nsb->_vdata_index[i]))
                                : nullptr;
        switch (nsb->_sendtype[i]) {
        case 0:
            nrn_net_send(vdata, weight, pnt, nsb->_nsb_t[i], nsb->_nsb_flag[i]);
            break;
        case 1:
            net_event(pnt, nsb->_nsb_t[i]);
            break;
        case 2:
            nrn_net_move(static_cast<Datum*>(vdata), pnt, nsb->_nsb_t[i]);
            break;
        default:
            break;
        }
    }
    nsb->_cnt = 0;
#if defined(NRN_ENABLE_GPU)
    if (nt->compute_gpu) {
        nrn_pragma_acc(update device(nsb->_cnt) if (nt->compute_gpu))
        nrn_pragma_omp(target update to(nsb->_cnt) if (nt->compute_gpu))
    }
#endif
}

void ensure_thread_net_send_buffers(NrnThread* nt) {
    if (!nt || !nt->_ml_list) {
        return;
    }
    for (int type: net_buf_send_types) {
        if (auto* ml = nt->_ml_list[type]) {
            net_send_buffer_ensure(ml);
        }
    }
    if (!nt->_net_send_buffer && nt->ncell > 0) {
        nt->_net_send_buffer_size = std::max(8, nt->ncell);
        nt->_net_send_buffer = alloc_buffer<int>(nt->_net_send_buffer_size);
    }
}

void flush_mechanism_net_send_buffers(NrnThread* nt) {
    if (!nt) {
        return;
    }
    for (auto* tml = nt->tml; tml; tml = tml->next) {
        if (auto* nsb = tml->ml->_net_send_buffer) {
            update_net_send_buffer_on_host(nt, nsb);
            deliver_net_send_buffer_events(nt, nsb);
        }
    }
}

}  // namespace neuron::gpu

extern "C" void hoc_register_net_send_buffering(int type) {
    if (type < 0) {
        return;
    }
    if (std::find(neuron::gpu::net_buf_send_types.begin(),
                  neuron::gpu::net_buf_send_types.end(),
                  type) == neuron::gpu::net_buf_send_types.end()) {
        neuron::gpu::net_buf_send_types.push_back(type);
    }
}