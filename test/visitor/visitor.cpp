#define CATCH_CONFIG_MAIN

#include <string>

#include "catch/catch.hpp"
#include "parser/nmodl_driver.hpp"
#include "visitors/verbatim_visitor.hpp"
#include "visitors/json_visitor.hpp"
#include "visitors/perf_visitor.hpp"
#include "visitors/symtab_visitor.hpp"

using json = nlohmann::json;

//=============================================================================
// Verbatim visitor tests
//=============================================================================

std::vector<std::string> run_verbatim_visitor(const std::string& text) {
    nmodl::Driver driver;
    driver.parse_string(text);
    auto ast = driver.ast();

    VerbatimVisitor v;
    v.visit_program(ast.get());
    return v.verbatim_blocks();
}

TEST_CASE("Verbatim Visitor") {
    SECTION("Single Block") {
        std::string text = "VERBATIM int a; ENDVERBATIM";
        auto blocks = run_verbatim_visitor(text);

        REQUIRE(blocks.size() == 1);
        REQUIRE(blocks.front().compare(" int a; ") == 0);
    }

    SECTION("Multiple Blocks") {
        std::string text = "VERBATIM int a; ENDVERBATIM VERBATIM float b; ENDVERBATIM";
        auto blocks = run_verbatim_visitor(text);

        REQUIRE(blocks.size() == 2);
        REQUIRE(blocks[0].compare(" int a; ") == 0);
        REQUIRE(blocks[1].compare(" float b; ") == 0);
    }
}

//=============================================================================
// JSON visitor tests
//=============================================================================

std::string run_json_visitor(const std::string& text, bool compact = false) {
    nmodl::Driver driver;
    driver.parse_string(text);
    auto ast = driver.ast();

    std::stringstream ss;
    JSONVisitor v(ss);

    /// if compact is true then we get compact json output
    v.compact_json(compact);

    v.visit_program(ast.get());
    return ss.str();
}

TEST_CASE("JSON Visitor") {
    SECTION("JSON object test") {
        std::string nmodl_text = "NEURON {}";
        json expected = R"(
            {
              "Program": [
                {
                  "NeuronBlock": [
                    {
                      "StatementBlock": []
                    }
                  ]
                }
              ]
            }
        )"_json;

        auto json_text = run_json_visitor(nmodl_text);
        json result = json::parse(json_text);

        REQUIRE(expected == result);
    }

    SECTION("JSON text test (compact format)") {
        std::string nmodl_text = "NEURON {}";
        std::string expected = R"({"Program":[{"NeuronBlock":[{"StatementBlock":[]}]}]})";

        auto result = run_json_visitor(nmodl_text, true);
        REQUIRE(result == expected);
    }
}

//=============================================================================
// Symtab and Perf visitor tests
//=============================================================================

SCENARIO("Symbol table generation and Perf stat visitor pass") {
    GIVEN("A mod file and associated ast") {
        std::string nmodl_text = R"(
            NEURON  {
                SUFFIX NaTs2_t
                USEION na READ ena WRITE ina
                RANGE gNaTs2_tbar
            }

            PARAMETER   {
                gNaTs2_tbar = 0.00001 (S/cm2)
            }

            ASSIGNED    {
                v   (mV)
                ena (mV)
            }

            STATE {
                m
                h
            }

            BREAKPOINT  {
                LOCAL gNaTs2_t
                CONDUCTANCE gNaTs2_t USEION na
                SOLVE states METHOD cnexp
                gNaTs2_t = gNaTs2_tbar*m*m*m*h
                ina = gNaTs2_t*(v-ena)
            }

            FUNCTION hBetaf(v) {
                    hBetaf = (-0.015 * (-v -60))/(1-(exp((-v -60)/6)))
            }
        )";

        nmodl::Driver driver;
        driver.parse_string(nmodl_text);
        auto ast = driver.ast();

        WHEN("Symbol table generator pass runs") {
            ModelSymbolTable symtab;
            SymtabVisitor v(&symtab);
            v.visit_program(ast.get());

            using namespace symtab::details;

            THEN("Can lookup for defined variables") {
                auto symbol = symtab.lookup("m");
                REQUIRE(symbol->has_properties(NmodlInfo::dependent_def));
                REQUIRE_FALSE(symbol->has_properties(NmodlInfo::local_var));

                symbol = symtab.lookup("gNaTs2_tbar");
                REQUIRE(symbol->has_properties(NmodlInfo::param_assign));
                REQUIRE(symbol->has_properties(NmodlInfo::range_var));

                symbol = symtab.lookup("ena");
                REQUIRE(symbol->has_properties(NmodlInfo::read_ion_var));
            }
            THEN("Can lookup for defined functions") {
                auto symbol = symtab.lookup("hBetaf");
                REQUIRE(symbol->has_properties(NmodlInfo::function_block));
            }
            THEN("Non existent variable lookup returns nullptr") {
                REQUIRE(symtab.lookup("xyz") == nullptr);
            }

            WHEN("Perf visitor pass runs after symtab visitor") {
                PerfVisitor v;
                v.visit_program(ast.get());
                auto result = v.get_total_perfstat();

                THEN("Performance counters are updated") {
                    REQUIRE(result.add_count == 0);
                    REQUIRE(result.sub_count == 4);
                    REQUIRE(result.mul_count == 6);
                    REQUIRE(result.div_count == 2);
                    REQUIRE(result.exp_count == 1);
                    REQUIRE(result.global_read_count == 7);
                    REQUIRE(result.global_write_count == 1);
                    REQUIRE(result.local_read_count == 3);
                    REQUIRE(result.local_write_count == 2);
                    REQUIRE(result.func_call_count == 1);
                    REQUIRE(result.neg_count == 3);
                }
            }
        }

        WHEN("Perf visitor pass runs before symtab visitor") {
            PerfVisitor v;
            THEN("exception is thrown") {
                REQUIRE_THROWS_WITH(v.visit_program(ast.get()), Catch::Contains("table not setup"));
            }
        }
    }
}