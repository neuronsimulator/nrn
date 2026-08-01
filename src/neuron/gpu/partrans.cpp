#include "neuron/gpu/partrans.hpp"

#include "neuron/gpu/config.hpp"
#include "neuron/gpu/device_state.hpp"
#include "neuron/gpu/offload.hpp"
#include "neuron/gpu/phase_timer.hpp"
#include "neuron/model_data.hpp"

#include "coreneuron/utils/offload.hpp"
#include "multicore.h"
#include "nrn_ansi.h"

extern int nrn_nthread;
extern void hoc_execerror(const char*, const char*);

#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <mutex>
#include <type_traits>

namespace neuron::gpu {

// Forward decls for helpers used before their definitions (S5 traffic notes).
void gap_traffic_note_v_gather(int n_src) noexcept;
void gap_traffic_note_mech_gather(int n_src, int n_from_device) noexcept;
void gap_traffic_note_scatter(int n_ok, int field_fallback, std::size_t field_bytes) noexcept;
void gap_traffic_note_full_v_pull() noexcept;
void gap_traffic_note_bulk_mech() noexcept;
void gap_traffic_note_same_thread(int n_edges) noexcept;
void gap_traffic_note_buffer_edges(int n_edges) noexcept;
void print_gap_traffic_stats(char const* where) noexcept;

namespace {

bool native_gap_gpu_active() {
    return enabled() && backend_native();
}

bool env_truthy(char const* name) noexcept {
    char const* const e = std::getenv(name);
    return e && e[0] && e[0] != '0';
}

/** Opt-in only: allow host V pull / silent skip when device gap gather cannot map. */
bool allow_gap_host_fallback() noexcept {
    static int cached = -1;
    if (cached < 0) {
        cached = env_truthy("NRN_GPU_GAP_HOST_FALLBACK") ? 1 : 0;
    }
    return cached != 0;
}

GapTrafficStats g_traffic{};
bool g_traffic_atexit_registered = false;

void register_traffic_atexit() {
    if (g_traffic_atexit_registered) {
        return;
    }
    g_traffic_atexit_registered = true;
    if (gap_traffic_stats_enabled()) {
        std::atexit([] { print_gap_traffic_stats("atexit"); });
    }
}

std::uint64_t g_gap_gather_ok = 0;
std::uint64_t g_gap_gather_fallback = 0;
std::uint64_t g_gap_scatter_miss = 0;
bool g_gap_fallback_warned = false;
// Multi-thread lastpart runs real worker std::threads (pc.nthread(n,1)). Concurrent
// OpenACC HtoD of the same HalfGap SoA (and interleaved hh STATE) can yield
// CUDA_ERROR_INVALID_CONTEXT on reduced multi-thread gap models. Serialize device
// gap push; host target writes stay per-thread outside this lock.
std::mutex g_gap_device_push_mutex;

void note_gap_host_fallback(char const* where, char const* reason) noexcept {
    ++g_gap_gather_fallback;
    ++g_traffic.gather_fallback;
    if (!g_gap_fallback_warned) {
        g_gap_fallback_warned = true;
        std::fprintf(stderr,
                     "WARNING: native GPU gap transfer host fallback at %s (%s). "
                     "Opt-in only (NRN_GPU_GAP_HOST_FALLBACK=1). "
                     "gather_ok=%llu gather_fallback=%llu scatter_miss=%llu\n",
                     where ? where : "?",
                     reason ? reason : "unknown",
                     static_cast<unsigned long long>(g_gap_gather_ok),
                     static_cast<unsigned long long>(g_gap_gather_fallback),
                     static_cast<unsigned long long>(g_gap_scatter_miss));
    }
}

[[noreturn]] void fail_gap_device(char const* where, char const* reason) {
    std::fprintf(stderr,
                 "ERROR: native GPU gap transfer cannot use device path at %s: %s\n"
                 "  gather_ok=%llu gather_fallback=%llu scatter_miss=%llu\n"
                 "  Product path requires device gather/scatter. For debugging only, "
                 "set NRN_GPU_GAP_HOST_FALLBACK=1.\n",
                 where ? where : "?",
                 reason ? reason : "unknown",
                 static_cast<unsigned long long>(g_gap_gather_ok),
                 static_cast<unsigned long long>(g_gap_gather_fallback),
                 static_cast<unsigned long long>(g_gap_scatter_miss));
    hoc_execerror(
        "Native GPU gap transfer: device residency incomplete; silent host "
        "fallback is not allowed by default. See stderr. "
        "Set NRN_GPU_GAP_HOST_FALLBACK=1 only for transitional debugging.",
        nullptr);
}

template <typename Storage>
void sync_soa_storage_to_device(Storage const& storage) {
    storage.for_each_vector_for_gpu_upload(
        [](auto const& /*tag*/, auto const& vec, int /*field_index*/, int /*array_dim*/) {
            if (vec.empty()) {
                return;
            }
            using Value = typename std::decay_t<decltype(vec)>::value_type;
            if constexpr (std::is_same_v<Value, double>) {
                double const* const data = vec.data();
                if (nrn_target_is_present(data)) {
                    nrn_target_update_on_device(data, vec.size());
                }
            }
        });
}

// Track last copyin length so a grown mailbox never hits partial-present
// (present-at-address alone is not enough when n_mailbox increases).
int g_mailbox_device_n = 0;
double* g_mailbox_device_host = nullptr;

void ensure_mailbox_on_device(double* mailbox, int n_mailbox) {
    if (!mailbox || n_mailbox <= 0) {
        return;
    }
    // Free-before-grow / free-before-rebind: same host pointer with a larger
    // length (or a new pointer at a recycled address) must not keep a smaller map.
    if (g_mailbox_device_host == mailbox && g_mailbox_device_n >= n_mailbox &&
        nrn_target_is_present(mailbox)) {
        return;
    }
    if (g_mailbox_device_host && g_mailbox_device_n > 0 &&
        nrn_target_is_present(g_mailbox_device_host)) {
        nrn_target_delete(g_mailbox_device_host, static_cast<std::size_t>(g_mailbox_device_n));
    } else if (nrn_target_is_present(mailbox)) {
        // Stale size unknown — binary-search drop (not acc_delete of guessed len).
        target_drop_present_unknown_size(mailbox);
    }
    nrn_target_copyin(mailbox, static_cast<std::size_t>(n_mailbox));
    g_mailbox_device_host = mailbox;
    g_mailbox_device_n = n_mailbox;
}

}  // namespace

void gather_gap_voltage_sources_to_outsrc(int const* v_node_index_per_outsrc,
                                          int n_outsrc,
                                          double* outsrc_buf) {
#if defined(NRN_ENABLE_GPU)
    if (!native_gap_gpu_active() || n_outsrc <= 0 || !v_node_index_per_outsrc || !outsrc_buf) {
        return;
    }
    NrnThread& nt0 = nrn_threads[0];
    if (nt0.end <= 0) {
        return;
    }
    auto* const vec_v = nt0.node_voltage_storage();
    double* d_v = static_cast<double*>(acc_deviceptr(vec_v));
    if (!d_v) {
        d_v = nrn_target_is_present(vec_v);
    }
    if (!d_v) {
        if (allow_gap_host_fallback()) {
            note_gap_host_fallback("gather_outsrc", "device vec_v not mapped");
            return;
        }
        fail_gap_device("gather_outsrc", "device vec_v not mapped");
    }
    ensure_mailbox_on_device(outsrc_buf, n_outsrc);
    double* d_out = nrn_target_is_present(outsrc_buf);
    if (!d_out) {
        d_out = static_cast<double*>(acc_deviceptr(outsrc_buf));
    }
    if (!d_out) {
        if (allow_gap_host_fallback()) {
            note_gap_host_fallback("gather_outsrc", "outsrc_buf not present on device");
            return;
        }
        fail_gap_device("gather_outsrc", "outsrc_buf not present on device");
    }
    ++g_gap_gather_ok;
    int const saved_compute_gpu = nt0.compute_gpu;
    nt0.compute_gpu = 1;
    int const* const vnode_p = v_node_index_per_outsrc;
    nrn_pragma_acc(parallel loop deviceptr(d_v, d_out) copyin(vnode_p [0:n_outsrc])
                       async(nt0.stream_id))
    nrn_pragma_omp(target teams distribute parallel for simd map(to: vnode_p[0:n_outsrc]) if(nt0.compute_gpu))
    for (int i = 0; i < n_outsrc; ++i) {
        int const ix = vnode_p[i];
        if (ix >= 0) {
            d_out[i] = d_v[ix];
        }
    }
    nrn_pragma_acc(wait(nt0.stream_id))
    nrn_pragma_acc(update host(outsrc_buf [0:n_outsrc]))
    nrn_pragma_omp(target update from(outsrc_buf [0:n_outsrc]) if (nt0.compute_gpu))
    nt0.compute_gpu = saved_compute_gpu;
#else
    (void) v_node_index_per_outsrc;
    (void) n_outsrc;
    (void) outsrc_buf;
#endif
}

void gather_gap_voltage_sources_multithread(
    std::vector<std::vector<int>> const& outsrc_index_by_thread,
    std::vector<std::vector<int>> const& v_node_index_by_thread,
    int n_outsrc,
    double* outsrc_buf) {
#if defined(NRN_ENABLE_GPU)
    if (!native_gap_gpu_active() || n_outsrc <= 0 || !outsrc_buf) {
        return;
    }
    ensure_mailbox_on_device(outsrc_buf, n_outsrc);
    double* d_out = nrn_target_is_present(outsrc_buf);
    if (!d_out) {
        d_out = static_cast<double*>(acc_deviceptr(outsrc_buf));
    }
    if (!d_out) {
        if (allow_gap_host_fallback()) {
            note_gap_host_fallback("gather_outsrc_mt", "outsrc_buf not present on device");
            return;
        }
        fail_gap_device("gather_outsrc_mt", "outsrc_buf not present on device");
    }
    int const nthread = static_cast<int>(outsrc_index_by_thread.size());
    bool any_gpu{false};
    for (int tid = 0; tid < nthread; ++tid) {
        auto const& outsrc_indices = outsrc_index_by_thread[tid];
        auto const& v_indices = v_node_index_by_thread[tid];
        int const n = static_cast<int>(outsrc_indices.size());
        if (n <= 0 || outsrc_indices.size() != v_indices.size()) {
            continue;
        }
        NrnThread& nt = nrn_threads[tid];
        if (nt.end <= 0) {
            continue;
        }
        auto* const vec_v = nt.node_voltage_storage();
        double* d_v = static_cast<double*>(acc_deviceptr(vec_v));
        if (!d_v) {
            d_v = nrn_target_is_present(vec_v);
        }
        if (!d_v) {
            if (allow_gap_host_fallback()) {
                note_gap_host_fallback("gather_outsrc_mt", "device vec_v not mapped");
                continue;
            }
            fail_gap_device("gather_outsrc_mt", "device vec_v not mapped");
        }
        int const saved_compute_gpu = nt.compute_gpu;
        nt.compute_gpu = 1;
        any_gpu = true;
        int const* const outsrc_idx = outsrc_indices.data();
        int const* const v_node_idx = v_indices.data();
        nrn_pragma_acc(parallel loop deviceptr(d_v, d_out) copyin(outsrc_idx [0:n], v_node_idx [0:n])
                           async(nt.stream_id))
        nrn_pragma_omp(target teams distribute parallel for simd map(to: outsrc_idx[0:n], v_node_idx[0:n]) if(nt.compute_gpu))
        for (int i = 0; i < n; ++i) {
            int const ix = v_node_idx[i];
            if (ix >= 0) {
                d_out[outsrc_idx[i]] = d_v[ix];
            }
        }
        nt.compute_gpu = saved_compute_gpu;
    }
    if (any_gpu) {
        for (int tid = 0; tid < nthread; ++tid) {
            nrn_pragma_acc(wait(nrn_threads[tid].stream_id))
        }
        nrn_pragma_acc(update host(outsrc_buf [0:n_outsrc]))
        nrn_pragma_omp(target update from(outsrc_buf [0:n_outsrc]))
        ++g_gap_gather_ok;
    }
#else
    (void) outsrc_index_by_thread;
    (void) v_node_index_by_thread;
    (void) n_outsrc;
    (void) outsrc_buf;
#endif
}

bool gather_gap_voltage_mailbox(std::vector<std::vector<int>> const& slot_by_tid,
                                std::vector<std::vector<int>> const& vnode_by_tid,
                                double* mailbox,
                                int n_mailbox) {
#if defined(NRN_ENABLE_GPU)
    phase_timer::Scope const timer{phase_timer::Id::gap_gather};
    phase_timer::bump(phase_timer::Id::gap_gather);
    if (!native_gap_gpu_active() || n_mailbox <= 0 || !mailbox) {
        return false;
    }
    ensure_mailbox_on_device(mailbox, n_mailbox);
    int const nthread = static_cast<int>(slot_by_tid.size());
    bool any{false};
    for (int tid = 0; tid < nthread && tid < nrn_nthread; ++tid) {
        auto const& slots = slot_by_tid[tid];
        auto const& vnodes = vnode_by_tid[tid];
        int const n = static_cast<int>(slots.size());
        if (n <= 0 || slots.size() != vnodes.size()) {
            continue;
        }
        NrnThread& nt = nrn_threads[tid];
        if (nt.end <= 0) {
            continue;
        }
        auto* const vec_v = nt.node_voltage_storage();
        // Match post_solve: use deviceptr(acc_deviceptr), not present(vec_v) —
        // present fails when the SoA mapping is not a plain present entry.
        double* d_v = static_cast<double*>(acc_deviceptr(vec_v));
        if (!d_v) {
            // Prefer NEURON present-table (nrn_target_copyin paths).
            d_v = nrn_target_is_present(vec_v);
        }
        if (!d_v) {
            if (allow_gap_host_fallback()) {
                note_gap_host_fallback("gather_mailbox", "device vec_v not mapped");
                continue;
            }
            fail_gap_device("gather_mailbox", "device vec_v not mapped");
        }
        double* d_mailbox = nrn_target_is_present(mailbox);
        if (!d_mailbox) {
            d_mailbox = static_cast<double*>(acc_deviceptr(mailbox));
        }
        if (!d_mailbox) {
            if (allow_gap_host_fallback()) {
                note_gap_host_fallback("gather_mailbox", "mailbox not present on device");
                continue;
            }
            fail_gap_device("gather_mailbox", "mailbox not present on device");
        }
        int const saved = nt.compute_gpu;
        nt.compute_gpu = 1;
        any = true;
        int const* const slot_p = slots.data();
        int const* const vnode_p = vnodes.data();
        nrn_pragma_acc(parallel loop deviceptr(d_v, d_mailbox) copyin(slot_p [0:n], vnode_p [0:n])
                           async(nt.stream_id))
        nrn_pragma_omp(target teams distribute parallel for simd map(to: slot_p[0:n], vnode_p[0:n]) if(nt.compute_gpu))
        for (int i = 0; i < n; ++i) {
            int const ix = vnode_p[i];
            int const s = slot_p[i];
            if (ix >= 0 && s >= 0 && s < n_mailbox) {
                d_mailbox[s] = d_v[ix];
            }
        }
        nt.compute_gpu = saved;
    }
    if (any) {
        for (int tid = 0; tid < nthread && tid < nrn_nthread; ++tid) {
            nrn_pragma_acc(wait(nrn_threads[tid].stream_id))
        }
        nrn_pragma_acc(update host(mailbox [0:n_mailbox]))
        nrn_pragma_omp(target update from(mailbox [0:n_mailbox]))
        gap_traffic_note_d2h_bulk(static_cast<std::size_t>(n_mailbox) * sizeof(double));
        ++g_gap_gather_ok;
        ++g_traffic.gather_ok;
        gap_traffic_note_v_gather(n_mailbox);
    }
    return any;
#else
    (void) slot_by_tid;
    (void) vnode_by_tid;
    (void) mailbox;
    (void) n_mailbox;
    return false;
#endif
}

void sync_insrc_buf_to_device(double* insrc_buf, int n_insrc) {
#if defined(NRN_ENABLE_GPU)
    phase_timer::Scope const timer{phase_timer::Id::gap_insrc};
    phase_timer::bump(phase_timer::Id::gap_insrc);
    if (!native_gap_gpu_active() || n_insrc <= 0 || !insrc_buf) {
        return;
    }
    for (int tid = 0; tid < nrn_nthread; ++tid) {
        NrnThread& nt = nrn_threads[tid];
        if (!nt.compute_gpu) {
            continue;
        }
        int const saved_compute_gpu = nt.compute_gpu;
        nt.compute_gpu = 1;
        nrn_pragma_acc(update device(insrc_buf [0:n_insrc]) async(nt.stream_id))
        nrn_pragma_omp(target update to(insrc_buf [0:n_insrc]) if (nt.compute_gpu))
        nrn_pragma_acc(wait(nt.stream_id))
        gap_traffic_note_h2d_bulk(static_cast<std::size_t>(n_insrc) * sizeof(double));
        nt.compute_gpu = saved_compute_gpu;
        return;
    }
#else
    (void) insrc_buf;
    (void) n_insrc;
#endif
}

namespace {

/** Resolve device address for a host scalar that may be mid-SoA.
 *  Direct present/deviceptr often fails for interior pointers; fall back to
 *  base-of-field + offset when p lies inside a present double vector of a
 *  gap-target mechanism type. */
double* device_ptr_for_host_scalar(double* p, int const* mech_types, int n_types) {
    if (!p) {
        return nullptr;
    }
    if (double* d = nrn_target_is_present(p)) {
        return d;
    }
    if (double* d = static_cast<double*>(acc_deviceptr(p))) {
        return d;
    }
    if (!mech_types || n_types <= 0) {
        return nullptr;
    }
    for (int ti = 0; ti < n_types; ++ti) {
        int const type = mech_types[ti];
        if (!neuron::model().is_valid_mechanism(type)) {
            continue;
        }
        double* found = nullptr;
        neuron::model().mechanism_data(type).for_each_vector_for_gpu_upload(
            [&](auto const& /*tag*/, auto const& vec, int /*field_index*/, int /*array_dim*/) {
                if (found || vec.empty()) {
                    return;
                }
                using Value = typename std::decay_t<decltype(vec)>::value_type;
                if constexpr (std::is_same_v<Value, double>) {
                    double* const base = const_cast<double*>(vec.data());
                    std::size_t const n = vec.size();
                    if (p < base || p >= base + n) {
                        return;
                    }
                    double* d_base = nrn_target_is_present(base);
                    if (!d_base) {
                        d_base = static_cast<double*>(acc_deviceptr(base));
                    }
                    if (d_base) {
                        found = d_base + (p - base);
                    }
                }
            });
        if (found) {
            return found;
        }
    }
    return nullptr;
}

/** Push only SoA double columns that contain at least one of host_ptrs (typically vgap).
 *  Never rewrites device-authoritative fields that are not transfer targets. */
void push_target_fields_containing(double* const* host_ptrs,
                                   int n,
                                   int const* mech_types,
                                   int n_types) {
    if (!host_ptrs || n <= 0 || !mech_types || n_types <= 0) {
        return;
    }
    for (int ti = 0; ti < n_types; ++ti) {
        int const type = mech_types[ti];
        if (!neuron::model().is_valid_mechanism(type)) {
            continue;
        }
        neuron::model().mechanism_data(type).for_each_vector_for_gpu_upload(
            [&](auto const& /*tag*/, auto const& vec, int /*field_index*/, int /*array_dim*/) {
                if (vec.empty()) {
                    return;
                }
                using Value = typename std::decay_t<decltype(vec)>::value_type;
                if constexpr (std::is_same_v<Value, double>) {
                    double* const base = const_cast<double*>(vec.data());
                    std::size_t const nvec = vec.size();
                    bool hit = false;
                    for (int i = 0; i < n; ++i) {
                        double* const p = host_ptrs[i];
                        if (p && p >= base && p < base + nvec) {
                            hit = true;
                            break;
                        }
                    }
                    if (!hit) {
                        return;
                    }
                    if (nrn_target_is_present(base)) {
                        nrn_target_update_on_device(base, nvec);
                    }
                }
            });
    }
}

/**
 * Resolve host scalar to device field base + row offset (for bulk device scatter).
 * Prefer mid-SoA field mapping; fall back to direct present of the scalar.
 * @return true if *out_d_base and *out_off are set.
 */
bool resolve_scatter_loc(double* p,
                         int const* mech_types,
                         int n_types,
                         double** out_d_base,
                         int* out_off) {
    if (!p || !out_d_base || !out_off) {
        return false;
    }
    if (mech_types && n_types > 0) {
        for (int ti = 0; ti < n_types; ++ti) {
            int const type = mech_types[ti];
            if (!neuron::model().is_valid_mechanism(type)) {
                continue;
            }
            bool found = false;
            neuron::model().mechanism_data(type).for_each_vector_for_gpu_upload(
                [&](auto const& /*tag*/, auto const& vec, int /*field_index*/, int /*array_dim*/) {
                    if (found || vec.empty()) {
                        return;
                    }
                    using Value = typename std::decay_t<decltype(vec)>::value_type;
                    if constexpr (std::is_same_v<Value, double>) {
                        double* const base = const_cast<double*>(vec.data());
                        std::size_t const nvec = vec.size();
                        if (p < base || p >= base + nvec) {
                            return;
                        }
                        double* d_base = nrn_target_is_present(base);
                        if (!d_base) {
                            d_base = static_cast<double*>(acc_deviceptr(base));
                        }
                        if (d_base) {
                            *out_d_base = d_base;
                            *out_off = static_cast<int>(p - base);
                            found = true;
                        }
                    }
                });
            if (found) {
                return true;
            }
        }
    }
    // Direct present of scalar (rare for mid-SoA; useful for bare node fields).
    if (double* d = nrn_target_is_present(p)) {
        *out_d_base = d;
        *out_off = 0;
        return true;
    }
    if (double* d = static_cast<double*>(acc_deviceptr(p))) {
        *out_d_base = d;
        *out_off = 0;
        return true;
    }
    return false;
}

/**
 * CoreNEURON-style scatter: pack host target values, bulk H→D, device write into
 * field bases via offsets. One host-API bulk update of the value buffer per unique
 * SoA column (+ copyin of offsets), not O(n) scalar memcpy_to_device.
 * @return number of targets successfully scheduled on device.
 */
int scatter_targets_bulk_device(double* const* host_ptrs,
                               int n,
                               int const* mech_types,
                               int n_types,
                               int stream_id) {
    if (!host_ptrs || n <= 0) {
        return 0;
    }
    // Group edges by device field base (ringtest HalfGap: one vgap column).
    struct Edge {
        int off;
        double val;
    };
    // Pointer identity of d_base is stable for the step.
    std::vector<double*> bases;
    std::vector<std::vector<Edge>> groups;
    bases.reserve(4);
    groups.reserve(4);

    auto group_for = [&](double* d_base) -> std::vector<Edge>& {
        for (size_t g = 0; g < bases.size(); ++g) {
            if (bases[g] == d_base) {
                return groups[g];
            }
        }
        bases.push_back(d_base);
        groups.emplace_back();
        return groups.back();
    };

    int ok = 0;
    for (int i = 0; i < n; ++i) {
        double* const p = host_ptrs[i];
        if (!p) {
            continue;
        }
        double* d_base = nullptr;
        int off = 0;
        if (!resolve_scatter_loc(p, mech_types, n_types, &d_base, &off) || !d_base) {
            continue;
        }
        group_for(d_base).push_back(Edge{off, *p});
        ++ok;
    }
    if (ok == 0) {
        return 0;
    }

    // Persistent host staging for vals/offs (main-thread scatter only; resized;
    // re-copyin when host pointer / capacity mapping is stale).
    static std::vector<double> vals_staging;
    static std::vector<int> offs_staging;
    static int vals_device_n = 0;
    static double* vals_device_host = nullptr;
    static int offs_device_n = 0;
    static int* offs_device_host = nullptr;

    auto ensure_double_buf = [&](double* host, int nbuf) {
        if (!host || nbuf <= 0) {
            return;
        }
        if (vals_device_host == host && vals_device_n >= nbuf && nrn_target_is_present(host)) {
            return;
        }
        if (vals_device_host && vals_device_n > 0 && nrn_target_is_present(vals_device_host)) {
            nrn_target_delete(vals_device_host, static_cast<std::size_t>(vals_device_n));
        } else if (nrn_target_is_present(host)) {
            target_drop_present_unknown_size(host);
        }
        nrn_target_copyin(host, static_cast<std::size_t>(nbuf));
        vals_device_host = host;
        vals_device_n = nbuf;
    };
    auto ensure_int_buf = [&](int* host, int nbuf) {
        if (!host || nbuf <= 0) {
            return;
        }
        if (offs_device_host == host && offs_device_n >= nbuf && nrn_target_is_present(host)) {
            return;
        }
        if (offs_device_host && offs_device_n > 0 && nrn_target_is_present(offs_device_host)) {
            nrn_target_delete(offs_device_host, static_cast<std::size_t>(offs_device_n));
        } else if (nrn_target_is_present(host)) {
            target_drop_present_unknown_size(host);
        }
        nrn_target_copyin(host, static_cast<std::size_t>(nbuf));
        offs_device_host = host;
        offs_device_n = nbuf;
    };

    for (size_t g = 0; g < bases.size(); ++g) {
        auto const& edges = groups[g];
        int const ne = static_cast<int>(edges.size());
        if (ne <= 0) {
            continue;
        }
        double* const d_base = bases[g];
        vals_staging.resize(static_cast<size_t>(ne));
        offs_staging.resize(static_cast<size_t>(ne));
        for (int i = 0; i < ne; ++i) {
            vals_staging[static_cast<size_t>(i)] = edges[static_cast<size_t>(i)].val;
            offs_staging[static_cast<size_t>(i)] = edges[static_cast<size_t>(i)].off;
        }
        double* const vals_p = vals_staging.data();
        int* const offs_p = offs_staging.data();
        ensure_double_buf(vals_p, ne);
        ensure_int_buf(offs_p, ne);
        // Bulk H→D of packed values (and offsets if mapping was reused with dirty host).
        nrn_pragma_acc(update device(vals_p [0:ne], offs_p [0:ne]) async(stream_id))
        nrn_pragma_omp(target update to(vals_p[0:ne], offs_p[0:ne]))
        gap_traffic_note_h2d_bulk(static_cast<std::size_t>(ne) * (sizeof(double) + sizeof(int)));

        double* d_vals = nrn_target_is_present(vals_p);
        int* d_offs = nrn_target_is_present(offs_p);
        if (!d_vals) {
            d_vals = static_cast<double*>(acc_deviceptr(vals_p));
        }
        if (!d_offs) {
            d_offs = static_cast<int*>(acc_deviceptr(offs_p));
        }
        if (!d_vals || !d_offs) {
            // Fall back: field column push for residual (still not per-scalar).
            continue;
        }
        nrn_pragma_acc(parallel loop deviceptr(d_base, d_vals, d_offs) async(stream_id))
        nrn_pragma_omp(target teams distribute parallel for simd if(1))
        for (int i = 0; i < ne; ++i) {
            int const off = d_offs[i];
            if (off >= 0) {
                d_base[off] = d_vals[i];
            }
        }
        nrn_pragma_acc(wait(stream_id))
    }
    return ok;
}

}  // namespace

bool gather_gap_mech_range_mailbox(double* const* host_ptrs,
                                   int n,
                                   int const* mech_types,
                                   int n_types,
                                   double* mailbox) {
#if defined(NRN_ENABLE_GPU)
    phase_timer::Scope const timer{phase_timer::Id::gap_gather};
    phase_timer::bump(phase_timer::Id::gap_gather);
    if (!native_gap_gpu_active() || n <= 0 || !host_ptrs || !mailbox) {
        return false;
    }
    // Sparse per-source D→H: mechanism SoA is not a single vec_v. Mid-SoA host
    // scalars resolve via field base + offset (same helper as target scatter).
    int n_device = 0;
    for (int i = 0; i < n; ++i) {
        double* const p = host_ptrs[i];
        if (!p) {
            mailbox[i] = 0.0;
            continue;
        }
        double* d = device_ptr_for_host_scalar(p, mech_types, n_types);
        if (d) {
#if defined(_OPENACC)
            acc_memcpy_from_device(&mailbox[i], d, sizeof(double));
            ++n_device;
#else
            mailbox[i] = *p;
#endif
        } else {
            // Host-only residual (mech not device-resident): use host value.
            mailbox[i] = *p;
        }
    }
    if (n_device > 0) {
        ++g_gap_gather_ok;
        ++g_traffic.gather_ok;
        gap_traffic_note_d2h_scalar(n_device);
    }
    gap_traffic_note_mech_gather(n, n_device);
    return n_device > 0;
#else
    (void) host_ptrs;
    (void) n;
    (void) mech_types;
    (void) n_types;
    (void) mailbox;
    return false;
#endif
}

void scatter_gap_targets_to_device(double* const* host_ptrs, int n) {
    // Untyped: still use bulk path (resolve without mech types → direct present).
    scatter_gap_targets_to_device(host_ptrs, n, nullptr, 0);
}

void scatter_gap_targets_to_device(double* const* host_ptrs,
                                   int n,
                                   int const* mech_types,
                                   int n_types) {
#if defined(NRN_ENABLE_GPU)
    phase_timer::Scope const timer{phase_timer::Id::gap_scatter};
    phase_timer::bump(phase_timer::Id::gap_scatter);
    if (!native_gap_gpu_active() || n <= 0 || !host_ptrs) {
        return;
    }
    std::lock_guard<std::mutex> const lock{g_gap_device_push_mutex};
    // Wait for all threads' device work before host→device target push (main thread only).
    for (int ith = 0; ith < nrn_nthread; ++ith) {
        nrn_pragma_acc(wait(nrn_threads[ith].stream_id))
    }
    // Product path (P4 de-chatty): CoreNEURON-like bulk val buffer + device scatter
    // into field bases. Replaces O(n) scalar acc_memcpy_to_device (~256 H→D/step
    // on ringtest gap — was ~55% wall). Host targets already hold values.
    int stream_id = 0;
    for (int tid = 0; tid < nrn_nthread; ++tid) {
        if (nrn_threads[tid].compute_gpu) {
            stream_id = nrn_threads[tid].stream_id;
            break;
        }
    }
    int nonnull = 0;
    for (int i = 0; i < n; ++i) {
        if (host_ptrs[i]) {
            ++nonnull;
        }
    }
    int ok = scatter_targets_bulk_device(host_ptrs, n, mech_types, n_types, stream_id);
    int miss = nonnull - ok;
    if (miss < 0) {
        miss = 0;
    }
    if (miss > 0) {
        g_gap_scatter_miss += static_cast<std::uint64_t>(miss);
        g_traffic.scatter_miss += static_cast<std::uint64_t>(miss);
        // Residual: push only SoA columns that contain targets (never full mech).
        if (mech_types && n_types > 0) {
            std::size_t field_bytes = 0;
            for (int ti = 0; ti < n_types; ++ti) {
                int const type = mech_types[ti];
                if (!neuron::model().is_valid_mechanism(type)) {
                    continue;
                }
                neuron::model().mechanism_data(type).for_each_vector_for_gpu_upload(
                    [&](auto const& /*tag*/, auto const& vec, int /*fi*/, int /*ad*/) {
                        using Value = typename std::decay_t<decltype(vec)>::value_type;
                        if constexpr (std::is_same_v<Value, double>) {
                            double* const base = const_cast<double*>(vec.data());
                            for (int i = 0; i < n; ++i) {
                                double* const p = host_ptrs[i];
                                if (p && p >= base && p < base + vec.size()) {
                                    field_bytes += vec.size() * sizeof(double);
                                    break;
                                }
                            }
                        }
                    });
            }
            push_target_fields_containing(host_ptrs, n, mech_types, n_types);
            if (field_bytes > 0) {
                gap_traffic_note_h2d_bulk(field_bytes);
            }
            gap_traffic_note_scatter(ok + miss, 1, field_bytes);
        } else {
            // Last resort: per-scalar (should be rare; no field base).
            int scalar_ok = 0;
            for (int i = 0; i < n; ++i) {
                double* const p = host_ptrs[i];
                if (!p) {
                    continue;
                }
                double* d = device_ptr_for_host_scalar(p, mech_types, n_types);
                if (d) {
                    nrn_target_memcpy_to_device(d, p, 1);
                    ++scalar_ok;
                }
            }
            if (scalar_ok > 0) {
                gap_traffic_note_h2d_scalar(scalar_ok);
                ok += scalar_ok;
            }
            gap_traffic_note_scatter(ok, 0, static_cast<std::size_t>(ok) * sizeof(double));
        }
    } else {
        gap_traffic_note_scatter(ok, 0, static_cast<std::size_t>(ok) * sizeof(double));
    }
    if (std::getenv("NRN_GAP_DEBUG")) {
        static int once = 0;
        if (once < 5) {
            std::fprintf(stderr,
                         "scatter_gap_targets bulk ok=%d miss=%d n=%d types=%d\n",
                         ok,
                         miss,
                         n,
                         n_types);
            ++once;
        }
    }
    if (miss > 0 && ok == 0 && model_is_on_device() && !allow_gap_host_fallback()) {
        fail_gap_device("scatter_targets",
                       "no gap target scalars mapped on device after bulk scatter");
    }
#else
    (void) host_ptrs;
    (void) n;
    (void) mech_types;
    (void) n_types;
#endif
}

void sync_gap_target_mechs_to_device(int const* mech_types, int n_types) {
#if defined(NRN_ENABLE_GPU)
    // Legacy full-mech SoA push — clobbers device CURRENT/STATE fields (v_unused, i, …).
    // Product path uses scatter_gap_targets_to_device + vgap-field-only fallback.
    // Opt-in only for transitional debugging.
    if (!env_truthy("NRN_GAP_BULK_MECH_PUSH")) {
        return;
    }
    if (!native_gap_gpu_active() || !mech_types || n_types <= 0) {
        return;
    }
    std::lock_guard<std::mutex> const lock{g_gap_device_push_mutex};
    static bool warned = false;
    if (!warned) {
        warned = true;
        std::fprintf(stderr,
                     "WARNING: NRN_GAP_BULK_MECH_PUSH=1 — full gap-target mech SoA H→D "
                     "(clobbers device fields; not product path)\n");
    }
    gap_traffic_note_bulk_mech();
    for (int i = 0; i < n_types; ++i) {
        int const type = mech_types[i];
        if (!neuron::model().is_valid_mechanism(type)) {
            continue;
        }
        sync_soa_storage_to_device(neuron::model().mechanism_data(type));
    }
#else
    (void) mech_types;
    (void) n_types;
#endif
}

void sync_mechanism_storage_to_device_after_partrans() {
#if defined(NRN_ENABLE_GPU)
    // Deprecated: full SoA clobber. No-op unless explicit debug env.
    if (!env_truthy("NRN_GAP_BULK_MECH_PUSH")) {
        return;
    }
    if (!native_gap_gpu_active()) {
        return;
    }
    gap_traffic_note_bulk_mech();
    neuron::model().apply_to_mechanisms(
        [&](auto const& mech_data) { sync_soa_storage_to_device(mech_data); });
#endif
}

// --- S5 traffic audit + same-thread opt-in ---------------------------------

GapTrafficStats const& gap_traffic_stats() noexcept {
    return g_traffic;
}

bool gap_traffic_stats_enabled() noexcept {
    static int cached = -1;
    if (cached < 0) {
        // Explicit env, or auto-on with phase timer for A+B exploration.
        cached = (env_truthy("NRN_GAP_TRAFFIC_STATS") || phase_timer::enabled()) ? 1 : 0;
    }
    return cached != 0;
}

bool gap_same_thread_device_enabled() noexcept {
    static int cached = -1;
    if (cached < 0) {
        cached = env_truthy("NRN_GAP_SAME_THREAD_DEVICE") ? 1 : 0;
    }
    return cached != 0;
}

void print_gap_traffic_stats(char const* where) noexcept {
    if (!gap_traffic_stats_enabled()) {
        return;
    }
    auto const& s = g_traffic;
    double const avg_d2h =
        s.steps > 0 ? static_cast<double>(s.bytes_d2h) / static_cast<double>(s.steps) : 0.0;
    double const avg_h2d =
        s.steps > 0 ? static_cast<double>(s.bytes_h2d) / static_cast<double>(s.steps) : 0.0;
    double const avg_h2d_scalar =
        s.steps > 0 ? static_cast<double>(s.h2d_scalar_calls) / static_cast<double>(s.steps) : 0.0;
    std::fprintf(stderr,
                 "NRN gap traffic stats (%s):\n"
                 "  steps=%llu buffer_edges=%llu same_thread_device=%llu\n"
                 "  vsrc=%llu msrc=%llu (device=%llu) tar_scatter=%llu field_fb=%llu\n"
                 "  bytes_d2h=%llu bytes_h2d=%llu (avg/step d2h=%.1f h2d=%.1f)\n"
                 "  host_api: d2h_bulk=%llu h2d_bulk=%llu d2h_scalar=%llu h2d_scalar=%llu "
                 "(avg h2d_scalar/step=%.1f)\n"
                 "  full_v_pulls=%llu bulk_mech_pushes=%llu\n"
                 "  gather_ok=%llu gather_fallback=%llu scatter_miss=%llu\n",
                 where ? where : "?",
                 static_cast<unsigned long long>(s.steps),
                 static_cast<unsigned long long>(s.buffer_path_edges),
                 static_cast<unsigned long long>(s.same_thread_device),
                 static_cast<unsigned long long>(s.vsrc_gathered),
                 static_cast<unsigned long long>(s.msrc_gathered),
                 static_cast<unsigned long long>(s.msrc_from_device),
                 static_cast<unsigned long long>(s.tar_scattered),
                 static_cast<unsigned long long>(s.tar_field_fallback),
                 static_cast<unsigned long long>(s.bytes_d2h),
                 static_cast<unsigned long long>(s.bytes_h2d),
                 avg_d2h,
                 avg_h2d,
                 static_cast<unsigned long long>(s.d2h_bulk_calls),
                 static_cast<unsigned long long>(s.h2d_bulk_calls),
                 static_cast<unsigned long long>(s.d2h_scalar_calls),
                 static_cast<unsigned long long>(s.h2d_scalar_calls),
                 avg_h2d_scalar,
                 static_cast<unsigned long long>(s.full_v_pulls),
                 static_cast<unsigned long long>(s.bulk_mech_pushes),
                 static_cast<unsigned long long>(s.gather_ok),
                 static_cast<unsigned long long>(s.gather_fallback),
                 static_cast<unsigned long long>(s.scatter_miss));
    // Product policy: full-V / bulk should be zero unless opt-in debug envs.
    if (s.full_v_pulls > 0 || s.bulk_mech_pushes > 0) {
        std::fprintf(stderr,
                     "  NOTE: full_v_pull or bulk_mech_push > 0 — not product default "
                     "(NRN_GPU_GAP_HOST_FALLBACK / NRN_GAP_BULK_MECH_PUSH).\n");
    }
    // CoreNEURON guide: few bulk updates, not O(edges) scalar host-API.
    if (s.steps > 0 && s.h2d_scalar_calls > s.steps * 4) {
        std::fprintf(stderr,
                     "  HINT: h2d_scalar_calls >> steps — native scatter is chatty vs "
                     "CoreNEURON bulk insrc + device scatter kernel.\n");
    }
}

void gap_traffic_reset() noexcept {
    g_traffic = GapTrafficStats{};
}

void gap_traffic_note_step() noexcept {
    register_traffic_atexit();
    ++g_traffic.steps;
}

void gap_traffic_note_v_gather(int n_src) noexcept {
    if (n_src <= 0) {
        return;
    }
    // Source count only; bytes owned by gap_traffic_note_d2h_bulk/scalar.
    g_traffic.vsrc_gathered += static_cast<std::uint64_t>(n_src);
}

void gap_traffic_note_mech_gather(int n_src, int n_from_device) noexcept {
    if (n_src > 0) {
        g_traffic.msrc_gathered += static_cast<std::uint64_t>(n_src);
    }
    if (n_from_device > 0) {
        g_traffic.msrc_from_device += static_cast<std::uint64_t>(n_from_device);
        // bytes from gap_traffic_note_d2h_scalar at call site
    }
}

void gap_traffic_note_scatter(int n_ok, int field_fallback, std::size_t field_bytes) noexcept {
    if (n_ok > 0) {
        g_traffic.tar_scattered += static_cast<std::uint64_t>(n_ok);
    }
    if (field_fallback > 0) {
        g_traffic.tar_field_fallback += static_cast<std::uint64_t>(field_fallback);
    }
    // field_bytes may include column push beyond per-scalar; scalar bytes via
    // gap_traffic_note_h2d_scalar. Add residual field_bytes only when larger.
    std::uint64_t const scalar_bytes = static_cast<std::uint64_t>(n_ok) * sizeof(double);
    if (field_bytes > scalar_bytes) {
        g_traffic.bytes_h2d += field_bytes - scalar_bytes;
    }
}

void gap_traffic_note_d2h_bulk(std::size_t bytes) noexcept {
    ++g_traffic.d2h_bulk_calls;
    g_traffic.bytes_d2h += bytes;
}

void gap_traffic_note_h2d_bulk(std::size_t bytes) noexcept {
    ++g_traffic.h2d_bulk_calls;
    g_traffic.bytes_h2d += bytes;
}

void gap_traffic_note_d2h_scalar(int n) noexcept {
    if (n <= 0) {
        return;
    }
    g_traffic.d2h_scalar_calls += static_cast<std::uint64_t>(n);
    g_traffic.bytes_d2h += static_cast<std::uint64_t>(n) * sizeof(double);
}

void gap_traffic_note_h2d_scalar(int n) noexcept {
    if (n <= 0) {
        return;
    }
    g_traffic.h2d_scalar_calls += static_cast<std::uint64_t>(n);
    g_traffic.bytes_h2d += static_cast<std::uint64_t>(n) * sizeof(double);
}

void gap_traffic_note_full_v_pull() noexcept {
    ++g_traffic.full_v_pulls;
}

void gap_traffic_note_bulk_mech() noexcept {
    ++g_traffic.bulk_mech_pushes;
}

void gap_traffic_note_same_thread(int n_edges) noexcept {
    if (n_edges > 0) {
        g_traffic.same_thread_device += static_cast<std::uint64_t>(n_edges);
    }
}

void gap_traffic_note_buffer_edges(int n_edges) noexcept {
    if (n_edges > 0) {
        g_traffic.buffer_path_edges += static_cast<std::uint64_t>(n_edges);
    }
}

int same_thread_voltage_device_copy(std::vector<std::vector<int>> const& vnode_by_tid,
                                   std::vector<std::vector<double*>> const& host_tar_by_tid,
                                   int const* mech_types,
                                   int n_types) {
#if defined(NRN_ENABLE_GPU)
    if (!native_gap_gpu_active() || !gap_same_thread_device_enabled()) {
        return 0;
    }
    int done = 0;
    int const nthread = static_cast<int>(vnode_by_tid.size());
    std::lock_guard<std::mutex> const lock{g_gap_device_push_mutex};
    for (int tid = 0; tid < nthread && tid < nrn_nthread; ++tid) {
        auto const& vnodes = vnode_by_tid[tid];
        auto const& tars =
            tid < static_cast<int>(host_tar_by_tid.size()) ? host_tar_by_tid[tid]
                                                           : std::vector<double*>{};
        int const n = static_cast<int>(vnodes.size());
        if (n <= 0 || static_cast<int>(tars.size()) != n) {
            continue;
        }
        NrnThread& nt = nrn_threads[tid];
        if (nt.end <= 0) {
            continue;
        }
        auto* const vec_v = nt.node_voltage_storage();
        double* d_v = static_cast<double*>(acc_deviceptr(vec_v));
        if (!d_v) {
            d_v = nrn_target_is_present(vec_v);
        }
        if (!d_v) {
            continue;
        }
        // Sparse: resolve each target mid-SoA and copy one double from d_v[ix].
        for (int i = 0; i < n; ++i) {
            int const ix = vnodes[i];
            double* const htar = tars[i];
            if (ix < 0 || !htar) {
                continue;
            }
            double* d_tar = device_ptr_for_host_scalar(htar, mech_types, n_types);
            if (!d_tar) {
                continue;
            }
#if defined(_OPENACC)
            // Host staging of one value via device read+write avoids a multi-base kernel.
            double val = 0.0;
            acc_memcpy_from_device(&val, d_v + ix, sizeof(double));
            acc_memcpy_to_device(d_tar, &val, sizeof(double));
            // Also mirror to host tar so host-side consumers see the value.
            *htar = val;
            ++done;
#else
            (void) d_v;
#endif
        }
    }
    gap_traffic_note_same_thread(done);
    // Device→host→device is still sparse (2 doubles per edge); no full-V pull.
    g_traffic.bytes_d2h += static_cast<std::uint64_t>(done) * sizeof(double);
    g_traffic.bytes_h2d += static_cast<std::uint64_t>(done) * sizeof(double);
    return done;
#else
    (void) vnode_by_tid;
    (void) host_tar_by_tid;
    (void) mech_types;
    (void) n_types;
    return 0;
#endif
}

int same_thread_mech_device_copy(double* const* host_src,
                                double* const* host_tar,
                                int n,
                                int const* mech_types,
                                int n_types) {
#if defined(NRN_ENABLE_GPU)
    if (!native_gap_gpu_active() || !gap_same_thread_device_enabled() || n <= 0 || !host_src ||
        !host_tar) {
        return 0;
    }
    std::lock_guard<std::mutex> const lock{g_gap_device_push_mutex};
    int done = 0;
    for (int i = 0; i < n; ++i) {
        double* const hs = host_src[i];
        double* const ht = host_tar[i];
        if (!hs || !ht) {
            continue;
        }
        double* d_s = device_ptr_for_host_scalar(hs, mech_types, n_types);
        double* d_t = device_ptr_for_host_scalar(ht, mech_types, n_types);
        if (!d_s || !d_t) {
            continue;
        }
#if defined(_OPENACC)
        double val = 0.0;
        acc_memcpy_from_device(&val, d_s, sizeof(double));
        acc_memcpy_to_device(d_t, &val, sizeof(double));
        *ht = val;
        ++done;
#endif
    }
    gap_traffic_note_same_thread(done);
    g_traffic.bytes_d2h += static_cast<std::uint64_t>(done) * sizeof(double);
    g_traffic.bytes_h2d += static_cast<std::uint64_t>(done) * sizeof(double);
    return done;
#else
    (void) host_src;
    (void) host_tar;
    (void) n;
    (void) mech_types;
    (void) n_types;
    return 0;
#endif
}

}  // namespace neuron::gpu
