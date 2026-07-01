#include "neuron/gpu/fadvance_gpu.hpp"

#include "neuron/gpu/config.hpp"
#include "neuron/gpu/device_state.hpp"
#include "neuron/gpu/net_events.hpp"
#include "neuron/gpu/net_send_buffer.hpp"
#include "neuron/gpu/download.hpp"
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
extern void (*nrnmpi_v_transfer_)();

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
        nrn::Instrumentor::phase p("deliver-events");
        deliver_net_events_host(nth);
    }

    ensure_thread_net_send_buffers(nth);
    nrn_random_play();
    advance_first_half_time(nt);
    fixed_play_continuous(nth);
    sync_after_vecplay(nt);
    bool const host_post_solve = nt.end > 0 && post_solve_needs_host_fallback(nt);
    if (nt.end > 0) {
        setup_tree_matrix(cache_token, nt);
        sync_matrix_to_device_before_solve(nt);
        flush_mechanism_net_send_buffers(nth);
        {
            nrn::Instrumentor::phase p("matrix-solver");
            if (neuron::interleave_permute_type) {
                neuron::solve_interleaved(nt.id);
            } else {
                nrn_solve(nth);
            }
        }
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
            if (should_flush_download()) {
                batch_download_post_solve(nt);
            }
        }
        advance_download_step_counter();
    }
    if (nrnthread_v_transfer_) {
        if (nt.end > 0) {
            if (host_post_solve) {
                // Host nrn_update_voltage already updated vec_v; push to device and
                // leave host voltages intact for partrans gather (device→host would
                // overwrite with stale GPU state).
                sync_gap_after_host_voltage_update(nt);
            } else {
                sync_gap_after_voltage_update(nt);
            }
            if (nrnmpi_v_transfer_) {
                (*nrnmpi_v_transfer_)();
            }
        }
        nrn_fixed_step_lastpart(cache_token, nt);
    } else {
        nrn_fixed_step_lastpart(cache_token, nt);
    }
    if (nt.end > 0) {
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
