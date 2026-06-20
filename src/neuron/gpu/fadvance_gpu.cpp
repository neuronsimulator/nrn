#include "neuron/gpu/fadvance_gpu.hpp"

#include "neuron/gpu/config.hpp"
#include "neuron/gpu/device_state.hpp"
#include "neuron/gpu/net_events.hpp"
#include "neuron/model_data.hpp"

#include "coreneuron/permute/cellorder.hpp"
#include "hocdec.h"
#include "multicore.h"
#include "neuron.h"
#include "node_order_optim/node_order_optim.h"
#include "nrn_ansi.h"
#include "nrncvode.h"
#include "oc_ansi.h"
#include "utils/profile/profiler_interface.h"

#include <atomic>
#include <cstddef>

extern void (*nrnthread_v_transfer_)(NrnThread* nt);

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
    auto* const nth = &nt;

    int const saved_compute_gpu = nt.compute_gpu;
    nt.compute_gpu = 1;

    {
        nrn::Instrumentor::phase p("deliver-events");
        deliver_net_events_host(nth);
    }

    nrn_random_play();
    advance_first_half_time(nt);
    fixed_play_continuous(nth);
    setup_tree_matrix(cache_token, nt);
    {
        nrn::Instrumentor::phase p("matrix-solver");
        if (neuron::interleave_permute_type) {
            neuron::solve_interleaved(nt.id);
        } else {
            nrn_solve(nth);
        }
    }
    {
        nrn::Instrumentor::phase p("second-order-cur");
        second_order_cur(nth);
    }
    {
        nrn::Instrumentor::phase p("update");
        nrn_update_voltage(cache_token, nt);
    }

    if (!nrnthread_v_transfer_) {
        nrn_fixed_step_lastpart(cache_token, nt);
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

#if defined(NRN_ENABLE_GPU)

static void hoc_nrn_gpu_set_enable() {
    set_enable(*hoc_getarg(1) != 0.0);
    hoc_retpushx(1.0);
}

static void hoc_nrn_gpu_set_backend() {
    set_backend(hoc_gargstr(1));
    hoc_retpushx(1.0);
}

static void hoc_nrn_gpu_get_enable() {
    hoc_retpushx(enabled() ? 1.0 : 0.0);
}

static void hoc_nrn_gpu_get_backend() {
    static char native_storage[] = "native";
    static char coreneuron_storage[] = "coreneuron";
    static char* native_backend = native_storage;
    static char* coreneuron_backend = coreneuron_storage;
    hoc_ret();
    if (backend_native()) {
        hoc_pushstr(&native_backend);
    } else {
        hoc_pushstr(&coreneuron_backend);
    }
}

void register_hoc_functions() {
    static bool registered = false;
    if (registered) {
        return;
    }
    registered = true;
    VoidFunc functions[] = {{"_nrn_gpu_set_enable", hoc_nrn_gpu_set_enable},
                            {"_nrn_gpu_set_backend", hoc_nrn_gpu_set_backend},
                            {"_nrn_gpu_get_enable", hoc_nrn_gpu_get_enable},
                            {"_nrn_gpu_get_backend", hoc_nrn_gpu_get_backend},
                            {nullptr, nullptr}};
    hoc_register_var(nullptr, nullptr, functions);
}

#else

void register_hoc_functions() {}

#endif

}  // namespace neuron::gpu
