#pragma once

/**
 * Native GPU trajectory (Vector.record without full SoA).
 *
 * Design: doc/gpu/trajectory-native.md
 * T1: host plan. T2: sparse gather + append sinks + Gate F cover.
 */

#include <cstddef>
#include <vector>

struct IvocVect;
struct NrnThread;
class PlayRecord;

namespace neuron {
struct model_sorted_token;
}

namespace neuron::gpu {

/** Where a recorded scalar lives. */
enum class TrajectorySourceKind {
    Time,         // NrnThread::_t (host)
    Voltage,      // node voltage SoA slot (device during psolve)
    FastImem,     // node sav_rhs
    Mechanism,    // non-art mech RANGE (T2: not gatherable → unsupported)
    Unsupported,  // full-SoA fallback for that psolve
};

struct TrajectoryChannel {
    TrajectorySourceKind kind = TrajectorySourceKind::Unsupported;
    int thread_id = 0;
    /** Core-style type: voltage=-1, i_membrane_=-2, else Memb_list type. */
    int type = 0;
    /** Node slot or mechanism legacy_index (flat). */
    int index = 0;
    /** Host address when resolvable (Time → &nt._t). */
    double* host_src = nullptr;
    /** Device pointer for Voltage/FastImem after bind (T2). */
    double* device_src = nullptr;
    /** Sink Vector (y_ or t_). */
    IvocVect* sink = nullptr;
    PlayRecord* play_record = nullptr;
    bool supported = false;
};

struct TrajectoryPlan {
    std::vector<TrajectoryChannel> channels;
    int n_supported = 0;
    int n_unsupported = 0;
    /** True when every fixed_record_ entry is supported and gatherable. */
    bool complete = false;
    bool has_graph_record = false;
    /** 0 = auto; else forced chunk length (T3). */
    int chunk_size = 0;
    bool valid = false;
    /** Device sources bound for current model layout. */
    bool device_bound = false;
};

void trajectory_plan_rebuild();
void trajectory_plan_invalidate() noexcept;

[[nodiscard]] bool trajectory_plan_valid() noexcept;
[[nodiscard]] bool trajectory_plan_active() noexcept;
[[nodiscard]] bool trajectory_plan_complete() noexcept;
/**
 * True when native trajectory fully covers continuous Vector.record so Gate F
 * need not pull full SoA for recording alone.
 */
[[nodiscard]] bool trajectory_covers_fixed_record() noexcept;

[[nodiscard]] TrajectoryPlan const& trajectory_plan() noexcept;
[[nodiscard]] int trajectory_default_chunk_size() noexcept;

/**
 * Ensure plan is rebuilt and device sources bound (call when model is on device).
 * No-op if plan already bound and valid.
 */
void trajectory_prepare_for_psolve();

/**
 * Sample one fixed step for this thread into Vector sinks (sparse device→host).
 * Call at the same phase as host fixed_record_continuous (after BEFORE_STEP).
 */
void trajectory_sample_step(NrnThread& nt);

/** Optional end-of-psolve hook (samples already appended per step in T2). */
void trajectory_finalize_psolve() noexcept;

namespace detail {
void reset_trajectory_plan_for_testing();
}  // namespace detail

}  // namespace neuron::gpu
