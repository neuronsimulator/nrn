#include "prcellstate_checkpoint.hpp"

#include "../../nrnconf.h"
#include "multicore.h"
#include "netcon.h"
#include "nrn_ansi.h"

#if defined(NRN_ENABLE_GPU)
#include "neuron/gpu/config.hpp"
#include "neuron/gpu/download.hpp"
#include "neuron/gpu/sync.hpp"
#endif

#include <cmath>
#include <cstdio>

extern void nrn_prcellstate(int gid, const char* suffix);
extern double dt;

namespace {

struct Config {
    int gid = -1;
    long step_trigger = -1;
    double t_trigger = 0.0;
    bool use_time = false;
    bool fired = false;
};

Config g_config;
long g_psolve_step = 0;

const char* phase_name(PrcellCheckpointPhase phase) {
    switch (phase) {
    case PrcellCheckpointPhase::post_setup:
        return "post_setup";
    case PrcellCheckpointPhase::post_solve:
        return "post_solve";
    case PrcellCheckpointPhase::pre_nonvint:
        return "pre_nonvint";
    case PrcellCheckpointPhase::post_nonvint:
        return "post_nonvint";
    }
    return "unknown";
}

const char* backend_tag() {
#if defined(NRN_ENABLE_GPU)
    if (neuron::gpu::enabled() && neuron::gpu::backend_native()) {
        return "gpu";
    }
#endif
    return "cpu";
}

bool thread_owns_gid(NrnThread& nt) {
    if (g_config.gid < 0) {
        return false;
    }
    PreSyn* ps = nrn_gid2outputpresyn(g_config.gid);
    return ps && ps->nt_ == &nt;
}

bool step_matches() {
    return !g_config.use_time && g_psolve_step == g_config.step_trigger;
}

bool time_matches(NrnThread& nt) {
    if (!g_config.use_time) {
        return false;
    }
    double const tol = 0.51 * nt._dt;
    return std::fabs(nt._t - g_config.t_trigger) <= tol;
}

bool trigger_matches(NrnThread& nt) {
    if (g_config.gid < 0 || g_config.fired) {
        return false;
    }
    return step_matches() || time_matches(nt);
}

void build_suffix(char* buf, std::size_t n, PrcellCheckpointPhase phase) {
    if (g_config.use_time) {
        std::snprintf(buf,
                      n,
                      "%s_t%.9g_%s",
                      backend_tag(),
                      g_config.t_trigger,
                      phase_name(phase));
    } else {
        std::snprintf(buf,
                      n,
                      "%s_s%ld_%s",
                      backend_tag(),
                      g_config.step_trigger,
                      phase_name(phase));
    }
}

void dump_phase(PrcellCheckpointPhase phase) {
#if defined(NRN_ENABLE_GPU)
    if (neuron::gpu::enabled() && neuron::gpu::backend_native()) {
        neuron::gpu::sync_all_device_streams();
        neuron::gpu::sync_node_soa_to_host_for_host_reads();
    }
#endif
    char suffix[200];
    build_suffix(suffix, sizeof(suffix), phase);
    nrn_prcellstate(g_config.gid, suffix);
    if (phase == PrcellCheckpointPhase::post_nonvint) {
        g_config.fired = true;
    }
}

}  // namespace

void nrn_prcellstate_checkpoint_arm(int gid, long step, double t) {
    g_config.gid = gid;
    g_config.fired = false;
    if (step < 0) {
        g_config.use_time = true;
        g_config.step_trigger = -1;
        g_config.t_trigger = t;
    } else {
        g_config.use_time = false;
        g_config.step_trigger = step;
        g_config.t_trigger = t;
    }
}

void nrn_prcellstate_checkpoint_clear() noexcept {
    g_config = {};
    g_psolve_step = 0;
}

void nrn_prcellstate_checkpoint_psolve_begin() noexcept {
    g_psolve_step = 0;
    g_config.fired = false;
}

void nrn_prcellstate_checkpoint_fixed_step_end() noexcept {
    ++g_psolve_step;
}

void nrn_prcellstate_checkpoint_maybe(PrcellCheckpointPhase phase, NrnThread& nt) {
    if (!thread_owns_gid(nt) || !trigger_matches(nt)) {
        return;
    }
    dump_phase(phase);
}
