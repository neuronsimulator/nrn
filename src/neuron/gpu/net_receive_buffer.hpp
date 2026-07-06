#pragma once

#include <cstddef>
#include <utility>
#include <vector>

struct Memb_list;
struct NrnThread;

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

extern std::vector<std::pair<NetBufReceive_t, int>> net_buf_receive;

namespace detail {
void net_receive_buffer_order(NetReceiveBuffer_t* nrb);
[[nodiscard]] int net_receive_buffer_device_cnt(NetReceiveBuffer_t const* host_nrb);
}  // namespace detail

}  // namespace neuron::gpu

extern "C" void hoc_register_net_receive_buffering(neuron::gpu::NetBufReceive_t, int);