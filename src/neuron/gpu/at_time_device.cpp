// Device-side at_time for native GPU mechs (IClamp BREAKPOINT on OpenACC).
// Host CVode-aware at_time remains in src/nrncvode/cvodestb.cpp.
// OpenACC bind: device calls to at_time resolve to this fixed-step body.

#include "neuron/gpu/offload.hpp"
#include "multicore.h"

// Fixed-step only (same math as the non-CVode branch of host at_time).
// nohost: do not emit a second host symbol that would collide with cvodestb.
nrn_pragma_acc(routine seq nohost)
bool at_time_device_fixed(NrnThread* nt, double te) {
    double x = te - 1e-11;
    return (x <= nt->_t && x > (nt->_t - nt->_dt));
}

// When device code calls at_time(...), bind to the fixed-step device routine.
nrn_pragma_acc(routine(at_time) seq bind(at_time_device_fixed))
