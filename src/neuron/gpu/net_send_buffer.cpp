#include "neuron/gpu/net_send_buffer.hpp"

#include "neuron/gpu/offload.hpp"
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

int net_send_buffer_capacity(Memb_list const* ml) {
    if (!ml) {
        return 1024;
    }
    // CoreNEURON uses nodecount * 2; use a higher floor for dense models.
    return std::max(1024, ml->nodecount * 2);
}

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

#if defined(NRN_ENABLE_GPU)
void delete_net_send_buffer_from_device(NetSendBuffer_t* nsb) {
    if (!nsb || !nrn_target_is_present(nsb)) {
        return;
    }
    if (nsb->_sendtype) {
        nrn_target_delete(nsb->_sendtype, static_cast<std::size_t>(nsb->_size));
    }
    if (nsb->_vdata_index) {
        nrn_target_delete(nsb->_vdata_index, static_cast<std::size_t>(nsb->_size));
    }
    if (nsb->_pnt_index) {
        nrn_target_delete(nsb->_pnt_index, static_cast<std::size_t>(nsb->_size));
    }
    if (nsb->_weight_index) {
        nrn_target_delete(nsb->_weight_index, static_cast<std::size_t>(nsb->_size));
    }
    if (nsb->_nsb_t) {
        nrn_target_delete(nsb->_nsb_t, static_cast<std::size_t>(nsb->_size));
    }
    if (nsb->_nsb_flag) {
        nrn_target_delete(nsb->_nsb_flag, static_cast<std::size_t>(nsb->_size));
    }
    nrn_target_delete(nsb, 1);
}

void upload_net_send_buffer_fields(NetSendBuffer_t* nsb, NetSendBuffer_t* d_nsb) {
    int* d_iptr = nrn_target_copyin(nsb->_sendtype, static_cast<std::size_t>(nsb->_size));
    nrn_target_memcpy_to_device(&(d_nsb->_sendtype), &d_iptr, 1);

    d_iptr = nrn_target_copyin(nsb->_vdata_index, static_cast<std::size_t>(nsb->_size));
    nrn_target_memcpy_to_device(&(d_nsb->_vdata_index), &d_iptr, 1);

    d_iptr = nrn_target_copyin(nsb->_pnt_index, static_cast<std::size_t>(nsb->_size));
    nrn_target_memcpy_to_device(&(d_nsb->_pnt_index), &d_iptr, 1);

    d_iptr = nrn_target_copyin(nsb->_weight_index, static_cast<std::size_t>(nsb->_size));
    nrn_target_memcpy_to_device(&(d_nsb->_weight_index), &d_iptr, 1);

    double* d_dptr = nrn_target_copyin(nsb->_nsb_t, static_cast<std::size_t>(nsb->_size));
    nrn_target_memcpy_to_device(&(d_nsb->_nsb_t), &d_dptr, 1);

    d_dptr = nrn_target_copyin(nsb->_nsb_flag, static_cast<std::size_t>(nsb->_size));
    nrn_target_memcpy_to_device(&(d_nsb->_nsb_flag), &d_dptr, 1);
}

void upload_net_send_buffer_to_device(Memb_list* ml) {
    auto* const nsb = ml->_net_send_buffer;
    if (!nsb) {
        return;
    }
    if (nrn_target_is_present(nsb)) {
        if (!nsb->reallocated) {
            return;
        }
        delete_net_send_buffer_from_device(nsb);
    }
    auto* const d_nsb = nrn_target_copyin(nsb, 1);
    upload_net_send_buffer_fields(nsb, d_nsb);

    auto* const d_ml = nrn_target_deviceptr(ml);
    nrn_target_memcpy_to_device(&(d_ml->_net_send_buffer), &d_nsb, 1);
    nsb->reallocated = 0;
}
#endif

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
    if (_cnt < _size) {
        return;
    }
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

void NetSendBuffer_t::reserve(int capacity) {
    if (capacity <= _size) {
        return;
    }
    grow_buffer(&_sendtype, _size, capacity);
    grow_buffer(&_vdata_index, _size, capacity);
    grow_buffer(&_pnt_index, _size, capacity);
    grow_buffer(&_weight_index, _size, capacity);
    grow_buffer(&_nsb_t, _size, capacity);
    grow_buffer(&_nsb_flag, _size, capacity);
    _size = capacity;
    reallocated = 1;
}

std::size_t NetSendBuffer_t::size_of_object() const {
    std::size_t nbytes = 0;
    nbytes += static_cast<std::size_t>(_size) * sizeof(int) * 4;
    nbytes += static_cast<std::size_t>(_size) * sizeof(double) * 2;
    return nbytes;
}

void net_send_buffer_ensure(Memb_list* ml) {
    if (!ml) {
        return;
    }
    int const capacity = net_send_buffer_capacity(ml);
    if (!ml->_net_send_buffer) {
        ml->_net_send_buffer = new NetSendBuffer_t(capacity);
        return;
    }
    ml->_net_send_buffer->reserve(capacity);
}

void update_net_send_buffer_on_host(NrnThread* nt, NetSendBuffer_t* nsb) {
#if defined(NRN_ENABLE_GPU)
    if (!nt || !nsb || !nt->compute_gpu) {
        return;
    }
    if (!nrn_target_is_present(nsb)) {
        return;
    }
    nrn_pragma_acc(update self(nsb->_cnt) if (nt->compute_gpu))
    nrn_pragma_omp(target update from(nsb->_cnt) if (nt->compute_gpu))
    if (nsb->_cnt > nsb->_size) {
        fprintf(stderr, "ERROR: NetSendBuffer exceeded during GPU execution (thread %d)\n", nt->id);
        std::abort();
    }
    if (!nsb->_cnt) {
        return;
    }
    // clang-format off
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
    // clang-format on
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
        auto* const pnt = reinterpret_cast<Point_process*>(
            static_cast<intptr_t>(nsb->_pnt_index[i]));
        auto* const weight = nsb->_weight_index[i] >= 0
                                 ? reinterpret_cast<double*>(
                                       static_cast<intptr_t>(nsb->_weight_index[i]))
                                 : nullptr;
        auto* const vdata = nsb->_vdata_index[i] >= 0
                                ? reinterpret_cast<void*>(
                                      static_cast<intptr_t>(nsb->_vdata_index[i]))
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

void ensure_thread_net_send_buffers_host(NrnThread* nt) {
    if (!nt) {
        return;
    }
    for (auto* tml = nt->tml; tml; tml = tml->next) {
        if (std::find(net_buf_send_types.begin(), net_buf_send_types.end(), tml->index) ==
            net_buf_send_types.end()) {
            continue;
        }
        if (auto* const ml = tml->ml) {
            net_send_buffer_ensure(ml);
        }
    }
}

void ensure_thread_net_send_buffers(NrnThread* nt) {
    ensure_thread_net_send_buffers_host(nt);
    for (auto* tml = nt->tml; tml; tml = tml->next) {
        if (std::find(net_buf_send_types.begin(), net_buf_send_types.end(), tml->index) ==
            net_buf_send_types.end()) {
            continue;
        }
        auto* const ml = tml->ml;
        if (!ml) {
            continue;
        }
#if defined(NRN_ENABLE_GPU)
        upload_net_send_buffer_to_device(ml);
#endif
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