#include "neuron/gpu/net_events.hpp"

#include "neuron/gpu/config.hpp"
#include "neuron/gpu/device_state.hpp"
#include "neuron/gpu/net_receive_buffer.hpp"
#include "neuron/gpu/net_send_buffer.hpp"
#include "neuron/gpu/offload.hpp"
#include "neuron/gpu/phase_timer.hpp"

#include "multicore.h"
#include "nrn_ansi.h"
#include "nrncvode.h"
#include "nrnoc_ml.h"

#include <atomic>
#include <optional>
#include <vector>

namespace neuron::gpu {
namespace {

std::atomic<std::size_t> g_deliver_net_events_count{0};
std::atomic<std::size_t> g_deliver_post_step_events_count{0};
std::atomic<std::size_t> g_spike_exchange_count{0};

}  // namespace

namespace {

/** Stage 3: order + upload NetReceiveBuffer, then run registered net_buf_receive. */
void flush_net_receive_buffers(NrnThread* nt) {
    if (!nt || !nt->compute_gpu || net_buf_receive.empty()) {
        return;
    }
    // Fast empty: no synaptic events this half-step — skip ensure + kernels.
    bool any_events = false;
    for (auto const& entry: net_buf_receive) {
        int const type = entry.second;
        if (type < 0 || !nt->_ml_list) {
            any_events = true;  // conservative: full path
            break;
        }
        Memb_list* const ml = nt->_ml_list[type];
        if (ml && ml->_net_receive_buffer && ml->_net_receive_buffer->_cnt > 0) {
            any_events = true;
            break;
        }
    }
    if (!any_events) {
        return;
    }

    phase_timer::Scope const timer{phase_timer::Id::deliver_nrb};
    phase_timer::bump(phase_timer::Id::deliver_nrb);

    // Prefer step-scoped sorted token (set by fixed_step_thread / lastpart).
    // Fall back to one ensure for this flush only if none is active.
    // Never call ensure per synaptic type (was Traub deliver-nrb residual).
    std::optional<neuron::model_sorted_token> ensure_holder;
    neuron::model_sorted_token const* sorted = flush_sorted_token();
    if (!sorted) {
        ensure_holder.emplace(nrn_ensure_model_data_are_sorted());
        set_flush_sorted_token(&*ensure_holder);
        sorted = &*ensure_holder;
    }
    update_net_receive_buffer(nt);

    // Wait coalesce: launch all type kernels without per-type stream wait /
    // NRB zero / NSB drain; one wait then finalize. net_buf_flush_active tells
    // generated net_buf_receive to skip self-finalize.
    std::vector<Memb_list*> launched;
    launched.reserve(net_buf_receive.size());
    set_net_buf_flush_active(true);
    for (auto const& entry: net_buf_receive) {
        if (!entry.first) {
            continue;
        }
        int const type = entry.second;
        Memb_list* const ml = (nt->_ml_list && type >= 0) ? nt->_ml_list[type] : nullptr;
        if (ml && ml->_net_receive_buffer && ml->_net_receive_buffer->_cnt == 0) {
            continue;  // skip empty types
        }
        if (ml && ml->_net_receive_buffer && ml->_net_receive_buffer->_cnt > 0) {
            launched.push_back(ml);
        }
        (*entry.first)(nt);
    }
    set_net_buf_flush_active(false);

    if (nt->compute_gpu && !launched.empty()) {
        nrn_pragma_acc(wait(nt->stream_id))
    }
    for (Memb_list* ml: launched) {
        NetReceiveBuffer_t* nrb = ml->_net_receive_buffer;
        if (!nrb) {
            continue;
        }
        nrb->_displ_cnt = 0;
        nrb->_cnt = 0;
        if (nt->compute_gpu && nrb->device_uploaded) {
            nrn_pragma_acc(update device(nrb->_cnt, nrb->_displ_cnt))
            nrn_pragma_omp(target update to(nrb->_cnt, nrb->_displ_cnt))
        }
        // NetSendBuffer host drain (was per-type after each net_buf wait).
        NetSendBuffer_t* nsb = ml->_net_send_buffer;
        if (nsb) {
            if (nt->compute_gpu) {
                nrn_pragma_acc(update self(nsb->_cnt))
                nrn_pragma_omp(target update from(nsb->_cnt))
                update_net_send_buffer_on_host(nt, nsb);
            }
            deliver_net_send_buffer_events(nt, ml, nsb);
            if (nt->compute_gpu) {
                nrn_pragma_acc(update device(nsb->_cnt))
                nrn_pragma_omp(target update to(nsb->_cnt))
            }
        }
    }

    if (ensure_holder) {
        set_flush_sorted_token(nullptr);
    }
}

/**
 * Gap/partrans defers lastpart after fixed_step restores compute_gpu=0.
 * finalize_nonvint then restores that 0 before deliver_post_step. Without
 * compute_gpu=1, NET_RECEIVE applies host SoA only and flush is skipped —
 * device CURRENT never sees synaptic g (reduced_dentate GC EPSP residual).
 */
int force_compute_gpu_for_device_deliver(NrnThread* nt) {
    if (!nt || !enabled() || !backend_native() || !model_is_on_device()) {
        return nt ? nt->compute_gpu : 0;
    }
    int const saved = nt->compute_gpu;
    if (!saved) {
        nt->compute_gpu = 1;
        nrn_pragma_acc(update device(nt->compute_gpu) if (nrn_target_is_present(nt)))
        nrn_pragma_omp(target update to(nt->compute_gpu) if (nrn_target_is_present(nt)))
    }
    return saved;
}

void restore_compute_gpu_after_deliver(NrnThread* nt, int saved) {
    if (!nt || nt->compute_gpu == saved) {
        return;
    }
    nt->compute_gpu = saved;
    if (nrn_target_is_present(nt)) {
        nrn_pragma_acc(update device(nt->compute_gpu))
        nrn_pragma_omp(target update to(nt->compute_gpu))
    }
}

}  // namespace

void deliver_net_events_host(NrnThread* nt) {
    ++g_deliver_net_events_count;
    // Start-of-step delivery (til = t + 0.5*dt) enqueues NET_RECEIVE when compute_gpu.
    // Must flush before setup_tree_matrix / nrn_cur so synaptic g is visible this step.
    // Sub-buckets: deliver-thresh / deliver-tq (in NetCvode) + deliver-nrb (flush).
    int const saved = force_compute_gpu_for_device_deliver(nt);
    deliver_net_events(nt);
    flush_net_receive_buffers(nt);
    restore_compute_gpu_after_deliver(nt, saved);
}

void deliver_post_step_events_host(NrnThread* nt) {
    ++g_deliver_post_step_events_count;
    // End-of-step delivery (til = t) enqueues; flush for next step and for end-of-run dumps.
    // Must keep compute_gpu=1 when the model is on device (see force_compute_gpu...).
    // Nested under lastpart-deliver; deliver-tq + deliver-nrb still accumulate.
    int const saved = force_compute_gpu_for_device_deliver(nt);
    {
        phase_timer::Scope const timer{phase_timer::Id::deliver_tq};
        phase_timer::bump(phase_timer::Id::deliver_tq);
        nrn_deliver_events(nt);
    }
    flush_net_receive_buffers(nt);
    restore_compute_gpu_after_deliver(nt, saved);
}

void spike_exchange_after_group(NrnThread* nt) {
    if (!enabled() || !backend_native()) {
        return;
    }
#if NRNMPI
    ++g_spike_exchange_count;
    nrn_spike_exchange(nt);
#else
    (void) nt;
#endif
}

namespace detail {

std::size_t deliver_net_events_count_for_testing() {
    return g_deliver_net_events_count.load();
}

std::size_t deliver_post_step_events_count_for_testing() {
    return g_deliver_post_step_events_count.load();
}

std::size_t spike_exchange_count_for_testing() {
    return g_spike_exchange_count.load();
}

void reset_net_events_for_testing() {
    g_deliver_net_events_count.store(0);
    g_deliver_post_step_events_count.store(0);
    g_spike_exchange_count.store(0);
}

}  // namespace detail

}  // namespace neuron::gpu
