#include <catch2/catch_test_macros.hpp>

#include "ast/program.hpp"
#include "parser/nmodl_driver.hpp"
#include "test/unit/utils/test_utils.hpp"
#include "visitors/checkparent_visitor.hpp"
#include "visitors/cvode_visitor.hpp"
#include "visitors/nmodl_visitor.hpp"
#include "visitors/symtab_visitor.hpp"
#include "visitors/visitor_utils.hpp"

using namespace nmodl;
using namespace visitor;
using namespace test;
using namespace test_utils;

using nmodl::parser::NmodlDriver;


auto run_cvode_visitor(const std::string& text) {
    NmodlDriver driver;
    const auto& ast = driver.parse_string(text);
    SymtabVisitor().visit_program(*ast);
    CvodeVisitor().visit_program(*ast);

    return ast;
}


TEST_CASE("Make sure CVODE block is generated properly", "[visitor][cvode]") {
    GIVEN("No DERIVATIVE block") {
        auto nmodl_text = "NEURON { SUFFIX example }";
        auto ast = run_cvode_visitor(nmodl_text);
        THEN("No CVODE block is added") {
            auto blocks = collect_nodes(*ast, {ast::AstNodeType::CVODE_BLOCK});
            REQUIRE(blocks.empty());
        }
    }
    GIVEN("DERIVATIVE block") {
        auto nmodl_text = R"(
            NEURON	{
                SUFFIX example
            }

            STATE {X Y[2] Z}

            DERIVATIVE equation {
                CONSERVE X + Z = 5
                X' = -X + Z * Z
                Z' = Z * X
                Y'[1] = -Y[0]
                Y'[0] = -Y[1]
            }
)";
        auto ast = run_cvode_visitor(nmodl_text);
        THEN("CVODE block is added") {
            auto blocks = collect_nodes(*ast, {ast::AstNodeType::CVODE_BLOCK});
            REQUIRE(blocks.size() == 1);
            THEN("No primed variables exist in the CVODE block") {
                auto primed_vars = collect_nodes(*blocks[0], {ast::AstNodeType::PRIME_NAME});
                REQUIRE(primed_vars.empty());
            }
            THEN("No CONSERVE statements are present in the CVODE block") {
                auto conserved_stmts = collect_nodes(*blocks[0], {ast::AstNodeType::CONSERVE});
                REQUIRE(conserved_stmts.empty());
            }
        }
    }
    GIVEN("Multiple DERIVATIVE blocks") {
        auto nmodl_text = R"(
            NEURON	{
                SUFFIX example
            }

            STATE {X}

            DERIVATIVE equation {
                X' = -X
            }

            DERIVATIVE equation2 {
                X' = -X * X
            }
)";
        THEN("An error is raised") {
            REQUIRE_THROWS_AS(run_cvode_visitor(nmodl_text), std::runtime_error);
        }
    }
}
