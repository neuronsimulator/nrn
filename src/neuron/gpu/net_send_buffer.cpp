#include "neuron/gpu/net_send_buffer.hpp"

#include "neuron/event_order.hpp"
#include "neuron/gpu/net_receive_buffer.hpp"
#include "neuron/gpu/offload.hpp"
#include "multicore.h"
#include "nrnoc_ml.h"
#include "section_fwd.hpp"

#include <algorithm>
#include <atomic>
#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <mutex>
#include <numeric>
#include <queue>
#include <utility>
#include <vector>

// Heap-free: weight identity is SoA base index (−1 if none).
extern void nrn_net_send(void* v, int weight_index, Point_process* pnt, double td, double flag);
extern void net_event(Point_process* pnt, double time);
extern void nrn_net_move(Datum* v, Point_process* pnt, double tt);

namespace neuron::gpu {

std::vector<int> net_buf_send_types;

namespace {

/** Lighter than SelfEvent+TQ for device net_send → same-mech NET_RECEIVE. */
struct PendingSelfReceive {
    double t = 0.0;
    double flag = 0.0;
    int weight_index = -1;
    int pnt_index = -1;
    std::uint64_t seq = 0;  // stable order for equal t
    Memb_list* ml = nullptr;
};

struct PendingSelfCompare {
    // min-heap by (t, seq)
    bool operator()(PendingSelfReceive const& a, PendingSelfReceive const& b) const {
        if (a.t != b.t) {
            return a.t > b.t;
        }
        return a.seq > b.seq;
    }
};

using PendingSelfHeap =
    std::priority_queue<PendingSelfReceive, std::vector<PendingSelfReceive>, PendingSelfCompare>;

// One heap per NrnThread id. Grown on demand; cleared at finitialize.
std::vector<PendingSelfHeap> g_pending_self;
std::mutex g_pending_self_mutex;
std::atomic<std::uint64_t> g_pending_seq{0};

PendingSelfHeap& pending_heap_for(NrnThread* nt) {
    int const tid = nt && nt->id >= 0 ? nt->id : 0;
    if (static_cast<int>(g_pending_self.size()) <= tid) {
        std::lock_guard<std::mutex> lock(g_pending_self_mutex);
        if (static_cast<int>(g_pending_self.size()) <= tid) {
            g_pending_self.resize(static_cast<std::size_t>(tid) + 1);
        }
    }
    return g_pending_self[static_cast<std::size_t>(tid)];
}

void enqueue_pending_self_receive(NrnThread* nt,
                                  Memb_list* ml,
                                  int pnt_index,
                                  int weight_index,
                                  double t,
                                  double flag) {
    PendingSelfReceive e;
    e.t = t;
    e.flag = flag;
    e.weight_index = weight_index;
    e.pnt_index = pnt_index;
    e.ml = ml;
    e.seq = g_pending_seq.fetch_add(1, std::memory_order_relaxed);
    pending_heap_for(nt).push(e);
}

/**
 * Buffered ops per instance/receive for pre-size (device cannot grow mid-kernel).
 * Covers net_send + net_move + net_event + spare. Override with
 * NRN_GPU_NET_SEND_BUFFER_HEADROOM (positive integer).
 */
int sends_per_event_headroom() noexcept {
    static int const headroom = [] {
        char const* const env = std::getenv("NRN_GPU_NET_SEND_BUFFER_HEADROOM");
        if (env && env[0]) {
            int const v = std::atoi(env);
            if (v >= 1 && v <= 64) {
                return v;
            }
        }
        return 4;
    }();
    return headroom;
}

}  // namespace

int net_send_buffer_sends_per_event_headroom() noexcept {
    return sends_per_event_headroom();
}

int net_send_buffer_capacity(Memb_list const* ml) {
    if (!ml) {
        return 1024;
    }
    // CoreNEURON used nodecount*2; headroom ≥ 4 for multi-send NET_RECEIVE / self-events.
    return std::max(1024, ml->nodecount * sends_per_event_headroom());
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
        // Caller must have dropped OpenACC present for *buf at old_size first.
        std::free(*buf);
    }
    *buf = new_buf;
}

#if defined(NRN_ENABLE_GPU)
/** Drop device mirrors of field arrays (and the NSB header if present).
 *  Always pass the host array lengths currently on device (usually _size). */
void delete_net_send_buffer_fields_from_device(NetSendBuffer_t* nsb, int field_size) {
    if (!nsb || field_size <= 0) {
        return;
    }
    auto const n = static_cast<std::size_t>(field_size);
    if (nsb->_sendtype) {
        nrn_target_delete(nsb->_sendtype, n);
    }
    if (nsb->_vdata_index) {
        nrn_target_delete(nsb->_vdata_index, n);
    }
    if (nsb->_pnt_index) {
        nrn_target_delete(nsb->_pnt_index, n);
    }
    if (nsb->_weight_index) {
        nrn_target_delete(nsb->_weight_index, n);
    }
    if (nsb->_nsb_t) {
        nrn_target_delete(nsb->_nsb_t, n);
    }
    if (nsb->_nsb_flag) {
        nrn_target_delete(nsb->_nsb_flag, n);
    }
    if (nrn_target_is_present(nsb)) {
        nrn_target_delete(nsb, 1);
    }
}

void delete_net_send_buffer_from_device(NetSendBuffer_t* nsb) {
    if (!nsb) {
        return;
    }
    // Fields may still be present even if the NSB header is not (partial upload
    // or free-before-resize). Always try field delete at current _size.
    delete_net_send_buffer_fields_from_device(nsb, nsb->_size);
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
    if (!nsb->reallocated && nrn_target_is_present(nsb)) {
        return;
    }
    // reallocated (or never uploaded): drop any field maps at old addresses first.
    delete_net_send_buffer_from_device(nsb);
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
#if defined(NRN_ENABLE_GPU)
    // Free-before-host-free: never leave orphan present maps for recycled addresses.
    delete_net_send_buffer_from_device(this);
#endif
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
    reserve(new_size);
}

void NetSendBuffer_t::reserve(int capacity) {
    if (capacity <= _size) {
        return;
    }
#if defined(NRN_ENABLE_GPU)
    // Critical ownership: drop device maps at old _size before realloc/free of
    // host field arrays. Otherwise host allocator reuses the address at a new
    // size → OpenACC partial-present (e.g. 80 vs 100).
    delete_net_send_buffer_from_device(this);
#endif
    grow_buffer(&_sendtype, _size, capacity);
    grow_buffer(&_vdata_index, _size, capacity);
    grow_buffer(&_pnt_index, _size, capacity);
    grow_buffer(&_weight_index, _size, capacity);
    grow_buffer(&_nsb_t, _size, capacity);
    grow_buffer(&_nsb_flag, _size, capacity);
    _size = capacity;
    reallocated = 1;
}

void NetSendBuffer_t::record_peak(int peak_cnt) {
    if (peak_cnt <= 0) {
        return;
    }
    if (peak_cnt > _high_water) {
        _high_water = peak_cnt;
    }
    // Stay ahead of observed load for the next device kernel (no mid-kernel grow).
    if (peak_cnt * 2 > _size) {
        reserve(std::max({peak_cnt * 2, _size * 2, 8}));
    }
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

void net_send_buffer_ensure_for_events(Memb_list* ml, int min_events) {
    if (!ml) {
        return;
    }
    net_send_buffer_ensure(ml);
    auto* const nsb = ml->_net_send_buffer;
    if (!nsb) {
        return;
    }
    // Device cannot grow mid-kernel. Size to max of default, this flush's peak
    // (min_events × headroom), and 2× historical high-water.
    int const headroom = sends_per_event_headroom();
    int const from_events = std::max(min_events, 0) * headroom;
    int const from_hw = nsb->_high_water > 0 ? nsb->_high_water * 2 : 0;
    int const need = std::max({net_send_buffer_capacity(ml), from_events, from_hw, 8});
    nsb->reserve(need);
#if defined(NRN_ENABLE_GPU)
    // reserve() sets reallocated; re-copyin only when Memb_list is already on device
    // (host-only unit tests / pre-upload setup skip this).
    if (nrn_target_is_present(ml) || nrn_target_is_present(nsb)) {
        upload_net_send_buffer_to_device(ml);
    }
#endif
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
        // Not silent: device skipped writes for i >= _size; refuse to continue.
        fprintf(stderr,
                "ERROR: NetSendBuffer exceeded during GPU execution (thread %d): "
                "cnt=%d size=%d high_water=%d. Increase NRN_GPU_NET_SEND_BUFFER_HEADROOM "
                "or reduce concurrent net_send/net_move per step.\n",
                nt->id,
                nsb->_cnt,
                nsb->_size,
                nsb->_high_water);
        std::abort();
    }
    nsb->record_peak(nsb->_cnt);
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

void deliver_net_send_buffer_events(NrnThread* nt, Memb_list* ml, NetSendBuffer_t* nsb) {
    if (!nt || !ml || !nsb || !nsb->_cnt) {
        return;
    }
    if (!ml->pdata) {
        fprintf(stderr,
                "ERROR: deliver_net_send_buffer_events requires Memb_list pdata (thread %d)\n",
                nt->id);
        std::abort();
    }

    // Parallel device enqueue uses atomic capture (racey slot order). Re-order
    // before host enqueue when n>1. Default: instance, t, sendtype. With
    // NRN_DETERMINISTIC_EVENTS=1: full key (t, class, mech, instance, flag, …).
    int const n = nsb->_cnt;
    int const mech_type = ml->type();
    // Stack path for the common single-event drain (avoids vector + sort).
    int stack_order = 0;
    int const* order_ptr = &stack_order;
    std::vector<int> order;
    if (n > 1) {
        order.resize(static_cast<std::size_t>(n));
        std::iota(order.begin(), order.end(), 0);
        if (neuron::event_order::enabled()) {
            std::stable_sort(order.begin(), order.end(), [&](int a, int b) {
                // tgt_gid often unavailable for density PP; instance + weight_index distinguish.
                auto const ka = neuron::event_order::net_send_key(nsb->_nsb_t[a],
                                                                 nsb->_sendtype[a],
                                                                 /*tgt_gid*/ -1,
                                                                 mech_type,
                                                                 nsb->_pnt_index[a],
                                                                 nsb->_nsb_flag[a],
                                                                 nsb->_weight_index[a],
                                                                 a);
                auto const kb = neuron::event_order::net_send_key(nsb->_nsb_t[b],
                                                                 nsb->_sendtype[b],
                                                                 /*tgt_gid*/ -1,
                                                                 mech_type,
                                                                 nsb->_pnt_index[b],
                                                                 nsb->_nsb_flag[b],
                                                                 nsb->_weight_index[b],
                                                                 b);
                return neuron::event_order::less(ka, kb);
            });
        } else {
            std::stable_sort(order.begin(), order.end(), [&](int a, int b) {
                if (nsb->_pnt_index[a] != nsb->_pnt_index[b]) {
                    return nsb->_pnt_index[a] < nsb->_pnt_index[b];
                }
                if (nsb->_nsb_t[a] != nsb->_nsb_t[b]) {
                    return nsb->_nsb_t[a] < nsb->_nsb_t[b];
                }
                return nsb->_sendtype[a] < nsb->_sendtype[b];
            });
        }
        order_ptr = order.data();
    }

    for (int k = 0; k < n; ++k) {
        int const i = order_ptr[k];
        // Index ABI only: instance id + tqitem ppvar field + weight_index.
        // POINT_PROCESS is dparam[1] for point processes (multicore/prcellstate).
        int const id = nsb->_pnt_index[i];
        int const tq_field = nsb->_vdata_index[i];
        int const weight_index = nsb->_weight_index[i];
        if (id < 0 || id >= ml->nodecount) {
            fprintf(stderr,
                    "ERROR: NetSendBuffer bad instance id %d (nodecount %d, thread %d)\n",
                    id,
                    ml->nodecount,
                    nt->id);
            std::abort();
        }
        Datum* const ppvar = ml->pdata[id];
        if (!ppvar) {
            fprintf(stderr, "ERROR: NetSendBuffer null pdata for id %d\n", id);
            std::abort();
        }
        Point_process* const pnt = ppvar[1].get<Point_process*>();
        Datum* const tqitem = (tq_field >= 0) ? &ppvar[tq_field] : nullptr;

        switch (nsb->_sendtype[i]) {
        case 0:
            // Native GPU density: SelfEvent+TQ was ~1.5 s of deliver-nrb on Traub
            // (NMDA ramp net_send every spike). Pending list + NRB promote is the
            // same semantics for type-0 without net_move on the tqitem.
            if (nt->compute_gpu) {
                enqueue_pending_self_receive(
                    nt, ml, id, weight_index, nsb->_nsb_t[i], nsb->_nsb_flag[i]);
                // flag==1 would store TQItem in *tqitem for net_move; pending path
                // does not support net_move of that self-event (Traub NMDA OK).
                (void) tqitem;
            } else {
                nrn_net_send(tqitem, weight_index, pnt, nsb->_nsb_t[i], nsb->_nsb_flag[i]);
            }
            break;
        case 1:
            net_event(pnt, nsb->_nsb_t[i]);
            break;
        case 2:
            nrn_net_move(tqitem, pnt, nsb->_nsb_t[i]);
            break;
        default:
            break;
        }
    }
    nsb->_cnt = 0;
#if defined(NRN_ENABLE_GPU)
    if (nt->compute_gpu) {
        // record_peak (in update_net_send_buffer_on_host) may have grown host
        // arrays; re-copyin before the next device net_send_buffering.
        if (nsb->reallocated &&
            (nrn_target_is_present(ml) || nrn_target_is_present(nsb))) {
            upload_net_send_buffer_to_device(ml);
        } else if (nrn_target_is_present(nsb)) {
            nrn_pragma_acc(update device(nsb->_cnt) if (nt->compute_gpu))
            nrn_pragma_omp(target update to(nsb->_cnt) if (nt->compute_gpu))
        }
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
    // Threshold hit lists live on ThreadThresholdTable (check_thresh), not
    // NrnThread::_net_send_buffer (avoids OpenACC-present shell clobber).
#if defined(NRN_ENABLE_GPU)
    // Device upload requires OpenACC host API — caller holds OpenACCHostApiLock
    // when nthread>1 (fadvance_gpu).
    for (auto* tml = nt->tml; tml; tml = tml->next) {
        if (std::find(net_buf_send_types.begin(), net_buf_send_types.end(), tml->index) ==
            net_buf_send_types.end()) {
            continue;
        }
        auto* const ml = tml->ml;
        if (!ml) {
            continue;
        }
        upload_net_send_buffer_to_device(ml);
    }
#endif
}

void flush_mechanism_net_send_buffers(NrnThread* nt) {
    if (!nt) {
        return;
    }
    for (auto* tml = nt->tml; tml; tml = tml->next) {
        if (auto* nsb = tml->ml->_net_send_buffer) {
            update_net_send_buffer_on_host(nt, nsb);
            deliver_net_send_buffer_events(nt, tml->ml, nsb);
        }
    }
}

void promote_pending_self_receives(NrnThread* nt, double til) {
    if (!nt) {
        return;
    }
    auto& heap = pending_heap_for(nt);
    while (!heap.empty() && heap.top().t <= til) {
        PendingSelfReceive const e = heap.top();
        heap.pop();
        if (!e.ml) {
            continue;
        }
        // Same as SelfEvent → pnt_receive on compute_gpu: enqueue NRB only.
        net_receive_buffer_enqueue(nt, e.ml, e.pnt_index, e.weight_index, e.flag);
    }
}

void clear_pending_self_receives(NrnThread* nt) noexcept {
    if (!nt || nt->id < 0) {
        return;
    }
    auto const tid = static_cast<std::size_t>(nt->id);
    if (tid < g_pending_self.size()) {
        PendingSelfHeap empty;
        g_pending_self[tid].swap(empty);
    }
}

void clear_all_pending_self_receives() noexcept {
    for (auto& heap: g_pending_self) {
        PendingSelfHeap empty;
        heap.swap(empty);
    }
    g_pending_seq.store(0, std::memory_order_relaxed);
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