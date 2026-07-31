#include "neuron/gpu/check_thresh.hpp"

#include "neuron/event_order.hpp"
#include "neuron/gpu/config.hpp"
#include "neuron/gpu/device_state.hpp"
#include "neuron/gpu/net_send_buffer.hpp"
#include "neuron/gpu/offload.hpp"
#include "neuron/gpu/phase_timer.hpp"
#include "neuron/gpu/sync.hpp"

#include "multicore.h"
#include "netcon.h"
#include "nrn_ansi.h"

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <mutex>
#include <numeric>
#include <vector>

extern void hoc_execerror(const char*, const char*);

namespace neuron::gpu {
namespace {

// Detect-path accounting (native product: device kernel only under compute_gpu).
// Host detect when !compute_gpu is the normal CPU path, not a "fallback".
std::uint64_t g_thresh_device_calls = 0;
std::uint64_t g_thresh_host_cpu_calls = 0;
std::uint64_t g_thresh_host_fallback_calls = 0;
char const* g_thresh_first_fallback_reason = nullptr;
bool g_thresh_fallback_warned = false;
std::once_flag g_thresh_stats_atexit_once;

bool env_truthy(char const* name) noexcept {
    char const* const e = std::getenv(name);
    return e && e[0] && e[0] != '0';
}

/** Opt-in only: allow host detect + V pull when device residency is incomplete. */
bool allow_thresh_host_fallback() noexcept {
    static int cached = -1;
    if (cached < 0) {
        cached = env_truthy("NRN_GPU_THRESH_HOST_FALLBACK") ? 1 : 0;
    }
    return cached != 0;
}

bool thresh_stats_enabled() noexcept {
    static int cached = -1;
    if (cached < 0) {
        cached = env_truthy("NRN_GPU_THRESH_STATS") ? 1 : 0;
    }
    return cached != 0;
}

void print_thresh_detect_stats(char const* where) noexcept {
    std::fprintf(stderr,
                 "NRN GPU threshold detect stats (%s): device_kernel=%llu "
                 "host_cpu=%llu host_fallback=%llu first_fallback_reason=%s\n",
                 where ? where : "?",
                 static_cast<unsigned long long>(g_thresh_device_calls),
                 static_cast<unsigned long long>(g_thresh_host_cpu_calls),
                 static_cast<unsigned long long>(g_thresh_host_fallback_calls),
                 g_thresh_first_fallback_reason ? g_thresh_first_fallback_reason : "(none)");
}

void ensure_thresh_stats_atexit() noexcept {
    if (!thresh_stats_enabled()) {
        return;
    }
    std::call_once(g_thresh_stats_atexit_once, [] {
        std::atexit([] { print_thresh_detect_stats("atexit"); });
    });
}

char const* thresh_device_block_reason(double* d_v,
                                      int* thvar_row,
                                      double* threshold,
                                      int* flag,
                                      int* nsbuffer) noexcept {
#if defined(NRN_ENABLE_GPU) && (defined(_OPENACC) || defined(_OPENMP))
    if (!d_v) {
        return "device vec_v not mapped (acc_deviceptr/present)";
    }
    if (!nrn_target_is_present(thvar_row)) {
        return "thvar_row not present on device";
    }
    if (!nrn_target_is_present(threshold)) {
        return "threshold column not present on device";
    }
    if (!nrn_target_is_present(flag)) {
        return "flag column not present on device";
    }
    if (!nsbuffer) {
        return "net_send hit buffer is null";
    }
    if (!nrn_target_is_present(nsbuffer)) {
        return "net_send hit buffer not present on device";
    }
    return nullptr;
#else
    (void) d_v;
    (void) thvar_row;
    (void) threshold;
    (void) flag;
    (void) nsbuffer;
    return "OpenACC/OpenMP GPU offload not compiled in";
#endif
}

void note_thresh_host_fallback(char const* reason) noexcept {
    ++g_thresh_host_fallback_calls;
    if (!g_thresh_first_fallback_reason) {
        g_thresh_first_fallback_reason = reason;
    }
    if (!g_thresh_fallback_warned) {
        g_thresh_fallback_warned = true;
        std::fprintf(stderr,
                     "WARNING: native GPU threshold detect using host fallback (%s). "
                     "This is opt-in only (NRN_GPU_THRESH_HOST_FALLBACK=1) and has a "
                     "performance cost (voltage pull + host scan). "
                     "device_kernel=%llu host_fallback=%llu. "
                     "Unset the env to make missing device residency a hard error.\n",
                     reason ? reason : "unknown",
                     static_cast<unsigned long long>(g_thresh_device_calls),
                     static_cast<unsigned long long>(g_thresh_host_fallback_calls));
    }
}

[[noreturn]] void fail_thresh_device_unavailable(char const* reason, int thread_id) {
    std::fprintf(stderr,
                 "ERROR: native GPU threshold detect cannot run device kernel "
                 "(thread %d): %s\n"
                 "  device_kernel_calls=%llu host_fallback_calls=%llu host_cpu_calls=%llu\n"
                 "  Product path requires device detect. For debugging multi-process "
                 "OpenACC only, set NRN_GPU_THRESH_HOST_FALLBACK=1 (logs + counts).\n"
                 "  NRN_GPU_THRESH_STATS=1 prints totals at exit.\n",
                 thread_id,
                 reason ? reason : "unknown",
                 static_cast<unsigned long long>(g_thresh_device_calls),
                 static_cast<unsigned long long>(g_thresh_host_fallback_calls),
                 static_cast<unsigned long long>(g_thresh_host_cpu_calls));
    hoc_execerror(
        "Native GPU threshold detect: device residency incomplete; host fallback "
        "is not allowed by default. See stderr for reason and counters. "
        "Set NRN_GPU_THRESH_HOST_FALLBACK=1 only for transitional debugging.",
        nullptr);
}

/**
 * Per-thread Th0 detect set (doc/gpu/threshold-detection.md).
 * slots[i] is the sole identity of detect source i; _net_send_buffer stores i.
 * Column arrays are SoA mirrors for (future) device detect kernels.
 */
struct ThreadThresholdTable {
    std::vector<ThresholdPresynSlot> slots;
    std::vector<int> h_thvar_row;
    std::vector<double> h_threshold;
    std::vector<int> h_flag;
    std::size_t device_capacity = 0;
};

std::vector<ThreadThresholdTable> g_tables;
bool g_tables_dirty = true;
// Multi-thread lastpart workers call check_thresh concurrently. NVHPC OpenACC
// host APIs (copyin/delete/update/parallel launch) are not host-thread-safe:
// concurrent detect produced present-table corruption (partial-present with
// garbage sizes, size 100 vs 80) and aborted multi-thread native runs with
// thresholds (natrans, 100-cell NetCon). Serialize rebuild/upload *and* the
// device detect kernel path.
std::mutex g_thresh_table_mutex;
std::mutex g_thresh_device_mutex;

void ensure_tables_sized() {
    if (g_tables.size() < static_cast<std::size_t>(nrn_nthread)) {
        g_tables.resize(nrn_nthread);
    }
}

void free_device_arrays(ThreadThresholdTable& table) {
#if defined(NRN_ENABLE_GPU) && defined(_OPENACC)
    auto const count = table.device_capacity;
    if (count == 0) {
        return;
    }
    // Process-exit path: CUDA/OpenACC may already be deinitialized. Skip acc_delete.
    if (device_resources_finalized()) {
        table.device_capacity = 0;
        return;
    }
    // Only delete if the host buffer is still present on the device.
    // Callers must free *before* host vector resize so data() still matches the
    // present mapping (device_capacity is the copyin length).
    if (table.h_thvar_row.data() && nrn_target_is_present(table.h_thvar_row.data())) {
        nrn_target_delete(table.h_thvar_row.data(), count);
    }
    if (table.h_threshold.data() && nrn_target_is_present(table.h_threshold.data())) {
        nrn_target_delete(table.h_threshold.data(), count);
    }
    if (table.h_flag.data() && nrn_target_is_present(table.h_flag.data())) {
        nrn_target_delete(table.h_flag.data(), count);
    }
    table.device_capacity = 0;
#else
    (void) table;
#endif
}

void upload_device_arrays(ThreadThresholdTable& table) {
#if defined(NRN_ENABLE_GPU) && defined(_OPENACC)
    // Caller already free'd when rebuilding; still free if capacity set (re-upload).
    free_device_arrays(table);
    auto const count = table.h_thvar_row.size();
    if (count == 0 || !table.h_thvar_row.data() || !table.h_threshold.data() ||
        !table.h_flag.data()) {
        return;
    }
    (void) nrn_target_copyin(table.h_thvar_row.data(), count);
    (void) nrn_target_copyin(table.h_threshold.data(), count);
    (void) nrn_target_copyin(table.h_flag.data(), count);
    table.device_capacity = count;
#else
    (void) table;
#endif
}

void rebuild_thread_table(int tid) {
    ensure_tables_sized();
    auto& table = g_tables[tid];
    // Free device mapping while host vectors still match device_capacity.
    free_device_arrays(table);

    int const n = collect_threshold_presyn_slots(nrn_threads + tid, nullptr, 0);
    table.slots.resize(static_cast<std::size_t>(n));
    if (n > 0) {
        collect_threshold_presyn_slots(nrn_threads + tid,
                                       table.slots.data(),
                                       n);
    }

    auto const count = table.slots.size();
    table.h_thvar_row.resize(count);
    table.h_threshold.resize(count);
    table.h_flag.resize(count);
    for (std::size_t i = 0; i < count; ++i) {
        table.h_thvar_row[i] = table.slots[i].thvar_row;
        table.h_threshold[i] = table.slots[i].threshold;
        table.h_flag[i] = table.slots[i].flag;
    }
    upload_device_arrays(table);
}

void ensure_thread_table(int tid) {
    (void) tid;
    // Double-checked locking: workers hit this every detect step; rebuild is rare.
    // Use positive condition (if dirty) so optimizers cannot invert the sense of
    // the flag relative to the rebuild body (observed inverted codegen risk).
    if (g_tables_dirty) {
        std::lock_guard<std::mutex> const lock{g_thresh_table_mutex};
        if (g_tables_dirty) {
            for (int i = 0; i < nrn_nthread; ++i) {
                rebuild_thread_table(i);
            }
            g_tables_dirty = false;
        }
    }
}

void ensure_thread_net_send_buffer(NrnThread& nt, int min_count) {
    ensure_thread_net_send_buffers(&nt);
    int const needed = std::max(min_count, 8);
    if (nt._net_send_buffer_size < needed) {
        int* const old = nt._net_send_buffer;
        int const old_size = nt._net_send_buffer_size;
        nt._net_send_buffer_size = needed;
        nt._net_send_buffer = static_cast<int*>(std::calloc(nt._net_send_buffer_size, sizeof(int)));
        if (old) {
            // Must drop OpenACC present even if NEURON present-table missed it
            // (pragma copyin paths). Otherwise host allocator may reuse the
            // address for a larger buffer → partial-present (size 80 vs 100).
            if (nrn_target_is_present(old)) {
                nrn_target_delete(old, static_cast<std::size_t>(old_size));
            }
#if defined(NRN_ENABLE_GPU) && defined(_OPENACC)
            else if (acc_is_present(old, static_cast<std::size_t>(old_size) * sizeof(int))) {
                acc_delete(old, static_cast<std::size_t>(old_size) * sizeof(int));
            }
#endif
            std::free(old);
        }
    }
#if defined(NRN_ENABLE_GPU) && defined(_OPENACC)
    if (!nt._net_send_buffer || nt._net_send_buffer_size <= 0) {
        return;
    }
    if (!nrn_target_is_present(nt._net_send_buffer)) {
        // If OpenACC still has a stale present at this address (orphan), drop it.
        std::size_t const nbytes =
            static_cast<std::size_t>(nt._net_send_buffer_size) * sizeof(int);
        if (acc_is_present(nt._net_send_buffer, nbytes)) {
            acc_delete(nt._net_send_buffer, nbytes);
        }
        (void) nrn_target_copyin(nt._net_send_buffer,
                                 static_cast<std::size_t>(nt._net_send_buffer_size));
    }
#else
    (void) nt;
#endif
}

// CoreNEURON-shaped hysteresis (must stay simple for OpenACC inlining).
[[nodiscard]] bool pscheck(double v, double thresh, int* flag) {
    if (v > thresh) {
        if (*flag == 0) {
            *flag = 1;
            return true;
        }
    } else {
        *flag = 0;
    }
    return false;
}

/** Host serial detect over Th0 columns (CPU path, or opt-in fallback). */
void detect_threshold_hits_host(int count,
                                int const* thvar_row,
                                double const* threshold,
                                int* flag,
                                double const* vec_v,
                                int* nsbuffer,
                                int nsbuffer_size,
                                int& net_send_buf_count) {
    net_send_buf_count = 0;
    for (int i = 0; i < count; ++i) {
        if (pscheck(vec_v[thvar_row[i]], threshold[i], &flag[i])) {
            int const idx = net_send_buf_count++;
            if (idx < nsbuffer_size) {
                nsbuffer[idx] = i;  // slot index
            }
        }
    }
}

}  // namespace

void invalidate_threshold_tables() noexcept {
    std::lock_guard<std::mutex> const lock{g_thresh_table_mutex};
    g_tables_dirty = true;
    for (auto& table: g_tables) {
        free_device_arrays(table);
        table.slots.clear();
        table.h_thvar_row.clear();
        table.h_threshold.clear();
        table.h_flag.clear();
    }
}

void reseed_threshold_flags_from_host_voltage() noexcept {
#if defined(NRN_ENABLE_GPU)
    // PreSyn::flag_ can lag device↔host handoffs. At psolve entry after a host
    // half-step, set hysteresis from the live host voltage: already-above must
    // not re-fire on the first device step.
    for (int tid = 0; tid < nrn_nthread; ++tid) {
        NrnThread* const nt = nrn_threads + tid;
        int const n = collect_threshold_presyn_slots(nt, nullptr, 0);
        if (n <= 0) {
            continue;
        }
        std::vector<ThresholdPresynSlot> slots(static_cast<std::size_t>(n));
        collect_threshold_presyn_slots(nt, slots.data(), n);
        double const* const vec_v = nt->node_voltage_storage();
        if (!vec_v) {
            continue;
        }
        for (int i = 0; i < n; ++i) {
            auto* const ps = static_cast<PreSyn*>(slots[static_cast<std::size_t>(i)].presyn);
            if (!ps) {
                continue;
            }
            int const row = slots[static_cast<std::size_t>(i)].thvar_row;
            if (row < 0 || row >= nt->end) {
                continue;
            }
            // ConditionEvent: flag_ true = above threshold (already crossed).
            ps->flag_ = (vec_v[row] > slots[static_cast<std::size_t>(i)].threshold);
        }
    }
    invalidate_threshold_tables();
#else
    (void) 0;
#endif
}

void invalidate_auxiliary_device_uploads() noexcept {
    invalidate_threshold_tables();
#if defined(NRN_ENABLE_GPU) && defined(_OPENACC)
    for (int ith = 0; ith < nrn_nthread; ++ith) {
        auto& nt = nrn_threads[ith];
        if (nt._net_send_buffer && nt._net_send_buffer_size > 0 &&
            nrn_target_is_present(nt._net_send_buffer)) {
            nrn_target_delete(nt._net_send_buffer,
                              static_cast<std::size_t>(nt._net_send_buffer_size));
        }
    }
#endif
}

bool check_thresh_presyn_on_device(NrnThread* nt, double teps) noexcept {
#if defined(NRN_ENABLE_GPU)
    if (!enabled() || !backend_native() || !nt || !nt->compute_gpu || nt->end <= 0) {
        return false;
    }
    phase_timer::Scope const timer{phase_timer::Id::deliver_events};
    ensure_thread_table(nt->id);
    auto& table = g_tables[nt->id];
    int const count = static_cast<int>(table.slots.size());
    // Empty table: still "handled" so caller does not fall back to a full psl_thr_
    // walk for SoA sources that do not exist.
    if (count == 0) {
        return true;
    }

    // Serialize all OpenACC host APIs for threshold (copyin of nsbuffer + detect
    // kernel + host updates). Worker std::threads under pc.nthread(n,1) must not
    // call into the OpenACC runtime concurrently.
    std::lock_guard<std::mutex> const device_lock{g_thresh_device_mutex};

    ensure_thread_net_send_buffer(*nt, count);

    if (table.device_capacity != static_cast<std::size_t>(count)) {
        // Nested lock: g_thresh_table_mutex is only taken here under device_lock,
        // and ensure_thread_table takes table_mutex alone — no reverse order.
        std::lock_guard<std::mutex> const lock{g_thresh_table_mutex};
        if (table.device_capacity != static_cast<std::size_t>(count)) {
            rebuild_thread_table(nt->id);
        }
        if (table.device_capacity != static_cast<std::size_t>(count)) {
            return false;
        }
    }

#if defined(NRN_ENABLE_GPU) && defined(_OPENACC)
    nrn_pragma_acc(wait(nt->stream_id))
#endif

    int* const thvar_row = table.h_thvar_row.data();
    double* const threshold = table.h_threshold.data();
    int* const flag = table.h_flag.data();
    auto* const vec_v = nt->node_voltage_storage();
    int* const nsbuffer = nt->_net_send_buffer;
    int const nsbuffer_size = nt->_net_send_buffer_size;
    // Never call OpenACC present/copyin/deviceptr on null host pointers (SEGV).
    bool const host_ptrs_ok =
        thvar_row && threshold && flag && vec_v && nsbuffer && nsbuffer_size > 0;
    if (!host_ptrs_ok) {
        if (allow_thresh_host_fallback()) {
            note_thresh_host_fallback("null host pointer for thresh tables or net_send buffer");
        } else {
            fail_thresh_device_unavailable(
                "null host pointer for thresh tables or net_send buffer", nt->id);
        }
    }

    // Th1: OpenACC detect over slot columns + device vec_v (CoreNEURON pscheck shape).
    // Hit list = slot indices. Host still delivers. Host vec_v not required (Th2).
    // deviceptr for V: no present(host V) during psolve (host must not re-enter device).
    //
    // Product policy (native compute_gpu): device kernel only. Missing residency is a
    // hard error — not a silent host path (wrong product claim + D2H traffic).
    // Opt-in debug: NRN_GPU_THRESH_HOST_FALLBACK=1 (counts + one-time warning).
    // Stats: NRN_GPU_THRESH_STATS=1 prints totals at process exit.
    ensure_thresh_stats_atexit();
    nt->_net_send_buffer_cnt = 0;
    int net_send_buf_count = 0;

    bool ran_device = false;
#if defined(NRN_ENABLE_GPU) && (defined(_OPENACC) || defined(_OPENMP))
    if (nt->compute_gpu && host_ptrs_ok) {
        double* d_v = static_cast<double*>(acc_deviceptr(vec_v));
        if (!d_v) {
            d_v = nrn_target_is_present(vec_v);
        }
        char const* const block = thresh_device_block_reason(
            d_v, thvar_row, threshold, flag, nsbuffer);
        if (!block) {
            ran_device = true;
            ++g_thresh_device_calls;
            // clang-format off
            nrn_pragma_acc(parallel loop present(thvar_row [0:count],
                                                 threshold [0:count],
                                                 flag [0:count],
                                                 nsbuffer [0:nsbuffer_size])
                               deviceptr(d_v)
                               copy(net_send_buf_count) if (nt->compute_gpu) async(nt->stream_id))
            nrn_pragma_omp(target teams distribute parallel for map(tofrom: net_send_buf_count) if(nt->compute_gpu))
            // clang-format on
            for (int i = 0; i < count; ++i) {
                int idx = 0;
                int const thidx = thvar_row[i];
                double const v = d_v[thidx];
                double const thresh = threshold[i];
                if (pscheck(v, thresh, &flag[i])) {
                    nrn_pragma_acc(atomic capture)
                    nrn_pragma_omp(atomic capture)
                    idx = net_send_buf_count++;
                    if (idx < nsbuffer_size) {
                        nsbuffer[idx] = i;  // slot index
                    }
                }
            }
            nrn_pragma_acc(wait(nt->stream_id))
            nt->_net_send_buffer_cnt = net_send_buf_count;

            if (net_send_buf_count > nsbuffer_size) {
                fprintf(stderr,
                        "ERROR: threshold hit list exceeded (thread %d): hits=%d capacity=%d\n",
                        nt->id,
                        net_send_buf_count,
                        nsbuffer_size);
                std::abort();
            }

            // Flags + hit list live on device during the kernel; pull for host deliver/sync.
            // clang-format off
            nrn_pragma_acc(update host(flag [0:count]) async(nt->stream_id))
            nrn_pragma_omp(target update from(flag [0:count]))
            // clang-format on
            if (net_send_buf_count > 0) {
                // clang-format off
                nrn_pragma_acc(update host(nsbuffer [0:net_send_buf_count]) async(nt->stream_id))
                nrn_pragma_omp(target update from(nsbuffer [0:net_send_buf_count]))
                // clang-format on
            }
            nrn_pragma_acc(wait(nt->stream_id))
        } else if (allow_thresh_host_fallback()) {
            note_thresh_host_fallback(block);
        } else {
            fail_thresh_device_unavailable(block, nt->id);
        }
    }
#endif
    if (!ran_device) {
        // Host detect: normal when !compute_gpu; opt-in only when compute_gpu.
        // Requires valid host pointers (same guard as device path).
        if (!host_ptrs_ok) {
            net_send_buf_count = 0;
            nt->_net_send_buffer_cnt = 0;
        } else {
            if (nt->compute_gpu) {
                // Allowed only via NRN_GPU_THRESH_HOST_FALLBACK (counted above).
                sync_voltages_to_host_before_check_thresh(*nt);
            } else {
                ++g_thresh_host_cpu_calls;
            }
            detect_threshold_hits_host(count,
                                       thvar_row,
                                       threshold,
                                       flag,
                                       vec_v,
                                       nsbuffer,
                                       nsbuffer_size,
                                       net_send_buf_count);
            nt->_net_send_buffer_cnt = net_send_buf_count;
            if (net_send_buf_count > nsbuffer_size) {
                fprintf(stderr,
                        "ERROR: threshold hit list exceeded (thread %d): hits=%d capacity=%d\n",
                        nt->id,
                        net_send_buf_count,
                        nsbuffer_size);
                std::abort();
            }
        }
    }

    sync_threshold_presyn_flags(table.slots.data(), flag, count);

    // Host deliver only (CoreNEURON-shaped: detect fills buffer, host does send).
    // Atomic capture leaves racey hit order; with NRN_DETERMINISTIC_EVENTS=1 sort
    // by src_gid then slot so PreSyn::send enqueue order is stable across runs.
    if (net_send_buf_count > 0) {
        int const n_hits = nt->_net_send_buffer_cnt;
        std::vector<int> order(static_cast<std::size_t>(n_hits));
        std::iota(order.begin(), order.end(), 0);
        if (neuron::event_order::enabled()) {
            std::stable_sort(order.begin(), order.end(), [&](int ia, int ib) {
                int const sa = nsbuffer[ia];
                int const sb = nsbuffer[ib];
                auto gid_of = [&](int slot) -> int {
                    if (slot < 0 || slot >= count) {
                        return -1;
                    }
                    return table.slots[static_cast<std::size_t>(slot)].gid;
                };
                auto const ka = neuron::event_order::threshold_hit_key(gid_of(sa), sa);
                auto const kb = neuron::event_order::threshold_hit_key(gid_of(sb), sb);
                return neuron::event_order::less(ka, kb);
            });
        }
        for (int k = 0; k < n_hits; ++k) {
            int const i = order[static_cast<std::size_t>(k)];
            int const slot_index = nsbuffer[i];
            if (slot_index < 0 || slot_index >= count) {
                continue;
            }
            deliver_threshold_spike(nt, table.slots[static_cast<std::size_t>(slot_index)].presyn, teps);
        }
    }
    return true;
#else
    (void) nt;
    (void) teps;
    return false;
#endif
}

}  // namespace neuron::gpu