#include "neuron/gpu/partrans.hpp"

#include "neuron/gpu/config.hpp"
#include "neuron/gpu/device_state.hpp"
#include "neuron/gpu/offload.hpp"
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

void ensure_mailbox_on_device(double* mailbox, int n_mailbox) {
    if (!mailbox || n_mailbox <= 0) {
        return;
    }
    if (!nrn_target_is_present(mailbox)) {
        nrn_target_copyin(mailbox, static_cast<std::size_t>(n_mailbox));
    }
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
        ++g_gap_gather_ok;
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

}  // namespace

void scatter_gap_targets_to_device(double* const* host_ptrs, int n) {
#if defined(NRN_ENABLE_GPU)
    if (!native_gap_gpu_active() || n <= 0 || !host_ptrs) {
        return;
    }
    std::lock_guard<std::mutex> const lock{g_gap_device_push_mutex};
    // Wait for all threads' device work before host→device target push (main thread only).
    for (int ith = 0; ith < nrn_nthread; ++ith) {
        nrn_pragma_acc(wait(nrn_threads[ith].stream_id))
    }
    int ok = 0, miss = 0;
    for (int i = 0; i < n; ++i) {
        double* const p = host_ptrs[i];
        if (!p) {
            continue;
        }
        // Direct present / deviceptr (works once the field base is on device).
        double* d = nrn_target_is_present(p);
        if (!d) {
            d = static_cast<double*>(acc_deviceptr(p));
        }
        if (d) {
            nrn_target_memcpy_to_device(d, p, 1);
            ++ok;
        } else {
            ++miss;
            ++g_gap_scatter_miss;
        }
    }
    if (std::getenv("NRN_GAP_DEBUG")) {
        static int once = 0;
        if (once < 5) {
            std::fprintf(stderr,
                         "scatter_gap_targets ok=%d miss=%d n=%d (vgap-field fallback if miss)\n",
                         ok,
                         miss,
                         n);
            ++once;
        }
    }
#else
    (void) host_ptrs;
    (void) n;
#endif
}

void scatter_gap_targets_to_device(double* const* host_ptrs,
                                   int n,
                                   int const* mech_types,
                                   int n_types) {
#if defined(NRN_ENABLE_GPU)
    if (!native_gap_gpu_active() || n <= 0 || !host_ptrs) {
        return;
    }
    std::lock_guard<std::mutex> const lock{g_gap_device_push_mutex};
    for (int ith = 0; ith < nrn_nthread; ++ith) {
        nrn_pragma_acc(wait(nrn_threads[ith].stream_id))
    }
    int ok = 0, miss = 0;
    for (int i = 0; i < n; ++i) {
        double* const p = host_ptrs[i];
        if (!p) {
            continue;
        }
        double* d = device_ptr_for_host_scalar(p, mech_types, n_types);
        if (d) {
            nrn_target_memcpy_to_device(d, p, 1);
            ++ok;
        } else {
            ++miss;
            ++g_gap_scatter_miss;
        }
    }
    // Product path: if any mid-SoA scalar still misses, push only the double
    // SoA columns that contain targets (e.g. HalfGap vgap) — never full mech.
    if (miss > 0) {
        push_target_fields_containing(host_ptrs, n, mech_types, n_types);
    }
    if (std::getenv("NRN_GAP_DEBUG")) {
        static int once = 0;
        if (once < 5) {
            std::fprintf(stderr,
                         "scatter_gap_targets(typed) ok=%d miss=%d n=%d field_fallback=%d\n",
                         ok,
                         miss,
                         n,
                         miss > 0 ? 1 : 0);
            ++once;
        }
    }
    // After field fallback, residual misses are only expected if the model is
    // not on device yet (finitialize before ensure_on_device). During psolve
    // that is a hard error unless opt-in host fallback.
    if (miss > 0 && model_is_on_device() && !allow_gap_host_fallback()) {
        // Re-check: field fallback may have fixed residency for next step.
        // Count miss but do not abort every step — first-step miss is common.
        // Fail only if *all* scalars miss after field push (nothing mapped).
        if (ok == 0) {
            fail_gap_device("scatter_targets",
                           "no gap target scalars mapped on device after vgap-field push");
        }
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
    neuron::model().apply_to_mechanisms(
        [&](auto const& mech_data) { sync_soa_storage_to_device(mech_data); });
#endif
}

}  // namespace neuron::gpu
