#include "neuron/gpu/phase_timer.hpp"

#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <mutex>

namespace neuron::gpu::phase_timer {
namespace {

constexpr int phase_count = static_cast<int>(Id::count);

char const* phase_name(Id id) noexcept {
    switch (id) {
    case Id::deliver_events:
        return "deliver-events";
    case Id::vecplay_sync:
        return "vecplay-sync";
    case Id::setup_tree_matrix:
        return "setup-tree-matrix";
    case Id::matrix_sync:
        return "matrix-sync";
    case Id::matrix_solver:
        return "matrix-solver";
    case Id::post_solve:
        return "post-solve";
    case Id::download_flush:
        return "download-flush";
    case Id::lastpart:
        return "lastpart";
    case Id::gap_sync:
        return "gap-sync";
    case Id::count:
        break;
    }
    return "unknown";
}

std::atomic<bool> g_enabled{false};
std::once_flag g_init_once;
std::atomic<double> g_seconds[phase_count]{};
std::atomic<std::uint64_t> g_calls[phase_count]{};

void init_from_env() noexcept {
    char const* const value = std::getenv("NRN_NATIVE_GPU_PHASE_TIMER");
    g_enabled.store(value && value[0] == '1');
}

}  // namespace

bool enabled() noexcept {
    std::call_once(g_init_once, init_from_env);
    return g_enabled.load(std::memory_order_relaxed);
}

void reset() noexcept {
    if (!enabled()) {
        return;
    }
    for (int i = 0; i < phase_count; ++i) {
        g_seconds[i].store(0.0, std::memory_order_relaxed);
        g_calls[i].store(0, std::memory_order_relaxed);
    }
}

void add(Id id, double seconds) noexcept {
    if (!enabled()) {
        return;
    }
    auto const index = static_cast<int>(id);
    if (index < 0 || index >= phase_count) {
        return;
    }
    double prev = g_seconds[index].load(std::memory_order_relaxed);
    while (!g_seconds[index].compare_exchange_weak(prev, prev + seconds, std::memory_order_relaxed)) {
    }
}

void bump(Id id) noexcept {
    if (!enabled()) {
        return;
    }
    auto const index = static_cast<int>(id);
    if (index < 0 || index >= phase_count) {
        return;
    }
    g_calls[index].fetch_add(1, std::memory_order_relaxed);
}

void print_summary(FILE* out) noexcept {
    if (!enabled() || out == nullptr) {
        return;
    }
    double total = 0.0;
    for (int i = 0; i < phase_count; ++i) {
        total += g_seconds[i].load(std::memory_order_relaxed);
    }
    std::fprintf(out, "NRN_NATIVE_GPU_PHASE_TIMER summary (wall s, %% of tracked total):\n");
    for (int i = 0; i < phase_count; ++i) {
        double const sec = g_seconds[i].load(std::memory_order_relaxed);
        if (sec <= 0.0 && g_calls[i].load(std::memory_order_relaxed) == 0) {
            continue;
        }
        double const pct = total > 0.0 ? 100.0 * sec / total : 0.0;
        std::fprintf(out,
                     "  %-20s %12.6f  %6.2f%%  calls=%llu\n",
                     phase_name(static_cast<Id>(i)),
                     sec,
                     pct,
                     static_cast<unsigned long long>(
                         g_calls[i].load(std::memory_order_relaxed)));
    }
    std::fprintf(out, "  %-20s %12.6f\n", "tracked-total", total);
}

Scope::Scope(Id id) noexcept
    : id_{id} {
    if (!enabled()) {
        return;
    }
    active_ = true;
    start_ = std::chrono::steady_clock::now();
}

Scope::~Scope() noexcept {
    if (!active_) {
        return;
    }
    auto const elapsed = std::chrono::steady_clock::now() - start_;
    add(id_, std::chrono::duration<double>(elapsed).count());
}

}  // namespace neuron::gpu::phase_timer