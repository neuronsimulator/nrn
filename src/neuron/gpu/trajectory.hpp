#pragma once

/**
 * Native GPU trajectory (Vector.record without full SoA).
 *
 * Design: doc/gpu/trajectory-native.md
 * T1 plan · T2 sparse sample + Gate F · T3 chunked staging + GraphLine (single pd)
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
    Mechanism,    // non-art mech RANGE (not gatherable yet → unsupported)
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
    /** Device pointer for Voltage/FastImem after bind. */
    double* device_src = nullptr;
    /** Sink Vector (y_, t_, or GLineRecord::v_). */
    IvocVect* sink = nullptr;
    PlayRecord* play_record = nullptr;
    bool supported = false;
    /**
     * Host staging for this channel (T3). Samples accumulate here until
     * flush (chunk full or psolve end), then append to sink.
     */
    std::vector<double> staging;
};

struct TrajectoryPlan {
    std::vector<TrajectoryChannel> channels;
    int n_supported = 0;
    int n_unsupported = 0;
    /** True when every fixed_record_ entry is supported and gatherable. */
    bool complete = false;
    bool has_graph_record = false;
    /**
     * Env override or 0 for auto. Effective size is effective_chunk_size
     * (0 = full-stretch: flush only at psolve end).
     */
    int chunk_size = 0;
    /** Resolved C for this psolve: 0 = full-stretch; >0 = flush every C samples/channel. */
    int effective_chunk = 0;
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
 */
void trajectory_prepare_for_psolve();

/**
 * Sample one fixed step for this thread into host staging (sparse device→host).
 * Flushes staging→sinks when the chunk is full (chunked mode).
 */
void trajectory_sample_step(NrnThread& nt);

/** Flush remaining staging and unbind for next layout. */
void trajectory_finalize_psolve() noexcept;

namespace detail {
void reset_trajectory_plan_for_testing();
/** Test helper: resolve effective chunk from env + has_graph (no model). */
[[nodiscard]] int resolve_effective_chunk_for_testing(int env_chunk, bool has_graph) noexcept;
}  // namespace detail

}  // namespace neuron::gpu
