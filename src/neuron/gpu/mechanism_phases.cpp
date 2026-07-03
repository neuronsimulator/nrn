#include "neuron/gpu/mechanism_phases.hpp"

#include "neuron/gpu/config.hpp"
#include "neuron/gpu/lastpart.hpp"
#include "neuron/gpu/post_solve.hpp"
#include "neuron/gpu/sync.hpp"

#include "membfunc.h"
#include "multicore.h"
#include "nonvintblock.h"
#include "nrncvode.h"
#include "hocdec.h"

#include <algorithm>
#include <fmt/format.h>
#include <sstream>
#include <vector>

extern int use_sparse13;

namespace neuron::gpu {
namespace {

std::vector<MechanismGpuPhase>& phase_table() {
    static std::vector<MechanismGpuPhase> table;
    return table;
}

void ensure_phase_table_size(int type) {
    auto& table = phase_table();
    if (type >= 0 && static_cast<std::size_t>(type) >= table.size()) {
        table.resize(static_cast<std::size_t>(type) + 1, MechanismGpuPhase::None);
    }
}

[[nodiscard]] char const* mech_name(int type) noexcept {
    if (type < 0 || type >= n_memb_func) {
        return "?";
    }
    Symbol const* const sym = memb_func[type].sym;
    if (sym == nullptr || sym->name == nullptr) {
        return "?";
    }
    return sym->name;
}

void append_blocking_mechs(std::string& out,
                           char const* label,
                           bool (*predicate)(int) noexcept,
                           bool (*hook_present)(int) noexcept) {
    std::vector<std::string> names;
    for (int ith = 0; ith < nrn_nthread; ++ith) {
        NrnThread const& nt = nrn_threads[ith];
        if (nt.end <= 0) {
            continue;
        }
        for (auto* tml = nt.tml; tml; tml = tml->next) {
            if (!hook_present(tml->index) || predicate(tml->index)) {
                continue;
            }
            names.emplace_back(mech_name(tml->index));
        }
    }
    if (names.empty()) {
        out += fmt::format("{}: full device\n", label);
        return;
    }
    std::sort(names.begin(), names.end());
    names.erase(std::unique(names.begin(), names.end()), names.end());
    std::ostringstream joined;
    for (std::size_t i = 0; i < names.size(); ++i) {
        if (i > 0) {
            joined << ", ";
        }
        joined << names[i];
    }
    out += fmt::format("{}: host ({})\n", label, joined.str());
}

[[nodiscard]] bool has_current_hook(int type) noexcept {
    return type >= 0 && type < n_memb_func && memb_func[type].current != nullptr;
}

[[nodiscard]] bool has_jacobian_hook(int type) noexcept {
    return type >= 0 && type < n_memb_func && memb_func[type].jacob != nullptr;
}

[[nodiscard]] bool has_state_hook(int type) noexcept {
    return type >= 0 && type < n_memb_func && memb_func[type].state != nullptr;
}

[[nodiscard]] bool solve_phase_on_device(NrnThread const& nt) noexcept {
#if defined(NRN_ENABLE_GPU)
    if (!nonvint_state_on_device(nt)) {
        return false;
    }
    for (auto* tml = nt.tml; tml; tml = tml->next) {
        if (has_state_hook(tml->index) && !mechanism_solve_on_device(tml->index)) {
            return false;
        }
    }
    return true;
#else
    (void) nt;
    return false;
#endif
}

}  // namespace

void register_mechanism_gpu_phases(int type, MechanismGpuPhase phases) noexcept {
#if defined(NRN_ENABLE_GPU)
    if (type < 0) {
        return;
    }
    ensure_phase_table_size(type);
    phase_table()[static_cast<std::size_t>(type)] |= phases;
#else
    (void) type;
    (void) phases;
#endif
}

bool mechanism_current_on_device(int type) noexcept {
#if defined(NRN_ENABLE_GPU)
    if (type < 0 || static_cast<std::size_t>(type) >= phase_table().size()) {
        return false;
    }
    return has_gpu_phase(phase_table()[static_cast<std::size_t>(type)], MechanismGpuPhase::Current);
#else
    (void) type;
    return false;
#endif
}

bool mechanism_jacobian_on_device(int type) noexcept {
#if defined(NRN_ENABLE_GPU)
    if (type < 0 || static_cast<std::size_t>(type) >= phase_table().size()) {
        return false;
    }
    return has_gpu_phase(phase_table()[static_cast<std::size_t>(type)], MechanismGpuPhase::Jacobian);
#else
    (void) type;
    return false;
#endif
}

bool mechanism_solve_on_device(int type) noexcept {
#if defined(NRN_ENABLE_GPU)
    if (type < 0 || static_cast<std::size_t>(type) >= phase_table().size()) {
        return false;
    }
    return has_gpu_phase(phase_table()[static_cast<std::size_t>(type)], MechanismGpuPhase::Solve);
#else
    (void) type;
    return false;
#endif
}

std::string native_gpu_fixed_step_phase_report() {
#if defined(NRN_ENABLE_GPU)
    std::string out;
    out += fmt::format("gpu_enable={} backend={} devices={}\n",
                       enabled(),
                       backend_native() ? "native" : "coreneuron",
                       device_count());

    if (!enabled() || !backend_native()) {
        out += "fixed-step GPU phases: inactive (enable native backend)\n";
        return out;
    }

    if (nrn_nthread <= 0) {
        out += "fixed-step GPU phases: no threads (model not initialized?)\n";
        return out;
    }

    NrnThread const& nt0 = nrn_threads[0];
    out += fmt::format("matrix_rhs/d stay on device: {}\n",
                       matrix_rhs_d_stays_on_device_for_solve(nt0) ? "yes" : "no");
    if (nrn_nonvint_block) {
        out += "  blocked by: Python nonvint block\n";
    }
    if (::use_sparse13) {
        out += "  blocked by: sparse13\n";
    }
    if (nt0._ecell_memb_list) {
        out += "  blocked by: extracellular\n";
    }

    out += fmt::format("CURRENT+JACOBIAN fast path (skip matrix sync): {}\n",
                       matrix_currents_on_device(nt0) ? "yes" : "no");
    append_blocking_mechs(out, "  CURRENT", mechanism_current_on_device, has_current_hook);
    append_blocking_mechs(out, "  JACOBIAN", mechanism_jacobian_on_device, has_jacobian_hook);
    if (nt0.tml && nt0.tml->index == CAP && !mechanism_jacobian_on_device(CAP)) {
        out += "  JACOBIAN: host (capacitance)\n";
    }

    out += fmt::format("SOLVE/nonvint on device: {}\n",
                       solve_phase_on_device(nt0) ? "yes" : "no");
    if (!nonvint_state_on_device(nt0)) {
        out += "  (set NRN_NATIVE_GPU_DEVICE_NONVINT=1 and ensure all STATE mods register Solve)\n";
    }
    append_blocking_mechs(out, "  SOLVE", mechanism_solve_on_device, has_state_hook);

    out += fmt::format("post_solve on device: {}\n",
                       !post_solve_needs_host_fallback(nt0) ? "yes" : "no");
    out += fmt::format("lastpart host fallback: {}\n",
                       lastpart_host_phases_required(nt0) ? "yes" : "no");
    out += fmt::format("host_voltage_authoritative: {}\n",
                       host_voltage_is_authoritative(nt0) ? "yes" : "no");
    return out;
#else
    return "NRN_ENABLE_GPU is off at build time\n";
#endif
}

namespace detail {

void reset_mechanism_gpu_phases_for_testing() noexcept {
    phase_table().clear();
}

}  // namespace detail

}  // namespace neuron::gpu