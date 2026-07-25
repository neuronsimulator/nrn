#pragma once

/**
 * GPU-native PreSyn threshold detection (CoreNEURON-shaped).
 *
 * Th0 contract — see doc/gpu/threshold-detection.md:
 *   - Detect set = compact ThresholdPresynSlot table only (no InputPreSyn type).
 *   - NrnThread::_net_send_buffer holds slot indices, not PreSyn*.
 *   - Spike deliver is host-only (PreSyn::send).
 * Th1: OpenACC detect over the slot table (device vec_v + atomic hit buffer).
 * Th2 (pending): skip voltage host pull when Th1 succeeds.
 */

struct NrnThread;

namespace neuron::gpu {

/**
 * One voltage-threshold source on a thread (CoreNEURON "output PreSyn" role).
 *
 * Built from psl_thr_ PreSyns with modern SoA thvar_. Index in this table is
 * what appears in NrnThread::_net_send_buffer after a crossing.
 */
struct ThresholdPresynSlot {
    /** Node voltage SoA row relative to this thread (0 .. nt->end). */
    int thvar_row = 0;
    double threshold = 0.0;
    /** Hysteresis for pscheck: 0 = below, 1 = above. */
    int flag = 0;
    /** Host-only; used after detect for deliver_threshold_spike. Not for kernels. */
    void* presyn = nullptr;
};

/** Rebuild presyn threshold tables after topology / psl_thr_ / node sort changes. */
void invalidate_threshold_tables() noexcept;

/** Drop OpenACC mirrors owned outside UploadState (threshold + net_send buffers). */
void invalidate_auxiliary_device_uploads() noexcept;

/**
 * Fill the Th0 detect set for one thread from psl_thr_.
 * Returns count. If slots is null, only counts. Host-only (netcvode.cpp).
 */
int collect_threshold_presyn_slots(NrnThread* nt, ThresholdPresynSlot* slots, int capacity);

/** Host deliver one PreSyn spike after threshold hit list is ready. */
void deliver_threshold_spike(NrnThread* nt, void* presyn, double teps);

/** Mirror detect-path hysteresis flags back to host PreSyn::flag_. */
void sync_threshold_presyn_flags(ThresholdPresynSlot const* slots, int const* flags, int count);

/**
 * Threshold detection entry for gpu-native.
 * Th0 shape: slot table sole detect set; hit buffer = slot indices; host deliver.
 * Th1: OpenACC pscheck over slot columns + device vec_v; host pull of flags/hits
 * then deliver. Voltage host sync before this call remains until Th2.
 * Returns true when the table path handled SoA-threshold PreSyns (caller skips
 * re-walking those on psl_thr_).
 */
[[nodiscard]] bool check_thresh_presyn_on_device(NrnThread* nt, double teps) noexcept;

/**
 * Gate E: every threshold PreSyn on the thread uses modern SoA voltage handles
 * (slot table can be built for device detect).
 */
[[nodiscard]] bool threshold_detection_on_device(NrnThread const& nt) noexcept;

}  // namespace neuron::gpu
