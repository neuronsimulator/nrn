#include "neuron/gpu/check_thresh.hpp"

#include "neuron/gpu/config.hpp"
#include "neuron/gpu/offload.hpp"
#include "neuron/gpu/phase_timer.hpp"

#include "multicore.h"
#include "nrn_ansi.h"

#include <algorithm>
#include <cstdlib>
#include <vector>

namespace neuron::gpu {
namespace {

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
    int const needed = std::max(min_count, 8);
    if (!nt._net_send_buffer || nt._net_send_buffer_size < needed) {
        if (nt._net_send_buffer) {
            std::free(nt._net_send_buffer);
        }
        nt._net_send_buffer_size = needed;
        nt._net_send_buffer = static_cast<int*>(std::calloc(nt._net_send_buffer_size, sizeof(int)));
    }
#if defined(NRN_ENABLE_GPU) && defined(_OPENACC)
    if (!nt._net_send_buffer) {
        return;
    }
    if (!nrn_target_is_present(nt._net_send_buffer)) {
        (void) nrn_target_copyin(nt._net_send_buffer,
                                 static_cast<std::size_t>(nt._net_send_buffer_size));
    }
    if (auto* const d_nt = static_cast<NrnThread*>(nrn_target_deviceptr(&nt))) {
        int* const d_buf = static_cast<int*>(nrn_target_deviceptr(nt._net_send_buffer));
        nrn_target_memcpy_to_device(&(d_nt->_net_send_buffer), &d_buf, 1);
    }
#else
    (void) nt;
#endif
}

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

bool check_thresh_presyn_on_device(NrnThread* nt, double teps) noexcept {
#if defined(NRN_ENABLE_GPU)
    if (!enabled() || !backend_native() || !nt || !nt->compute_gpu || nt->end <= 0) {
        return false;
    }
    phase_timer::Scope const timer{phase_timer::Id::deliver_events};
    ensure_thread_table(nt->id);
    auto& table = g_tables[nt->id];
    int const count = static_cast<int>(table.slots.size());
    if (count == 0) {
        return true;
    }

    ensure_thread_net_send_buffer(*nt, count);

    if (table.device_capacity != static_cast<std::size_t>(count)) {
        return false;
    }

    int* const thvar_row = table.h_thvar_row.data();
    double* const threshold = table.h_threshold.data();
    int* const flag = table.h_flag.data();

    nt->_net_send_buffer_cnt = 0;
    int net_send_buf_count = 0;
    auto* const vec_v = nt->node_voltage_storage();
    int* const nsbuffer = nt->_net_send_buffer;
    int const nsbuffer_size = nt->_net_send_buffer_size;

    nrn_pragma_acc(parallel loop present(nt [0:1], thvar_row [0:count], threshold [0:count],
                                         flag [0:count], nsbuffer [0:nsbuffer_size],
                                         vec_v [0:nt->end])
                       copy(net_send_buf_count) if (nt->compute_gpu) async(nt->stream_id))
    nrn_pragma_omp(target teams distribute parallel for map(tofrom: net_send_buf_count)
                       if(nt->compute_gpu))
    for (int i = 0; i < count; ++i) {
        int const thidx = thvar_row[i];
        double const v = vec_v[thidx];
        double const thresh = threshold[i];
        int idx = 0;
        if (pscheck(v, thresh, &flag[i])) {
            nrn_pragma_acc(atomic capture)
            nrn_pragma_omp(atomic capture)
            idx = net_send_buf_count++;
            if (idx < nsbuffer_size) {
                nsbuffer[idx] = i;
            }
        }
    }
    nrn_pragma_acc(wait(nt->stream_id))
    nt->_net_send_buffer_cnt = net_send_buf_count;

    if (net_send_buf_count > 0) {
        nrn_pragma_acc(update host(nsbuffer [0:net_send_buf_count]) async(nt->stream_id))
        nrn_pragma_omp(target update from(nsbuffer [0:net_send_buf_count]) if (nt->compute_gpu))
        nrn_pragma_acc(wait(nt->stream_id))
        for (int i = 0; i < nt->_net_send_buffer_cnt; ++i) {
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