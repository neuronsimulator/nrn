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
            REQUIRE_THAT(generated, ContainsSubstring("nrn_pragma_acc(data present(nt, ml)"));
        }
    }
}
