#pragma once

#include <cstddef>
#include <vector>

struct Memb_list;
struct NrnThread;

namespace neuron::gpu {

/**
 * Buffers net_send/net_event/net_move calls from GPU mechanism kernels until
 * host flush (CoreNEURON-style). Rows store **indices**, not host pointers:
 *   _sendtype: 0 net_send, 1 net_event, 2 net_move
 *   _vdata_index: tqitem dparam field index (−1 if unused)
 *   _weight_index: Weight SoA base index (−1 if none; heap-free ABI)
 *   _pnt_index: mechanism instance id (Memb_list row)
 * Host deliver resolves Point_process* / Datum* via ml->pdata[id].
 */
struct NetSendBuffer_t {
    int* _sendtype = nullptr;
    int* _vdata_index = nullptr;
    int* _pnt_index = nullptr;
    int* _weight_index = nullptr;
    double* _nsb_t = nullptr;
    double* _nsb_flag = nullptr;
    int _cnt = 0;
    int _size = 0;
    int reallocated = 0;

    explicit NetSendBuffer_t(int size);
    ~NetSendBuffer_t();

    void grow();
    void reserve(int capacity);

    [[nodiscard]] std::size_t size_of_object() const;
};

int net_send_buffer_capacity(Memb_list const* ml);
void net_send_buffer_ensure(Memb_list* ml);
void update_net_send_buffer_on_host(NrnThread* nt, NetSendBuffer_t* nsb);
/** Resolve NetSendBuffer rows via host Memb_list pdata (instance id + field indices). */
void deliver_net_send_buffer_events(NrnThread* nt, Memb_list* ml, NetSendBuffer_t* nsb);
/** @deprecated Prefer the Memb_list overload; ml is inferred null (legacy pointer packing). */
void deliver_net_send_buffer_events(NrnThread* nt, NetSendBuffer_t* nsb);
void ensure_thread_net_send_buffers(NrnThread* nt);
void ensure_thread_net_send_buffers_host(NrnThread* nt);
void flush_mechanism_net_send_buffers(NrnThread* nt);

extern std::vector<int> net_buf_send_types;

}  // namespace neuron::gpu

extern "C" void hoc_register_net_send_buffering(int type);