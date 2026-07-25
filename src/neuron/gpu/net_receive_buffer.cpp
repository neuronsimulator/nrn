#include "neuron/gpu/net_receive_buffer.hpp"

#include "neuron/container/network/weights.hpp"
#include "neuron/gpu/offload.hpp"
#include "neuron/gpu/sync.hpp"
#include "neuron/model_data.hpp"
#include "membfunc.h"
#include "multicore.h"
#include "nrnoc_ml.h"

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <queue>
#include <utility>
#include <vector>

extern short* nrn_is_artificial_;
extern int n_memb_func;

namespace neuron::gpu {

std::vector<std::pair<NetBufReceive_t, int>> net_buf_receive;

double* weight_soa_values() {
    auto& store = neuron::model().weights();
    if (store.size() == 0) {
        return nullptr;
    }
    return &store.get<neuron::container::network::Weight::field::Value>(0);
}

std::size_t weight_soa_count() {
    return neuron::model().weights().size();
}

namespace {

using NRB_P = std::pair<int, int>;

struct NrbEntryCompare {
    bool operator()(NRB_P const& a, NRB_P const& b) const {
        if (a.first == b.first) {
            return a.second > b.second;
        }
        return a.first > b.first;
    }
};

template <typename T>
T* alloc_buffer(int size) {
    return static_cast<T*>(std::calloc(static_cast<std::size_t>(size), sizeof(T)));
}

void grow_buffer(int** buf, int old_size, int new_size) {
    int* new_buf = alloc_buffer<int>(new_size);
    if (*buf) {
        std::memcpy(new_buf, *buf, static_cast<std::size_t>(old_size) * sizeof(int));
        std::free(*buf);
    }
    *buf = new_buf;
}

void grow_buffer(double** buf, int old_size, int new_size) {
    double* new_buf = alloc_buffer<double>(new_size);
    if (*buf) {
        std::memcpy(new_buf, *buf, static_cast<std::size_t>(old_size) * sizeof(double));
        std::free(*buf);
    }
    *buf = new_buf;
}

void delete_net_receive_buffer_from_device(NetReceiveBuffer_t* nrb) {
#if defined(NRN_ENABLE_GPU)
    if (!nrb || !nrb->device_uploaded || !nrn_target_is_present(nrb)) {
        return;
    }
    if (nrb->_pnt_index) {
        nrn_target_delete(nrb->_pnt_index, static_cast<std::size_t>(nrb->_size));
    }
    if (nrb->_weight_index) {
        nrn_target_delete(nrb->_weight_index, static_cast<std::size_t>(nrb->_size));
    }
    if (nrb->_nrb_t) {
        nrn_target_delete(nrb->_nrb_t, static_cast<std::size_t>(nrb->_size));
    }
    if (nrb->_nrb_flag) {
        nrn_target_delete(nrb->_nrb_flag, static_cast<std::size_t>(nrb->_size));
    }
    if (nrb->_displ) {
        nrn_target_delete(nrb->_displ, static_cast<std::size_t>(nrb->_size + 1));
    }
    if (nrb->_nrb_index) {
        nrn_target_delete(nrb->_nrb_index, static_cast<std::size_t>(nrb->_size));
    }
    nrn_target_delete(nrb, 1);
    nrb->device_uploaded = 0;
#else
    (void) nrb;
#endif
}

void upload_net_receive_buffer_fields(NetReceiveBuffer_t* nrb, NetReceiveBuffer_t* d_nrb) {
#if defined(NRN_ENABLE_GPU)
    int* d_pnt_index = nrn_target_copyin(nrb->_pnt_index, static_cast<std::size_t>(nrb->_size));
    nrn_target_memcpy_to_device(&(d_nrb->_pnt_index), &d_pnt_index, 1);

    int* d_weight_index =
        nrn_target_copyin(nrb->_weight_index, static_cast<std::size_t>(nrb->_size));
    nrn_target_memcpy_to_device(&(d_nrb->_weight_index), &d_weight_index, 1);

    double* d_nrb_t = nrn_target_copyin(nrb->_nrb_t, static_cast<std::size_t>(nrb->_size));
    nrn_target_memcpy_to_device(&(d_nrb->_nrb_t), &d_nrb_t, 1);

    double* d_nrb_flag = nrn_target_copyin(nrb->_nrb_flag, static_cast<std::size_t>(nrb->_size));
    nrn_target_memcpy_to_device(&(d_nrb->_nrb_flag), &d_nrb_flag, 1);

    int* d_displ = nrn_target_copyin(nrb->_displ, static_cast<std::size_t>(nrb->_size + 1));
    nrn_target_memcpy_to_device(&(d_nrb->_displ), &d_displ, 1);

    int* d_nrb_index = nrn_target_copyin(nrb->_nrb_index, static_cast<std::size_t>(nrb->_size));
    nrn_target_memcpy_to_device(&(d_nrb->_nrb_index), &d_nrb_index, 1);
#else
    (void) nrb;
    (void) d_nrb;
#endif
}

[[nodiscard]] bool type_uses_net_receive_buffer(int type) {
    return std::any_of(net_buf_receive.begin(), net_buf_receive.end(), [type](auto const& entry) {
        return entry.second == type;
    });
}

}  // namespace

std::size_t NetReceiveBuffer_t::size_of_object() const noexcept {
    if (_size <= 0) {
        return 0;
    }
    std::size_t nbytes = 0;
    nbytes += static_cast<std::size_t>(_size) * sizeof(int) * 3;
    nbytes += static_cast<std::size_t>(_size + 1) * sizeof(int);
    nbytes += static_cast<std::size_t>(_size) * sizeof(double) * 2;
    return nbytes;
}

int net_receive_buffer_capacity(Memb_list const* ml) {
    if (!ml) {
        return 8;
    }
    return std::max(8, ml->nodecount);
}

void net_receive_buffer_ensure(Memb_list* ml) {
    if (!ml) {
        return;
    }
    int const capacity = net_receive_buffer_capacity(ml);
    if (!ml->_net_receive_buffer) {
        auto* const nrb = static_cast<NetReceiveBuffer_t*>(
            std::calloc(1, sizeof(NetReceiveBuffer_t)));
        nrb->_size = capacity;
        nrb->_pnt_index = alloc_buffer<int>(capacity);
        nrb->_displ = alloc_buffer<int>(capacity + 1);
        nrb->_nrb_index = alloc_buffer<int>(capacity);
        nrb->_weight_index = alloc_buffer<int>(capacity);
        nrb->_nrb_t = alloc_buffer<double>(capacity);
        nrb->_nrb_flag = alloc_buffer<double>(capacity);
        nrb->reallocated = 1;
        ml->_net_receive_buffer = nrb;
        return;
    }
    auto* const nrb = ml->_net_receive_buffer;
    if (capacity <= nrb->_size) {
        return;
    }
    int const old_size = nrb->_size;
    grow_buffer(&nrb->_pnt_index, old_size, capacity);
    grow_buffer(&nrb->_displ, old_size + 1, capacity + 1);
    grow_buffer(&nrb->_nrb_index, old_size, capacity);
    grow_buffer(&nrb->_weight_index, old_size, capacity);
    grow_buffer(&nrb->_nrb_t, old_size, capacity);
    grow_buffer(&nrb->_nrb_flag, old_size, capacity);
    nrb->_size = capacity;
    nrb->reallocated = 1;
}

bool net_receive_buffer_enqueue(NrnThread* nt,
                                Memb_list* ml,
                                int pnt_index,
                                int weight_index,
                                double flag) {
    if (!nt || !ml || !ml->_net_receive_buffer) {
        return false;
    }
    auto* const nrb = ml->_net_receive_buffer;
    if (nrb->_cnt >= nrb->_size) {
        realloc_net_receive_buffer(nt, ml);
    }
    int const id = nrb->_cnt++;
    nrb->_pnt_index[id] = pnt_index;
    nrb->_weight_index[id] = weight_index;
    nrb->_nrb_t[id] = nt->_t;
    nrb->_nrb_flag[id] = flag;
    return true;
}

void realloc_net_receive_buffer(NrnThread* nt, Memb_list* ml) {
    if (!nt || !ml) {
        return;
    }
    auto* const nrb = ml->_net_receive_buffer;
    if (!nrb) {
        return;
    }

#if defined(NRN_ENABLE_GPU)
    if (nt->compute_gpu && nrn_target_is_present(nrb)) {
        delete_net_receive_buffer_from_device(nrb);
    }
#endif

    int const old_size = nrb->_size;
    int const new_size = std::max(old_size * 2, 8);
    grow_buffer(&nrb->_pnt_index, old_size, new_size);
    grow_buffer(&nrb->_weight_index, old_size, new_size);
    grow_buffer(&nrb->_nrb_t, old_size, new_size);
    grow_buffer(&nrb->_nrb_flag, old_size, new_size);
    grow_buffer(&nrb->_displ, old_size + 1, new_size + 1);
    grow_buffer(&nrb->_nrb_index, old_size, new_size);
    nrb->_size = new_size;
    nrb->reallocated = 1;

#if defined(NRN_ENABLE_GPU)
    if (nt->compute_gpu) {
        upload_net_receive_buffer_to_device(ml);
    }
#endif
}

namespace detail {

void net_receive_buffer_order(NetReceiveBuffer_t* nrb) {
    if (!nrb || nrb->_cnt == 0) {
        if (nrb) {
            nrb->_displ_cnt = 0;
        }
        return;
    }

    std::priority_queue<NRB_P, std::vector<NRB_P>, NrbEntryCompare> nrbq;
    for (int i = 0; i < nrb->_cnt; ++i) {
        nrbq.emplace(nrb->_pnt_index[i], i);
    }

    int displ_cnt = 0;
    int index_cnt = 0;
    int last_instance_index = -1;
    nrb->_displ[0] = 0;

    while (!nrbq.empty()) {
        NRB_P const p = nrbq.top();
        nrb->_nrb_index[index_cnt++] = p.second;
        if (p.first != last_instance_index) {
            ++displ_cnt;
        }
        nrb->_displ[displ_cnt] = index_cnt;
        last_instance_index = p.first;
        nrbq.pop();
    }
    nrb->_displ_cnt = displ_cnt;
}

int net_receive_buffer_device_cnt(NetReceiveBuffer_t const* host_nrb) {
#if defined(NRN_ENABLE_GPU)
    if (!host_nrb || !host_nrb->device_uploaded) {
        return -1;
    }
    int device_cnt = -1;
    NetReceiveBuffer_t* const d_nrb =
        static_cast<NetReceiveBuffer_t*>(nrn_target_deviceptr(host_nrb));
    nrn_pragma_acc(parallel loop copy(device_cnt))
    nrn_pragma_omp(target teams distribute parallel for map(tofrom:device_cnt))
    for (int i = 0; i < 1; ++i) {
        device_cnt = d_nrb->_cnt;
    }
    return device_cnt;
#else
    (void) host_nrb;
    return -1;
#endif
}

}  // namespace detail

void update_net_receive_buffer(NrnThread* nt) {
    if (!nt) {
        return;
    }
    for (auto* tml = nt->tml; tml; tml = tml->next) {
        int const type = tml->index;
        if (nrn_is_artificial_ && nrn_is_artificial_[type]) {
            continue;
        }
        auto* const ml = tml->ml;
        if (!ml) {
            continue;
        }
        auto* const nrb = ml->_net_receive_buffer;
        if (!nrb || !nrb->_cnt) {
            continue;
        }
        detail::net_receive_buffer_order(nrb);
#if defined(NRN_ENABLE_GPU)
        if (!nt->compute_gpu || !nrb->device_uploaded) {
            continue;
        }
        // clang-format off
        nrn_pragma_acc(update device(nrb->_cnt,
                                     nrb->_displ_cnt,
                                     nrb->_pnt_index[:nrb->_cnt],
                                     nrb->_weight_index[:nrb->_cnt],
                                     nrb->_nrb_t[:nrb->_cnt],
                                     nrb->_nrb_flag[:nrb->_cnt],
                                     nrb->_displ[:nrb->_displ_cnt + 1],
                                     nrb->_nrb_index[:nrb->_cnt])
                                     async(nt->stream_id))
        nrn_pragma_omp(target update to(nrb->_cnt,
                                        nrb->_displ_cnt,
                                        nrb->_pnt_index[:nrb->_cnt],
                                        nrb->_weight_index[:nrb->_cnt],
                                        nrb->_nrb_t[:nrb->_cnt],
                                        nrb->_nrb_flag[:nrb->_cnt],
                                        nrb->_displ[:nrb->_displ_cnt + 1],
                                        nrb->_nrb_index[:nrb->_cnt]))
        // clang-format on
#endif
    }
#if defined(NRN_ENABLE_GPU)
    nrn_pragma_acc(wait(nt->stream_id))
#endif
}

void ensure_thread_net_receive_buffers_host(NrnThread* nt) {
    if (!nt) {
        return;
    }
    for (auto* tml = nt->tml; tml; tml = tml->next) {
        int const type = tml->index;
        if (!type_uses_net_receive_buffer(type)) {
            continue;
        }
        if (nrn_is_artificial_ && nrn_is_artificial_[type]) {
            continue;
        }
        if (auto* const ml = tml->ml) {
            net_receive_buffer_ensure(ml);
        }
    }
}

void ensure_thread_net_receive_buffers(NrnThread* nt) {
    ensure_thread_net_receive_buffers_host(nt);
    for (auto* tml = nt->tml; tml; tml = tml->next) {
        int const type = tml->index;
        if (!type_uses_net_receive_buffer(type)) {
            continue;
        }
        if (auto* const ml = tml->ml) {
            upload_net_receive_buffer_to_device(ml);
        }
    }
}

void upload_net_receive_buffer_to_device(Memb_list* ml) {
#if defined(NRN_ENABLE_GPU)
    auto* const nrb = ml ? ml->_net_receive_buffer : nullptr;
    if (!nrb) {
        return;
    }
    if (nrb->device_uploaded && !nrb->reallocated) {
        return;
    }
    if (nrb->device_uploaded) {
        delete_net_receive_buffer_from_device(nrb);
    }
    auto* const d_nrb = static_cast<NetReceiveBuffer_t*>(nrn_target_copyin(nrb, 1));
    upload_net_receive_buffer_fields(nrb, d_nrb);
    nrb->reallocated = 0;
    nrb->device_uploaded = 1;
#else
    (void) ml;
#endif
}

void free_net_receive_buffer(Memb_list* ml) {
    if (!ml || !ml->_net_receive_buffer) {
        return;
    }
    auto* const nrb = ml->_net_receive_buffer;
#if defined(NRN_ENABLE_GPU)
    delete_net_receive_buffer_from_device(nrb);
#endif
    std::free(nrb->_pnt_index);
    std::free(nrb->_displ);
    std::free(nrb->_nrb_index);
    std::free(nrb->_weight_index);
    std::free(nrb->_nrb_t);
    std::free(nrb->_nrb_flag);
    std::free(nrb);
    ml->_net_receive_buffer = nullptr;
}

void augment_device_matrix_for_net_receive_mechs(neuron::model_sorted_token const& token,
                                                 NrnThread* nt) {
#if defined(NRN_ENABLE_GPU)
    if (!nt || !nt->compute_gpu || net_buf_receive.empty()) {
        return;
    }
    if (!matrix_rhs_d_stays_on_device_for_solve(*nt)) {
        return;
    }
    // Device matrix is missing CURRENT/JACOBIAN contributions from net-receive
    // mechanisms (ExpSyn): mech SoA (g/i/g_unused) is correct, but vec_rhs/vec_d
    // do not show the synaptic terms (see post_setup prcellstate at t=1.025).
    // Merge: pull device matrix (other mechs), re-run host cur/jacob for registered
    // net_buf types only, push matrix back for the device solver.
    sync_matrix_to_host_before_solve(*nt);

    int const saved_compute_gpu = nt->compute_gpu;
    nt->compute_gpu = 0;
    for (auto const& entry: net_buf_receive) {
        int const type = entry.second;
        if (type < 0 || type >= n_memb_func) {
            continue;
        }
        Memb_list* ml = (nt->_ml_list) ? nt->_ml_list[type] : nullptr;
        if (!ml || ml->nodecount <= 0) {
            continue;
        }
        if (memb_func[type].current) {
            memb_func[type].current(token, nt, ml, type);
        }
        if (memb_func[type].jacob) {
            memb_func[type].jacob(token, nt, ml, type);
        }
    }
    nt->compute_gpu = saved_compute_gpu;

    sync_matrix_to_device_before_solve(*nt);
#else
    (void) token;
    (void) nt;
#endif
}

}  // namespace neuron::gpu

extern "C" void hoc_register_net_receive_buffering(neuron::gpu::NetBufReceive_t f, int type) {
    if (type < 0 || !f) {
        return;
    }
    if (std::any_of(neuron::gpu::net_buf_receive.begin(),
                    neuron::gpu::net_buf_receive.end(),
                    [type](auto const& entry) { return entry.second == type; })) {
        return;
    }
    neuron::gpu::net_buf_receive.emplace_back(f, type);
}