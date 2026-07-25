#pragma once

#include <cstddef>
#include <utility>
#include <vector>

struct Memb_list;
struct NrnThread;

namespace neuron {
struct model_sorted_token;
}

namespace neuron::gpu {

/**
 * CPU-side queue of NET_RECEIVE events to execute on the GPU (CoreNEURON-style).
 * CPU spike delivery enqueues; update_net_receive_buffer() copies metadata to the
 * device before registered net_buf_receive kernels run.
 */
struct NetReceiveBuffer_t {
    int* _displ = nullptr;
    int* _nrb_index = nullptr;
    int* _pnt_index = nullptr;
    int* _weight_index = nullptr;
    double* _nrb_t = nullptr;
    double* _nrb_flag = nullptr;
    int _cnt = 0;
    int _displ_cnt = 0;
    int _size = 0;
    int _pnt_offset = 0;
    /** Set when host capacity grows so device array storage is re-copyin'd. */
    int reallocated = 0;
    /** Set after upload_net_receive_buffer_to_device succeeds. */
    int device_uploaded = 0;

    [[nodiscard]] std::size_t size_of_object() const noexcept;
};

using NetBufReceive_t = void (*)(NrnThread*);

int net_receive_buffer_capacity(Memb_list const* ml);
void net_receive_buffer_ensure(Memb_list* ml);
bool net_receive_buffer_enqueue(NrnThread* nt,
                                Memb_list* ml,
                                int pnt_index,
                                int weight_index,
                                double flag);
void realloc_net_receive_buffer(NrnThread* nt, Memb_list* ml);
void update_net_receive_buffer(NrnThread* nt);
void ensure_thread_net_receive_buffers(NrnThread* nt);
void ensure_thread_net_receive_buffers_host(NrnThread* nt);
void upload_net_receive_buffer_to_device(Memb_list* ml);
void free_net_receive_buffer(Memb_list* ml);

/**
 * Host pointer to the contiguous Weight SoA Value column.
 * Present on device after upload_sorted_model (heap-free packing A).
 * Device kernels index as weights[weight_index + arg].
 * Returns nullptr if the pool is empty.
 */
[[nodiscard]] double* weight_soa_values();
[[nodiscard]] std::size_t weight_soa_count();

/**
 * Stage 3b: after device CURRENT/JACOBIAN assembly, ensure net-receive mechanisms
 * (e.g. ExpSyn) contribute to the device Hines matrix.
 *
 * Observed: OpenACC cur/jacob update mechanism SoA (g, i, g_unused) correctly but
 * vec_rhs/vec_d writes from those mechs are not visible on the device matrix used
 * by the solver. Pull the device matrix, re-run host cur/jacob for registered
 * net_buf_receive types only, push the matrix back.
 */
void augment_device_matrix_for_net_receive_mechs(neuron::model_sorted_token const& token,
                                                 NrnThread* nt);

extern std::vector<std::pair<NetBufReceive_t, int>> net_buf_receive;

namespace detail {
void net_receive_buffer_order(NetReceiveBuffer_t* nrb);
[[nodiscard]] int net_receive_buffer_device_cnt(NetReceiveBuffer_t const* host_nrb);
}  // namespace detail

}  // namespace neuron::gpu

extern "C" void hoc_register_net_receive_buffering(neuron::gpu::NetBufReceive_t, int);