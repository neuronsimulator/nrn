#pragma once

#include <chrono>
#include <cstdio>

namespace neuron::gpu::phase_timer {

/** Native GPU fixed-step phase buckets (see PHASE-D-OVERVIEW.md). */
enum class Id : int {
    deliver_events,
    vecplay_sync,
    setup_tree_matrix,
    matrix_sync,
    matrix_solver,
    post_solve,
    download_flush,
    lastpart,
    gap_sync,
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