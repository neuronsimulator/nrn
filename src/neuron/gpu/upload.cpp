#include "neuron/gpu/upload.hpp"

#include "coreneuron/permute/cellorder.hpp"
#include "multicore.h"
#include "neuron/container/mechanism_data.hpp"
#include "neuron/container/node_data.hpp"
#include "neuron/gpu/net_send_buffer.hpp"
#include "neuron/gpu/offload.hpp"
#include "neuron/gpu/sync.hpp"
#include "neuron/model_data.hpp"
#include "node_order_optim/node_order_optim.h"
#include "nrnoc_ml.h"

#include <stdexcept>

extern int nrn_nthread;

namespace neuron {
extern int interleave_permute_type;
extern InterleaveInfo* interleave_info;
}  // namespace neuron

namespace neuron::gpu {
namespace {

template <typename T>
void copyin_pod_array(T const* host, std::size_t count, UploadState& state) {
    if (!host || count == 0) {
        return;
    }
#if defined(NRN_ENABLE_GPU) && defined(_OPENACC)
    if (state.is_present(host)) {
        return;
    }
    nrn_target_copyin(host, count);
    state.record(host, count, sizeof(T));
#else
    (void) state;
    throw std::runtime_error("neuron::gpu::upload requires NRN_ENABLE_GPU with OpenACC");
#endif
}

template <typename Storage>
void upload_soa_storage(Storage const& storage, UploadState& state) {
    storage.for_each_vector_for_gpu_upload(
        [&](auto const& /*tag*/, auto const& vec, int /*field_index*/, int /*array_dim*/) {
            if (vec.empty()) {
                return;
            }
            copyin_pod_array(vec.data(), vec.size(), state);
        });
}

void upload_interleave_info_type1(InterleaveInfo& info, int ncell, UploadState& state) {
    if (!info.stride || !info.firstnode || !info.lastnode || !info.cellsize || ncell <= 0) {
        return;
    }

    InterleaveInfo* d_info = nrn_target_copyin(&info, 1);
    state.record(&info, 1, sizeof(InterleaveInfo));

    int* d_stride = nrn_target_copyin(info.stride, static_cast<std::size_t>(info.nstride + 1));
    state.record(info.stride, static_cast<std::size_t>(info.nstride + 1), sizeof(int));
    nrn_target_memcpy_to_device(&(d_info->stride), &d_stride, 1);

    auto const ncell_sz = static_cast<std::size_t>(ncell);
    int* d_firstnode = nrn_target_copyin(info.firstnode, ncell_sz);
    state.record(info.firstnode, ncell_sz, sizeof(int));
    nrn_target_memcpy_to_device(&(d_info->firstnode), &d_firstnode, 1);

    int* d_lastnode = nrn_target_copyin(info.lastnode, ncell_sz);
    state.record(info.lastnode, ncell_sz, sizeof(int));
    nrn_target_memcpy_to_device(&(d_info->lastnode), &d_lastnode, 1);

    int* d_cellsize = nrn_target_copyin(info.cellsize, ncell_sz);
    state.record(info.cellsize, ncell_sz, sizeof(int));
    nrn_target_memcpy_to_device(&(d_info->cellsize), &d_cellsize, 1);
}

void upload_interleave_info_type2(InterleaveInfo& info, UploadState& state) {
    if (!info.stride || !info.firstnode || !info.lastnode || !info.stridedispl || !info.cellsize) {
        return;
    }
    if (info.nwarp <= 0) {
        return;
    }

    InterleaveInfo* d_info = nrn_target_copyin(&info, 1);
    state.record(&info, 1, sizeof(InterleaveInfo));

    auto const nstride = static_cast<std::size_t>(info.nstride);
    int* d_stride = nrn_target_copyin(info.stride, nstride);
    state.record(info.stride, nstride, sizeof(int));
    nrn_target_memcpy_to_device(&(d_info->stride), &d_stride, 1);

    auto const nwarp_p1 = static_cast<std::size_t>(info.nwarp + 1);
    int* d_firstnode = nrn_target_copyin(info.firstnode, nwarp_p1);
    state.record(info.firstnode, nwarp_p1, sizeof(int));
    nrn_target_memcpy_to_device(&(d_info->firstnode), &d_firstnode, 1);

    int* d_lastnode = nrn_target_copyin(info.lastnode, nwarp_p1);
    state.record(info.lastnode, nwarp_p1, sizeof(int));
    nrn_target_memcpy_to_device(&(d_info->lastnode), &d_lastnode, 1);

    int* d_stridedispl = nrn_target_copyin(info.stridedispl, nwarp_p1);
    state.record(info.stridedispl, nwarp_p1, sizeof(int));
    nrn_target_memcpy_to_device(&(d_info->stridedispl), &d_stridedispl, 1);

    auto const nwarp = static_cast<std::size_t>(info.nwarp);
    int* d_cellsize = nrn_target_copyin(info.cellsize, nwarp);
    state.record(info.cellsize, nwarp, sizeof(int));
    nrn_target_memcpy_to_device(&(d_info->cellsize), &d_cellsize, 1);
}

void upload_interleave_infos(UploadState& state) {
    if (!interleave_info || interleave_permute_type == 0) {
        return;
    }
    for (int ith = 0; ith < nrn_nthread; ++ith) {
        auto& info = interleave_info[ith];
        if (interleave_permute_type == 2) {
            upload_interleave_info_type2(info, state);
        } else if (interleave_permute_type == 1) {
            upload_interleave_info_type1(info, interleave_ncell_for_thread(ith), state);
        }
    }
}

void upload_thread_parent_indices(UploadState& state) {
    for (int ith = 0; ith < nrn_nthread; ++ith) {
        auto& nt = nrn_threads[ith];
        if (!nt._v_parent_index || nt.end <= 0) {
            continue;
        }
        copyin_pod_array(nt._v_parent_index, static_cast<std::size_t>(nt.end), state);
    }
}

void upload_mechanism_nodeindices(UploadState& state) {
    for (int ith = 0; ith < nrn_nthread; ++ith) {
        auto& nt = nrn_threads[ith];
        for (auto* tml = nt.tml; tml; tml = tml->next) {
            auto* const ml = tml->ml;
            if (!ml || !ml->nodeindices || ml->nodecount <= 0) {
                continue;
            }
            copyin_pod_array(ml->nodeindices, static_cast<std::size_t>(ml->nodecount), state);
        }
    }
}

void upload_nrnthread_shells(UploadState& state) {
    for (int ith = 0; ith < nrn_nthread; ++ith) {
        auto& nt = nrn_threads[ith];
        if (nt.end <= 0) {
            continue;
        }
        auto* d_nt = nrn_target_copyin(&nt, 1);
        state.record(&nt, 1, sizeof(NrnThread));
        if (nt._v_parent_index) {
            int* d_parent = nrn_target_deviceptr(nt._v_parent_index);
            nrn_target_memcpy_to_device(&(d_nt->_v_parent_index), &d_parent, 1);
        }
    }
}

}  // namespace

void UploadState::record(void const* host, std::size_t count, std::size_t sizeof_elem) {
    mirrors_.push_back({host, count, sizeof_elem, false});
}

void UploadState::record_cpu_owned(void* host, std::size_t count, std::size_t sizeof_elem) {
    mirrors_.push_back({host, count, sizeof_elem, true});
}

bool UploadState::is_present(void const* host_ptr) const {
#if defined(NRN_ENABLE_GPU) && defined(_OPENACC)
    if (!host_ptr) {
        return false;
    }
    return nrn_target_is_present(host_ptr) != nullptr;
#else
    (void) host_ptr;
    return false;
#endif
}

void UploadState::teardown() {
#if defined(NRN_ENABLE_GPU) && defined(_OPENACC)
    sync_all_device_streams();
    for (auto it = mirrors_.rbegin(); it != mirrors_.rend(); ++it) {
        auto const& mirror = *it;
        if (!mirror.host || mirror.count == 0) {
            continue;
        }
        if (mirror.sizeof_elem == sizeof(int)) {
            nrn_target_delete(const_cast<int*>(static_cast<int const*>(mirror.host)), mirror.count);
        } else if (mirror.sizeof_elem == sizeof(double)) {
            nrn_target_delete(const_cast<double*>(static_cast<double const*>(mirror.host)),
                              mirror.count);
        } else if (mirror.sizeof_elem == sizeof(InterleaveInfo)) {
            nrn_target_delete(const_cast<InterleaveInfo*>(
                                  static_cast<InterleaveInfo const*>(mirror.host)),
                              mirror.count);
        } else if (mirror.sizeof_elem == sizeof(NrnThread)) {
            nrn_target_delete(const_cast<NrnThread*>(static_cast<NrnThread const*>(mirror.host)),
                              mirror.count);
        } else if (mirror.sizeof_elem == sizeof(Memb_list)) {
            nrn_target_delete(const_cast<Memb_list*>(static_cast<Memb_list const*>(mirror.host)),
                              mirror.count);
        } else if (mirror.sizeof_elem == sizeof(Datum)) {
            nrn_target_delete(const_cast<Datum*>(static_cast<Datum const*>(mirror.host)),
                              mirror.count);
        } else if (mirror.sizeof_elem == sizeof(Datum*)) {
            nrn_target_delete(const_cast<Datum**>(static_cast<Datum* const*>(mirror.host)),
                              mirror.count);
        } else if (mirror.sizeof_elem == sizeof(Memb_list*)) {
            nrn_target_delete(const_cast<Memb_list**>(static_cast<Memb_list* const*>(mirror.host)),
                              mirror.count);
        } else if (mirror.sizeof_elem == sizeof(NetSendBuffer_t)) {
            nrn_target_delete(const_cast<NetSendBuffer_t*>(
                                  static_cast<NetSendBuffer_t const*>(mirror.host)),
                              mirror.count);
        }
        if (mirror.cpu_owned) {
            delete[] reinterpret_cast<Datum**>(const_cast<void*>(mirror.host));
        }
    }
#endif
    mirrors_.clear();
}

void upload_sorted_model(model_sorted_token const& sorted, UploadState& state) {
    upload_soa_storage(sorted.node_data_token.container(), state);

    neuron::model().apply_to_mechanisms(
        [&](auto& mech_data) { upload_soa_storage(mech_data, state); });

    upload_thread_parent_indices(state);
    upload_mechanism_nodeindices(state);
    upload_nrnthread_shells(state);
    upload_mechanism_lists(state);
    upload_interleave_infos(state);
}

namespace detail {

void upload_interleave_info_for_testing(int permute_type,
                                        InterleaveInfo& info,
                                        int ncell,
                                        UploadState& state) {
    if (permute_type == 2) {
        upload_interleave_info_type2(info, state);
    } else if (permute_type == 1) {
        upload_interleave_info_type1(info, ncell, state);
    }
}

}  // namespace detail

}  // namespace neuron::gpu
