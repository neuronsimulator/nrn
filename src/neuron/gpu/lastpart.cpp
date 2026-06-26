#include "neuron/gpu/lastpart.hpp"

#include "neuron/gpu/config.hpp"
#include "neuron/gpu/phase_timer.hpp"

#include "coreneuron/utils/offload.hpp"
#include "multicore.h"
#include "nonvintblock.h"
#include "nrn_ansi.h"

extern int use_sparse13;

namespace neuron::gpu {

bool nonvint_can_run_on_device(NrnThread const& nt) noexcept {
#if defined(NRN_ENABLE_GPU)
    if (!enabled() || !backend_native() || !nt.compute_gpu || nt.end <= 0) {
        return false;
    }
    if (nrn_nonvint_block || ::use_sparse13) {
        return false;
    }
    if (nt._ecell_memb_list) {
        return false;
    }
    return true;
#else
    (void) nt;
    return false;
#endif
}

void prepare_nonvint_on_device(NrnThread& nt) {
#if defined(NRN_ENABLE_GPU)
    if (!nonvint_can_run_on_device(nt)) {
        return;
    }
    phase_timer::bump(phase_timer::Id::vecplay_sync);
    auto* const vec_v = nt.node_voltage_storage();
    nrn_pragma_acc(update device(vec_v [0:nt.end]) if (nt.compute_gpu) async(nt.stream_id))
    nrn_pragma_omp(target update to(vec_v [0:nt.end]) if (nt.compute_gpu))
    nrn_pragma_acc(update device(nt._t) if (nt.compute_gpu) async(nt.stream_id))
    nrn_pragma_omp(target update to(nt._t) if (nt.compute_gpu))
    nrn_pragma_acc(wait(nt.stream_id))
#else
    (void) nt;
#endif
}

void finalize_nonvint_on_device(NrnThread& nt) {
#if defined(NRN_ENABLE_GPU)
    if (!nonvint_can_run_on_device(nt)) {
        return;
    }
    nrn_pragma_acc(wait(nt.stream_id))
#else
    (void) nt;
#endif
}

}  // namespace neuron::gpu