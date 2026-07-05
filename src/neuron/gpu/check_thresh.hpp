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

/** Drop OpenACC mirrors owned outside UploadState (threshold + net_send buffers). */
void invalidate_auxiliary_device_uploads() noexcept;

/** Fill slots for one thread; returns count (host-only, implemented in netcvode.cpp). */
int collect_threshold_presyn_slots(NrnThread* nt, ThresholdPresynSlot* slots, int capacity);

/** Deliver one presyn spike after device threshold detection (host-only). */
void deliver_threshold_spike(NrnThread* nt, void* presyn, double teps);

/** Mirror device hysteresis flags back to host PreSyn::flag_. */
void sync_threshold_presyn_flags(ThresholdPresynSlot const* slots, int const* flags, int count);

/**
 * Device-resident presyn threshold crossing detection.
 * Returns true when the GPU path ran (including empty threshold lists).
 */
[[nodiscard]] bool check_thresh_presyn_on_device(NrnThread* nt, double teps) noexcept;

/** Gate E: every threshold PreSyn on the thread uses modern SoA voltage handles. */
[[nodiscard]] bool threshold_detection_on_device(NrnThread const& nt) noexcept;

}  // namespace neuron::gpu