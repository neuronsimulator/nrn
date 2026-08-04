#pragma once

#include <chrono>
#include <cstdio>

namespace neuron::gpu::phase_timer {

/**
 * Native GPU fixed-step phase buckets.
 *
 * Gap sub-phases (P4 A+B): map to CoreNEURON "gap-v-transfer" pieces —
 * gather (device + D→H), host pack/MPI, insrc H→D, bulk target scatter
 * (packed vals + device write; not O(n) scalar H→D).
 * gap_sync remains for rare host-post_solve residual; sub-phases accumulate
 * independently (not auto-summed into gap_sync).
 *
 * Lastpart sub-phases (P4): nest under coarse lastpart wall. Prefer absolute
 * seconds; tracked-total double-counts lastpart + lastpart-*.
 *   play / xfer / nonvint / record / deliver
 *
 * Setup-tree-matrix sub-phases (P4 density): nest under setup-tree-matrix.
 *   setup-rhs — nrn_rhs (zero + CURRENT + axial)
 *   setup-lhs — nrn_lhs (zero d + JACOBIAN + axial)
 */
enum class Id : int {
    deliver_events,    // coarse wall (start-of-step)
    deliver_thresh,    // nested: threshold detect + host flag/hit
    deliver_tq,        // nested: binq + main TQ NetCon/SelfEvent fanout
    deliver_nrb,       // nested: NRB order/upload + net_buf_receive
    deliver_nrb_order,     // nested under deliver-nrb: host instance order
    deliver_nrb_upload,    // nested: H→D event metadata
    deliver_nrb_launch,    // nested: net_buf_receive kernel launches
    deliver_nrb_finalize,  // nested: stream wait + NRB zero + NSB drain
    deliver_nrb_wait,      // nested under finalize: stream wait only
    deliver_nrb_nsb,       // nested under finalize: NetSendBuffer host drain
    deliver_nrb_nsb_d2h,   // nested under nsb: D→H cnt + fields
    deliver_nrb_nsb_host,  // nested under nsb: order + nrn_net_send / net_event
    vecplay_sync,
    setup_tree_matrix,  // coarse wall
    setup_rhs,          // nrn_rhs
    setup_lhs,          // nrn_lhs
    matrix_sync,
    matrix_solver,
    post_solve,
    download_flush,
    lastpart,          // coarse wall (multithread job or nested no-gap)
    lastpart_play,     // fixed_play_continuous
    lastpart_xfer,     // thread_transfer / extra_scatter_gather
    lastpart_nonvint,  // device STATE path
    lastpart_record,   // AFTER_SOLVE + trajectory/record
    lastpart_deliver,  // post-step deliver (thresh + events)
    gap_sync,          // coarse / host post-solve residual
    gap_gather,        // device source gather + bulk mailbox D→H
    gap_host,          // host pack / 1-rank copy / MPI
    gap_insrc,         // H→D insrc_buf after MPI
    gap_scatter,       // bulk target scatter (main-thread; de-chatty)
    count
};

[[nodiscard]] bool enabled() noexcept;

void reset() noexcept;

/** Accumulate wall time for a phase (thread-safe sum). */
void add(Id id, double seconds) noexcept;

/** Increment a call counter (e.g. per-step OpenACC update). */
void bump(Id id) noexcept;

/** Print summary to stderr when enabled; no-op otherwise. */
void print_summary(FILE* out = stderr) noexcept;

/** RAII timer; zero overhead when phase_timer is disabled. */
class Scope {
  public:
    explicit Scope(Id id) noexcept;
    ~Scope() noexcept;

    Scope(Scope const&) = delete;
    Scope& operator=(Scope const&) = delete;

  private:
    Id id_;
    std::chrono::steady_clock::time_point start_{};
    bool active_{false};
};

}  // namespace neuron::gpu::phase_timer