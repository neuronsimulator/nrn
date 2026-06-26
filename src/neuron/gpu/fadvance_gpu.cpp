#include "neuron/gpu/fadvance_gpu.hpp"

#include "neuron/gpu/config.hpp"
#include "neuron/gpu/device_state.hpp"
#include "neuron/gpu/net_events.hpp"
#include "neuron/gpu/net_send_buffer.hpp"
#include "neuron/gpu/download.hpp"
#include "neuron/gpu/phase_timer.hpp"
#include "neuron/gpu/post_solve.hpp"
#include "neuron/gpu/sync.hpp"
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

    {
        phase_timer::Scope const timer{phase_timer::Id::deliver_events};
        nrn::Instrumentor::phase p("deliver-events");
        deliver_net_events_host(nth);
    }

    ensure_thread_net_send_buffers(nth);
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
        if (!matrix_rhs_d_stays_on_device_for_solve(nt)) {
            sync_matrix_to_device_before_solve(nt);
        }
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
                if (nrnthread_vi_compute_) {
                    sync_voltages_to_host_after_post_solve(nt);
                    nrnthread_vi_compute_(&nt);
                }
            }
        }
        if (should_flush_download()) {
            phase_timer::Scope const timer{phase_timer::Id::download_flush};
            batch_download_post_solve(nt);
        }
        advance_download_step_counter();
    }
    if (nrnthread_v_transfer_ && nt.end > 0) {
        phase_timer::Scope const timer{phase_timer::Id::gap_sync};
        if (host_post_solve) {
            sync_gap_after_host_voltage_update(nt);
        } else {
            // Host thread_transfer reads source handles; stage voltages for gather.
            sync_gap_after_voltage_update(nt);
        }
    }
    // Partrans: nrnmpi_v_transfer + lastpart are dispatched from nrn_fixed_step
    // (same as CPU). Only run lastpart here when no gap transfer is configured.
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
