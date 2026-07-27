#pragma once

/**
 * Native GPU trajectory plan (T1): host-side inventory of Vector.record sources.
 *
 * Design: doc/gpu/trajectory-native.md
 * T1: build/resolve only. T2: device staging + gather + flush + Gate F.
 */

#include <cstddef>
#include <vector>

struct IvocVect;
struct NrnThread;
class PlayRecord;

namespace neuron::gpu {

/** Where a recorded scalar lives (host resolve; device ptr filled in T2). */
enum class TrajectorySourceKind {
    Time,         // NrnThread::_t
    Voltage,      // node voltage SoA slot
    FastImem,     // node sav_rhs (i_membrane_)
    Mechanism,    // non-art cell mech RANGE via legacy index
    Unsupported,  // not resolvable → full-SoA fallback for that psolve
};

struct TrajectoryChannel {
    TrajectorySourceKind kind = TrajectorySourceKind::Unsupported;
    int thread_id = 0;
    /** Core-style type: voltage=-1, i_membrane_=-2, else Memb_list type. */
    int type = 0;
    /** Node slot or mechanism legacy_index (flat). */
    int index = 0;
    /** Host address when resolvable (Time → &nt._t; else SoA element). Null if unsupported. */
    double* host_src = nullptr;
    /** Device pointer — set in T2 after model is on device. */
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
    /** True when every fixed_record_ entry for all threads is supported. */
    bool complete = false;
    /** GraphLine / GVector present → prefer chunked mode (T3). */
    bool has_graph_record = false;
    /** 0 = auto (full-stretch if !has_graph_record); else forced chunk length. */
    int chunk_size = 0;
    bool valid = false;
};

/**
 * Rebuild plan from net_cvode fixed_record_ (all threads).
 * Safe when there is no net_cvode / empty record list (empty complete plan).
 */
void trajectory_plan_rebuild();

/** Drop plan (topology change, record list change, or test reset). */
void trajectory_plan_invalidate() noexcept;

[[nodiscard]] bool trajectory_plan_valid() noexcept;
/** At least one supported channel. */
[[nodiscard]] bool trajectory_plan_active() noexcept;
/** All fixed_record entries supported — Gate F may skip full SoA for record alone (T2). */
[[nodiscard]] bool trajectory_plan_complete() noexcept;

[[nodiscard]] TrajectoryPlan const& trajectory_plan() noexcept;

/** Default chunk when GraphLine present (T3); readable in T1 for mode selection. */
[[nodiscard]] int trajectory_default_chunk_size() noexcept;

namespace detail {
void reset_trajectory_plan_for_testing();
}  // namespace detail

}  // namespace neuron::gpu
