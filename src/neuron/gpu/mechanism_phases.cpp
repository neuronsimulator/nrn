#include "neuron/gpu/mechanism_phases.hpp"

#include "neuron/gpu/check_thresh.hpp"
#include "neuron/gpu/config.hpp"
#include "neuron/gpu/lastpart.hpp"
#include "neuron/gpu/post_solve.hpp"
#include "neuron/gpu/sync.hpp"

#include "membfunc.h"
#include "multicore.h"
#include "nonvintblock.h"
#include "nrn_ansi.h"
#include "nrncvode.h"
#include "hocdec.h"

#include <algorithm>
#include <cstdlib>
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
            if (nrn_is_ion(tml->index) || !hook_present(tml->index) || predicate(tml->index)) {
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

[[nodiscard]] bool solve_phase_qualifies_for_gpu_native(NrnThread const& nt) noexcept {
#if defined(NRN_ENABLE_GPU)
    if (!nonvint_qualifies_for_gpu_native(nt)) {
        return false;
    }
    for (auto* tml = nt.tml; tml; tml = tml->next) {
        if (nrn_is_ion(tml->index)) {
            continue;
        }
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

struct GateStatus {
    bool matrix_on_device = false;
    bool currents_on_device = false;
    bool solve_on_device = false;
    bool post_solve_on_device = false;
    bool threshold_on_device = false;
};

[[nodiscard]] GateStatus gate_status(NrnThread const& nt) noexcept {
    GateStatus gates;
#if defined(NRN_ENABLE_GPU)
    gates.matrix_on_device = matrix_rhs_d_qualifies_for_gpu_native(nt);
    gates.currents_on_device = matrix_currents_qualify_for_gpu_native(nt);
    gates.solve_on_device = solve_phase_qualifies_for_gpu_native(nt);
    gates.post_solve_on_device = !post_solve_needs_host_fallback(nt);
    gates.threshold_on_device = threshold_detection_on_device(nt);
#else
    (void) nt;
#endif
    return gates;
}

[[nodiscard]] NrnThread const* first_active_thread() noexcept {
    if (nrn_nthread <= 0) {
        return nullptr;
    }
    for (int ith = 0; ith < nrn_nthread; ++ith) {
        if (nrn_threads[ith].end > 0) {
            return nrn_threads + ith;
        }
    }
    return nullptr;
}

[[nodiscard]] bool thread_qualifies_for_full_gpu_native(NrnThread const& nt) noexcept {
#if defined(NRN_ENABLE_GPU)
    if (nt.end <= 0) {
        return true;
    }
    auto const gates = gate_status(nt);
    return gates.matrix_on_device && gates.currents_on_device && gates.solve_on_device &&
           gates.post_solve_on_device && gates.threshold_on_device;
#else
    (void) nt;
    return false;
#endif
}

[[nodiscard]] bool allow_unqualified_gpu_native() noexcept {
    char const* const env = std::getenv("NRN_GPU_ALLOW_UNQUALIFIED");
    return env != nullptr && env[0] != '\0' && env[0] != '0';
}

void append_blocking_gates(std::string& out, GateStatus const& gates) {
    std::vector<char const*> blocked;
    if (!gates.matrix_on_device) {
        blocked.emplace_back("A (matrix on device)");
    }
    if (!gates.currents_on_device) {
        blocked.emplace_back("B (CURRENT+JACOBIAN on device)");
    }
    if (!gates.solve_on_device) {
        blocked.emplace_back("C (SOLVE/nonvint on device)");
    }
    if (!gates.post_solve_on_device) {
        blocked.emplace_back("D (post_solve on device)");
    }
    if (!gates.threshold_on_device) {
        blocked.emplace_back("E (threshold on device)");
    }
    if (blocked.empty()) {
        out += "blocking_gates: none\n";
        return;
    }
    std::ostringstream joined;
    for (std::size_t i = 0; i < blocked.size(); ++i) {
        if (i > 0) {
            joined << ", ";
        }
        joined << blocked[i];
    }
    out += fmt::format("blocking_gates: {}\n", joined.str());
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

bool model_qualifies_for_full_gpu_native() noexcept {
#if defined(NRN_ENABLE_GPU)
    if (!enabled() || !backend_native() || nrn_nthread <= 0) {
        return false;
    }
    bool any_active = false;
    for (int ith = 0; ith < nrn_nthread; ++ith) {
        NrnThread const& nt = nrn_threads[ith];
        if (nt.end <= 0) {
            continue;
        }
        any_active = true;
        if (!thread_qualifies_for_full_gpu_native(nt)) {
            return false;
        }
    }
    return any_active;
#else
    return false;
#endif
}

std::string native_gpu_qualification_report() {
#if defined(NRN_ENABLE_GPU)
    std::string out;
    out += fmt::format("gpu_enable={} backend={} devices={}\n",
                       enabled(),
                       backend_native() ? "native" : "coreneuron",
                       device_count());

    if (!enabled() || !backend_native()) {
        out += "QUALIFIED: no\n";
        out += "reason: native GPU backend not active (pc.gpu_enable(1) and pc.gpu_backend(\"native\"))\n";
        return out;
    }

    if (nrn_nthread <= 0) {
        out += "QUALIFIED: no\n";
        out += "reason: no threads (model not initialized?)\n";
        return out;
    }

    bool const qualified = model_qualifies_for_full_gpu_native();
    out += fmt::format("QUALIFIED: {}\n", qualified ? "yes" : "no");

    NrnThread const* const nt_detail = first_active_thread();
    if (!nt_detail) {
        out += "reason: no active threads with nodes\n";
        return out;
    }
    auto const gates = gate_status(*nt_detail);
    if (!qualified) {
        append_blocking_gates(out, gates);
    }

    out += fmt::format("\n--- gate detail (thread {}) ---\n", nt_detail->id);
    out += fmt::format("Gate A matrix_rhs/d stay on device: {}\n",
                       gates.matrix_on_device ? "yes" : "no");
    if (nrn_nonvint_block) {
        out += "  blocked by: Python nonvint block\n";
    }
    if (::use_sparse13) {
        out += "  blocked by: sparse13\n";
    }
    if (nt_detail->_ecell_memb_list) {
        out += "  blocked by: extracellular\n";
    }

    out += fmt::format("Gate B CURRENT+JACOBIAN on device: {}\n",
                       gates.currents_on_device ? "yes" : "no");
    append_blocking_mechs(out, "  CURRENT", mechanism_current_on_device, has_current_hook);
    append_blocking_mechs(out, "  JACOBIAN", mechanism_jacobian_on_device, has_jacobian_hook);
    if (nt_detail->tml && nt_detail->tml->index == CAP && !mechanism_jacobian_on_device(CAP)) {
        out += "  JACOBIAN: host (capacitance)\n";
    }

    out += fmt::format("Gate C SOLVE/nonvint on device: {}\n",
                       gates.solve_on_device ? "yes" : "no");
    if (!nonvint_qualifies_for_gpu_native(*nt_detail)) {
        out += "  (set NRN_NATIVE_GPU_DEVICE_NONVINT=1 and ensure all STATE mods register Solve)\n";
    }
    append_blocking_mechs(out, "  SOLVE", mechanism_solve_on_device, has_state_hook);

    out += fmt::format("Gate D post_solve on device: {}\n",
                       gates.post_solve_on_device ? "yes" : "no");
    out += fmt::format("Gate E threshold on device: {}\n",
                       gates.threshold_on_device ? "yes" : "no");
    out += fmt::format("Gate F lastpart host fallback (informational): {}\n",
                       lastpart_host_phases_required(*nt_detail) ? "yes" : "no");
    out += fmt::format("host_voltage_authoritative: {}\n",
                       host_voltage_is_authoritative(*nt_detail) ? "yes" : "no");
    if (!qualified && allow_unqualified_gpu_native()) {
        out += "\nNRN_GPU_ALLOW_UNQUALIFIED is set; psolve will not abort on this report.\n";
    }
    return out;
#else
    return "NRN_ENABLE_GPU is off at build time\n";
#endif
}

void require_gpu_native_qualification_or_stop() {
#if defined(NRN_ENABLE_GPU)
    if (!enabled() || !backend_native() || allow_unqualified_gpu_native()) {
        return;
    }
    if (model_qualifies_for_full_gpu_native()) {
        return;
    }
    auto const report = native_gpu_qualification_report();
    fprintf(stderr, "%s", report.c_str());
    hoc_execerror(
        "Model not qualified for GPU-native fixed-step integration. "
        "See qualification report above. "
        "Set NRN_GPU_ALLOW_UNQUALIFIED=1 to run transitional mode for development.",
        nullptr);
#else
#endif
}

std::string native_gpu_fixed_step_phase_report() {
    return native_gpu_qualification_report();
}

namespace detail {

void reset_mechanism_gpu_phases_for_testing() noexcept {
    phase_table().clear();
}

}  // namespace detail

}  // namespace neuron::gpu