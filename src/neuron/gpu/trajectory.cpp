#include "neuron/gpu/trajectory.hpp"

#include "neuron/gpu/config.hpp"

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
        return 0;  // auto
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
    // Mechanism SoA host element left null in T1; type+index is enough for T2 device resolve.
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

/** is_time: record nt._t into sink. Otherwise resolve pd into voltage/imem/mech. */
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
        ch.supported = (!require_sink || sink != nullptr) && index >= 0 && index < nt.end;
    } else {
        ch.kind = TrajectorySourceKind::Mechanism;
        ch.host_src = host_pointer_for(nt, type, index);
        ch.supported = (!require_sink || sink != nullptr) && type > 0 && index >= 0;
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
            // T1: only GraphLine with a single resolved pd_ (no multi-var expressions).
            if (pr->pd_) {
                add_channel(nt, pr, nullptr, pr->pd_, false, /*require_sink*/ false);
            } else {
                add_unsupported(pr, tid);
            }
            break;
        case GVectorRecordType:
            g_plan.has_graph_record = true;
            add_unsupported(pr, tid);  // T3
            break;
#endif
        default:
            // Discrete / other continuous types not handled in v1.
            add_unsupported(pr, tid);
            break;
        }
    }

    g_plan.complete = (g_plan.n_unsupported == 0);
    g_plan.valid = true;
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

TrajectoryPlan const& trajectory_plan() noexcept {
    return g_plan;
}

namespace detail {
void reset_trajectory_plan_for_testing() {
    trajectory_plan_invalidate();
}
}  // namespace detail

}  // namespace neuron::gpu
