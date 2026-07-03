#pragma once

struct NrnThread;

namespace neuron::gpu {

/** True when host vec_v may be newer than device (host post-solve or fixed VecPlay). */
[[nodiscard]] bool host_voltage_is_authoritative(NrnThread const& nt) noexcept;

/** Pull node voltages to host before host-side VecPlay / stimulus updates. */
void sync_before_vecplay(NrnThread& nt);

/** Push node voltages back to device after VecPlay updates. */
void sync_after_vecplay(NrnThread& nt);

/** Push node voltages to device after host lastpart (nonvint) for the next GPU step. */
void sync_voltages_to_device_after_lastpart(NrnThread& nt);

/** Push node voltages to device immediately before OpenACC axial matrix assembly. */
void sync_voltages_to_device_before_axial(NrnThread& nt);

/** True when every mechanism CURRENT/JACOBIAN (incl. capacitance) is OpenACC on device. */
[[nodiscard]] bool matrix_currents_on_device(NrnThread const& nt) noexcept;

/** Zero vec_rhs / sav_rhs on device at the start of nrn_rhs (device-current path). */
void zero_matrix_rhs_on_device(NrnThread& nt, int begin, int end) noexcept;

/** Zero vec_d / sav_d on device at the start of nrn_lhs (device-current path). */
void zero_matrix_diagonal_on_device(NrnThread& nt, int begin, int end) noexcept;

/** sav_rhs -= vec_rhs on device after GPU mechanism CURRENT. */
void transform_sav_rhs_membrane_only_on_device(NrnThread& nt, int begin, int end) noexcept;

/** Push mechanism-updated matrix state to device before OpenACC axial loops. */
void sync_matrix_to_device_after_mechanisms(NrnThread& nt);

/** Push vec_d / sav_d only (preserve device vec_rhs after rhs axial). */
void sync_diagonal_to_device_after_mechanisms(NrnThread& nt);

/** Single vec_d push after host jacob/cap when currents stayed on device. */
void sync_diagonal_to_device_before_axial_lhs(NrnThread& nt) noexcept;

/** True when rhs/d can remain on device through nonvint and into the GPU solver.
 *  False when sparse13, Python nonvint blocks, or extracellular (_ecell_memb_list) are active. */
[[nodiscard]] bool matrix_rhs_d_stays_on_device_for_solve(NrnThread const& nt) noexcept;

/** Gate A for qualification (structural blockers only; ignores transient nt.compute_gpu). */
[[nodiscard]] bool matrix_rhs_d_qualifies_for_gpu_native(NrnThread const& nt) noexcept;

/** Gate B for qualification (mechanism registration; ignores transient nt.compute_gpu). */
[[nodiscard]] bool matrix_currents_qualify_for_gpu_native(NrnThread const& nt) noexcept;

/** Pull GPU axial matrix state to host before host nonvint adjustments. */
void sync_matrix_to_host_before_solve(NrnThread& nt);

/** Push post-nonvint matrix state to device before the GPU Hines solver. */
void sync_matrix_to_device_before_solve(NrnThread& nt);

/** Pull vec_rhs solution to host after the GPU solver (host post-solve fallback only). */
void sync_rhs_to_host_after_solve(NrnThread& nt);

/** Pull post-solve node voltages to host for HOC reads and VecPlay. */
void sync_voltages_to_host_after_post_solve(NrnThread& nt);

/** Pull device voltages before threshold detection (host check_thresh reads host SoA). */
void sync_voltages_to_host_before_check_thresh(NrnThread& nt);

/** Pull device node voltages to host before host-side nonvint STATE integration. */
void sync_voltages_to_host_before_nonvint(NrnThread& nt);

/** Pull solved vec_rhs to host before host second_order_cur ahead of nonvint. */
void sync_rhs_to_host_before_nonvint(NrnThread& nt);

/** Pull fast_imem sav_rhs to host after GPU fast_imem (smaller than vec_rhs sync). */
void sync_fast_imem_to_host_after_post_solve(NrnThread& nt);

/** Pull device post-solve voltages to host before gap gather (device post-solve path). */
void sync_gap_after_voltage_update(NrnThread& nt);

/** Push host post-solve voltages to device before gap gather (host fallback path). */
void sync_gap_after_host_voltage_update(NrnThread& nt);

/** Block until all NrnThread OpenACC streams have completed. */
void sync_all_device_streams();

}  // namespace neuron::gpu
