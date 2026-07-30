#include "neuron/gpu/fadvance_gpu.hpp"

#include "neuron/gpu/config.hpp"
#include "neuron/gpu/device_state.hpp"
#include "neuron/gpu/net_events.hpp"
#include "neuron/gpu/net_send_buffer.hpp"
#include "neuron/gpu/download.hpp"
#include "neuron/gpu/offload.hpp"
#include "neuron/gpu/phase_timer.hpp"
#include "neuron/gpu/post_solve.hpp"
#include "neuron/gpu/sync.hpp"
#include "prcellstate_checkpoint.hpp"
#include "neuron/model_data.hpp"

#include "coreneuron/permute/cellorder.hpp"
#include "multicore.h"
#include "neuron.h"
#include "node_order_optim/node_order_optim.h"
#include "nrn_ansi.h"
#include "nrncvode.h"
#include "utils/profile/profiler_interface.h"

#include <atomic>
#include <cstddef>

extern void (*nrnthread_v_transfer_)(NrnThread* nt);
extern void (*nrnthread_vi_compute_)(NrnThread* nt);

namespace neuron::gpu {
namespace {

std::atomic<std::size_t> g_fixed_step_dispatch_count{0};

void advance_first_half_time(NrnThread& nt) {
    nt._t += .5 * nt._dt;
}

}  // namespace

void fixed_step_thread(model_sorted_token const& cache_token,
                       device_token const& /*dev*/,
                       NrnThread& nt) {
    ++g_fixed_step_dispatch_count;
    if (nt.id == 0) {
        warn_native_gpu_multithread_policy();
    }
    auto* const nth = &nt;

    int const saved_compute_gpu = nt.compute_gpu;
    nt.compute_gpu = 1;
    // Keep device nt.compute_gpu in sync: device net_send_buffering and present
    // if() clauses read the device copy (stale 0 → non-atomic cnt races).
    nrn_pragma_acc(update device(nt.compute_gpu) if (nrn_target_is_present(&nt)))
    nrn_pragma_omp(target update to(nt.compute_gpu) if (nrn_target_is_present(&nt)))

    ensure_thread_net_send_buffers(nth);
    {
        phase_timer::Scope const timer{phase_timer::Id::deliver_events};
        nrn::Instrumentor::phase p("deliver-events");
        deliver_net_events_host(nth);
    }
    nrn_random_play();
    advance_first_half_time(nt);
    fixed_play_continuous(nth);
    {
        phase_timer::Scope const timer{phase_timer::Id::vecplay_sync};
        sync_after_vecplay(nt);
    }
    bool const host_post_solve = nt.end > 0 && post_solve_needs_host_fallback(nt);
    if (nt.end > 0) {
        {
            phase_timer::Scope const timer{phase_timer::Id::setup_tree_matrix};
            setup_tree_matrix(cache_token, nt);
        }
        nrn_prcellstate_checkpoint_maybe(PrcellCheckpointPhase::post_setup, nt);
        flush_mechanism_net_send_buffers(nth);
        {
            phase_timer::Scope const timer{phase_timer::Id::matrix_solver};
            nrn::Instrumentor::phase p("matrix-solver");
            if (neuron::interleave_permute_type) {
                neuron::solve_interleaved(nt.id);
            } else {
                nrn_solve(nth);
            }
        }
        {
            phase_timer::Scope const timer{phase_timer::Id::post_solve};
            if (host_post_solve) {
                sync_rhs_to_host_after_solve(nt);
                {
                    nrn::Instrumentor::phase p("second-order-cur");
                    second_order_cur(nth);
                }
                {
                    nrn::Instrumentor::phase p("update");
                    nrn_update_voltage(cache_token, nt);
                }
            } else {
                {
                    nrn::Instrumentor::phase p("update");
                    post_solve_on_device(cache_token, nt);
                }
                // Full device fence after post_solve — no V host transfer.
                // Gap gather (main thread) waits all streams before mailbox read.
                sync_all_device_streams();
                if (nrnthread_vi_compute_) {
                    sync_voltages_to_host_after_post_solve(nt);
                    nrnthread_vi_compute_(&nt);
                }
            }
        }
        nrn_prcellstate_checkpoint_maybe(PrcellCheckpointPhase::post_solve, nt);
        advance_download_step_counter();
    }
    if (nrnthread_v_transfer_ && nt.end > 0 && host_post_solve) {
        phase_timer::Scope const timer{phase_timer::Id::gap_sync};
        sync_gap_after_host_voltage_update(nt);
    }
    // Partrans: nrnmpi_v_transfer + lastpart are dispatched from nrn_fixed_step
    // after all threads finish post_solve (gather needs every thread's V).
    // lastpart (thread_transfer + STATE) runs as a second multi-thread job with
    // prepare_nonvint setting compute_gpu=1 again — still all-threads-on-device.
    if (!nrnthread_v_transfer_) {
        phase_timer::Scope const timer{phase_timer::Id::lastpart};
        nrn_fixed_step_lastpart(cache_token, nt);
    }
    if (nt.end > 0) {
        phase_timer::Scope const timer{phase_timer::Id::vecplay_sync};
        sync_after_vecplay(nt);
    }

    nt.compute_gpu = saved_compute_gpu;
}

namespace detail {

std::size_t fixed_step_dispatch_count_for_testing() {
    return g_fixed_step_dispatch_count.load();
}

void reset_fixed_step_dispatch_for_testing() {
    g_fixed_step_dispatch_count.store(0);
}

}  // namespace detail

}  // namespace neuron::gpu
