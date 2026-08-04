#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include <filesystem>

#include "ast/program.hpp"
#include "codegen/codegen_neuron_acc_visitor.hpp"
#include "parser/nmodl_driver.hpp"
#include "utils/test_utils.hpp"
#include "visitors/function_callpath_visitor.hpp"
#include "visitors/inline_visitor.hpp"
#include "visitors/neuron_solve_visitor.hpp"
#include "visitors/solve_block_visitor.hpp"
#include "visitors/symtab_visitor.hpp"

using Catch::Matchers::ContainsSubstring;

using namespace nmodl;
using namespace visitor;
using namespace codegen;

using nmodl::parser::NmodlDriver;
using nmodl::test_utils::reindent_text;

namespace {

std::shared_ptr<CodegenNeuronAccVisitor> create_neuron_acc_visitor(
    const std::shared_ptr<ast::Program>& ast,
    std::stringstream& ss) {
    SymtabVisitor().visit_program(*ast);
    InlineVisitor().visit_program(*ast);
    NeuronSolveVisitor().visit_program(*ast);
    SolveBlockVisitor().visit_program(*ast);
    FunctionCallpathVisitor().visit_program(*ast);

    // CVODE codegen requires CvodeVisitor; ACC pragma tests target fixed-step paths.
    return std::make_shared<CodegenNeuronAccVisitor>("_test", ss, "double", false, false);
}

std::string get_neuron_acc_code(const std::string& nmodl_text) {
    const auto& ast = NmodlDriver().parse_string(nmodl_text);
    std::stringstream ss;
    auto visitor = create_neuron_acc_visitor(ast, ss);
    visitor->visit_program(*ast);
    return reindent_text(ss.str());
}

std::string get_neuron_acc_code_from_file(const std::filesystem::path& mod_path) {
    const auto& ast = NmodlDriver().parse_file(mod_path);
    std::stringstream ss;
    auto visitor = create_neuron_acc_visitor(ast, ss);
    visitor->visit_program(*ast);
    return reindent_text(ss.str());
}

}  // namespace

SCENARIO("NEURON OpenACC codegen emits offload pragmas", "[codegen][neuron][acc]") {
    GIVEN("a Hodgkin-Huxley style mechanism") {
        const std::string nmodl_text = R"(
            NEURON {
                SUFFIX hhacc
                USEION na READ ena WRITE ina
                RANGE gnabar, gna, ina
            }
            PARAMETER {
                gnabar = 0.12
            }
            STATE {
                m
            }
            ASSIGNED {
                v (mV)
                ena (mV)
                gna
                ina (mA/cm2)
                minf
                mtau (ms)
            }
            BREAKPOINT {
                SOLVE states METHOD cnexp
                gna = gnabar*m*m*m
                ina = gna*(v - ena)
            }
            INITIAL {
                rates(v)
                m = minf
            }
            DERIVATIVE states {
                rates(v)
                m' = (minf-m)/mtau
            }
            PROCEDURE rates(v(mV)) {
                minf = 0.5
                mtau = 1
            }
        )";

        THEN("generated code includes NEURON GPU offload helpers and OpenACC loops") {
            const auto generated = get_neuron_acc_code(nmodl_text);
            REQUIRE_THAT(generated, ContainsSubstring("#include <neuron/gpu/offload.hpp>"));
            REQUIRE_THAT(generated, ContainsSubstring("C++-OpenAcc-NEURON"));
            REQUIRE_THAT(generated, ContainsSubstring("nrn_pragma_acc(parallel loop"));
            REQUIRE_THAT(generated, ContainsSubstring("if(nt->compute_gpu)"));
            REQUIRE_THAT(generated, ContainsSubstring("async(nt->stream_id)"));
        }
    }

    GIVEN("the canonical hh.mod shipped with NEURON") {
        const auto mod_path = std::filesystem::path(NRN_SOURCE_DIR) / "src/nrnoc/hh.mod";
        REQUIRE(std::filesystem::exists(mod_path));

        THEN("prototype gate: hh.mod ACC output contains OpenACC pragmas") {
            const auto generated = get_neuron_acc_code_from_file(mod_path);
            REQUIRE_THAT(generated, ContainsSubstring("#include <neuron/gpu/offload.hpp>"));
            REQUIRE_THAT(generated, ContainsSubstring("nrn_pragma_acc(parallel loop"));
            REQUIRE_THAT(generated, ContainsSubstring("nrn_pragma_acc(data present(nt, _ml_arg"));
        }

        THEN("Session A residual: force-inline unique STATE rates body (TABLE path)") {
            const auto generated = get_neuron_acc_code_from_file(mod_path);
            // Thin specialized versions still emitted (available; general rates for HOC/table).
            REQUIRE_THAT(generated, ContainsSubstring("rates_hh_state"));
            REQUIRE_THAT(generated, ContainsSubstring("f_rates_hh_state"));
            REQUIRE_THAT(generated,
                         ContainsSubstring(
                             "inline static int rates_hh_state(hh_Instance& inst, double& _kl_minf"));
            REQUIRE(generated.find("rates_hh_state(hh_Instance& inst, hh_NodeData&") ==
                    std::string::npos);
            // STATE force-inlines rates TABLE body — no call to rates_hh_state from
            // nrn_state_hh (f_rates_hh_state(inst, may still appear inside the emitted
            // thin rates_hh_state helper body).
            {
                const auto state_begin = generated.find("static void nrn_state_hh");
                REQUIRE(state_begin != std::string::npos);
                const auto state_end = generated.find("static void nrn_", state_begin + 20);
                const auto state_fn = generated.substr(
                    state_begin,
                    (state_end == std::string::npos ? generated.size() : state_end) - state_begin);
                REQUIRE(state_fn.find("rates_hh_state(") == std::string::npos);
            }
            REQUIRE_THAT(generated, ContainsSubstring("nrn_state_hh"));
            // Hand-edit shape: present(hh_global) names + TABLE path inside STATE.
            REQUIRE_THAT(generated, ContainsSubstring("hh_global.usetable"));
            REQUIRE_THAT(generated, ContainsSubstring("hh_global.t_minf"));
            // Slim present: no unused _thread on specialized STATE.
            // (present clause lists m,h,n columns + hh_global, not _thread before them)
            REQUIRE_THAT(generated, ContainsSubstring("nrn_state_hh"));
        }

        THEN("Session B: force-inline thin CURRENT (min present, no fat nrn_current)") {
            const auto generated = get_neuron_acc_code_from_file(mod_path);
            REQUIRE_THAT(generated, ContainsSubstring("nrn_cur_hh"));
            // Force-inline: no device call / definition of fat or thin nrn_current_hh.
            REQUIRE(generated.find("nrn_current_hh") == std::string::npos);
            // Hand-edit shape: numerical di/dv with local voltage.
            REQUIRE_THAT(generated, ContainsSubstring("double _cur_v = v + 0.001"));
            REQUIRE_THAT(generated, ContainsSubstring("double _cur_v = v"));
            REQUIRE_THAT(generated, ContainsSubstring("double I1 = _nrn_cur_sum"));
            REQUIRE_THAT(generated, ContainsSubstring("double I0 = _nrn_cur_sum"));
            // Stack ion / intermediate ASSIGNED (not present as SoA for ena).
            REQUIRE_THAT(generated, ContainsSubstring("double ena = "));
            REQUIRE_THAT(generated, ContainsSubstring("double ek = "));
            REQUIRE_THAT(generated, ContainsSubstring("double gna;"));
            // Min present: params + STATE + g_unused; no fat present_fp_19 style for ena.
            REQUIRE_THAT(generated, ContainsSubstring("vec_rhs[:nt->end]"));
            // Device CURRENT folds jacob: present + write vec_d when compute_gpu.
            REQUIRE_THAT(generated, ContainsSubstring("vec_d[:nt->end]"));
            REQUIRE_THAT(generated, ContainsSubstring("if (nt->compute_gpu)"));
            REQUIRE(generated.find("nrn_cur_hh") != std::string::npos);
            // Extract nrn_cur_hh and require vec_d update with g.
            const auto cur_begin = generated.find("static void nrn_cur_hh");
            REQUIRE(cur_begin != std::string::npos);
            const auto cur_end = generated.find("static void nrn_", cur_begin + 20);
            const auto cur_fn = generated.substr(
                cur_begin,
                (cur_end == std::string::npos ? generated.size() : cur_end) - cur_begin);
            REQUIRE_THAT(cur_fn, ContainsSubstring("vec_d[node_id]"));
        }

        THEN("Session D: slim JACOB (no global present; g_unused + vec_d only)") {
            const auto generated = get_neuron_acc_code_from_file(mod_path);
            const auto jacob_begin = generated.find("static void nrn_jacob_hh");
            REQUIRE(jacob_begin != std::string::npos);
            const auto jacob_end = generated.find("static void nrn_", jacob_begin + 20);
            const auto jacob_fn = generated.substr(
                jacob_begin,
                (jacob_end == std::string::npos ? generated.size() : jacob_end) - jacob_begin);
            // Device path is the first if-block; host else may still use make_instance.
            const auto device_end = jacob_fn.find("} else {");
            REQUIRE(device_end != std::string::npos);
            const auto device_fn = jacob_fn.substr(0, device_end);
            // Device path: no GLOBAL enter / stale update / present(hh_global).
            REQUIRE(device_fn.find("hh_global_gpu_resident") == std::string::npos);
            REQUIRE(device_fn.find("update device (hh_global)") == std::string::npos);
            REQUIRE(device_fn.find("hh_global") == std::string::npos);
            REQUIRE(device_fn.find("make_instance_hh") == std::string::npos);
            REQUIRE_THAT(device_fn, ContainsSubstring("make_node_data_hh"));
            REQUIRE_THAT(device_fn, ContainsSubstring("vec_d[:nt->end]"));
            REQUIRE_THAT(device_fn, ContainsSubstring("nrn_pragma_acc(parallel loop"));
        }
    }

    // VERBATIM keeps general ABI via procedure_safe_for_state_specialization;
    // full VERBATIM ACC codegen needs host macro scaffolding beyond this harness.

    GIVEN("GLOBAL scalar used in STATE body (Traub cad ceiling shape)") {
        // cnexp STATE + BREAKPOINT clamp on GLOBAL must use bare present(*_global),
        // not inst.global-> (host address) — Traub cad ACC_TIME ~425→32 µs.
        const std::string nmodl_text = R"(
            NEURON {
                SUFFIX CadCeil
                USEION ca READ ica WRITE cai
                RANGE phi, beta
                GLOBAL ceiling
            }
            PARAMETER {
                phi = 1
                beta = 1
            }
            STATE { cai }
            ASSIGNED { ica }
            BREAKPOINT {
                SOLVE state METHOD cnexp
                if (cai > ceiling) { cai = ceiling }
            }
            DERIVATIVE state {
                cai' = -phi * ica - beta * cai
            }
        )";

        THEN("STATE uses bare CadCeil_global.ceiling, not inst.global->ceiling") {
            const auto generated = get_neuron_acc_code(nmodl_text);
            const auto state_begin = generated.find("static void nrn_state_CadCeil");
            REQUIRE(state_begin != std::string::npos);
            const auto state_end = generated.find("static void nrn_", state_begin + 20);
            const auto state_fn = generated.substr(
                state_begin,
                (state_end == std::string::npos ? generated.size() : state_end) - state_begin);
            REQUIRE_THAT(state_fn, ContainsSubstring("CadCeil_global.ceiling"));
            REQUIRE(state_fn.find("inst.global->ceiling") == std::string::npos);
        }
    }

    GIVEN("STATE-only ASSIGNED intermediates (Traub NMDA Mg_factor shape)") {
        // A1_/A2_ written in STATE procedure, not live for CURRENT → stack temps.
        // Mg_unblocked written in STATE but CURRENT reads it → remains SoA present.
        const std::string nmodl_text = R"(
            NEURON {
                POINT_PROCESS NmdaStack
                RANGE g, i, e
                NONSPECIFIC_CURRENT i
            }
            PARAMETER {
                e = 0
                Mg = 1.5
            }
            ASSIGNED {
                v
                i
                g
                A1_
                A2_
                Mg_unblocked
            }
            STATE { A }
            BREAKPOINT {
                SOLVE state METHOD cnexp
                g = A
                i = g * Mg_unblocked * (v - e)
            }
            DERIVATIVE state {
                Mg_factor()
                A' = 0
            }
            PROCEDURE Mg_factor() {
                A1_ = exp(-0.016 * v)
                A2_ = 1000.0 * Mg * exp(-0.045 * v)
                Mg_unblocked = 1.0 / (1.0 + A1_ + A2_)
            }
        )";

        THEN("STATE stacks A1_/A2_; keeps Mg_unblocked SoA present") {
            const auto generated = get_neuron_acc_code(nmodl_text);
            const auto state_begin = generated.find("static void nrn_state_NmdaStack");
            REQUIRE(state_begin != std::string::npos);
            const auto state_end = generated.find("static void nrn_", state_begin + 20);
            const auto state_fn = generated.substr(
                state_begin,
                (state_end == std::string::npos ? generated.size() : state_end) - state_begin);
            // Stack declarations for pure STATE temps.
            REQUIRE_THAT(state_fn, ContainsSubstring("double A1_;"));
            REQUIRE_THAT(state_fn, ContainsSubstring("double A2_;"));
            // Body uses bare stack names (not SoA A1_[id]).
            REQUIRE_THAT(state_fn, ContainsSubstring("A1_ ="));
            REQUIRE_THAT(state_fn, ContainsSubstring("A2_ ="));
            REQUIRE(state_fn.find("A1_[id]") == std::string::npos);
            REQUIRE(state_fn.find("A2_[id]") == std::string::npos);
            // Mg_unblocked must remain SoA (_present_fp_N[id]) for CURRENT.
            REQUIRE_THAT(state_fn, ContainsSubstring("_present_fp_"));
            // At least one present_fp write (Mg_unblocked) remains.
            REQUIRE(state_fn.find("_present_fp_") != std::string::npos);
            REQUIRE(state_fn.find("[id] =") != std::string::npos);
        }
    }

    GIVEN("the canonical passive.mod shipped with NEURON") {
        const auto mod_path = std::filesystem::path(NRN_SOURCE_DIR) / "src/nrnoc/passive.mod";
        REQUIRE(std::filesystem::exists(mod_path));

        THEN("built-in pas CURRENT codegen includes OpenACC offload pragmas") {
            const auto generated = get_neuron_acc_code_from_file(mod_path);
            REQUIRE_THAT(generated, ContainsSubstring("#include <neuron/gpu/offload.hpp>"));
            REQUIRE_THAT(generated, ContainsSubstring("nrn_cur_pas"));
            REQUIRE_THAT(generated, ContainsSubstring("nrn_pragma_acc(parallel loop"));
            REQUIRE_THAT(generated, ContainsSubstring("if(nt->compute_gpu)"));
        }
    }

    GIVEN("USEION READ ena also listed in PARAMETER (Traub naf shape)") {
        // Traub channel MODs put `ena` in PARAMETER as well as USEION READ.
        // Session B must still stack-load ion ena (not stale SoA shadow).
        const std::string nmodl_text = R"(
            NEURON {
                SUFFIX TraubEnaParam
                USEION na READ ena WRITE ina
                RANGE gbar, ina
            }
            PARAMETER {
                gbar = 0.1 (mho/cm2)
                v (mV) ena (mV)
            }
            ASSIGNED {
                ina (mA/cm2)
            }
            STATE { m }
            BREAKPOINT {
                SOLVE states METHOD cnexp
                ina = gbar * m * (v - ena)
            }
            DERIVATIVE states {
                m' = (1 - m)
            }
        )";

        THEN("Session B CURRENT uses stack ena from ion dptr, not present_fp ena") {
            const auto generated = get_neuron_acc_code(nmodl_text);
            const auto cur_begin = generated.find("static void nrn_cur_TraubEnaParam");
            REQUIRE(cur_begin != std::string::npos);
            const auto cur_end = generated.find("static void nrn_", cur_begin + 20);
            const auto cur_fn = generated.substr(
                cur_begin,
                (cur_end == std::string::npos ? generated.size() : cur_end) - cur_begin);
            // Stack load from ion dptr.
            REQUIRE_THAT(cur_fn, ContainsSubstring("double ena = "));
            REQUIRE_THAT(cur_fn, ContainsSubstring("_present_dptr_"));
            // Body must use bare stack ena in (v - ena), not SoA shadow.
            const bool uses_stack_ena =
                cur_fn.find("_cur_v - ena") != std::string::npos ||
                cur_fn.find("(_cur_v - ena)") != std::string::npos;
            REQUIRE(uses_stack_ena);
            REQUIRE(cur_fn.find("_cur_v - _present_fp_") == std::string::npos);
        }
    }

    GIVEN("ELECTRODE_CURRENT with empty BREAKPOINT (IClamp_const shape)") {
        // Traub ModelDB 82894: i set only in INITIAL; CURRENT still does
        // current += i. Session B min-present must declare that SoA column.
        const std::string nmodl_text = R"(
            NEURON {
                POINT_PROCESS IClampConstAcc
                RANGE amp, i
                ELECTRODE_CURRENT i
            }
            PARAMETER {
                amp (nA)
            }
            ASSIGNED { i (nA) }
            INITIAL {
                i = amp
            }
            BREAKPOINT {
            }
        )";

        THEN("Session B force-inline presents electrode current SoA column") {
            const auto generated = get_neuron_acc_code(nmodl_text);
            REQUIRE_THAT(generated, ContainsSubstring("nrn_cur_IClampConstAcc"));
            // Force-inline path (empty BREAKPOINT is safe).
            REQUIRE(generated.find("nrn_current_IClampConstAcc") == std::string::npos);
            REQUIRE_THAT(generated, ContainsSubstring("current += _present_fp_"));
            // Must declare the present_fp that current += uses (i is ASSIGNED,
            // not written in BREAKPOINT → SoA, not stack). g_unused alone is not enough.
            const auto cur_begin = generated.find("static void nrn_cur_IClampConstAcc");
            REQUIRE(cur_begin != std::string::npos);
            const auto cur_end = generated.find("static void nrn_", cur_begin + 20);
            const auto cur_fn = generated.substr(
                cur_begin,
                (cur_end == std::string::npos ? generated.size() : cur_end) - cur_begin);
            // present clause or fpfield_ptr for the i column (index 1: amp=0, i=1, …).
            REQUIRE_THAT(cur_fn, ContainsSubstring("_present_fp_1 = _lmc.template fpfield_ptr<1>()"));
            REQUIRE_THAT(cur_fn, ContainsSubstring("current += _present_fp_1[id]"));
        }
    }

    GIVEN("ExpSyn-like POINT_PROCESS with NET_RECEIVE") {
        const std::string nmodl_text = R"(
            NEURON {
                POINT_PROCESS ExpSynAcc
                RANGE tau, e, i
                NONSPECIFIC_CURRENT i
            }
            PARAMETER {
                tau = 0.1 (ms)
                e = 0 (mV)
            }
            ASSIGNED {
                v (mV)
                i (nA)
            }
            STATE {
                g (uS)
            }
            INITIAL {
                g = 0
            }
            BREAKPOINT {
                SOLVE state METHOD cnexp
                i = g*(v - e)
            }
            DERIVATIVE state {
                g' = -g/tau
            }
            NET_RECEIVE(weight (uS)) {
                g = g + weight
            }
        )";

        THEN("Stage 2 emits enqueue pnt_receive, net_buf_receive, and registration") {
            const auto generated = get_neuron_acc_code(nmodl_text);
            REQUIRE_THAT(generated,
                         ContainsSubstring("#include <neuron/gpu/net_receive_buffer.hpp>"));
            REQUIRE_THAT(generated, ContainsSubstring("nt->compute_gpu"));
            REQUIRE_THAT(generated,
                         ContainsSubstring("neuron::gpu::net_receive_buffer_enqueue"));
            REQUIRE_THAT(generated, ContainsSubstring("net_buf_receive_ExpSynAcc"));
            REQUIRE_THAT(generated,
                         ContainsSubstring("neuron::gpu::weight_soa_values()"));
            REQUIRE_THAT(generated,
                         ContainsSubstring(
                             "hoc_register_net_receive_buffering(net_buf_receive_ExpSynAcc"));
            REQUIRE_THAT(generated, ContainsSubstring("weights + weight_index"));
            // Stage 3c: multi-instance PP on one node needs atomic matrix updates.
            REQUIRE_THAT(generated, ContainsSubstring("nrn_pragma_acc(atomic update)"));
            REQUIRE_THAT(generated, ContainsSubstring("vec_rhs[node_id]"));
        }

        THEN("CURRENT mfactor uses node area SoA deviceptr, not area pdata dptr") {
            // Traub residual: PP CURRENT was (*_present_dptr_0[id+base]) pointer
            // chase for area. Prefer _d_area[node_id] (same shape as voltages).
            const auto generated = get_neuron_acc_code(nmodl_text);
            const auto cur_begin = generated.find("static void nrn_cur_ExpSynAcc");
            REQUIRE(cur_begin != std::string::npos);
            const auto cur_end = generated.find("static void nrn_", cur_begin + 20);
            const auto cur_fn = generated.substr(
                cur_begin,
                (cur_end == std::string::npos ? generated.size() : cur_end) - cur_begin);
            REQUIRE_THAT(cur_fn, ContainsSubstring("node_area_storage()"));
            REQUIRE_THAT(cur_fn, ContainsSubstring("_d_area"));
            REQUIRE_THAT(cur_fn, ContainsSubstring("1.e2/_d_area[node_id]"));
            // Must not pull area through the ion/pdata dptr table in CURRENT.
            REQUIRE(cur_fn.find("mfactor = 1.e2/(*_present_dptr_") == std::string::npos);
            const bool area_in_deviceptr =
                cur_fn.find("_d_voltages, _d_area") != std::string::npos ||
                cur_fn.find("deviceptr(_d_area") != std::string::npos;
            REQUIRE(area_in_deviceptr);
        }
    }
}
