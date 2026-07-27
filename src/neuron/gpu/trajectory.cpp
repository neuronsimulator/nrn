#include "neuron/gpu/trajectory.hpp"

#include "neuron/gpu/config.hpp"
#include "neuron/gpu/device_state.hpp"
#include "neuron/gpu/offload.hpp"

#include "ivocvect.h"
#include "multicore.h"
#include "nrncvode.h"
#include "nrncore_write/callbacks/nrncore_callbacks.h"
#include "nrncore_write/utils/nrncore_utils.h"
#include "vrecitem.h"

#include <cstdio>
#include <cstdlib>

namespace neuron::gpu {
namespace {

TrajectoryPlan g_plan{};

int env_chunk_size() noexcept {
    char const* const env = std::getenv("NRN_GPU_TRAJECTORY_CHUNK");
    if (!env || !env[0]) {
        return 0;
    }
    int const v = std::atoi(env);
    return v < 0 ? 0 : v;
}

double* host_pointer_for(NrnThread& nt, int type, int index) {
    if (type == voltage) {
        if (index < 0 || index >= nt.end) {
            return nullptr;
        }
        return nt.node_voltage_storage() + index;
    }
    if (type == i_membrane_) {
        auto* const p = nt.node_sav_rhs_storage();
        if (!p || index < 0 || index >= nt.end) {
            return nullptr;
        }
        return p + index;
    }
    (void) nt;
    (void) index;
    return nullptr;
}

void append_channel(TrajectoryChannel ch) {
    if (ch.supported) {
        ++g_plan.n_supported;
    } else {
        ++g_plan.n_unsupported;
    }
    g_plan.channels.push_back(std::move(ch));
}

void add_unsupported(PlayRecord* pr, int tid) {
    TrajectoryChannel ch;
    ch.kind = TrajectorySourceKind::Unsupported;
    ch.play_record = pr;
    ch.thread_id = tid;
    ch.supported = false;
    append_channel(ch);
}

void add_channel(NrnThread& nt,
                 PlayRecord* pr,
                 IvocVect* sink,
                 neuron::container::data_handle<double> const& pd,
                 bool is_time,
                 bool require_sink) {
    TrajectoryChannel ch;
    ch.thread_id = nt.id;
    ch.sink = sink;
    ch.play_record = pr;

    if (is_time) {
        ch.kind = TrajectorySourceKind::Time;
        ch.type = 0;
        ch.index = 0;
        ch.host_src = &nt._t;
        ch.supported = !require_sink || sink != nullptr;
        append_channel(ch);
        return;
    }

    int type = 0;
    int index = 0;
    if (nrn_dblpntr2nrncore(pd, nt, type, index) || type == 0) {
        ch.kind = TrajectorySourceKind::Unsupported;
        ch.supported = false;
        append_channel(ch);
        return;
    }

    ch.type = type;
    ch.index = index;
    if (type == voltage) {
        ch.kind = TrajectorySourceKind::Voltage;
        ch.host_src = host_pointer_for(nt, type, index);
        ch.supported = (!require_sink || sink != nullptr) && ch.host_src != nullptr;
    } else if (type == i_membrane_) {
        ch.kind = TrajectorySourceKind::FastImem;
        ch.host_src = host_pointer_for(nt, type, index);
        // Gatherable only when fast_imem storage exists.
        ch.supported = (!require_sink || sink != nullptr) && ch.host_src != nullptr;
    } else {
        // Mechanism RANGE: defer device field map to a later phase.
        ch.kind = TrajectorySourceKind::Mechanism;
        ch.host_src = nullptr;
        ch.supported = false;
        append_channel(ch);
        return;
    }
    append_channel(ch);
}

void warn_unsupported_once() {
    static bool warned = false;
    if (warned || g_plan.n_unsupported == 0) {
        return;
    }
    warned = true;
    fprintf(stderr,
            "Info: native GPU trajectory plan has %d unsupported Vector.record "
            "source(s); those psolves fall back to full SoA for recording "
            "(see doc/gpu/trajectory-native.md).\n",
            g_plan.n_unsupported);
}

/** Bind device_src for Voltage/FastImem channels on this thread. */
void bind_device_sources_for_thread(NrnThread& nt) {
#if defined(NRN_ENABLE_GPU) && defined(_OPENACC)
    if (!nt.compute_gpu && !model_is_on_device()) {
        return;
    }
    auto* const vec_v = nt.node_voltage_storage();
    auto* const vec_im = nt.node_sav_rhs_storage();
    double* d_v = vec_v ? static_cast<double*>(acc_deviceptr(vec_v)) : nullptr;
    double* d_im = vec_im ? static_cast<double*>(acc_deviceptr(vec_im)) : nullptr;

    for (auto& ch: g_plan.channels) {
        if (ch.thread_id != nt.id || !ch.supported) {
            continue;
        }
        if (ch.kind == TrajectorySourceKind::Voltage && d_v && ch.index >= 0 &&
            ch.index < nt.end) {
            ch.device_src = d_v + ch.index;
        } else if (ch.kind == TrajectorySourceKind::FastImem && d_im && ch.index >= 0 &&
                   ch.index < nt.end) {
            ch.device_src = d_im + ch.index;
        } else if (ch.kind == TrajectorySourceKind::Time) {
            ch.device_src = nullptr;  // host nt._t
        }
    }
#else
    (void) nt;
#endif
}

/**
 * Sparse device→host of one scalar via host array element update.
 * Host storage is the SoA base; only [index:1] is transferred.
 */
double pull_device_scalar(double* host_base, int index, int end, int stream_id) {
#if defined(NRN_ENABLE_GPU) && defined(_OPENACC)
    if (!host_base || index < 0 || index >= end) {
        return 0.0;
    }
    // clang-format off
    nrn_pragma_acc(update host(host_base[index:1]) async(stream_id))
    // clang-format on
    nrn_pragma_acc(wait(stream_id))
    return host_base[index];
#else
    (void) stream_id;
    if (!host_base || index < 0 || index >= end) {
        return 0.0;
    }
    return host_base[index];
#endif
}

}  // namespace

int trajectory_default_chunk_size() noexcept {
    return 50;
}

void trajectory_plan_invalidate() noexcept {
    g_plan = TrajectoryPlan{};
}

void trajectory_plan_rebuild() {
    trajectory_plan_invalidate();

#if !defined(NRN_ENABLE_GPU)
    g_plan.valid = true;
    g_plan.complete = true;
    return;
#else
    if (!enabled() || !backend_native()) {
        g_plan.valid = true;
        g_plan.complete = false;
        return;
    }

    g_plan.chunk_size = env_chunk_size();

    auto* const fr = nrn_fixed_record_list();
    if (!fr) {
        g_plan.valid = true;
        g_plan.complete = true;
        return;
    }

    for (auto* pr: *fr) {
        if (!pr) {
            continue;
        }
        int const tid = pr->ith_;
        if (tid < 0 || tid >= nrn_nthread) {
            add_unsupported(pr, tid);
            continue;
        }
        NrnThread& nt = nrn_threads[tid];

        switch (pr->type()) {
        case TvecRecordType: {
            auto* const tr = static_cast<TvecRecord*>(pr);
            add_channel(nt, pr, tr->t_, {}, true, /*require_sink*/ true);
            break;
        }
        case YvecRecordType: {
            auto* const yr = static_cast<YvecRecord*>(pr);
            add_channel(nt, pr, yr->y_, pr->pd_, false, /*require_sink*/ true);
            break;
        }
#if HAVE_IV
        case GLineRecordType:
            g_plan.has_graph_record = true;
            // T2: GraphLine needs sink/plot path (T3). Mark unsupported for Gate F cover.
            add_unsupported(pr, tid);
            break;
        case GVectorRecordType:
            g_plan.has_graph_record = true;
            add_unsupported(pr, tid);
            break;
#endif
        default:
            add_unsupported(pr, tid);
            break;
        }
    }

    g_plan.complete = (g_plan.n_unsupported == 0);
    g_plan.valid = true;
    g_plan.device_bound = false;
    warn_unsupported_once();
#endif
}

bool trajectory_plan_valid() noexcept {
    return g_plan.valid;
}

bool trajectory_plan_active() noexcept {
    return g_plan.valid && g_plan.n_supported > 0;
}

bool trajectory_plan_complete() noexcept {
    return g_plan.valid && g_plan.complete;
}

bool trajectory_covers_fixed_record() noexcept {
#if defined(NRN_ENABLE_GPU)
    if (!enabled() || !backend_native() || !model_is_on_device()) {
        return false;
    }
    if (!g_plan.valid) {
        return false;
    }
    // Empty record list: nothing to cover (Gate F does not need SoA for record).
    if (!nrn_has_fixed_record_continuous()) {
        return true;
    }
    return g_plan.complete && g_plan.n_supported > 0;
#else
    return false;
#endif
}

TrajectoryPlan const& trajectory_plan() noexcept {
    return g_plan;
}

void trajectory_prepare_for_psolve() {
#if defined(NRN_ENABLE_GPU)
    if (!enabled() || !backend_native() || !model_is_on_device()) {
        return;
    }
    if (!g_plan.valid || !g_plan.device_bound) {
        trajectory_plan_rebuild();
    }
    if (!g_plan.complete || g_plan.n_supported == 0) {
        return;
    }
    for (int ith = 0; ith < nrn_nthread; ++ith) {
        bind_device_sources_for_thread(nrn_threads[ith]);
    }
    g_plan.device_bound = true;
#endif
}

void trajectory_sample_step(NrnThread& nt) {
#if defined(NRN_ENABLE_GPU)
    if (!trajectory_covers_fixed_record() || !g_plan.device_bound) {
        return;
    }

    auto* const vec_v = nt.node_voltage_storage();
    auto* const vec_im = nt.node_sav_rhs_storage();

    for (auto& ch: g_plan.channels) {
        if (ch.thread_id != nt.id || !ch.supported || !ch.sink) {
            continue;
        }
        double val = 0.0;
        switch (ch.kind) {
        case TrajectorySourceKind::Time:
            val = nt._t;
            break;
        case TrajectorySourceKind::Voltage:
            val = pull_device_scalar(vec_v, ch.index, nt.end, nt.stream_id);
            break;
        case TrajectorySourceKind::FastImem:
            val = pull_device_scalar(vec_im, ch.index, nt.end, nt.stream_id);
            break;
        default:
            continue;
        }
        ch.sink->push_back(val);
    }
#else
    (void) nt;
#endif
}

void trajectory_finalize_psolve() noexcept {
    // T2 appends per step; nothing buffered. Invalidate device bind for next layout.
    g_plan.device_bound = false;
}

namespace detail {
void reset_trajectory_plan_for_testing() {
    trajectory_plan_invalidate();
}
}  // namespace detail

}  // namespace neuron::gpu
