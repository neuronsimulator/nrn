#pragma once

#include <vector>

struct NrnThread;
class PreSyn;

namespace neuron::gpu {

struct ThresholdPresynEntry {
    int thvar_row = 0;
    double threshold = 0.0;
    PreSyn* presyn = nullptr;
    int flag = 0;
};

/** Fill presyn threshold metadata for one thread (host walk of psl_thr_). */
void collect_threshold_presyns(NrnThread* nt, std::vector<ThresholdPresynEntry>& out);

/** Rebuild presyn threshold tables after topology / presyn list changes. */
void invalidate_threshold_tables() noexcept;

/** Device-resident threshold crossing detection; returns presyn table indices that fired. */
void check_thresh_detect_on_device(NrnThread* nt, std::vector<int>& fired_indices);

/** Cached presyn metadata for thread tid (valid until invalidate_threshold_tables). */
std::vector<ThresholdPresynEntry> const& threshold_entries_for_thread(int tid);

}  // namespace neuron::gpu