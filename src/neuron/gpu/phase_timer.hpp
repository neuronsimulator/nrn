#pragma once

#include <chrono>
#include <cstdio>

namespace neuron::gpu::phase_timer {

/**
 * Native GPU fixed-step phase buckets.
 * Gap sub-phases (exploratory P4): map to CoreNEURON "gap-v-transfer" pieces —
 * gather (device + D→H), host pack/MPI, insrc H→D, bulk target scatter
 * (packed vals + device write; not O(n) scalar H→D).
 * gap_sync remains for rare host-post_solve residual and as a coarse alias sum
 * is not automatic (sub-phases accumulate independently).
 */
enum class Id : int {
    deliver_events,
    vecplay_sync,
    setup_tree_matrix,
    matrix_sync,
    matrix_solver,
    post_solve,
    download_flush,
    lastpart,
    gap_sync,     // coarse / host post-solve residual
    gap_gather,   // device source gather + bulk mailbox D→H
    gap_host,     // host pack / 1-rank copy / MPI
    gap_insrc,    // H→D insrc_buf after MPI
    gap_scatter,  // bulk target scatter (main-thread; de-chatty)
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