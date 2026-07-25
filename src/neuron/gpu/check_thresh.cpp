#include "neuron/gpu/check_thresh.hpp"

#include "neuron/gpu/config.hpp"
#include "neuron/gpu/net_send_buffer.hpp"
#include "neuron/gpu/offload.hpp"
#include "neuron/gpu/phase_timer.hpp"

#include "multicore.h"
#include "nrn_ansi.h"

#include <algorithm>
#include <cstdlib>
#include <vector>

namespace neuron::gpu {
namespace {

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
    nrn_target_delete(table.h_thvar_row.data(), count);
    nrn_target_delete(table.h_threshold.data(), count);
    nrn_target_delete(table.h_flag.data(), count);
    table.device_capacity = 0;
#else
    (void) table;
#endif
}

void upload_device_arrays(ThreadThresholdTable& table) {
#if defined(NRN_ENABLE_GPU) && defined(_OPENACC)
    free_device_arrays(table);
    auto const count = table.h_thvar_row.size();
    if (count == 0) {
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
    if (g_tables_dirty) {
        for (int i = 0; i < nrn_nthread; ++i) {
            rebuild_thread_table(i);
        }
        g_tables_dirty = false;
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
            if (nrn_target_is_present(old)) {
                nrn_target_delete(old, static_cast<std::size_t>(old_size));
            }
            std::free(old);
        }
    }
#if defined(NRN_ENABLE_GPU) && defined(_OPENACC)
    if (!nt._net_send_buffer) {
        return;
    }
    if (!nrn_target_is_present(nt._net_send_buffer)) {
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

/** Host serial detect over Th0 columns (fallback when device data plane is missing). */
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
    g_tables_dirty = true;
    for (auto& table: g_tables) {
        free_device_arrays(table);
        table.slots.clear();
        table.h_thvar_row.clear();
        table.h_threshold.clear();
        table.h_flag.clear();
    }
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

    ensure_thread_net_send_buffer(*nt, count);

    if (table.device_capacity != static_cast<std::size_t>(count)) {
        rebuild_thread_table(nt->id);
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
    int const end = nt->end;

    // Th1: OpenACC detect over slot columns + device vec_v (CoreNEURON pscheck shape).
    // Hit list = slot indices. Host still delivers. Voltage host pull remains until Th2.
    nt->_net_send_buffer_cnt = 0;
    int net_send_buf_count = 0;

#if defined(NRN_ENABLE_GPU) && (defined(_OPENACC) || defined(_OPENMP))
    // clang-format off
    nrn_pragma_acc(parallel loop present(thvar_row [0:count],
                                         threshold [0:count],
                                         flag [0:count],
                                         vec_v [0:end],
                                         nsbuffer [0:nsbuffer_size])
                       copy(net_send_buf_count) if (nt->compute_gpu) async(nt->stream_id))
    nrn_pragma_omp(target teams distribute parallel for map(tofrom: net_send_buf_count) if(nt->compute_gpu))
    // clang-format on
    for (int i = 0; i < count; ++i) {
        int idx = 0;
        int const thidx = thvar_row[i];
        double const v = vec_v[thidx];
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

    if (nt->compute_gpu) {
        // Flags + hit list live on device during the kernel; pull for host deliver/sync.
        // clang-format off
        nrn_pragma_acc(update host(flag [0:count]) async(nt->stream_id))
        nrn_pragma_omp(target update from(flag [0:count]))
        // clang-format on
        if (net_send_buf_count > 0) {
            int const ncopy = std::min(net_send_buf_count, nsbuffer_size);
            // clang-format off
            nrn_pragma_acc(update host(nsbuffer [0:ncopy]) async(nt->stream_id))
            nrn_pragma_omp(target update from(nsbuffer [0:ncopy]))
            // clang-format on
        }
        nrn_pragma_acc(wait(nt->stream_id))
    }
#else
    detect_threshold_hits_host(count,
                               thvar_row,
                               threshold,
                               flag,
                               vec_v,
                               nsbuffer,
                               nsbuffer_size,
                               net_send_buf_count);
    nt->_net_send_buffer_cnt = net_send_buf_count;
#endif

    sync_threshold_presyn_flags(table.slots.data(), flag, count);

    // Host deliver only (CoreNEURON-shaped: detect fills buffer, host does send).
    if (net_send_buf_count > 0) {
        int const n_hits = std::min(nt->_net_send_buffer_cnt, nsbuffer_size);
        for (int i = 0; i < n_hits; ++i) {
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