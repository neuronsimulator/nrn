#pragma once

struct NrnThread;

namespace neuron::gpu {

struct ThresholdPresynSlot {
    int thvar_row = 0;
    double threshold = 0.0;
    int flag = 0;
    void* presyn = nullptr;
};

/** Rebuild presyn threshold tables after topology / psl_thr_ changes. */
void invalidate_threshold_tables() noexcept;

/** Fill slots for one thread; returns count (host-only, implemented in netcvode.cpp). */
int collect_threshold_presyn_slots(NrnThread* nt, ThresholdPresynSlot* slots, int capacity);

/** Deliver one presyn spike after device threshold detection (host-only). */
void deliver_threshold_spike(NrnThread* nt, void* presyn, double teps);

/**
 * Device-resident presyn threshold crossing detection.
 * Returns true when the GPU path ran (including empty threshold lists).
 */
[[nodiscard]] bool check_thresh_presyn_on_device(NrnThread* nt, double teps) noexcept;

}  // namespace neuron::gpu