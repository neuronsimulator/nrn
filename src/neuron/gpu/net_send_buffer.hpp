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
/**
 * Device kernels cannot grow this buffer mid-region (OpenACC). Host must
 * pre-size via ensure / ensure_for_events. Overflow is never silent: device
 * skips writes when i >= _size and host aborts if _cnt > _size.
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
    /** Peak rows observed in a flush; drives adaptive reserve for the next kernel. */
    int _high_water = 0;

    explicit NetSendBuffer_t(int size);
    ~NetSendBuffer_t();

    void grow();
    void reserve(int capacity);
    /** Record observed row count; grow host capacity when ≥ half full (next pre-size). */
    void record_peak(int peak_cnt);

    [[nodiscard]] std::size_t size_of_object() const;
};

/** Default host capacity: max(1024, nodecount * sends_per_event_headroom). */
int net_send_buffer_capacity(Memb_list const* ml);
/** Upper bound on buffered ops (net_send/move/event) per instance or queued receive. */
int net_send_buffer_sends_per_event_headroom() noexcept;
void net_send_buffer_ensure(Memb_list* ml);
/**
 * Ensure capacity for a device flush with up to min_events sources (e.g. NRB
 * count or nodecount). Uses headroom × min_events and high-water history.
 * Re-uploads device arrays if grown.
 */
void net_send_buffer_ensure_for_events(Memb_list* ml, int min_events);
void update_net_send_buffer_on_host(NrnThread* nt, NetSendBuffer_t* nsb);
/** Resolve NetSendBuffer rows via host Memb_list pdata (instance id + field indices). */
void deliver_net_send_buffer_events(NrnThread* nt, Memb_list* ml, NetSendBuffer_t* nsb);
void ensure_thread_net_send_buffers(NrnThread* nt);
void ensure_thread_net_send_buffers_host(NrnThread* nt);
void flush_mechanism_net_send_buffers(NrnThread* nt);

/**
 * Traub residual #15: device net_send (type 0) from net_buf can bypass the
 * full SelfEvent + TQ path. Pending self-receives are promoted into NRB at the
 * same til windows as deliver_net_events / nrn_deliver_events.
 * Only type-0 net_send; net_event / net_move still use the TQ path.
 * net_move of a pending flag==1 self-event is not supported (Traub NMDA has no
 * net_move). Call clear_pending_self_receives on finitialize.
 */
void promote_pending_self_receives(NrnThread* nt, double til);
void clear_pending_self_receives(NrnThread* nt) noexcept;
void clear_all_pending_self_receives() noexcept;

extern std::vector<int> net_buf_send_types;

}  // namespace neuron::gpu

extern "C" void hoc_register_net_send_buffering(int type);