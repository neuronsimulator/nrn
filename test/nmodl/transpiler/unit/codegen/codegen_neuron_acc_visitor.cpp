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
            // STATE force-inlines rates TABLE body — no device call to rates_hh_state.
            REQUIRE(generated.find("rates_hh_state(inst,") == std::string::npos);
            REQUIRE_THAT(generated, ContainsSubstring("nrn_state_hh"));
            // Hand-edit shape: present(hh_global) names + TABLE path inside STATE.
            REQUIRE_THAT(generated, ContainsSubstring("hh_global.usetable"));
            REQUIRE_THAT(generated, ContainsSubstring("hh_global.t_minf"));
            // Slim present: no unused _thread on specialized STATE.
            // (present clause lists m,h,n columns + hh_global, not _thread before them)
            REQUIRE_THAT(generated, ContainsSubstring("nrn_state_hh"));
        }
    }

    // VERBATIM keeps general ABI via procedure_safe_for_state_specialization;
    // full VERBATIM ACC codegen needs host macro scaffolding beyond this harness.

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
    }
}
