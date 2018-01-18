#define CATCH_CONFIG_MAIN

#include <string>

#include "catch/catch.hpp"
#include "input/nmodl_constructs.h"
#include "parser/nmodl_driver.hpp"
#include "utils/string_utils.hpp"
#include "visitors/json_visitor.hpp"
#include "visitors/nmodl_visitor.hpp"
#include "visitors/perf_visitor.hpp"
#include "visitors/symtab_visitor.hpp"
#include "visitors/var_rename_visitor.hpp"
#include "visitors/verbatim_visitor.hpp"

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

//=============================================================================
// AST to NMODL printer tests
//=============================================================================

int count_leading_spaces(std::string text) {
    int length = text.size();
    stringutils::ltrim(text);
    int num_whitespaces = length - text.size();
    return num_whitespaces;
}

/// check if string has only whitespaces
bool is_empty(std::string text) {
    stringutils::trim(text);
    return text.empty();
}

/** Reindent nmodl text for text-to-text comparison
 *
 * Nmodl constructs defined in test database has extra leading whitespace.
 * This is done for readability reason in nmodl_constructs.cpp. For example,
 * we have following nmodl text with 8 leading whitespaces:


        NEURON {
            RANGE x
        }

 * We convert above paragraph to:

NEURON {
    RANGE x
}

 * i.e. we get first non-empty line and count number of leading whitespaces (X).
 * Then for every sub-sequent line, we remove first X characters (assuing those
 * all are whitespaces). This is done because when ast is transformed back to
 * nmodl, the nmodl output is without "extra" whitespaces in the provided input.
 */

std::string reindent_text(const std::string& text) {
    std::string indented_text;
    int num_whitespaces = 0;
    bool flag = false;
    std::string line;
    std::stringstream stream(text);

    while (std::getline(stream, line)) {
        if (!line.empty()) {
            /// count whitespaces for first non-empty line only
            if (!flag) {
                flag = true;
                num_whitespaces = count_leading_spaces(line);
            }

            /// make sure we don't remove non whitespaces characters
            if (!is_empty(line.substr(0, num_whitespaces))) {
                throw std::runtime_error("Test nmodl input not correctly formatted");
            }

            line.erase(0, num_whitespaces);
            indented_text += line;
        }
        /// discard empty lines at very beginning
        if (!stream.eof() && flag) {
            indented_text += "\n";
        }
    }
    return indented_text;
}

std::string run_nmodl_visitor(const std::string& text) {
    nmodl::Driver driver;
    driver.parse_string(text);
    auto ast = driver.ast();

    std::stringstream stream;
    NmodlPrintVisitor v(stream);

    v.visit_program(ast.get());
    return stream.str();
}

SCENARIO("Test for AST back to NMODL transformation") {
    for (const auto& construct : nmodl_valid_constructs) {
        auto test_case = construct.second;
        std::string input_nmodl_text = reindent_text(test_case.input);
        std::string output_nmodl_text = reindent_text(test_case.output);
        GIVEN(test_case.name) {
            THEN("Visitor successfully returns : " + input_nmodl_text) {
                auto result = run_nmodl_visitor(input_nmodl_text);
                REQUIRE(result == output_nmodl_text);
            }
        }
    }
}

//=============================================================================
// Variable rename tests
//=============================================================================
std::string run_var_rename_visitor(const std::string& text,
                                   std::vector<std::pair<std::string, std::string>> variables) {
    nmodl::Driver driver;
    driver.parse_string(text);
    auto ast = driver.ast();

    {
        for (const auto& variable : variables) {
            VarRenameVisitor v(variable.first, variable.second);
            v.visit_program(ast.get());
        }
    }
    std::stringstream stream;

    {
        NmodlPrintVisitor v(stream);
        v.visit_program(ast.get());
    }
    return stream.str();
}

SCENARIO("Renaming any variable in mod file with VarRenameVisitor") {
    GIVEN("A mod file") {
        // sample nmodl text
        std::string input_nmodl_text = R"(
            NEURON {
                SUFFIX NaTs2_t
                USEION na READ ena WRITE ina
                RANGE gNaTs2_tbar
            }

            PARAMETER {
                gNaTs2_tbar = 0.1 (S/cm2)
            }

            STATE {
                m
                h
            }

            COMMENT
                m and gNaTs2_tbar remain same here
            ENDCOMMENT

            BREAKPOINT {
                LOCAL gNaTs2_t
                gNaTs2_t = gNaTs2_tbar*m*m*m*h
                ina = gNaTs2_t*(v-ena)
            }

            FUNCTION mAlpha() {
            }
        )";

        /// expected result after renaming m, gNaTs2_tbar and mAlpha
        std::string output_nmodl_text = R"(
            NEURON {
                SUFFIX NaTs2_t
                USEION na READ ena WRITE ina
                RANGE new_gNaTs2_tbar
            }

            PARAMETER {
                new_gNaTs2_tbar = 0.1 (S/cm2)
            }

            STATE {
                mm
                h
            }

            COMMENT
                m and gNaTs2_tbar remain same here
            ENDCOMMENT

            BREAKPOINT {
                LOCAL gNaTs2_t
                gNaTs2_t = new_gNaTs2_tbar*mm*mm*mm*h
                ina = gNaTs2_t*(v-ena)
            }

            FUNCTION mBeta() {
            }
        )";

        std::string input = reindent_text(input_nmodl_text);
        std::string expected_output = reindent_text(output_nmodl_text);

        THEN("existing variables could be renamed") {
            std::vector<std::pair<std::string, std::string>> variables = {
                {"m", "mm"}, {"gNaTs2_tbar", "new_gNaTs2_tbar"}, {"mAlpha", "mBeta"},
            };
            auto result = run_var_rename_visitor(input, variables);
            REQUIRE(result == expected_output);
        }

        THEN("non-existing variables will be ignored") {
            std::vector<std::pair<std::string, std::string>> variables = {
                {"unknown_variable", "doesnot_matter"}};
            auto result = run_var_rename_visitor(input, variables);
            REQUIRE(result == input);
        }
    }
}