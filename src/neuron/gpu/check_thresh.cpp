#include "neuron/gpu/check_thresh.hpp"

#include "neuron/gpu/config.hpp"
#include "neuron/gpu/offload.hpp"

#include "multicore.h"
#include "nrn_ansi.h"

#include <cstdlib>
#include <vector>

namespace neuron::gpu {
namespace {

struct ThreadThresholdTable {
    std::vector<ThresholdPresynEntry> entries;
};

std::vector<ThreadThresholdTable> g_tables;
bool g_tables_dirty = true;

void ensure_tables_sized() {
    if (g_tables.size() < static_cast<std::size_t>(nrn_nthread)) {
        g_tables.resize(nrn_nthread);
    }
}

void rebuild_thread_table(int tid) {
    ensure_tables_sized();
    auto& table = g_tables[tid];
    table.entries.clear();
    collect_threshold_presyns(nrn_threads + tid, table.entries);
}

void ensure_thread_table(int tid) {
    if (g_tables_dirty) {
        for (int i = 0; i < nrn_nthread; ++i) {
            rebuild_thread_table(i);
        }
        g_tables_dirty = false;
    }
}

}  // namespace

void invalidate_threshold_tables() noexcept {
    g_tables_dirty = true;
}

void check_thresh_detect_on_device(NrnThread* nt, std::vector<int>& fired_indices) {
    fired_indices.clear();
#if defined(NRN_ENABLE_GPU)
    if (!enabled() || !backend_native() || !nt->compute_gpu) {
        return;
    }
    ensure_thread_table(nt->id);
    auto& entries = g_tables[nt->id].entries;
    int const count = static_cast<int>(entries.size());
    if (count == 0) {
        return;
    }

    int* thvar_row = static_cast<int*>(std::calloc(count, sizeof(int)));
    double* threshold = static_cast<double*>(std::calloc(count, sizeof(double)));
    int* flag = static_cast<int*>(std::calloc(count, sizeof(int)));
    for (int i = 0; i < count; ++i) {
        thvar_row[i] = entries[i].thvar_row;
        threshold[i] = entries[i].threshold;
        flag[i] = entries[i].flag;
    }

    if (!nt->_net_send_buffer || nt->_net_send_buffer_size < count) {
        nt->_net_send_buffer_size = std::max(count, 8);
        if (nt->_net_send_buffer) {
            std::free(nt->_net_send_buffer);
        }
        nt->_net_send_buffer = static_cast<int*>(
            std::calloc(nt->_net_send_buffer_size, sizeof(int)));
    }

    int net_send_buf_count = 0;
    auto* const vec_v = nt->node_voltage_storage();
    int* const nsbuffer = nt->_net_send_buffer;
    int const nsbuffer_size = nt->_net_send_buffer_size;

    nrn_pragma_acc(parallel loop copyin(thvar_row [0:count], threshold [0:count], flag [0:count],
                                         nsbuffer [0:nsbuffer_size])
                       present(nt [0:1], vec_v [0:nt->end])
                       copyout(nsbuffer [0:nsbuffer_size], flag [0:count])
                       copy(net_send_buf_count) if (nt->compute_gpu) async(nt->stream_id))
    nrn_pragma_omp(target teams distribute parallel for map(tofrom: net_send_buf_count)
                       if(nt->compute_gpu))
    for (int i = 0; i < count; ++i) {
        int const thidx = thvar_row[i];
        double const v = vec_v[thidx];
        double const thresh = threshold[i];
        int idx = 0;
        if (v > thresh) {
            if (flag[i] == 0) {
                flag[i] = 1;
                nrn_pragma_acc(atomic capture)
                nrn_pragma_omp(atomic capture)
                idx = net_send_buf_count++;
                if (idx < nsbuffer_size) {
                    nsbuffer[idx] = i;
                }
            }
        } else {
            flag[i] = 0;
        }
    }
    nrn_pragma_acc(wait(nt->stream_id))

    std::free(thvar_row);
    std::free(threshold);
    std::free(flag);

    for (int i = 0; i < count; ++i) {
        entries[i].flag = flag[i];
    }

    fired_indices.reserve(net_send_buf_count);
    for (int i = 0; i < net_send_buf_count; ++i) {
        fired_indices.push_back(nt->_net_send_buffer[i]);
    }
    nt->_net_send_buffer_cnt = net_send_buf_count;
#else
    (void) nt;
#endif
}

std::vector<ThresholdPresynEntry> const& threshold_entries_for_thread(int tid) {
    ensure_thread_table(tid);
    return g_tables[tid].entries;
}

}  // namespace neuron::gpu